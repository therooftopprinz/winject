#ifndef __LLC_UL_HPP__
#define __LLC_UL_HPP__

#include <llc/llc.hpp>

namespace winject
{

enum crc_type_e
{
    E_CRC_TYPE_NONE,
    E_CRC_TYPE_CRC32_04C11DB7
};

enum tx_mode_e
{
    E_TX_MODE_TM,
    E_TX_MODE_AM
};

struct llc_ul_config_t
{
    lcid_t     lcid            = 0;
    size_t     arq_window_size = 1024;
    crc_type_e crc_type        = E_CRC_TYPE_NONE;
    tx_mode_e  mode            = E_TX_MODE_TM;
};

class llc_ul
{
public:
    llc_ul(const llc_ul_config_t& config);
    ~llc_ul();

    void reconfigure(const llc_ul_config_t& config);
    llc_ul_config_t get_config();

    bool write_llc(bfc::sized_buffer& buffer)
private:
}

} // namespace winject

#endif // __LLC_UL_HPP__
