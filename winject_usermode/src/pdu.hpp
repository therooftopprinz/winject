#ifndef __WINJECTUM_PDU_HPP__
#define __WINJECTUM_PDU_HPP__

#include <vector>
#include <cstdint>
#include <cstddef>


using buffer_t = std::vector<uint8_t>;
struct pdu_t
{
    uint8_t *base = nullptr;
    size_t size = 0;
};

#endif // __WINJECTUM_PDU_HPP__
