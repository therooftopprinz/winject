#include "AppRRC.hpp"

AppRRC::AppRRC(const config_t& config)
    : config(config)
    , tx_scheduler(timer, *this)
{
    rrc_event_fd = eventfd(0, EFD_SEMAPHORE);

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
    close(rrc_event_fd); 

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
}

void AppRRC::stop()
{
    rrc_rx_running = false;
    timer.stop();
    timer2.stop();
    reactor.stop();
}

void AppRRC::on_console_read()
{
    bfc::BufferView bv{console_buff, sizeof(console_buff)};
    bfc::Ip4Port sender_addr;
    auto rv = console_sock.recvfrom(bv, sender_addr);
    if (rv>0)
    {
        console_buff[rv] = 0;
        int pFd = console_sock.handle();
        std::string result = cmdman.executeCommand(std::string_view((char*)console_buff, rv));
        auto res_copy = result;
        res_copy.pop_back();
        Logless(*main_logger, RRC_DBG, "DBG | AppRRC | console: #", res_copy.c_str());

        reactor.addWriteHandler(console_sock.handle(), [pFd, this, result, sender_addr]()
            {
                console_sock.sendto(bfc::BufferView((uint8_t*)result.data(), result.size()), sender_addr);
                reactor.removeWriteHandler(pFd);
            });
    }
}

std::string AppRRC::on_cmd_push(bfc::ArgsMap&& args)
{
    // RRC rrc;
    // rrc.requestID = rrc_req_id.fetch_add(1);
    // rrc.message = RRC_PushRequest{};
    // auto& message = std::get<RRC_PushRequest>(rrc.message);

    // fill_from_config(args.argAs<int>("lcid").value_or(0xF),
    //     true, true, message);

    // auto_send_rrc(10, rrc);
    return "pushing...\n";
}

std::string AppRRC::on_cmd_pull(bfc::ArgsMap&& args)
{
    // RRC rrc;
    // rrc.requestID = rrc_req_id.fetch_add(1);
    // rrc.message = RRC_PullRequest{};
    // auto& pull_request = std::get<RRC_PullRequest>(rrc.message);
    // pull_request.lcid = args.argAs<int>("lcid").value_or(0);
    // pull_request.includeLLCConfig = true;
    // pull_request.includePDCPConfig = true;

    // auto_send_rrc(10, rrc);
    return "pulling...\n";
}

std::string AppRRC::on_cmd_stop(bfc::ArgsMap&& args)
{
    push_rrc_event(rrc_event_stop_t{});
    return "stop:\n";
}

std::string AppRRC::on_cmd_log(bfc::ArgsMap&& args)
{
    auto logbit_str = args.arg("bit").value_or("");
    if (!logbit_str.empty())
    {
        auto logbit = std::stoul(logbit_str, nullptr, 2);
        size_t idx = 0;
        for (auto c : logbit_str)
        {
            main_logger->set_logbit(c=='1', idx++);
        }
    }
    else
    {
        auto idx = args.argAs<int>("index").value_or(0);
        auto value = args.argAs<int>("set").value_or(0);
        main_logger->set_logbit(value, idx);
    }
    return "level set.\n";
}

std::string AppRRC::on_cmd_stats(bfc::ArgsMap&& args)
{
    auto lcid = args.argAs<int>("lcid").value_or(0);
    std::stringstream ss;
    ss << "stats: \n";

    auto llc_it = llcs.find(lcid);
    if (llcs.end() != llc_it)
    {
        auto& llc = llc_it->second;
        auto& stats = llc->get_stats();
        ss << "LLC STATS:";
        ss << "\n  bytes_recv:    " << stats.bytes_recv.load();
        ss << "\n  bytes_sent:    " << stats.bytes_sent.load();
        ss << "\n  bytes_resent:  " << stats.bytes_resent.load();
        ss << "\n  pkt_recv:      " << stats.pkt_recv.load();
        ss << "\n  pkt_sent:      " << stats.pkt_sent.load();
        ss << "\n  pkt_resent:    " << stats.pkt_resent.load();
        ss << "\n";
    }
    else
    {
        ss << "llc lcid not found\n";
    }

    auto pdcp_it = pdcps.find(lcid);
    if (pdcps.end() != pdcp_it)
    {
        auto& pdcp = pdcp_it->second;
        auto& stats = pdcp->get_stats();
        ss << "PDCP STATS:";
        ss << "\n  tx_queue_size:      " << stats.tx_queue_size.load();
        ss << "\n  rx_reorder_size:    " << stats.rx_reorder_size.load();
        ss << "\n  rx_invalid_pdu:     " << stats.rx_invalid_pdu.load();
        ss << "\n  rx_ignored_pdu:     " << stats.rx_ignored_pdu.load();
        ss << "\n  rx_invalid_segment: " << stats.rx_invalid_segment.load();
        ss << "\n  rx_segment_rcvd:    " << stats.rx_segment_rcvd.load();
        ss << "\n";
    }
    else
    {
        ss << "pdcp lcid not found\n";
    }

    return ss.str();
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
        Logless(*main_logger, RRC_TRC, "TRC | AppRRC | wifi send buffer[#]:\n#", sz, buffer_str(tx_buff, sz).c_str());
        Logless(*main_logger, RRC_TRC, "TRC | AppRRC | send size #", sz);
    }
    if (main_logger->get_logbit(WTX_RAD))
    {
        Logless(*main_logger, RRC_TRC, "TRC | AppRRC | --- radiotap info ---\n#", winject::radiotap::to_string(radiotap).c_str());
    }
    if (main_logger->get_logbit(WTX_802))
    {
        Logless(*main_logger, RRC_TRC, "TRC | AppRRC | --- 802.11 info ---\n#", winject::ieee_802_11::to_string(tx_frame).c_str());
        Logless(*main_logger, RRC_TRC, "TRC | AppRRC |   frame body size: #", payload_size);
        Logless(*main_logger, RRC_TRC, "TRC | AppRRC | -----------------------");
    }
    wifi->send(tx_buff, sz);
}

