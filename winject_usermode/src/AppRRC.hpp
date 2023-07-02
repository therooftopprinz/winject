#ifndef __WINJECTUM_APPRRC_HPP__
#define __WINJECTUM_APPRRC_HPP__

#include <atomic>

#include <bfc/CommandManager.hpp>
#include <bfc/Udp.hpp>
#include <bfc/EpollReactor.hpp>
#include <bfc/Timer.hpp>

#include <winject/802_11.hpp>
#include <winject/802_11_filters.hpp>
#include <winject/radiotap.hpp>

#include "Logger.hpp"
#include "WIFI.hpp"

#include "DualWIFI.hpp"
#include "WIFIUDP.hpp"
#include "IRRC.hpp"
#include "TxScheduler.hpp"
#include "interface/rrc.hpp"

#include "LLC.hpp"
#include "PDCP.hpp"
#include "UDPEndPoint.hpp"

#include "rrc_utils.hpp"

class AppRRC : public IRRC
{
public:
    AppRRC(const config_t& config)
        : config(config)
        , tx_scheduler(timer, *this)
    {        
        std::stringstream srcss;
        uint32_t srcraw;
        srcss << std::hex << config.frame_config.hwdst;
        srcss >> srcraw;
        winject::ieee_802_11::filters::winject_src src_filter(srcraw);

        // @dont attach filter for wifi over udp
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
        setup_rrc();
    }

    ~AppRRC()
    {
        Logless(*main_logger, RRC_DBG, "DBG | AppRRC | AppRRC stopping...");
    }

    void stop()
    {
        stop_timer();
        stop_wifi_rx();
        reactor.stop();
    }

    void stop_timer()
    {
        if (timer_thread.joinable())
        {
            timer.stop();
            timer_thread.join();
        }
    }

    void stop_wifi_rx()
    {
        if (wifi_rx_thread.joinable())
        {
            wifi_rx_running = false;
            wifi_rx_thread.join();
        }
    }

    void on_console_read()
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

    std::string on_cmd_push(bfc::ArgsMap&& args)
    {
        std::shared_lock<std::shared_mutex> lg(this_mutex);

        RRC rrc;
        rrc.requestID = rrc_req_id++;
        rrc.message = RRC_PushRequest{};
        auto& message = std::get<RRC_PushRequest>(rrc.message);

        fill_from_config(
            args.argAs<int>("lcid").value_or(0),
            args.argAs<int>("include_frame").value_or(0),
            args.argAs<int>("include_llc").value_or(0),
            args.argAs<int>("include_pdcp").value_or(0),
            message);

        send_rrc(rrc);
        return "pushing...\n";
    }

    std::string on_cmd_exchange(bfc::ArgsMap&& args)
    {
        std::shared_lock<std::shared_mutex> lg(this_mutex);

        RRC rrc;
        rrc.requestID = rrc_req_id++;
        rrc.message = RRC_ExchangeRequest{};
        auto& message = std::get<RRC_ExchangeRequest>(rrc.message);
        message.lcid = args.argAs<int>("lcid").value_or(0);
        fill_from_config(
            message.lcid,
            args.argAs<int>("include_frame").value_or(0),
            args.argAs<int>("include_llc").value_or(0),
            args.argAs<int>("include_pdcp").value_or(0),
            message);

        send_rrc(rrc);
        return "exchanging...\n";
    }

    std::string on_cmd_pull(bfc::ArgsMap&& args)
    {
        std::shared_lock<std::shared_mutex> lg(this_mutex);

        RRC rrc;
        rrc.requestID = rrc_req_id++;
        rrc.message = RRC_PullRequest{};
        auto& msg = std::get<RRC_PullRequest>(rrc.message);
        msg.lcid = args.argAs<int>("lcid").value_or(0);
        msg.includeFrameConfig = args.argAs<int>("include_frame").value_or(0);
        msg.includeLLCConfig = args.argAs<int>("include_llc").value_or(0);
        msg.includePDCPConfig = args.argAs<int>("include_pdcp").value_or(0);

        send_rrc(rrc);

        return "pulling...\n";
    }

    std::string on_cmd_reset(bfc::ArgsMap&& args)
    {
        // std::unique_lock<std::shared_mutex> lg(this_mutex);
        return "reset:\n";
    }

