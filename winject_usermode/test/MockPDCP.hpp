#ifndef __MOCKPDCP_HPP__
#define __MOCKPDCP_HPP__

#include "IPDCP.hpp"

struct MockPDCP : public IPDCP
{
    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(const rx_info_t&) = 0;
};

#endif // __MOCKPDCP_HPP__