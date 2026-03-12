#ifndef __PDCP_PDCP_DL_HPP__
#define __PDCP_PDCP_DL_HPP__

#include <bfc/buffer.hpp>
#include "frame_defs.hpp"

#include <deque>
#include <map>
#include <unordered_set>

namespace winject
{

struct pdcp_dl_config_t
{
    // @brief Breakdown frame data into smaller packets.
    bool allow_segmentation = false;
    // @brief Allow reordering of the packets.
    bool allow_reordering = false;
};

/* @brief PDCP DL class
 * This class is responsible for the PDCP Downlink Logic
 */
class pdcp_dl
{
public:
    enum status_code_e
    {
        // @brief Ok
        STATUS_CODE_SUCCESS,
        // @brief PDCP segments match the given buffer.
        STATUS_CODE_BUFFER_INVALID_DATA,
    };

    pdcp_dl(const pdcp_dl_config_t& config);
    ~pdcp_dl();

    void          reset();
    void          reconfigure(const pdcp_dl_config_t& config);
    pdcp_dl_config_t
                  get_config();
    bool          on_pdcp_data(bfc::buffer_view pdcp);
    size_t        get_outstanding_bytes();
    size_t        get_outstanding_packet();
    bfc::buffer   pop();
    status_code_e get_status();

private:
    struct reassembly_state_t
    {
        bfc::buffer buffer;
        size_t accumulated_size = 0;
        size_t expected_size = 0;
        std::unordered_set<size_t> recvd_offsets;
        bool has_expected_size = false;
        bool is_complete = false;
    };

    void flush_completed_in_order();

    pdcp_dl_config_t config;
    status_code_e status = STATUS_CODE_SUCCESS;

    // Reassembly per PDCP SN
    std::map<pdcp_sn_t, reassembly_state_t> reassembly_map;
    // Next SN we are allowed to deliver (monotonically increasing, starts at 0)
    pdcp_sn_t next_expected_sn = 0;

    std::deque<bfc::buffer> completed_frames;
    size_t outstanding_bytes = 0;
};

} // namespace winject

#endif // __PDCP_PDCP_DL_HPP__
