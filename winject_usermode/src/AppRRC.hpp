#ifndef __WINJECTUM_APPRRC_HPP__
#define __WINJECTUM_APPRRC_HPP__

#include "WIFI.hpp"
#include "IRRC.hpp"
#include <bfc/CommandManager.hpp>
#include <bfc/Udp.hpp>
#include <bfc/EpollReactor.hpp>

class AppRRC : public IRRC
{
public:
    AppRRC(const config_t& config)
        : config(config)
        , wifi(config.app_config.wifi_device)
    {
        console_sock.bind(bfc::toIp4Port(
            config.app_config.udp_console_host, config.app_config.udp_console_port));

        cmdman.addCommand("push", nullptr);
        cmdman.addCommand("pull", nullptr);
        cmdman.addCommand("reset", nullptr);
        cmdman.addCommand("activate", nullptr);
        cmdman.addCommand("deactivate", nullptr);

        reactor.addHandler(console_sock.handle(), [this](){on_console_read();});
    }

    void on_console_read()
    {
        bfc::BufferView bv{console_buff, sizeof(console_buff)};
        bfc::Ip4Port sender_addr;
        auto rv = console_sock.recvfrom(bv, sender_addr);
        if (rv>0)
        {
            std::string result = cmdman.executeCommand(std::string_view((char*)console_buff, rv));
            console_sock.sendto(bfc::BufferView(result.data(), result.size()), sender_addr);
        }
    }

    void on_cmd_push(bfc::ArgsMap&& args)
    {
        
    }

    void on_cmd_pull(bfc::ArgsMap&& args)
    {

    }

    void on_cmd_reset(bfc::ArgsMap&& args)
    {}

    void on_cmd_activate(bfc::ArgsMap&& args)
    {}

    void on_cmd_deactivate(bfc::ArgsMap&& args)
    {}

    void run()
    {
        reactor.run();
    }

    void on_rlf(lcid_t)
    {
        
    }

private:
    config_t config;
    WIFI wifi;
    uint8_t console_buff[1024];
    bfc::UdpSocket console_sock;
    bfc::EpollReactor reactor;
    bfc::CommandManager cmdman; 
};

#endif // __WINJECTUM_APPRRC_HPP__
