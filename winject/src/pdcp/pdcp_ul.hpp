#ifndef __PDCP_PDCP_UL_HPP__
#define __PDCP_PDCP_UL_HPP__

#include <bfc/buffer.hpp>
#include <frame_defs.hpp>

#include <deque>

namespace winject
{

struct pdcp_ul_config_t
{
    // @brief Breakdown frame data into smaller packets.
    bool allow_segmentation = false;
    // @brief Allow reordering of the packets.
    bool allow_reordering = false;
    // @brief Minimum size of the packet to be committed.
    size_t min_commit_size  = 128;
};

/* @brief PDCP UL class
 * This class is responsible for the PDCP Uplink Logic
 */
class pdcp_ul
{
public:
    enum status_code_e
    {
        // @brief Ok
        STATUS_CODE_SUCCESS,
        // @brief Can't encode PDCP on the given buffer.
        STATUS_CODE_BUFFER_TOO_SMALL,
        // @brief No data to write.
        STATUS_CODE_NO_DATA_TO_WRITE
    };

    pdcp_ul(const pdcp_ul_config_t& config);
    ~pdcp_ul();

    void          reset();
    void          reconfigure(const pdcp_ul_config_t& config);
    pdcp_ul_config_t
                  get_config();
    bool          on_frame_data(bfc::buffer_view packet);
    size_t        get_outstanding_bytes();
    size_t        get_outstanding_packet();
    size_t        get_min_commit_size();
    ssize_t       write_pdcp(bfc::buffer_view buffer);
    status_code_e get_status();

private:
    pdcp_ul_config_t config;
    status_code_e status = STATUS_CODE_SUCCESS;
    std::deque<bfc::buffer> outstanding_buffers;
    size_t data_offset = 0;
    pdcp_sn_t sequence_number = 0;
    size_t total_in_bytes = 0;
    size_t total_out_bytes = 0;
};

} // namespace winject

#endif // __PDCP_PDCP_UL_HPP__