    std::string on_cmd_activate(bfc::ArgsMap&& args)
    {
        std::shared_lock<std::shared_mutex> lg(this_mutex);

        RRC rrc;
        rrc.requestID = rrc_req_id++;
        rrc.message = RRC_ActivateRequest{};
        auto& message = std::get<RRC_ActivateRequest>(rrc.message);
        message.llcid = args.argAs<int>("lcid").value_or(0);
        message.activateTx = args.argAs<int>("tx").value_or(0);
        message.activateRx = args.argAs<int>("rx").value_or(0);
        proc_activate_ongoing = true;
        proc_activate_should_activate_rx = message.activateTx;
        proc_activate_should_activate_tx = message.activateRx;
        proc_activate_lcid = message.llcid;

        send_rrc(rrc);
        return "activating...\n";
    }

    std::string on_cmd_deactivate(bfc::ArgsMap&& args)
    {
        // std::unique_lock<std::shared_mutex> lg(this_mutex);
        return "deactivate:\n";
    }

    std::string on_cmd_stop(bfc::ArgsMap&& args)
    {
        stop();
        return "stop:\n";
    }

    std::string on_cmd_log(bfc::ArgsMap&& args)
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

    std::string on_cmd_stats(bfc::ArgsMap&& args)
    {
        auto lcid = args.argAs<int>("lcid").value_or(0);
        std::stringstream ss;
        ss << "stats: \n";
        std::unique_lock<std::shared_mutex> lg(this_mutex);
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
            ss << "\n  tx_ignored_pkt:     " << stats.tx_ignored_pkt.load();
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

        lg.unlock();
        return ss.str();
    }

    void run()
    {
        reactor.run();
    }

    void on_rlf_tx(lcid_t lcid)
    {
        std::unique_lock<std::shared_mutex> lg(this_mutex);
        Logless(*main_logger, RRC_ERR, "ERR | AppRRC | RLF TX lcid=#", (int) lcid);
        auto llc = llcs.at(lcid);
        auto pdcp = pdcps.at(lcid);
        lg.unlock();
        pdcp->set_tx_enabled(false);    
        llc->set_tx_enabled(false);
    }

    void on_rlf_rx(lcid_t lcid)
    {
        std::unique_lock<std::shared_mutex> lg(this_mutex);
        Logless(*main_logger, RRC_ERR, "ERR | AppRRC | RLF RX lcid=#", (int) lcid);
        auto llc = llcs.at(lcid);
        auto pdcp = pdcps.at(lcid);
        lg.unlock();
        pdcp->set_rx_enabled(false);    
        llc->set_rx_enabled(false);
    }

    void perform_tx(size_t payload_size)
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

private:
    void setup_80211_base_frame()
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

    void setup_console()
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
        cmdman.addCommand("exchange", [this](bfc::ArgsMap&& args){return on_cmd_exchange(std::move(args));});
        cmdman.addCommand("reset", [this](bfc::ArgsMap&& args){return on_cmd_reset(std::move(args));});
        cmdman.addCommand("activate", [this](bfc::ArgsMap&& args){return on_cmd_activate(std::move(args));});
        cmdman.addCommand("deactivate", [this](bfc::ArgsMap&& args){return on_cmd_deactivate(std::move(args));});
        cmdman.addCommand("stop", [this](bfc::ArgsMap&& args){return on_cmd_stop(std::move(args));});
        cmdman.addCommand("log", [this](bfc::ArgsMap&& args){return on_cmd_log(std::move(args));});
        cmdman.addCommand("stats", [this](bfc::ArgsMap&& args){return on_cmd_stats(std::move(args));});

