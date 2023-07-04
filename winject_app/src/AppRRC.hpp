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
    AppRRC(const config_t& config);
    ~AppRRC();

    void stop();

    void run();

    void on_rlf_tx(lcid_t lcid) override;
    void on_rlf_rx(lcid_t lcid) override;

    void initialize_tx(lcid_t) override;
    void perform_tx(size_t payload_size) override;

private:
    void on_console_read();

    std::string on_cmd_exchange(bfc::ArgsMap&& args);
    std::string on_cmd_push(bfc::ArgsMap&& args);
    std::string on_cmd_pull(bfc::ArgsMap&& args);
    std::string on_cmd_stop(bfc::ArgsMap&& args);
    std::string on_cmd_log(bfc::ArgsMap&& args);
    std::string on_cmd_stats(bfc::ArgsMap&& args);

    void setup_80211_base_frame();
    void setup_console();
    void setup_llcs();
    void setup_pdcps();
    void setup_eps();
    void setup_scheduler();
    void setup_rrc();

    void run_wifi_rx();

    void process_rx_frame(uint8_t* start, size_t size);

    void run_rrc_rx();
    void on_rrc_event();
    void notify_rrc_event();
    void on_rrc(const RRC& rrc);

    template<typename T>
    void fill_from_config(
        int lcid,
        bool include_frame,
        bool include_llc,
        bool include_pdcp,
        T& message);

    template<typename T>
    void update_peer_config_and_reconfigure_rx(const T& msg);

    template<typename T>
    void notify_resp_handler(uint8_t request_id, const T& msg);

    void on_rrc_message(int req_id, const RRC_PullRequest& req);
    void on_rrc_message(int req_id, const RRC_PullResponse& rsp);
    void on_rrc_message(int req_id, const RRC_PushRequest& req);
    void on_rrc_message(int req_id, const RRC_ExchangeRequest& req);
    void on_rrc_message(int req_id, const RRC_ExchangeResponse& rsp);
    void on_rrc_message(int req_id, const RRC_PushResponse& msg);
    void send_rrc(const RRC& rrc);

    struct rrc_event_stop_t
    {};

    struct rrc_event_rlf_t
    {
        lcid_t lcid;
        enum mode_e {TX,RX};
        mode_e mode;
    };

    struct rrc_event_setup_t
    {
        lcid_t lcid;
    };

    enum response_index_e {
        E_RRC_PULL_RSP,
        E_RRC_PUSH_RSP,
        E_RRC_EXCH_RSP};
    using response_t = std::variant<RRC_PullResponse,
            RRC_PushResponse, RRC_ExchangeResponse>;

    void on_rrc_event(const rrc_event_stop_t&);
    void on_rrc_event(const rrc_event_rlf_t& rlf);
    void on_rrc_event(const rrc_event_setup_t& setup);

    template<typename T>
    void push_rrc_event(T&& event);

    template <typename T>
    void auto_send_rrc(size_t max_retry, const T& msg,
        std::function<void(uint8_t, const response_t&)> cb_ok = nullptr,
        std::function<void()> cb_fail = nullptr);

    using rrc_event_t = std::variant<
        rrc_event_stop_t,
        rrc_event_rlf_t,
        rrc_event_setup_t>;

    config_t config;
    config_t peer_config;
    std::mutex peer_config_mutex;

    bfc::Timer<> timer;
    bfc::Timer<> timer2;
    std::thread timer_thread;
    std::thread timer2_thread;
    std::thread wifi_rx_thread;
    std::atomic_bool wifi_rx_running = false;
    std::atomic_uint8_t rrc_req_id = 0;
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
    std::map<uint8_t, std::shared_ptr<IEndPoint>> eps;

    std::thread rrc_rx_thread;
    std::atomic_bool rrc_rx_running = false;

    std::deque<rrc_event_t> rrc_events;
    std::mutex rrc_event_mutex;
    int rrc_event_fd;

    struct rrc_request_context_t
    {
        uint64_t exp_timer;
        RRC request;
        std::function<void(uint8_t req_id, const response_t&)>
            handler = nullptr;
    };

    std::map<uint8_t, rrc_request_context_t> rrc_requests;
    std::mutex rrc_requests_mutex;

    struct lc_rrc_context_t
    {
        enum config_state_e {
            E_CFG_STATE_NULL,
            E_CFG_STATE_PENDING,
            E_CFG_STATE_CONFIGURED
        };

        config_state_e tx_config_state = E_CFG_STATE_NULL;
    };

    std::map<lcid_t, lc_rrc_context_t> channel_rrc_contexts;
    std::mutex channel_rrc_contexts_mutex;
};

#endif // __WINJECTUM_APPRRC_HPP__
