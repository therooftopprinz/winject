#ifndef __PDU_HPP__
#define __PDU_HPP__

#include <vector>
#include <cstdint>
#include <cstddef>

struct pdu_t
{
    uint8_t *base = nullptr;
    size_t size = 0;
    std::vector<uint8_t> buffer; 
};

#endif // __PDU_HPP__