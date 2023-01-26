#ifndef __ILLC_HPP__
#define __ILLC_HPP__

#include "pdu.hpp"

struct ILLC
{
    virtual ~ILLC(){} 

    virtual void get_pdu(pdu_t& pdu, size_t slot) = 0;
    virtual void get_prio_pdu(pdu_t& pdu, size_t slot) = 0;
    virtual void on_pdu(const pdu_t& pdu, size_t slot) = 0;
};

#endif // __ILLC_HPP__