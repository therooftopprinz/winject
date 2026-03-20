#ifndef __LLC_UL_HPP__
#define __LLC_UL_HPP__

#include <frames/basic_llc.hpp>
#include <sys/types.h>
#include <bfc/buffer.hpp>
#include <optional>
#include <deque>
#include <list>
#include <vector>
#include <cstdint>

namespace winject
{

enum llc_crc_type_e
{
    E_LLC_CRC_TYPE_NONE,
    E_LLC_CRC_TYPE_CRC32_04C11DB7
};

enum llc_tx_mode_e
{
    E_LLC_TX_MODE_TM,
    E_LLC_TX_MODE_AM
};

struct llc_ul_config_t
{
    lcid_t         lcid                    = 0;
    llc_crc_type_e crc_type                = E_LLC_CRC_TYPE_NONE;
    llc_tx_mode_e  mode                    = E_LLC_TX_MODE_TM;
    size_t         tx_size                 = 512;
    size_t         min_recheck_slot_number = 10;
    size_t         max_recheck_slot_number = 50;
    size_t         max_retx_count          = 10;
};

class llc_ul
{
    /**
     * @brief Transmit-side Logical Link Control (LLC) state machine.
     *
     * This class manages LLC UL transmit-side bookkeeping for both TM and AM modes:
     * it assigns SN (sequence numbers), tracks TX-slot availability, handles ACK
     * processing, and schedules retransmissions.
     *
     * The instance maintains a bitmask status register (`status`) describing
     * flow-control and unexpected-event conditions. Consumers can query it via
     * `get_status()`.
     *
     * @par Status bitmask
     * - `STATUS_BIT_SLOT_SKIPPED`: Slot number advanced unexpectedly (AM mode).
     * - `STATUS_BIT_DATA_PDU_ALLOCATED`: A DATA PDU was successfully allocated/enqueued.
     * - `STATUS_BIT_WRITE_BUFFER_TOO_SMALL`: Retransmission target buffer too small.
     * - `STATUS_BIT_INVALID_LLC_PDU`: Provided LLC PDU is invalid (failed header validation).
     * - `STATUS_BIT_NO_TX_SLOT`: No free TX slot found for enqueueing.
     * - `STATUS_BIT_NO_FREE_SN`: AM mode: SN slot already occupied.
     * - `STATUS_BIT_INVALID_CONFIG`: Configuration validation failed.
     * - `STATUS_BIT_SN_LOST`: SN exceeded maximum retransmission count.
     * - `STATUS_BIT_SPURIOUS_ACK_SN`: ACK received for unknown/untracked SN.
     * - `STATUS_BIT_SPURIOUS_ACK_TX`: ACK received for already-acknowledged TX.
     */
    static constexpr uint64_t STATUS_BIT_SLOT_SKIPPED           = 1 << 0; // reset required
    static constexpr uint64_t STATUS_BIT_DATA_PDU_ALLOCATED     = 1 << 1;
    static constexpr uint64_t STATUS_BIT_WRITE_BUFFER_TOO_SMALL = 1 << 2;
    static constexpr uint64_t STATUS_BIT_INVALID_LLC_PDU        = 1 << 3;
    static constexpr uint64_t STATUS_BIT_NO_TX_SLOT             = 1 << 4;
    static constexpr uint64_t STATUS_BIT_NO_FREE_SN             = 1 << 5;
    static constexpr uint64_t STATUS_BIT_INVALID_CONFIG         = 1 << 6; // reset required
    static constexpr uint64_t STATUS_BIT_SN_LOST                = 1 << 7;
    static constexpr uint64_t STATUS_BIT_SPURIOUS_ACK_SN        = 1 << 8;
    static constexpr uint64_t STATUS_BIT_SPURIOUS_ACK_TX        = 1 << 9;

    /**
     * @brief Construct an LLC UL transmitter from configuration.
     *
     * The constructor calls `validate_config()` and then `reset()`.
     * Since `reset()` clears `status`, configuration failures are not visible
     * via `get_status()` unless the configuration is applied through `reconfigure()`.
     *
     * @param config LLC UL configuration.
     */
    llc_ul(const llc_ul_config_t& config);

    /**
     * @brief Destroy the LLC UL transmitter.
     */
    ~llc_ul();

    /**
     * @brief Replace configuration and re-initialize internal state.
     *
     * This calls `reset()` and then `validate_config()`.
     *
     * @param config New configuration.
     *
     * @par Status codes set
     * - `STATUS_BIT_INVALID_CONFIG`: if `validate_config()` detects invalid values.
     */
    void reconfigure(const llc_ul_config_t& config);

