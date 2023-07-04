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

AppRRC::~AppRRC()
{
    close(rrc_event_fd); 

    stop();

    Logless(*main_logger, RRC_DBG, "DBG | AppRRC | AppRRC stopping...");

    if (wifi_rx_thread.joinable())
    {
        wifi_rx_thread.join();
    }

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
    wifi_rx_running = false;
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

std::string AppRRC::on_cmd_exchange(bfc::ArgsMap&& args)
{

    RRC rrc;
    rrc.requestID = rrc_req_id.fetch_add(1);
    rrc.message = RRC_ExchangeRequest{};
    auto& message = std::get<RRC_ExchangeRequest>(rrc.message);

    fill_from_config(args.argAs<int>("lcid").value_or(0xF),
        false, true, true, message);

    auto_send_rrc(10, rrc);
    return "exchanging...\n";
}

std::string AppRRC::on_cmd_push(bfc::ArgsMap&& args)
{
    RRC rrc;
    rrc.requestID = rrc_req_id.fetch_add(1);
    rrc.message = RRC_PushRequest{};
    auto& message = std::get<RRC_PushRequest>(rrc.message);

    fill_from_config(args.argAs<int>("lcid").value_or(0xF),
        false, true, true, message);

    auto_send_rrc(10, rrc);
    return "pushing...\n";
}

std::string AppRRC::on_cmd_pull(bfc::ArgsMap&& args)
{
    RRC rrc;
    rrc.requestID = rrc_req_id.fetch_add(1);
    rrc.message = RRC_PullRequest{};
    auto& pull_request = std::get<RRC_PullRequest>(rrc.message);
    pull_request.lcid = args.argAs<int>("lcid").value_or(0);
    pull_request.includeLLCConfig = true;
    pull_request.includePDCPConfig = true;

    auto_send_rrc(10, rrc);
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

void AppRRC::on_rlf_tx(lcid_t lcid)
{
    push_rrc_event(rrc_event_rlf_t{lcid, rrc_event_rlf_t::TX});
}

void AppRRC::on_rlf_rx(lcid_t lcid)
{
    push_rrc_event(rrc_event_rlf_t{lcid, rrc_event_rlf_t::RX});
}

void AppRRC::initialize_tx(lcid_t lcid)
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

    cmdman.addCommand("exchange", [this](bfc::ArgsMap&& args){return on_cmd_exchange(std::move(args));});
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
    for (auto& llc_ : config.llc_configs)
    {

        auto id = llc_.first;
        auto& tx_config = llc_.second;

        channel_rrc_contexts.emplace(id, lc_rrc_context_t{});

        ILLC::rx_config_t rx_config{};

        // Copy tx_config initially
        rx_config.crc_type = tx_config.crc_type;
        rx_config.mode = tx_config.mode;

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
    wifi_rx_thread = std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_WIFI_RX");
        run_wifi_rx();});
    timer2_thread = std::thread([this](){
        pthread_setname_np(pthread_self(), "RRC_TIMER2");
        timer2.run();});
}

