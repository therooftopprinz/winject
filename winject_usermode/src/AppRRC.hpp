#ifndef __WINJECTUM_APPRRC_HPP__
#define __WINJECTUM_APPRRC_HPP__

#include <atomic>

#include <bfc/CommandManager.hpp>
#include <bfc/Udp.hpp>
#include <bfc/EpollReactor.hpp>
#include <bfc/Timer.hpp>

#include <winject/802_11.hpp>
#include <winject/radiotap.hpp>

#include "Logger.hpp"

#include "WIFI.hpp"
#include "IRRC.hpp"
#include "TxScheduler.hpp"

#include "LLC.hpp"
#include "PDCP.hpp"

class AppRRC : public IRRC
{
public:
    AppRRC(const config_t& config)
        : config(config)
        , wifi(config.app_config.wifi_device)
        , tx_scheduler(timer, wifi)
    {
        setup_80211_base_frame();
        setup_console();
        setup_pdcps();
        setup_llcs();
        setup_eps();

        setup_scheduler();
        timer_thread = std::thread([this](){timer.run();});
        rx_thread = std::thread([this](){run_rx();});
    }

    ~AppRRC()
    {
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | AppRRC stopping...");
        rx_thread.join();
        timer_thread.join();
    }

    void stop()
    {
        rx_thread_running = false;
        reactor.stop();
        timer.stop();
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

            Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | console: #", result.c_str());

            reactor.addWriteHandler(console_sock.handle(), [pFd, this, result, sender_addr]()
                {
                    console_sock.sendto(bfc::BufferView((uint8_t*)result.data(), result.size()), sender_addr);
                    reactor.removeWriteHandler(pFd);
                });
        }
    }

    std::string on_cmd_push(bfc::ArgsMap&& args)
    {
        return "push:\n";
    }

    std::string on_cmd_pull(bfc::ArgsMap&& args)
    {
        return "pull:\n";
    }

    std::string on_cmd_reset(bfc::ArgsMap&& args)
    {
        return "reset:\n";
    }

    std::string on_cmd_activate(bfc::ArgsMap&& args)
    {
        return "activate:\n";
    }

    std::string on_cmd_deactivate(bfc::ArgsMap&& args)
    {
        return "deactivate:\n";
    }

    std::string on_cmd_stop(bfc::ArgsMap&& args)
    {
        stop();
        return "stop:\n";
    }

    void run()
    {
        reactor.run();
    }

    void on_rlf(lcid_t)
    {
        
    }