    /**
     * @brief Get the currently active configuration.
     *
     * @return Current configuration.
     *
     * @par Status codes set
     * None.
     */
    llc_ul_config_t get_config();

    /**
     * @brief Reset internal state and clear status.
     *
     * Clears `status` to `0` (all `STATUS_BIT_*` flags are removed), and resets
     * slot/SN counters and TX tracking containers.
     */
    void reset();

    /**
     * @brief Update the current TX slot number and advance retransmission bookkeeping.
     *
     * In TM mode (`config.mode == E_LLC_TX_MODE_TM`), the slot number is ignored and the
     * function always returns `true`.
     *
     * In AM mode, the slot number must be strictly sequential; if `slot_number != current_slot_number + 1`,
     * `STATUS_BIT_SLOT_SKIPPED` is set and the function returns `false`.
     *
     * @param slot_number New current slot number.
     * @return `true` when the slot advance succeeded; `false` on unexpected conditions.
     *
     * @par Status codes set
     * - `STATUS_BIT_SLOT_SKIPPED`: AM mode when the slot number is not expected.
     * - `STATUS_BIT_SPURIOUS_ACK_SN`: AM mode when transitioning a TX slot requires an SN entry
     *   but that SN bookkeeping is missing.
     */
    bool update_current_slot_number(size_t slot_number);

    /**
     * @brief Get the number of currently free TX slots.
     *
     * @return Free slot count (ring size minus used slots).
     *
     * @par Status codes set
     * None.
     */
    size_t get_free_tx_slot();

    /**
     * @brief Process an incoming ACK for the given LLC sequence number (SN).
     *
     * This updates internal SN/TX tracking when the ACK matches an outstanding element.
     *
     * @param sn LLC sequence number being acknowledged.
     * @return `true` if the ACK matched an outstanding TX element and state was updated;
     *   `false` if the ACK is unexpected/spurious.
     *
     * @par Status codes set
     * - `STATUS_BIT_SPURIOUS_ACK_SN`: SN is unknown/untracked for this transmitter.
     * - `STATUS_BIT_SPURIOUS_ACK_TX`: ACK corresponds to a TX element that was already acknowledged.
     */
    bool acknowledge(llc_sn_t sn);

    /**
     * @brief Get the size (in bytes) of the next retransmission PDU.
     *
     * @return Size of the next retransmission PDU, or `0` if there is nothing to retransmit.
     *
     * @par Status codes set
     * None.
     */
    size_t get_retransmission_size();

    /**
     * @brief Write the next retransmission PDU into `buffer` and enqueue it for TX-slot allocation.
     *
     * @param buffer Destination buffer view.
     * @return Number of bytes written on success, or `0` on failure / no-op.
     *
     * @par Status codes set
     * - `STATUS_BIT_WRITE_BUFFER_TOO_SMALL`: when `buffer.size()` is smaller than the retransmission PDU size.
     * - `STATUS_BIT_SPURIOUS_ACK_SN`: when the retransmission SN bookkeeping entry is missing.
     * - `STATUS_BIT_SN_LOST`: when retry count reached `config.max_retx_count`.
     * - `STATUS_BIT_NO_TX_SLOT`: if retransmission enqueueing cannot find a free TX slot.
     * - `STATUS_BIT_DATA_PDU_ALLOCATED`: when data PDU was successfully allocated/enqueued.
     */
    size_t write_retransmission_pdu(bfc::buffer_view buffer);

    /**
     * @brief Get the computed LLC header size (including optional CRC).
     *
     * @return LLC header size in bytes.
     *
     * @par Status codes set
     * None.
     */
    size_t get_llc_header_size() const;

    /**
     * @brief Prepare an LLC header from a candidate buffer.
     *
     * This parses the header from `buffer` and validates it. If invalid, it returns
     * `std::nullopt`. On success it sets `LCID`, the current SN, and the ACK bit.
     *
     * @param buffer Candidate buffer containing a raw LLC header.
     * @param is_ack Whether to set the ACK bit in the prepared LLC header.
     * @return Prepared and validated LLC header (`std::optional`), or `std::nullopt` if invalid.
     *
     * @par Status codes set
     * None.
     */
    std::optional<llc_t> prepare_llc_pdu(bfc::shared_buffer_view buffer, bool is_ack);

