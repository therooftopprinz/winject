#ifndef __LCRRC_HPP__
#define __LCRRC_HPP__

#include <mutex>
#include "Logger.hpp"

#include "bfc/ITimer.hpp"


#include "ILCRRC.hpp"
#include "rrc_utils.hpp"


class LCRRC : public ILCRRC
{
public:
    LCRRC(
        ILLC& llc,
        IPDCP& pdcp,
        IRRC& rrc,
        IEndPoint& ep,
        bfc::ITimer& timer
    )
        :llc(llc)
        ,pdcp(pdcp)
        ,rrc(rrc)
        ,ep(ep)
        ,timer(timer)
    {}

    void check_response(int req_id)
    {
        std::unique_lock<std::mutex> lg(rrc_requests_mutex);
        auto rrc_requests_it = rrc_requests.find(req_id);

        if (rrc_requests.end() == rrc_requests_it)
        {
            return;
        }

        auto rrc_request = std::move(rrc_requests_it->second);
        rrc_requests.erase(req_id);
        lg.unlock();

        timer.cancel(rrc_request.exp_timer);
        Logless(*main_logger, RRC_TRC,
            "TRC | LCRRC# | auto send timer cancel id=#, req_id=#",
            (int) llc.get_lcid(),
            rrc_request.exp_timer,
            (int) req_id);
    }

    void on_rrc_message(int req_id, const RRC_PullRequest& req) override
    {
        // RRC rrc;
        // rrc.requestID = req_id;
        // rrc.message = RRC_PullResponse{};
        // auto& message = std::get<RRC_PullResponse>(rrc.message);
        // fill_from_config(
        //     req.lcid,
        //     req.includeLLCConfig,
        //     req.includePDCPConfig,
        //     message);
        // send_rrc(rrc);
    }

    void on_rrc_message(int req_id, const RRC_PullResponse& rsp) override
    {
        check_response(req_id);
        // update_peer_config_and_reconfigure_rx(rsp);
        // notify_resp_handler(req_id, rsp);
    }

    void on_rrc_message(int req_id, const RRC_PushRequest& req) override
    {
        auto llc_tx_cfg = llc.get_tx_confg();

        if (ILLC::E_TX_MODE_TM == llc_tx_cfg.mode)
        {
            return;
        }

        if (ILLC::E_TX_MODE_AM != to_config(req.llcConfig.txConfig.mode))
        {
            Logless(*main_logger, RRC_ERR,
                "ERR | LCRRC# | on_push_req (ignored: mismatch mode)",
                (int) llc.get_lcid());
            return;
        }

        if (E_TXRX_STATE_ACTIVE == tx_state)
        {
            on_rlf();
            return;
        }

        if (E_TXRX_STATE_NOT_CONFIGURED == tx_state)
        {
            Logless(*main_logger, RRC_ERR,
                "ERR | LCRRC# | on_push_req (peer init)",
                (int) llc.get_lcid());
            reconfigure_rx(req);
            send_push_req();
            send_push_resp(req_id);
            change_rx_state(E_TXRX_STATE_CONFIGURED);
            return;
        }

        Logless(*main_logger, RRC_ERR,
            "ERR | LCRRC# | on_push_req (ignored: in progress)",
                (int) llc.get_lcid());
        send_push_resp(req_id);
        return;
    }

    void on_rrc_message(int req_id, const RRC_PushResponse& msg) override
    {
        check_response(req_id);

        auto llc_tx_cfg = llc.get_tx_confg();

        if (ILLC::E_TX_MODE_TM == llc_tx_cfg.mode)
        {
            return;
        }

        if (E_TXRX_STATE_NOT_CONFIGURED == tx_state)
        {
            Logless(*main_logger, RRC_ERR,
                "ERR | LCRRC# | on_push_rsp (ignored: unexpected)",
                (int) llc.get_lcid());
            return;
        }

        if (E_TXRX_STATE_CONFIGURED == tx_state ||
            E_TXRX_STATE_ACTIVE == tx_state)
        {
            Logless(*main_logger, RRC_ERR,
                "ERR | LCRRC# | on_push_rsp (ignored: retransmitted)",
                (int) llc.get_lcid());
            send_push_resp(req_id);
            return;
        }

        Logless(*main_logger, RRC_ERR,
            "ERR | LCRRC# | on_push_rsp (ignored: unexpected)",
            (int) llc.get_lcid());

        auto try_activate = [this](){
            if (E_TXRX_STATE_CONFIGURED == tx_state ||
                E_TXRX_STATE_CONFIGURED == rx_state)
            {
                activate();
            }
        };

        if (E_TXRX_STATE_CONFIGURED == tx_state ||
            E_TXRX_STATE_CONFIGURED == rx_state)
        {
            activate();
            return;
        }

        timer.schedule(std::chrono::nanoseconds(1000*1000*1000), try_activate);
    }

