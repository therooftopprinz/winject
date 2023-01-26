#ifndef __IPDCP_HPP__
#define __IPDCP_HPP__

#include "pdu.hpp"

struct IPDCP
{
    virtual ~IPDCP(){} 

    virtual void get_pdu(pdu_t& pdu, size_t slot) = 0;
    virtual void on_pdu(const pdu_t& pdu, size_t slot) = 0;
};

#endif // __IPDCP_HPP__