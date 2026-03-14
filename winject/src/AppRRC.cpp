/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "AppRRC.hpp"

inline sockaddr_in console_to_sockaddr4(const std::string& hostport)
{
    std::stringstream ss(hostport);
    std::string host;
    std::string port_str;

    if (!std::getline(ss, host, ':') || !std::getline(ss, port_str))
    {
        return {};
    }

    uint16_t port = static_cast<uint16_t>(std::stoul(port_str));
    return bfc::ip4_port_to_sockaddr(host, port);
}

AppRRC::AppRRC(const config_t& config)
    : config(config)
    , tx_scheduler(timer, *this)
    , console_sock(bfc::create_udp4())
{
    rrc_event_fd = eventfd(0, 0);

    if (-1 == rrc_event_fd)
    {
        throw std::runtime_error(strerror(errno));
    }

    std::stringstream srcss;
    uint32_t srcraw;
    srcss << std::hex << config.frame_config.hwdst;
    srcss >> srcraw;
    winject::ieee_802_11::filters::winject_src src_filter(srcraw);

    if (!config.app_config.woudp_rx.size() && !config.app_config.wifi_device2.size())
    {
        wifi = std::make_shared<WIFI>(config.app_config.wifi_device);
        winject::ieee_802_11::filters::attach(wifi->handle(), src_filter);
    }
    else if (config.app_config.wifi_device2.size())
    {
        wifi = std::make_shared<DualWIFI>(
            config.app_config.wifi_device,
            config.app_config.wifi_device2);
        winject::ieee_802_11::filters::attach(wifi->handle(), src_filter);
    }
    else
    {
        wifi = std::make_shared<WIFIUDP>(
            config.app_config.woudp_tx,
            config.app_config.woudp_rx);
    }

    stats.rx_antenna_dbm_avg46 = &main_monitor->get_metric("wifi_rx_antenna_dbm_avg46");

    setup_80211_base_frame();
    setup_console();
    setup_pdcps();
    setup_llcs();
    setup_eps();
    setup_scheduler();
    setup_lcrrc();
    setup_rrc();
}

AppRRC::~AppRRC()
{
    tx_scheduler.stop();
    stop();

    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | AppRRC stopping...");

    if (rrc_rx_thread.joinable())
    {
        rrc_rx_thread.join();
    }

    if (timer_thread.joinable())
    {
        timer_thread.join();
    }

    if (timer2_thread.joinable())
    {
        timer2_thread.join();
    }

    eps.clear();

    close(rrc_event_fd);
}

void AppRRC::on_console_read()
{
    bfc::buffer_view bv{console_buff, sizeof(console_buff)};
    sockaddr_in sender_addr;
    socklen_t sender_addr_sz = sizeof(sender_addr);
    auto rv = console_sock.recv(bv, 0, (sockaddr*)&sender_addr, &sender_addr_sz);
    if (rv>0)
    {
        console_buff[rv] = 0;
        int pFd = console_sock.fd();
        std::string result = cmdman.execute(std::string((char*)console_buff, (size_t)rv));
        auto res_copy = result;
        res_copy.pop_back();
        Logless(*main_logger, RRC_DBG, "DBG | AppRRC | console: #", res_copy.c_str());

        console_sock.send(
            bfc::const_buffer_view((uint8_t*)result.data(), result.size()),
            0,
            (const sockaddr*)&sender_addr,
            sizeof(sender_addr));
    }
}

std::string AppRRC::on_cmd_init(bfc::args_map&& args)
{
    lcid_t lcid = args.as<int>("lcid").value();
    push_rrc_event(rrc_event_setup_t{lcid, true});
    return "pushing...\n";
}

std::string AppRRC::on_cmd_stop(bfc::args_map&& args)
{
    stop();
    return "stop:\n";
}

std::string AppRRC::on_cmd_log(bfc::args_map&& args)
{
    auto logbit_str = args.arg("bit").value_or("");
    if (!logbit_str.empty())
    {
        size_t idx = 0;
        for (auto c : logbit_str)
        {
            main_logger->set_logbit(c=='1', idx++);
        }
    }
    else
    {
        auto idx = args.as<int>("index").value_or(0);
        auto value = args.as<int>("set").value_or(0);
        main_logger->set_logbit(value, idx);
    }
    return "level set.\n";
}

void AppRRC::run()
{
    pthread_setname_np(pthread_self(), "RRC_REACTOR");
    reactor.run();
}

void AppRRC::on_rlf(lcid_t lcid)
{
    push_rrc_event(rrc_event_rlf_t{lcid});
}