    void on_init() override
    {
        auto lcid = llc.get_lcid();
        auto llc_tx_cfg = llc.get_tx_confg();

        if (ILLC::E_TX_MODE_AM == llc_tx_cfg.mode)
        {
            on_init_am();
        }
        else
        {
            on_init_tm();
        }
    }

    void on_rlf() override
    {
        auto lcid = llc.get_lcid();

        Logless(*main_logger, RRC_ERR, "ERR | LCRRC# | RLF lcid=#",
            (int) llc.get_lcid(),
            (int) lcid);

        change_tx_state(E_TXRX_STATE_NOT_CONFIGURED);
        change_rx_state(E_TXRX_STATE_NOT_CONFIGURED);

        ep.set_tx_enabled(false);
        pdcp.set_tx_enabled(false);
        llc.set_tx_enabled(false);

        ep.set_rx_enabled(false);
        pdcp.set_rx_enabled(false);
        llc.set_rx_enabled(false);
    }

private:
    void activate()
    {
        Logless(*main_logger, RRC_ERR, "ERR | LCRRC# | Activated lcid=#",
            (int) llc.get_lcid(),
            (int) llc.get_lcid());

        pdcp.set_tx_enabled(true);
        ep.set_tx_enabled(true);
        change_tx_state(E_TXRX_STATE_ACTIVE);

        pdcp.set_rx_enabled(true);
        ep.set_rx_enabled(true);  
        change_rx_state(E_TXRX_STATE_ACTIVE);

        llc.set_tx_enabled(true);
        llc.set_rx_enabled(true);
    }

    void send_push_req()
    {
        RRC msg;
        msg.requestID = rrc.allocate_req_id();
        msg.message = RRC_PushRequest{};

        auto& request = std::get<RRC_PushRequest>(msg.message);

        request.lcid = llc.get_lcid();

        fill_from_config(request);

        auto on_push_fail =  [this](){
                Logless(*main_logger, RRC_ERR,
                    "ERR | LCRRC# | push_req failed, forcing rlf!",
                    (int) llc.get_lcid(),
                    (int) llc.get_lcid());
                on_rlf();
            };

        auto_send_rrc(3, msg, on_push_fail);
    }

    void send_push_resp(uint8_t req_id)
    {
        RRC msg;
        msg.requestID = req_id;
        msg.message = RRC_PushResponse{};
        auto& response = std::get<RRC_PushResponse>(msg.message);
        response.lcid = llc.get_lcid();
        rrc.send_rrc(msg);
    }

    enum txrx_state_e {
        E_TXRX_STATE_NOT_CONFIGURED, 
        E_TXRX_STATE_CONFIGURING,
        E_TXRX_STATE_CONFIGURED,
        E_TXRX_STATE_ACTIVE};

    const char* state_str[4] = {
        "E_TXRX_STATE_NOT_CONFIGURED", 
        "E_TXRX_STATE_CONFIGURING",
        "E_TXRX_STATE_CONFIGURED",
        "E_TXRX_STATE_ACTIVE"};

    void change_tx_state(txrx_state_e new_state)
    {
        Logless(*main_logger, RRC_ERR,
            "ERR | LCRRC# | Change TX state old=# new=#",
            (int) llc.get_lcid(),
            state_str[tx_state],
            state_str[new_state]);

        tx_state = new_state;
    }


    void change_rx_state(txrx_state_e new_state)
    {
        Logless(*main_logger, RRC_ERR,
            "ERR | LCRRC# | Change RX state old=# new=#",
            (int) llc.get_lcid(),
            state_str[tx_state],
            state_str[new_state]);

        rx_state = new_state;
    }

    void on_init_am()
    {
        if (E_TXRX_STATE_CONFIGURING == tx_state ||
            E_TXRX_STATE_CONFIGURED  == tx_state ||
            E_TXRX_STATE_ACTIVE == tx_state)
        {
            return;
        }

        Logless(*main_logger, RRC_ERR,
            "ERR | LCRRC# | on_init_am (self init)",
            (int) llc.get_lcid());

        change_tx_state(E_TXRX_STATE_CONFIGURING);
        send_push_req();
    }

    void on_init_tm()
    {
        // @todo Implement TM
        return;
    }

