#ifndef __WINJECTUM_INFO_DEPS_HPP__
#define __WINJECTUM_INFO_DEPS_HPP__

#include <cstddef>
#include <cstdint>

enum fec_type_e
{
    E_FEC_TYPE_RS_255_239,
    E_FEC_TYPE_RS_255_223,
    E_FEC_TYPE_RS_255_191,
    E_FEC_TYPE_RS_255_127
};

struct frame_info_t
{
    size_t slot_number;
    fec_type_e fec_type;
    size_t max_frame_payload;
};

class pdu_t;

struct tx_info_t
{
    const frame_info_t& in_frame_info;
    bool tx_available;
    pdu_t out_pdu;
    size_t out_allocated;
};

struct rx_info_t
{
    const frame_info_t& in_frame_info;
    pdu_t in_pdu;
};

#endif // __WINJECTUM_INFO_DEPS_HPP__