void AppRRC::on_init(lcid_t lcid)
{
    push_rrc_event(rrc_event_setup_t{lcid});
}

void AppRRC::perform_tx(size_t payload_size)
{
    tx_frame.set_body_size(payload_size);
    size_t sz = radiotap.size() + tx_frame.size();

    if (main_logger->get_logbit(WTX_BUF))
    {
        Logless(*main_logger, WTX_BUF, "TRC | AppRRC | wifi send buffer[#]:\n#", sz, buffer_str(tx_buff, sz).c_str());
        Logless(*main_logger, WTX_BUF, "TRC | AppRRC | send size #", sz);
    }
    if (main_logger->get_logbit(WTX_RAD))
    {
        Logless(*main_logger, WTX_RAD, "TRC | AppRRC | --- radiotap info ---\n#", winject::radiotap::to_string(radiotap).c_str());
    }
    if (main_logger->get_logbit(WTX_802))
    {
        Logless(*main_logger, WTX_802, "TRC | AppRRC | --- 802.11 info ---\n#", winject::ieee_802_11::to_string(tx_frame).c_str());
        Logless(*main_logger, WTX_802, "TRC | AppRRC |   frame body size: #", payload_size);
        Logless(*main_logger, WTX_802, "TRC | AppRRC | -----------------------");
    }
    wifi->send(tx_buff, sz);
}

void AppRRC::setup_80211_base_frame()
{
    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | Generating base frame for TX...");
    std::memset(tx_buff,0,sizeof(tx_buff));
    radiotap = winject::radiotap::radiotap_t(tx_buff);
    radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_FLAGS;
    radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_RATE;
    radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_TX_FLAGS;
    radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_MCS;
    radiotap.rescan(true);
    radiotap.header->version = 0;
    radiotap.rate->value = 65*2;
    radiotap.tx_flags->value |= winject::radiotap::tx_flags_t::E_FLAGS_NOACK;
    radiotap.tx_flags->value |= winject::radiotap::tx_flags_t::E_FLAGS_NOREORDER;
    radiotap.mcs->known |= winject::radiotap::mcs_t::E_KNOWN_BW;
    radiotap.mcs->known |= winject::radiotap::mcs_t::E_KNOWN_MCS;
    radiotap.mcs->known |= winject::radiotap::mcs_t::E_KNOWN_STBC;
    radiotap.mcs->mcs_index = 7;
    radiotap.mcs->flags |= winject::radiotap::mcs_t::E_FLAG_BW_20;
    radiotap.mcs->flags |= winject::radiotap::mcs_t::E_FLAG_STBC_1;

    tx_frame = winject::ieee_802_11::frame_t(radiotap.end(), tx_buff + sizeof(tx_buff));
    tx_frame.frame_control->protocol_type = winject::ieee_802_11::frame_control_t::E_TYPE_DATA;
    tx_frame.rescan();
    tx_frame.address1->set(0xFFFFFFFFFFFF); // IBSS Destination
    std::stringstream srcss;
    std::stringstream dstss;
    uint64_t srcraw;
    uint64_t dstraw;
    srcss << std::hex << config.frame_config.hwsrc;
    dstss << std::hex << config.frame_config.hwdst;
    srcss >> srcraw;
    dstss >> dstraw;
    uint64_t address2 = (srcraw << 24) | (dstraw);
    tx_frame.address2->set(address2); // IBSS Source
    tx_frame.address3->set(0xDEADBEEFCAFE); // IBSS BSSID
    tx_frame.set_enable_fcs(false);

    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | radiotap:\n#", winject::radiotap::to_string(radiotap).c_str());
    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | 802.11:\n#", winject::ieee_802_11::to_string(tx_frame).c_str());
}

void AppRRC::setup_console()
{
    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | Setting up console...");
    if (console_sock.bind(console_to_sockaddr4(
        config.app_config.udp_console)) < 0)
    {
        LoglessF(*main_logger, RRC_ERR, "ERR | AppRRC | Bind error(_) console is now disabled!", strerror(errno));
        return;
    }

    cmdman.add("init", [this](bfc::args_map&& args){return on_cmd_init(std::move(args));});
    cmdman.add("log", [this](bfc::args_map&& args){return on_cmd_log(std::move(args));});
    cmdman.add("stop", [this](bfc::args_map&& args){return on_cmd_stop(std::move(args));});

    reactor.add_read_rdy(console_sock.fd(), [this](){on_console_read();});
}

