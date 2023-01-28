#ifndef __ILLC_HPP__
#define __ILLC_HPP__

#include "pdu.hpp"
#include "frame_defs.hpp"
#include "info_defs.hpp"

struct ILLC
{
    virtual ~ILLC(){} 

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(const rx_info_t&) = 0;
};

#endif // __ILLC_HPP__w