#ifndef __WINJECTUM_APPRRC_HPP__
#define __WINJECTUM_APPRRC_HPP__

#include <bfc/CommandManager.hpp>
#include <bfc/Udp.hpp>
#include <bfc/EpollReactor.hpp>
#include <bfc/Timer.hpp>

#include "WIFI.hpp"
#include "IRRC.hpp"
#include "TxScheduler.hpp"

class AppRRC : public IRRC
{
public:
    AppRRC(const config_t& config)
        : config(config)
        , wifi(config.app_config.wifi_device)
        , tx_scheduler(timer, wifi)
    {
        console_sock.bind(bfc::toIp4Port(
            config.app_config.udp_console_host, config.app_config.udp_console_port));

        cmdman.addCommand("push", [this](bfc::ArgsMap&& args){return on_cmd_push(std::move(args));});
        cmdman.addCommand("pull", [this](bfc::ArgsMap&& args){return on_cmd_pull(std::move(args));});
        cmdman.addCommand("reset", [this](bfc::ArgsMap&& args){return on_cmd_reset(std::move(args));});
        cmdman.addCommand("activate", [this](bfc::ArgsMap&& args){return on_cmd_activate(std::move(args));});
        cmdman.addCommand("deactivate", [this](bfc::ArgsMap&& args){return on_cmd_deactivate(std::move(args));});

        reactor.addHandler(console_sock.handle(), [this](){on_console_read();});
    }

    void on_console_read()
    {
        bfc::BufferView bv{console_buff, sizeof(console_buff)};
        bfc::Ip4Port sender_addr;
        auto rv = console_sock.recvfrom(bv, sender_addr);
        if (rv>0)
        {
            console_buff[rv] = 0;
            std::string result = cmdman.executeCommand(std::string_view((char*)console_buff, rv));
            console_sock.sendto(bfc::BufferView((uint8_t*)result.data(), result.size()), sender_addr);
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

    void run()
    {
        reactor.run();
    }

    void on_rlf(lcid_t)
    {
        
    }

private:
    config_t config;
    bfc::Timer<> timer;
    WIFI wifi;
    TxScheduler tx_scheduler;

    uint8_t console_buff[1024];
    bfc::UdpSocket console_sock;
    bfc::EpollReactor reactor;
    bfc::CommandManager cmdman; 
};

#endif // __WINJECTUM_APPRRC_HPP__