    /**
     * @brief Validate and enqueue a new LLC PDU for transmission.
     *
     * In AM mode, this may reserve future TX slots for DATA PDUs and creates the SN
     * bookkeeping entry used by ACK processing and retransmissions.
     *
     * @param buffer Buffer view containing the LLC PDU data/header.
     * @return Number of bytes accepted/allocated for transmission, or `0` on failure/no-op.
     *
     * @par Status codes set
     * - `STATUS_BIT_DATA_PDU_ALLOCATED`: when the PDU was successfully allocated/enqueued.
     * - `STATUS_BIT_NO_FREE_SN`: (AM mode) when the SN bookkeeping slot for the current SN is occupied.
     * - `STATUS_BIT_INVALID_LLC_PDU`: when the provided LLC header is invalid.
     * - `STATUS_BIT_NO_TX_SLOT`: if enqueueing DATA PDUs cannot find a free TX slot (via `enqueue_tx_check()`).
     */
    size_t write_llc_pdu(bfc::shared_buffer_view buffer);

    /**
     * @brief Get the current LLC UL status bitmask.
     *
     * @return `status` bitmask (`STATUS_BIT_*`).
     *
     * @par Status codes set
     * None.
     */
    uint64_t get_status() const;

private:

    /**
     * @brief Compute CRC size configured for this LLC UL.
     *
     * @return CRC size in bytes (`0` when CRC is disabled).
     *
     * @par Status codes set
     * None.
     */
    size_t crc_size() const;

    /**
     * @brief Map a TX slot number to the corresponding TX ring index.
     *
     * @param slot TX slot number.
     * @return TX ring index.
     *
     * @par Status codes set
     * None.
     */
    size_t tx_ring_index(size_t slot) const;

    /**
     * @brief Map an LLC sequence number (SN) to the corresponding SN ring index.
     *
     * @param sn LLC sequence number.
     * @return SN ring index.
     *
     * @par Status codes set
     * None.
     */
    size_t sn_ring_index(llc_sn_t sn) const;

    /**
     * @brief Reserve a TX slot for an LLC PDU to be checked/transmitted later.
     *
     * When reservation succeeds, the moved `llc_pdu` is stored into the reserved
     * TX slot element.
     *
     * @param llc_pdu PDU buffer to enqueue (moved on success).
     * @param sn Associated LLC sequence number.
     * @param retry_count Retry count for retransmission scheduling.
     * @return Reserved absolute slot number, or `std::nullopt` if no slot is available.
     *
     * @par Status codes set
     * - `STATUS_BIT_NO_TX_SLOT`: when no free TX slot is found.
     */
    std::optional<size_t> enqueue_tx_check(bfc::shared_buffer_view&& llc_pdu, llc_sn_t sn, size_t retry_count);

    /**
     * @brief Compute the slot distance to re-check based on retry count.
     *
     * @param retry_count Retry index.
     * @return Number of slots between initial scheduling and re-check.
     *
     * @par Status codes set
     * None.
     */
    size_t recheck_slot_size(size_t retry_count) const;

    /**
     * @brief Set a single status bit in the internal status bitmask.
     *
     * @param bit Bit to set.
     */
    void set_sbit(uint64_t bit);

    /**
     * @brief Clear a single status bit in the internal status bitmask.
     *
     * @param bit Bit to clear.
     */
    void clear_sbit(uint64_t bit);

    /**
     * @brief Validate configuration parameters.
     *
     * If validation fails, sets `STATUS_BIT_INVALID_CONFIG`.
     *
     * @par Status codes set
     * - `STATUS_BIT_INVALID_CONFIG`: when any configuration constraint is violated.
     */
    void validate_config();

    struct tx_elem_t
    {
        bool                    acknowledged = true;
        llc_sn_t                sn          = 0;
        size_t                  retry_count = 0;
        bfc::shared_buffer_view llc_pdu;
    };

    struct sn_elem_t
    {
        std::optional<size_t>   check_tx_slot_number;
    };

    struct retx_elem_t
    {
        size_t                  retry_count = 0;
        llc_sn_t                sn;
        bfc::shared_buffer_view llc_pdu;
    };

    llc_ul_config_t                       config;
    uint64_t                              status = 0;

    size_t                                current_slot_number = 0;
    llc_sn_t                              sn_counter = 0;

    std::vector<tx_elem_t>                tx_ring;
    std::vector<std::optional<sn_elem_t>> sn_ring;
    std::list<retx_elem_t>                retx_list;

    size_t                                used_tx_slots = 0;
    size_t                                pending_llc_pdu_count = 0;
};

} // namespace winject

#endif // __LLC_UL_HPP__
