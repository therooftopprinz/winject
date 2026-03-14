#ifndef __PDCP_PDCP_DL_CB_HPP__
#define __PDCP_PDCP_DL_CB_HPP__

#include <bfc/buffer.hpp>
#include <bfc/sized_buffer.hpp>
#include "frame_defs.hpp"

#include <deque>
#include <unordered_set>
#include <vector>

namespace winject
{

struct pdcp_dl_cb_config_t
{
    // @brief Breakdown frame data into smaller packets.
    bool allow_segmentation = false;
    // @brief Allow reordering of the packets.
    bool allow_reordering = false;
    // @brief Length of the circular reorder buffer (slots). When slot is occupied by another SN, frame goes to overflow deque.
    size_t reorder_buffer_len = 512;
};

/* @brief PDCP DL class with circular-buffer reordering
 * Same interface as pdcp_dl but uses a circular buffer for reordering instead of a map.
 * When the circular buffer slot is occupied (different SN), frames are stored in an overflow deque.
 */
class pdcp_dl_cb
{
public:
    enum status_code_e
    {
        STATUS_CODE_SUCCESS,
        STATUS_CODE_BUFFER_INVALID_DATA,
    };

    pdcp_dl_cb(const pdcp_dl_cb_config_t& config);
    ~pdcp_dl_cb();

    void                reset();
    void                reconfigure(const pdcp_dl_cb_config_t& config);
    pdcp_dl_cb_config_t get_config();
    bool                on_pdcp_data(bfc::const_buffer_view pdcp);
    size_t              get_outstanding_bytes();
    size_t              get_outstanding_packet();
    bfc::sized_buffer   pop();
    status_code_e       get_status();

private:
    struct reassembly_state_t
    {
        bfc::sized_buffer buffer;
        size_t accumulated_size = 0;
        size_t expected_size = 0;
        std::unordered_set<size_t> recvd_offsets;
        bool has_expected_size = false;
        bool is_complete = false;
    };

    struct slot_t
    {
        pdcp_sn_t sn = 0;
        reassembly_state_t state;
        bool valid = false;
    };

    void flush_completed_in_order();
    size_t slot_index(pdcp_sn_t sn) const;
    reassembly_state_t* get_or_put_slot(pdcp_sn_t sn);
    reassembly_state_t* find_overflow(pdcp_sn_t sn);

    pdcp_dl_cb_config_t config;
    status_code_e status = STATUS_CODE_SUCCESS;

    std::vector<slot_t> reorder_ring;
    std::deque<std::pair<pdcp_sn_t, reassembly_state_t>> overflow_deque;

    pdcp_sn_t next_expected_sn = 0;
    std::deque<bfc::sized_buffer> completed_frames;
    size_t outstanding_bytes = 0;
};

} // namespace winject

#endif // __PDCP_PDCP_DL_CB_HPP__