private:
    void setup_80211_base_frame()
    {
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | Generating base frame for TX...");
        radiotap = winject::radiotap::radiotap_t(tx_buff);
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_FLAGS;
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_RATE;
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_TX_FLAGS;
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_MCS;
        radiotap.rescan(true);
        radiotap.header->version = 0;
        radiotap.flags->flags |= winject::radiotap::flags_t::E_FLAGS_FCS;
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
        srcss << std::hex << config.app_config.hwsrc;
        dstss << std::hex << config.app_config.hwdst;
        srcss >> srcraw;
        dstss >> dstraw;
        uint64_t address2 = (srcraw << 24) | (dstraw);
        tx_frame.address2->set(address2); // IBSS Source
        tx_frame.address3->set(0xDEADBEEFCAFE); // IBSS BSSID

        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | radiotap:\n#", winject::radiotap::to_string(radiotap).c_str());
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | 802.11:\n#", winject::ieee_802_11::to_string(tx_frame).c_str());
    }

    void setup_console()
    {
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | Setting up console...");
        if (console_sock.bind(bfc::toIp4Port(
            config.app_config.udp_console_host,
            config.app_config.udp_console_port)) < 0)
        {
            Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | Bind error(_) console is now disabled!", strerror(errno));
            return;
        }

        cmdman.addCommand("push", [this](bfc::ArgsMap&& args){return on_cmd_push(std::move(args));});
        cmdman.addCommand("pull", [this](bfc::ArgsMap&& args){return on_cmd_pull(std::move(args));});
        cmdman.addCommand("reset", [this](bfc::ArgsMap&& args){return on_cmd_reset(std::move(args));});
        cmdman.addCommand("activate", [this](bfc::ArgsMap&& args){return on_cmd_activate(std::move(args));});
        cmdman.addCommand("deactivate", [this](bfc::ArgsMap&& args){return on_cmd_deactivate(std::move(args));});
        cmdman.addCommand("stop", [this](bfc::ArgsMap&& args){return on_cmd_stop(std::move(args));});

        reactor.addReadHandler(console_sock.handle(), [this](){on_console_read();});
    }

    void setup_pdcps()
    {
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | Setting up PDCPs...");
        for (auto& pdcp_ : config.pdcp_configs)
        {
            auto id = pdcp_.first;
            auto& pdcp_config = pdcp_.second;

            IPDCP::tx_config_t tx_config{};
            IPDCP::rx_config_t rx_config{};

            tx_config.allow_segmentation = pdcp_config.allow_segmentation;
            tx_config.min_commit_size = pdcp_config.min_commit_size;

            rx_config.allow_segmentation = pdcp_config.allow_segmentation;

            auto pdcp = std::make_shared<PDCP>(tx_config, rx_config);
            pdcps.emplace(id, pdcp);
        }
    }

    void setup_llcs()
    {
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | Setting up LLCs...");
        for (auto& llc_ : config.llc_configs)
        {
            auto id = llc_.first;
            auto& llc_config = llc_.second;

            ILLC::tx_config_t tx_config{};
            ILLC::rx_config_t rx_config{};

            tx_config.mode = llc_config.mode;
            tx_config.arq_window_size = llc_config.arq_window_size;
            tx_config.max_retx_count =  llc_config.max_retx_count;
            tx_config.crc_type = tx_config.crc_type;

            rx_config.crc_type = tx_config.crc_type;

            auto pdcp = pdcps[id];
            auto llc = std::make_shared<LLC>(pdcp, *this, id,
                tx_config, rx_config);
            llcs.emplace(id, llc);
        }
    }

    void setup_eps()
    {
    }

    void setup_scheduler()
    {
        Logless(*main_logger, Logger::DEBUG, "DBG | AppRRC | Setting up scheduler...");
        ITxScheduler::buffer_config_t buffer_config{};
        buffer_config.buffer = tx_buff;
        buffer_config.buffer_size = sizeof(tx_buff);
        buffer_config.header_size = tx_frame.frame_body - tx_buff;
        buffer_config.frame_payload_max_size = tx_frame.end() - tx_frame.frame_body;

        tx_scheduler.reconfigure(buffer_config);

        for (auto& llc_ : llcs)
        {
            auto id = llc_.first;
            auto llc = llc_.second;
            tx_scheduler.add_llc(id, llc.get());
            ITxScheduler::llc_scheduling_config_t llc_sched_config{};
            llc_sched_config.llcid = id;
            auto sched_config_ = config.scheduling_configs.find(id);
            if (sched_config_ != config.scheduling_configs.end())
            {
                auto& sched_config = sched_config_->second;
                sched_config.nd_gpdu_max_size = sched_config.nd_gpdu_max_size;
                sched_config.quanta = sched_config.quanta;
            }
            tx_scheduler.reconfigure(llc_sched_config);
        }

        ITxScheduler::frame_scheduling_config_t frame_config{};
        frame_config.fec_type = fec_type_e::E_FEC_TYPE_NONE;
        frame_config.slot_interval_us = config.frame_info.slot_interval_us;

        tx_scheduler.reconfigure(frame_config);
    }

    void run_rx()
    {
        rx_thread_running = true;
        while (rx_thread_running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    config_t config;
    bfc::Timer<> timer;
    std::thread timer_thread;
    std::thread rx_thread;
    std::atomic_bool rx_thread_running = true;

    WIFI wifi;
    TxScheduler tx_scheduler;

    uint8_t console_buff[1024];

    uint8_t tx_buff[1024];
    winject::radiotap::radiotap_t radiotap;
    winject::ieee_802_11::frame_t tx_frame;

    bfc::UdpSocket console_sock;
    bfc::EpollReactor reactor;
    bfc::CommandManager cmdman;

    std::map<uint8_t, std::shared_ptr<LLC>> llcs;
    std::map<uint8_t, std::shared_ptr<PDCP>> pdcps;

};

#endif // __WINJECTUM_APPRRC_HPP__