void AppRRC::setup_pdcps()
{
    Logless(*main_logger, RRC_INF, "INF | AppRRC | Setting up PDCPs...");
    for (auto& pdcp_ : config.pdcp_configs)
    {
        auto id = pdcp_.first;
        auto& tx_config = pdcp_.second;

        IPDCP::rx_config_t rx_config{};

        // Copy tx_config initially
        rx_config.allow_segmentation = tx_config.allow_segmentation;
        rx_config.allow_reordering = tx_config.allow_reordering;
        rx_config.allow_rlf = tx_config.allow_rlf;
        rx_config.max_sn_distance = tx_config.max_sn_distance;

        auto pdcp = std::make_shared<PDCP>(*this, id, tx_config, rx_config);
        pdcps.emplace(id, pdcp);
    }
}

void AppRRC::setup_llcs()
{
    Logless(*main_logger, RRC_INF, "INF | AppRRC | Setting up LLCs...");
    for (auto& llc_tx_config : config.llc_tx_configs)
    {
        auto id = llc_tx_config.first;
        auto& tx_config = llc_tx_config.second;

        auto& rx_config = config.llc_rx_configs[id];
        rx_config.mode = tx_config.mode;
        rx_config.crc_type = tx_config.crc_type;

        auto pdcp = pdcps[id];
        auto llc = std::make_shared<LLC>(*pdcp, *this, id,
            tx_config, rx_config);

        llcs.emplace(id, llc);
    }
}

void AppRRC::setup_eps()
{
    Logless(*main_logger, RRC_INF, "INF | AppRRC | Setting up EPs...");
    for (auto& ep_ : config.ep_configs)
    {
        auto id = ep_.first;
        auto& ep_config = ep_.second;

        if (ep_config.type == "UDP")
        {
            auto pdcp_it = pdcps.find(id);
            if (pdcp_it == pdcps.end())
            {
                continue;
            }

            auto ep = std::make_shared<UDPEndPoint>(ep_config,
                *pdcp_it->second);

            eps.emplace(id, ep);
        }
        else if (ep_config.type == "TCPS")
        {
            auto pdcp_it = pdcps.find(id);
            if (pdcp_it == pdcps.end())
            {
                continue;
            }

            auto ep = std::make_shared<TCPServerEndPoint>(ep_config,
                *this,
                *pdcp_it->second);

            eps.emplace(id, ep);
        }
        else if (ep_config.type == "TCPC")
        {
            auto pdcp_it = pdcps.find(id);
            if (pdcp_it == pdcps.end())
            {
                continue;
            }

            auto ep = std::make_shared<TCPClientEndPoint>(ep_config,
                *this,
                *pdcp_it->second);

            eps.emplace(id, ep);
        }
    }
}

void AppRRC::setup_scheduler()
{
    Logless(*main_logger, RRC_INF, "INF | AppRRC | Setting up scheduler...");
    ITxScheduler::buffer_config_t buffer_config{};
    buffer_config.buffer = tx_buff;
    buffer_config.buffer_size = sizeof(tx_buff);
    buffer_config.header_size = tx_frame.frame_body - tx_buff;

    tx_scheduler.reconfigure(buffer_config);

    for (auto& llc_ : llcs)
    {
        auto id = llc_.first;
        auto llc = llc_.second;
        tx_scheduler.add_llc(id, llc.get());
        auto sched_config_ = config.scheduling_configs.find(id);
        if (sched_config_ != config.scheduling_configs.end())
        {
            auto& sched_config = sched_config_->second;
            sched_config.nd_gpdu_max_size = sched_config.nd_gpdu_max_size;
            sched_config.quanta = sched_config.quanta;
            sched_config.llcid = id;
            tx_scheduler.reconfigure(sched_config);
        }
    }

    ITxScheduler::frame_scheduling_config_t frame_config{};
    frame_config.fec_type = fec_type_e::E_FEC_TYPE_NONE;
    frame_config.slot_interval_us = config.frame_config.slot_interval_us;
    frame_config.frame_payload_size = config.frame_config.frame_payload_size;

    tx_scheduler.reconfigure(frame_config);

    timer_thread = std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_TIMER1");
        timer_running = true;
        while (timer_running)
        {
            timer.schedule(bfc::timer<>::current_time_us());
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
}

void AppRRC::setup_lcrrc()
{
    auto size = llcs.size();

    for (auto& [lcid, llc] : llcs)
    {
        if (eps.count(lcid))
        {
            auto ep = eps.at(lcid);
            auto pdcp = pdcps.at(lcid);
            auto llc = llcs.at(lcid);
            channel_rrc_contexts.emplace(lcid,
                std::make_shared<LCRRC>(*llc, *pdcp, *this, *ep, timer2));
        }
    }
}

void AppRRC::setup_rrc()
{
    reactor.add_read_rdy(rrc_event_fd, [this](){
            uint64_t one;
            read(rrc_event_fd, &one, sizeof(one));
            on_rrc_event();
        });

    rrc_rx_thread =  std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_RRC_RX");
        run_rrc_rx();});

    timer2_thread = std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_TIMER2");
        timer2_running = true;
        while (timer2_running)
        {
            timer2.schedule(bfc::timer<>::current_time_us());
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    reactor.add_read_rdy(wifi->handle(), [this](){
            on_wifi_rx();
        });
}