        reactor.addReadHandler(console_sock.handle(), [this](){on_console_read();});
    }

    void setup_pdcps()
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

    void setup_llcs()
    {
        Logless(*main_logger, RRC_INF, "INF | AppRRC | Setting up LLCs...");
        for (auto& llc_ : config.llc_configs)
        {
            auto id = llc_.first;
            auto& tx_config = llc_.second;

            ILLC::rx_config_t rx_config{};

            // Copy tx_config initially
            rx_config.crc_type = tx_config.crc_type;
            rx_config.mode = tx_config.mode;

            auto pdcp = pdcps[id];
            auto llc = std::make_shared<LLC>(pdcp, *this, id,
                tx_config, rx_config);

            llcs.emplace(id, llc);
        }
    }

    void setup_eps()
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
                    reactor,
                    *pdcp_it->second);

                ieps.emplace(id, ep);
           }
        }
    }

    void setup_scheduler()
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

        timer_thread = std::thread([this](){timer.run();});
    }

    void setup_rrc()
    {
        rrc_rx_thread =  std::thread([this](){run_rrc_rx();});
        wifi_rx_thread = std::thread([this](){run_wifi_rx();});
    }

    void run_wifi_rx()
    {
        wifi_rx_running = true;
        while (wifi_rx_running)
        {
            int rv = wifi->recv(rx_buff, sizeof(rx_buff));
            if (rv<0)
            {
                Logless(*main_logger, RRC_ERR, "ERR | AppRRC | wifi recv failed! errno=# error=#\n", errno, strerror(errno));
                stop();
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
                continue;
            }

            process_rx_frame(frame80211.frame_body, size);
        }
    }

    void process_rx_frame(uint8_t* start, size_t size)
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

            std::shared_lock<std::shared_mutex> lg(this_mutex);
            auto llc_it = llcs.find(lcid);
            if (llcs.end() == llc_it)
            {
                Logless(*main_logger, RRC_WRN, "WRN | AppRRC | Couldnt find the llc for lcid=#, skipping...", (int)lcid);
                advance_cursor(llc_pdu.get_SIZE());
                continue;
            }
            auto llc = llc_it->second;
            lg.unlock();

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

    void run_rrc_rx()
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
            auto b = pdcp0->to_rx(1000*1000);
            if (b.size())
            {
                RRC rrc;
                cum::per_codec_ctx context((std::byte*)b.data(), b.size());
                decode_per(rrc, context);
                on_rrc(rrc);
            }
        }
    }

    void on_rrc(const RRC& rrc)
    {
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

    template<typename T>
    void fill_from_config(
        int lcid,
        bool include_frame,
        bool include_llc,
        bool include_pdcp,
        T& message)
    {
        std::shared_lock<std::shared_mutex> lg(this_mutex);
        bool has_llc = config.llc_configs.count(lcid);
        bool has_sched = config.scheduling_configs.count(lcid);
        bool has_pdcp = config.pdcp_configs.count(lcid);

        if (include_llc && has_llc)
        {
            auto& llc_src = config.llc_configs.at(lcid);
            message.llcConfig = RRC_LLCConfig{};
            auto& llcConfig = message.llcConfig;
            llcConfig->llcid = lcid;
            llcConfig->txConfig.mode = to_rrc(llc_src.mode);
            llcConfig->txConfig.arqWindowSize = llc_src.arq_window_size;
            llcConfig->txConfig.maxRetxCount = llc_src.max_retx_count;
            llcConfig->txConfig.crcType = to_rrc(llc_src.crc_type);

            llcConfig->schedulingConfig.ndGpduMaxSize = 0;
            llcConfig->schedulingConfig.quanta = 0;
            if (has_sched)
            {
                auto& sched_src = config.scheduling_configs.at(lcid);
                llcConfig->schedulingConfig.ndGpduMaxSize = sched_src.nd_gpdu_max_size;
                llcConfig->schedulingConfig.quanta = sched_src.quanta;
            }
        }

        if (include_pdcp && has_pdcp)
        {
            auto& pdcp_src = config.pdcp_configs.at(lcid);
            message.pdcpConfig = RRC_PDCPConfig{};
            auto& pdcpConfig = message.pdcpConfig;
            pdcpConfig->lcid = lcid;
            // @todo : fill up correctly
            pdcpConfig->type = RRC_EPType::E_RRC_EPType_INTERNAL;
            pdcpConfig->allowSegmentation = pdcp_src.allow_segmentation;
            pdcpConfig->allowReordering = pdcp_src.allow_reordering;
            pdcpConfig->maxSnDistance = pdcp_src.max_sn_distance;
            pdcpConfig->minCommitSize = pdcp_src.min_commit_size;
        }

        if (include_frame)
        {
            message.frameConfig = RRC_FrameConfig{};
            auto& frameConfig = message.frameConfig;
            for (auto& fec_config_src : config.fec_configs)
            {
                frameConfig->fecConfig.emplace_back();
                auto& fecConfig = frameConfig->fecConfig.back();
                fecConfig.threshold = fec_config_src.threshold;
                fecConfig.type = to_rrc(fec_config_src.type);
            }
            message.frameConfig->slotInterval = config.frame_config.slot_interval_us;
            message.frameConfig->framePayloadSize = config.frame_config.frame_payload_size;
        }
    }

    void on_rrc_message(int req_id, const RRC_PullRequest& req)
    {
        RRC rrc;
        rrc.requestID = req_id;
        rrc.message = RRC_PullResponse{};
        auto& message = std::get<RRC_PullResponse>(rrc.message);
        fill_from_config(
            req.lcid,
            req.includeFrameConfig,
            req.includeLLCConfig,
            req.includePDCPConfig,
            message);
        send_rrc(rrc);
    }

    template<typename T>
    void update_peer_config_and_reconfigure_rx(const T& msg)
    {
        std::unique_lock<std::shared_mutex> lg(this_mutex);

        if (msg.frameConfig)
        {
            auto& frameConfig = msg.frameConfig;
            peer_config.frame_config.slot_interval_us = frameConfig->slotInterval;
            peer_config.frame_config.frame_payload_size = frameConfig->framePayloadSize;
        }

        if (msg.llcConfig)
        {
            auto& llcConfig = msg.llcConfig;
            auto& llc_config = peer_config.llc_configs[llcConfig->llcid];
            auto& sched_config = peer_config.scheduling_configs[llcConfig->llcid];

            llc_config.mode = to_config(llcConfig->txConfig.mode);
            llc_config.arq_window_size = llcConfig->txConfig.arqWindowSize;
            llc_config.crc_type = to_config(llcConfig->txConfig.crcType);
            llc_config.max_retx_count = llcConfig->txConfig.maxRetxCount;
            sched_config.nd_gpdu_max_size = llcConfig->schedulingConfig.ndGpduMaxSize;
            sched_config.quanta = llcConfig->schedulingConfig.quanta;

            Logless(*main_logger, RRC_DBG, "DBG | AppRRC | Reconfiguring LLC lcid=\"#\"", (int)msg.llcConfig->llcid);
            ILLC::rx_config_t llc_config_rx{};
            llc_config_rx.crc_type = llc_config.crc_type;
            llc_config_rx.mode = llc_config.mode;
            if (llcs.count(msg.llcConfig->llcid))
            {
                auto& llc = llcs.at(msg.llcConfig->llcid);
                llc->reconfigure(llc_config_rx);
                llc->set_rx_enabled(true);
            }
        }

        if (msg.pdcpConfig)
        {
            auto& pdcpConfig = msg.pdcpConfig;
            auto& tx_config = peer_config.pdcp_configs[msg.pdcpConfig->lcid];
            tx_config.allow_segmentation = pdcpConfig->allowSegmentation;
            tx_config.allow_reordering = pdcpConfig->allowReordering;
            if (pdcpConfig->maxSnDistance)
            {
                tx_config.max_sn_distance = *pdcpConfig->maxSnDistance;
            }
            tx_config.min_commit_size = pdcpConfig->minCommitSize;

            Logless(*main_logger, RRC_DBG, "DBG | AppRRC | Reconfiguring PDCP linked-lcid=\"#\"", (int)msg.llcConfig->llcid);
            IPDCP::rx_config_t pdcp_config_rx{};
            pdcp_config_rx.allow_rlf = tx_config.allow_rlf;
            pdcp_config_rx.allow_segmentation = tx_config.allow_segmentation;
            pdcp_config_rx.allow_reordering = tx_config.allow_reordering;
            pdcp_config_rx.max_sn_distance = tx_config.max_sn_distance;
            if (pdcps.count(msg.pdcpConfig->lcid))
            {
                auto& pdcp = pdcps.at(msg.pdcpConfig->lcid);
                pdcp->reconfigure(pdcp_config_rx);
                pdcp->set_rx_enabled(true);
            }
        }
    }

    void on_rrc_message(int req_id, const RRC_PullResponse& rsp)
    {
        update_peer_config_and_reconfigure_rx(rsp);
    }

    void on_rrc_message(int req_id, const RRC_PushRequest& req)
    {
        update_peer_config_and_reconfigure_rx(req);
        RRC rrc;
        rrc.requestID = req_id;
        rrc.message = RRC_PushResponse{};
        send_rrc(rrc);
    }

    void on_rrc_message(int req_id, const RRC_ExchangeRequest& req)
    {
        update_peer_config_and_reconfigure_rx(req);
        RRC rrc;
        rrc.requestID = req_id;
        rrc.message = RRC_ExchangeResponse{};
        auto& message = std::get<RRC_ExchangeResponse>(rrc.message);
        fill_from_config(
            req.lcid,
            req.frameConfig.has_value(),
            req.llcConfig.has_value(),
            req.pdcpConfig.has_value(),
            message);
        send_rrc(rrc);
    }

    void on_rrc_message(int req_id, const RRC_ExchangeResponse& rsp)
    {
        update_peer_config_and_reconfigure_rx(rsp);
    }

    void on_rrc_message(int req_id, const RRC_PushResponse& msg)
    {
    }

    void on_rrc_message(int req_id, const RRC_ActivateRequest& msg)
    {
        std::unique_lock<std::shared_mutex> lg(this_mutex);
        auto llc_ = llcs.find(msg.llcid);
        auto pdcp_ = pdcps.find(msg.llcid);
        if (msg.activateTx)
        {
            if (pdcps.end() != pdcp_)
            {
                pdcp_->second->set_rx_enabled(true);
            }
            if (llcs.end() != llc_)
            {
                llc_->second->set_rx_enabled(true);
            }
        }
        if (msg.activateRx)
        {
            if (pdcps.end() != pdcp_)
            {
                pdcp_->second->set_tx_enabled(true);
            }
            if (llcs.end() != llc_)
            {
                llc_->second->set_tx_enabled(true);
            }
        }

        RRC rrc;
        rrc.requestID = req_id;
        rrc.message = RRC_ActivateResponse{};
        auto& message = std::get<RRC_ActivateResponse>(rrc.message);
        send_rrc(rrc);
    }

    void on_rrc_message(int req_id, const RRC_ActivateResponse& msg)
    {
        std::unique_lock<std::shared_mutex> lg(this_mutex);
        if (!proc_activate_ongoing)
        {
            return;
        }
        proc_activate_ongoing = false;

        auto llc_ = llcs.find(proc_activate_lcid);
        auto pdcp_ = pdcps.find(proc_activate_lcid);
        if (proc_activate_should_activate_tx)
        {
            if (pdcps.end() != pdcp_)
            {
                pdcp_->second->set_tx_enabled(true);
            }
            if (llcs.end() != llc_)
            {
                llc_->second->set_tx_enabled(true);
            }
        }
        if (proc_activate_should_activate_rx)
        {
            if (pdcps.end() != pdcp_)
            {
                pdcp_->second->set_rx_enabled(true);
            }
            if (llcs.end() != llc_)
            {
                llc_->second->set_rx_enabled(true);
            }
        }
    }

    void send_rrc(const RRC& rrc)
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

    config_t config;
    config_t peer_config;

    bfc::Timer<> timer;
    std::thread timer_thread;
    std::thread wifi_rx_thread;
    std::atomic_bool wifi_rx_running = false;
    uint8_t rrc_req_id = 0;
    std::shared_ptr<IWIFI> wifi;
    TxScheduler tx_scheduler;

    uint8_t console_buff[1024];

    // @note : tx_buff size should be enough for frame_payload_size + 802 and radiotap headers
    uint8_t tx_buff[1024*2];
    uint8_t rx_buff[1024*2];

    winject::radiotap::radiotap_t radiotap;
    winject::ieee_802_11::frame_t tx_frame;

    bfc::UdpSocket console_sock;
    bfc::EpollReactor reactor;
    bfc::CommandManager cmdman;

    std::map<uint8_t, std::shared_ptr<ILLC>> llcs;
    std::map<uint8_t, std::shared_ptr<IPDCP>> pdcps;
    std::map<uint8_t, std::shared_ptr<IEndPoint>> ieps;

    std::thread rrc_rx_thread;
    std::atomic_bool rrc_rx_running = false;

    // RRC Procedure states
    bool proc_activate_ongoing = false;
    bool proc_activate_should_activate_tx = false;
    bool proc_activate_should_activate_rx = false;
    lcid_t proc_activate_lcid = -1;

    std::shared_mutex this_mutex;
};

#endif // __WINJECTUM_APPRRC_HPP__
