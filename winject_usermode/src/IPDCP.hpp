#ifndef __WINJECTUM_IPDCP_HPP__
#define __WINJECTUM_IPDCP_HPP__

#include "pdu.hpp"
#include "info_defs.hpp"

struct IPDCP
{
    virtual ~IPDCP(){} 

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(rx_info_t&) = 0;
};

#endif // __WINJECTUM_IPDCP_HPP__