void AppRRC::run_wifi_rx()
{
    wifi_rx_running = true;
    while (wifi_rx_running)
    {
        int rv = wifi->recv(rx_buff, sizeof(rx_buff));

        if (EAGAIN == errno || EWOULDBLOCK == errno)
        {
            continue;
        }
        if (rv<0)
        {
            Logless(*main_logger, RRC_ERR, "ERR | AppRRC | wifi recv failed! errno=# error=#", errno, strerror(errno));
            continue;
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
    for (auto& event : rrc_events)
    {
        std::visit([this](auto&& event){on_rrc_event(event);},
            event);
    }
    rrc_events.clear();
}

void AppRRC::notify_rrc_event()
{
    uint64_t one = 1;
    write(rrc_event_fd, &one, sizeof(one));
}

void AppRRC::on_rrc(const RRC& rrc)
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
void AppRRC::fill_from_config(
    int lcid,
    bool include_frame,
    bool include_llc,
    bool include_pdcp,
    T& message)
{
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
        pdcpConfig->allowRLF = pdcp_src.allow_rlf;
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

template<typename T>
void AppRRC::update_peer_config_and_reconfigure_rx(const T& msg)
{
    if (msg.frameConfig)
    {
        std::unique_lock<std::mutex> lg(peer_config_mutex);
        auto& frameConfig = msg.frameConfig;
        peer_config.frame_config.slot_interval_us = frameConfig->slotInterval;
        peer_config.frame_config.frame_payload_size = frameConfig->framePayloadSize;
    }

    if (msg.llcConfig)
    {
        std::unique_lock<std::mutex> lg(peer_config_mutex);
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
        std::unique_lock<std::mutex> lg(peer_config_mutex);
        auto& pdcpConfig = msg.pdcpConfig;
        auto& tx_config = peer_config.pdcp_configs[msg.pdcpConfig->lcid];

        tx_config.allow_rlf = pdcpConfig->allowRLF;
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

template<typename T>
void AppRRC::notify_resp_handler(uint8_t request_id, const T& msg)
{
    std::unique_lock<std::mutex> lg(rrc_requests_mutex);
    auto rrc_requests_it = rrc_requests.find(request_id);

    if (rrc_requests.end() == rrc_requests_it)
    {
        return;
    }

    auto rrc_request = std::move(rrc_requests_it->second);
    rrc_requests.erase(request_id);
    lg.unlock();

    timer2.cancel(rrc_request.exp_timer);
    Logless(*main_logger, RRC_TRC,
        "TRC | AppRRC | auto send timer2 cancel id=#, req_id=#",
        rrc_request.exp_timer,
        (int) request_id);


    if (rrc_request.handler)
    {
        rrc_request.handler(request_id, msg);
    }
}

void AppRRC::on_rrc_message(int req_id, const RRC_PullRequest& req)
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

void AppRRC::on_rrc_message(int req_id, const RRC_PullResponse& rsp)
{
    update_peer_config_and_reconfigure_rx(rsp);
    notify_resp_handler(req_id, rsp);
}

void AppRRC::on_rrc_message(int req_id, const RRC_PushRequest& req)
{
    update_peer_config_and_reconfigure_rx(req);
    RRC rrc;
    rrc.requestID = req_id;
    rrc.message = RRC_PushResponse{};
    send_rrc(rrc);
}

void AppRRC::on_rrc_message(int req_id, const RRC_PushResponse& msg)
{
    notify_resp_handler(req_id, msg);
}

void AppRRC::on_rrc_message(int req_id, const RRC_ExchangeRequest& req)
{
    update_peer_config_and_reconfigure_rx(req);
    RRC rrc;
    rrc.requestID = req_id;
    rrc.message = RRC_ExchangeResponse{};
    auto& message = std::get<RRC_ExchangeResponse>(rrc.message);

    if (req.frameConfig.has_value())
        fill_from_config(0, true, false, false, message);
    if (req.pdcpConfig.has_value())
        fill_from_config(req.pdcpConfig->lcid, false, false, true, message);
    if (req.llcConfig.has_value())
        fill_from_config(req.llcConfig->llcid, false, true, false, message);

    send_rrc(rrc);
}

void AppRRC::on_rrc_message(int req_id, const RRC_ExchangeResponse& rsp)
{
    update_peer_config_and_reconfigure_rx(rsp);
    notify_resp_handler(req_id, rsp);
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

void AppRRC::on_rrc_event(const rrc_event_stop_t&)
{
    stop();
}

void AppRRC::on_rrc_event(const rrc_event_rlf_t& rlf)
{
    auto lcid = rlf.lcid;
    if (rrc_event_rlf_t::TX == rlf.mode)
    {
        Logless(*main_logger, RRC_ERR, "ERR | AppRRC | RLF TX lcid=#", (int) lcid);
        auto llc = llcs.at(lcid);
        auto pdcp = pdcps.at(lcid);
        auto ep = eps.at(lcid);

        ep->set_tx_enabled(false);
        pdcp->set_tx_enabled(false);
        llc->set_tx_enabled(false);
    }
    else
    {
        Logless(*main_logger, RRC_ERR, "ERR | AppRRC | RLF RX lcid=#",
            (int) lcid);
        auto llc = llcs.at(lcid);
        auto pdcp = pdcps.at(lcid);
        auto ep = eps.at(lcid);

        ep->set_rx_enabled(false);
        pdcp->set_rx_enabled(false);
        llc->set_rx_enabled(false);
    }
}

void AppRRC::on_rrc_event(const rrc_event_setup_t& setup)
{
    auto lcid = setup.lcid;
    if (!llcs.count(lcid))
    {
        Logless(*main_logger, RRC_ERR,
            "ERR | AppRRC | can't setup lcid=# for TX, LLC not found",
            (int) lcid);
        return;
    }

    auto& llc = llcs.at(lcid);
    auto llc_tx_cfg = llc->get_tx_confg();

    std::unique_lock<std::mutex> lg(channel_rrc_contexts_mutex);
    auto& channel_ctx = channel_rrc_contexts[setup.lcid];

    Logless(*main_logger, RRC_ERR, "ERR | AppRRC | Setting up lcid=# for TX",
        (int) lcid);

    if (lc_rrc_context_t::E_CFG_STATE_PENDING == channel_ctx.tx_config_state)
    {
        return;
    }
    
    channel_ctx.tx_config_state = lc_rrc_context_t::E_CFG_STATE_PENDING;
    lg.unlock();

    RRC rrc;
    rrc.requestID = rrc_req_id.fetch_add(1);

    if (ILLC::E_TX_MODE_AM == llc_tx_cfg.mode)
    {
        rrc.message = RRC_ExchangeRequest{};
        auto& message = std::get<RRC_ExchangeRequest>(rrc.message);
        fill_from_config(lcid, false, true, true, message);
    }
    else
    {
        rrc.message = RRC_PushRequest{};
        auto& message = std::get<RRC_PushRequest>(rrc.message);
        fill_from_config(lcid, false, true, true, message);
    }

    auto on_success = [this, lcid](uint8_t req_id, const response_t& msg) {
            if (!(E_RRC_PUSH_RSP == msg.index() ||
                E_RRC_EXCH_RSP == msg.index()))
            {
                return;
            }
            auto llc = llcs.at(lcid);
            auto pdcp = pdcps.at(lcid);
            auto ep = eps.at(lcid);
            llc->set_tx_enabled(true);
            pdcp->set_tx_enabled(true);
            ep->set_tx_enabled(true);

            Logless(*main_logger, RRC_ERR, "ERR | AppRRC | lcid=# has been setup!",
                (int) lcid);

            std::unique_lock<std::mutex> lg(channel_rrc_contexts_mutex);
            auto& channel_ctx = channel_rrc_contexts[lcid];
            channel_ctx.tx_config_state = lc_rrc_context_t::E_CFG_STATE_CONFIGURED;
        };

    auto on_fail = [this, lcid]() {
            Logless(*main_logger, RRC_ERR, "ERR | AppRRC | lcid=# has failed to setup!",
                (int) lcid);
            std::unique_lock<std::mutex> lg(channel_rrc_contexts_mutex);
            auto& channel_ctx = channel_rrc_contexts[lcid];
            channel_ctx.tx_config_state = lc_rrc_context_t::E_CFG_STATE_NULL;
        };

    auto_send_rrc(3, rrc, on_success, on_fail);
}

template <typename T>
void AppRRC::auto_send_rrc(size_t max_retry, const T& rrc,
        std::function<void(uint8_t, const response_t&)> cb_ok,
        std::function<void()> cb_fail)
{
    Logless(*main_logger, RRC_ERR,
        "ERR | AppRRC | auto send rrc req_id=# remain_retry=#",
        (int) rrc.requestID,
        max_retry);

    if (!max_retry)
    {
        if (cb_fail)
        {
            cb_fail();
        }
        return;
    }

    auto expiry_handler = [this, max_retry, rrc, cb_ok, cb_fail]() {
            Logless(*main_logger, RRC_ERR,
                "ERR | AppRRC | auto send expired id=# remain_retry=#",
                (int) rrc.requestID,
                max_retry);

            std::unique_lock<std::mutex> lg(rrc_requests_mutex);
            auto rrc_requests_it = rrc_requests.find(rrc.requestID);
            if (rrc_requests.end() == rrc_requests_it)
            {
                Logless(*main_logger, RRC_WRN,
                    "WRN | AppRRC | auto send rrc req_id=# remain_retry=#",
                    (int) rrc.requestID,
                    max_retry);

                return;
            }

            rrc_requests.erase(rrc.requestID);
            lg.unlock();

            auto new_rrc = rrc;
            new_rrc.requestID = rrc_req_id.fetch_add(1);
            auto_send_rrc(max_retry-1, new_rrc, cb_ok, cb_fail);
        };

    // @todo configurable rrc timeout
    auto exp_id = timer2.schedule(std::chrono::nanoseconds(1000*1000*10),
        expiry_handler);

    Logless(*main_logger, RRC_TRC,
        "TRC | AppRRC | auto send timer2 scheduled id=#, req_id=#",
        exp_id,
        (int) rrc.requestID);

    std::unique_lock<std::mutex> lg(rrc_requests_mutex);
    rrc_requests.erase(rrc.requestID);
    auto res = rrc_requests.emplace(rrc.requestID, rrc_request_context_t{});
    auto& rrc_request = res.first->second;

    rrc_request.exp_timer = exp_id;
    rrc_request.handler = [this, cb_ok](uint8_t req_id, const response_t& rsp) {
            std::unique_lock<std::mutex> lg(rrc_requests_mutex);
            rrc_requests.erase(req_id);
            if (cb_ok)
            {
                cb_ok(req_id, rsp);
            }
        };
    lg.unlock();

    send_rrc(rrc);
}

template<typename T>
void AppRRC::push_rrc_event(T&& event)
{
    std::unique_lock<std::mutex> lg(rrc_event_mutex);
    rrc_events.emplace_back(std::forward<T&&>(event));
    notify_rrc_event();
}