    template<typename T>
    void reconfigure_rx(const T& msg)
    {
        auto& lcid = msg.lcid;
        {
            auto& llcConfig = msg.llcConfig;

            ILLC::rx_config_t llc_config_rx{};

            llc_config_rx.mode = to_config(llcConfig.txConfig.mode);
            llc_config_rx.crc_type = to_config(llcConfig.txConfig.crcType);

            Logless(*main_logger, RRC_DBG,
                "DBG | LCRRC# | Reconfiguring LLC lcid=\"#\"",
                (int) llc.get_lcid(),
                (int)lcid);

            llc.reconfigure(llc_config_rx);
            llc.set_rx_enabled(false);
        }
        {
            auto& pdcpConfig = msg.pdcpConfig;

            IPDCP::rx_config_t pdcp_config_rx{};

            pdcp_config_rx.allow_rlf = pdcpConfig.allowRLF;
            pdcp_config_rx.allow_segmentation = pdcpConfig.allowSegmentation;
            pdcp_config_rx.allow_reordering = pdcpConfig.allowReordering;

            if (pdcpConfig.maxSnDistance)
            {
                pdcp_config_rx.max_sn_distance = *pdcpConfig.maxSnDistance;
            }

            Logless(*main_logger, RRC_DBG,
                "DBG | LCRRC# | Reconfiguring PDCP linked-lcid=\"#\"",
                (int) llc.get_lcid(),
                (int)lcid);

            pdcp.reconfigure(pdcp_config_rx);
            pdcp.set_rx_enabled(false);
        }
    }

    template<typename T>
    void fill_from_config(T& message)
    {
        auto llc_src = llc.get_tx_confg();
        auto& llcConfig = message.llcConfig;
        llcConfig.txConfig.mode = to_rrc(llc_src.mode);
        llcConfig.txConfig.crcType = to_rrc(llc_src.crc_type);

        auto pdcp_src = pdcp.get_tx_config();
        auto& pdcpConfig = message.pdcpConfig;

        pdcpConfig.allowRLF = pdcp_src.allow_rlf;
        pdcpConfig.allowSegmentation = pdcp_src.allow_segmentation;
        pdcpConfig.allowReordering = pdcp_src.allow_reordering;
        pdcpConfig.maxSnDistance = pdcp_src.max_sn_distance;
    }

    template <typename T>
    void auto_send_rrc(size_t max_retry, const T& msg,
        std::function<void()> cb_fail)
    {
        Logless(*main_logger, RRC_ERR,
            "ERR | LCRRC# | auto send rrc req_id=# remain_retry=#",
            (int) llc.get_lcid(),
            (int) msg.requestID,
            max_retry);

        if (!max_retry)
        {
            if (cb_fail)
            {
                cb_fail();
            }
            return;
        }

        auto expiry_handler = [this, max_retry, msg, cb_fail]() {
                Logless(*main_logger, RRC_ERR,
                    "ERR | LCRRC# | auto send expired id=# remain_retry=#",
                    (int) llc.get_lcid(),
                    (int) msg.requestID,
                    max_retry);

                std::unique_lock<std::mutex> lg(rrc_requests_mutex);
                auto rrc_requests_it = rrc_requests.find(msg.requestID);
                if (rrc_requests.end() == rrc_requests_it)
                {
                    Logless(*main_logger, RRC_WRN,
                        "WRN | LCRRC# | auto send rrc req_id=# remain_retry=#",
                        (int) llc.get_lcid(),
                        (int) msg.requestID,
                        max_retry);

                    return;
                }

                rrc_requests.erase(msg.requestID);
                lg.unlock();

                auto new_rrc = msg;
                new_rrc.requestID = rrc.allocate_req_id();
                auto_send_rrc(max_retry-1, new_rrc, cb_fail);
            };

        // @todo configurable rrc timeout
        auto exp_id = timer.schedule(std::chrono::nanoseconds(1000*1000*100),
            expiry_handler);

        Logless(*main_logger, RRC_TRC,
            "TRC | LCRRC# | auto send timer scheduled id=#, req_id=#",
            (int) llc.get_lcid(),
            exp_id,
            (int) msg.requestID);

        std::unique_lock<std::mutex> lg(rrc_requests_mutex);
        rrc_requests.erase(msg.requestID);
        auto res = rrc_requests.emplace(msg.requestID, rrc_request_context_t{});
        auto& rrc_request = res.first->second;
        rrc_request.exp_timer = exp_id;
        lg.unlock();

        rrc.send_rrc(msg);
    }

    struct rrc_request_context_t
    {
        uint64_t exp_timer;
        RRC request;
    };

    std::map<uint8_t, rrc_request_context_t> rrc_requests;
    std::mutex rrc_requests_mutex;

    std::atomic<txrx_state_e> tx_state = E_TXRX_STATE_NOT_CONFIGURED;
    std::atomic<txrx_state_e> rx_state = E_TXRX_STATE_NOT_CONFIGURED;

    ILLC& llc;
    IPDCP& pdcp;
    IRRC& rrc;
    IEndPoint& ep;
    bfc::ITimer& timer;
}; 

#endif // __LCRRC_HPP__