void AppRRC::setup_80211_base_frame()
{
    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | Generating base frame for TX...");
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
    if (console_sock.bind(bfc::toIp4Port(
        config.app_config.udp_console)) < 0)
    {
        Logless(*main_logger, RRC_ERR, "ERR | AppRRC | Bind error(_) console is now disabled!", strerror(errno));
        return;
    }

    cmdman.addCommand("push", [this](bfc::ArgsMap&& args){return on_cmd_push(std::move(args));});
    cmdman.addCommand("pull", [this](bfc::ArgsMap&& args){return on_cmd_pull(std::move(args));});
    cmdman.addCommand("stop", [this](bfc::ArgsMap&& args){return on_cmd_stop(std::move(args));});
    cmdman.addCommand("log", [this](bfc::ArgsMap&& args){return on_cmd_log(std::move(args));});
    cmdman.addCommand("stats", [this](bfc::ArgsMap&& args){return on_cmd_stats(std::move(args));});

    reactor.addReadHandler(console_sock.handle(), [this](){on_console_read();});
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
        timer.run();});
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
    reactor.addReadHandler(rrc_event_fd, [this](){
            uint64_t one;
            read(rrc_event_fd, &one, sizeof(0));
            on_rrc_event();
        });

    rrc_rx_thread =  std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_RRC_RX");
        run_rrc_rx();});

    timer2_thread = std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_TIMER2");
        timer2.run();});

    reactor.addReadHandler(wifi->handle(), [this](){
            on_wifi_rx();
        });
}

void AppRRC::on_wifi_rx()
{
    int rv = wifi->recv(rx_buff, sizeof(rx_buff));

    if (rv<0)
    {
        Logless(*main_logger, RRC_ERR, "ERR | AppRRC | wifi recv failed! errno=# error=#", errno, strerror(errno));
        return;
    }

    winject::radiotap::radiotap_t radiotap(rx_buff);
    winject::ieee_802_11::frame_t frame80211(radiotap.end(), rx_buff+sizeof(rx_buff));

    auto frame80211end = rx_buff+rv;
    size_t size =  frame80211end-frame80211.frame_body-4;

    if (main_logger->get_logbit(WRX_BUF))
    {
        Logless(*main_logger, RRC_TRC, "TR2 | AppRRC | wifi recv buffer[#]:\n#", rv, buffer_str(rx_buff, rv).c_str());
    }
    Logless(*main_logger, RRC_TRC, "TRC | AppRRC | recv size #", rv);
    if (main_logger->get_logbit(WRX_RAD))
    {
        Logless(*main_logger, RRC_TRC, "TR2 | AppRRC | --- radiotap info ---\n#", winject::radiotap::to_string(radiotap).c_str());
    }
    if (main_logger->get_logbit(WRX_802))
    {
        Logless(*main_logger, RRC_TRC, "TR2 | AppRRC | --- 802.11 info ---\n#", winject::ieee_802_11::to_string(frame80211).c_str());
        Logless(*main_logger, RRC_TRC, "TR2 | AppRRC |   frame body size: #", size);
        Logless(*main_logger, RRC_TRC, "TR2 | AppRRC | -----------------------");
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

    uint8_t fec = *cursor;
    advance_cursor(sizeof(fec));

    while (frame_payload_remaining > 0)
    {
        llc_t llc_pdu(cursor, frame_payload_remaining);
        if (llc_pdu.get_header_size() > frame_payload_remaining)
        {
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
        info.in_pdu.base = llc_pdu.base;
        info.in_pdu.size = llc_pdu.get_SIZE();

        if (llc_pdu.get_SIZE() > frame_payload_remaining)
        {
            Logless(*main_logger, RRC_ERR, "ERR | AppRRC | LLC truncated (not enough data).", (int)lcid);
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

void AppRRC::notify_rrc_event()
{
    uint64_t one = 1;
    write(rrc_event_fd, &one, sizeof(one));
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
    stop();
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
        lcrrc->on_init();
    }
}

template<typename T>
void AppRRC::push_rrc_event(T&& event)
{
    std::unique_lock<std::mutex> lg(rrc_event_mutex);
    rrc_events.emplace_back(std::forward<T&&>(event));
    notify_rrc_event();
}
