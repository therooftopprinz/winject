#ifndef __WINJECTUM_IPDCP_HPP__
#define __WINJECTUM_IPDCP_HPP__

#include "pdu.hpp"
#include "info_defs.hpp"

struct IPDCP
{
    virtual ~IPDCP(){} 

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(rx_info_t&) = 0;
    virtual void on_tx_rlf() = 0;
    virtual void on_rx_rlf() = 0;

    virtual buffer_t to_rx(bool) = 0;
    virtual void to_tx(buffer_t) = 0;
};

#endif // __WINJECTUM_IPDCP_HPP__