void AppRRC::on_wifi_rx()
{
    int rv = wifi->recv(rx_buff, sizeof(rx_buff));

    if (rv<0)
    {
        LoglessF(*main_logger, RRC_ERR, "ERR | AppRRC | wifi recv failed! errno=# error=#", errno, strerror(errno));
        return;
    }

    winject::radiotap::radiotap_t radiotap(rx_buff);
    radiotap.rescan();

    if (radiotap.antenna_signal)
    {
        stats.rx_antenna_dbm_avg46->store(
                radiotap.antenna_signal->dbm_value * 0.4 +
                stats.rx_antenna_dbm_avg46->load() * 0.6
            );
    }

    winject::ieee_802_11::frame_t frame80211(radiotap.end(), rx_buff+sizeof(rx_buff));
    frame80211.rescan();

    auto frame80211end = rx_buff+rv;
    size_t size =  frame80211end-frame80211.frame_body-4;

    if (main_logger->get_logbit(WRX_BUF))
    {
        Logless(*main_logger, WRX_BUF, "TR2 | AppRRC | wifi recv buffer[#]:\n#", rv, buffer_str(rx_buff, rv).c_str());
    }
    Logless(*main_logger, RRC_TRC, "TRC | AppRRC | recv size #", rv);
    if (main_logger->get_logbit(WRX_RAD))
    {
        Logless(*main_logger, WRX_RAD, "TR2 | AppRRC | --- radiotap info ---\n#", winject::radiotap::to_string(radiotap).c_str());
    }
    if (main_logger->get_logbit(WRX_802))
    {
        Logless(*main_logger, WRX_802, "TR2 | AppRRC | --- 802.11 info ---\n#", winject::ieee_802_11::to_string(frame80211).c_str());
        Logless(*main_logger, WRX_802, "TR2 | AppRRC |   frame body size: #", size);
        Logless(*main_logger, WRX_802, "TR2 | AppRRC | -----------------------");
    }

    if (!frame80211.frame_body)
    {
        return;
    }

    process_rx_frame(frame80211.frame_body, size);
}

void AppRRC::process_rx_frame(uint8_t* start, size_t size)
{
    uint8_t* cursor = start;
    ssize_t frame_payload_remaining = size;

    auto advance_cursor = [&cursor, &frame_payload_remaining](size_t size)
    {
        cursor += size;
        frame_payload_remaining -= size;
    };

    fec_t fec_frame(cursor, frame_payload_remaining);
    fec_frame.rescan();

    // @todo : check and fix fec errors

    cursor = fec_frame.get_data_blocks();
    frame_payload_remaining = fec_frame.get_data_sz();

    while (frame_payload_remaining > 0)
    {
        llc_t llc_pdu(cursor, frame_payload_remaining);
        if (llc_pdu.get_header_size() > frame_payload_remaining)
        {
            break;
        }

        if (llc_pdu.get_SIZE() == 0 )
        {
            LoglessF(*main_logger, RRC_ERR, "ERR | AppRRC | LLC zero (can't continue).");
            break;
        }

        auto lcid = llc_pdu.get_LCID();

        auto llc_it = llcs.find(lcid);
        if (llcs.end() == llc_it)
        {
            Logless(*main_logger, RRC_WRN, "WRN | AppRRC | Couldnt find the llc for lcid=#, skipping...", (int)lcid);
            advance_cursor(llc_pdu.get_SIZE());
            continue;
        }

        auto llc = llc_it->second;

        rx_info_t info{};
        info.in_pdu.base = llc_pdu.get_base();
        info.in_pdu.size = llc_pdu.get_SIZE();

        if (llc_pdu.get_SIZE() > frame_payload_remaining)
        {
            LoglessF(*main_logger, RRC_ERR, "ERR | AppRRC | LLC truncated (not enough data).", (int)lcid);
            break;
        }

        llc->on_rx(info);
        advance_cursor(llc_pdu.get_SIZE());
    }
}

