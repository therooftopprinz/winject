#ifndef __ILCRRC_HPP__
#define __ILCRRC_HPP__

#include "IEndPoint.hpp"
#include "IRRC.hpp"
#include "IPDCP.hpp"
#include "ILLC.hpp"

#include "interface/rrc.hpp"

struct ILCRRC
{
    ~ILCRRC() {}

    virtual void on_init() = 0;
    virtual void on_rlf() = 0;
    virtual void on_rrc_message(int req_id, const RRC_ExchangeRequest& req) = 0;
    virtual void on_rrc_message(int req_id, const RRC_ExchangeResponse& rsp) = 0;
};

#endif // __ILCRRC_HPP__