void AppRRC::run_rrc_rx()
{
    auto lcc0 = llcs.at(0);
    auto pdcp0 = pdcps.at(0);

    pdcp0->set_rx_enabled(true);
    pdcp0->set_tx_enabled(true);
    lcc0->set_rx_enabled(true);
    lcc0->set_tx_enabled(true);

    rrc_rx_running = true;
    while (rrc_rx_running)
    {
        auto b = pdcp0->to_rx(1000*100);
        if (b.size())
        {
            RRC rrc;
            cum::per_codec_ctx context((std::byte*)b.data(), b.size());
            decode_per(rrc, context);
            on_rrc(rrc);
        }
    }
}

void AppRRC::on_rrc_event()
{
    std::unique_lock<std::mutex> lg(rrc_event_mutex);
    auto events = std::move(rrc_events);
    rrc_events.clear();
    lg.unlock();

    for (auto& event : events)
    {
        std::visit([this](auto&& event){on_rrc_event(event);},
            event);
    }
}

void AppRRC::on_rrc(const RRC& rrc)
{
    push_rrc_event(rrc_event_msg_t{rrc});
}

void AppRRC::on_rrc_event(const rrc_event_msg_t& msg)
{
    auto& rrc = msg.msg;

    if (main_logger->get_logbit(RRC_DBG))
    {
        std::string rrc_str;
        str("rrc", rrc, rrc_str, true);
        Logless(*main_logger, RRC_DBG, "DBG | AppRRC | received rrc msg=#",
            rrc_str.c_str());
    }

    std::visit([this, &rrc](auto& msg){
            on_rrc_message(rrc.requestID, msg);
        }, rrc.message);
}

template <typename T>
void AppRRC::on_rrc_message_lcrrc(int req_id, const T& msg)
{
    auto lcid = msg.lcid;
    if (channel_rrc_contexts.count(msg.lcid))
    {
        auto& lcrrc = channel_rrc_contexts.at(lcid);
        lcrrc->on_rrc_message(req_id, msg);
    }
}

void AppRRC::on_rrc_message(int req_id, const RRC_ExchangeRequest& msg)
{
    on_rrc_message_lcrrc(req_id, msg);
}

void AppRRC::on_rrc_message(int req_id, const RRC_ExchangeResponse& msg)
{
    on_rrc_message_lcrrc(req_id, msg);
}

void AppRRC::send_rrc(const RRC& rrc)
{
    auto pdcp0 = pdcps.at(0);
    buffer_t b(1024*5);
    cum::per_codec_ctx context((std::byte*)b.data(), b.size());
    encode_per(rrc, context);
    size_t encode_size = b.size() - context.size();
    if (main_logger->get_logbit(RRC_DBG))
    {
        std::string rrc_str;
        str("rrc", rrc, rrc_str, true);
        Logless(*main_logger, RRC_DBG, "DBG | AppRRC | sending rrc msg=#",
            rrc_str.c_str());
    }

    b.resize(encode_size);
    pdcp0->to_tx(std::move(b));
}

uint8_t AppRRC::allocate_req_id()
{
    return rrc_req_id.fetch_add(1);
}

void AppRRC::on_rrc_event(const rrc_event_stop_t&)
{
    rrc_rx_running = false;
    timer_running = false;
    timer2_running = false;
    reactor.stop();
}

void AppRRC::on_rrc_event(const rrc_event_rlf_t& rlf)
{
    auto lcid = rlf.lcid;
    if (channel_rrc_contexts.count(rlf.lcid))
    {
        auto& lcrrc = channel_rrc_contexts.at(lcid);
        lcrrc->on_rlf();
    }
}

void AppRRC::on_rrc_event(const rrc_event_setup_t& setup)
{
    auto lcid = setup.lcid;
    if (channel_rrc_contexts.count(setup.lcid))
    {
        auto& lcrrc = channel_rrc_contexts.at(lcid);
        lcrrc->on_init(setup.forced);
    }
}

template<typename T>
void AppRRC::push_rrc_event(T&& event)
{
    std::unique_lock<std::mutex> lg(rrc_event_mutex);
    rrc_events.emplace_back(std::forward<T&&>(event));
    uint64_t one = 1;
    write(rrc_event_fd, &one, sizeof(one));
}

void AppRRC::stop()
{
    push_rrc_event(rrc_event_stop_t{});
}