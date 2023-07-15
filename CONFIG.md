# WInject Configuration
This will guide you on configuring the WInject application.

## 1  frame_config
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config |
| type | sequence |

Configures the ppWIFI 802.11 frame structure and timing

### 1.1 - slot_interval_us
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/slot_interval_us |
| type | unsigned |
| default | 1000 |

Frame transmission interval in microseconds.

### 1.2 - frame_payload_size
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/slot_interval_us |
| type | unsigned |
| default | 1450 |

802.11 frame data maximum payload size.
This has to be configured according to the HW MTU:
* If the min `frame_payload_size` is less than any LLC and payload allocation,
    there wont be any LLC transmission at all.
* If the `frame_payload_size` is greater than the configured hardware
    capabilities then the transmitted frame will be truncated.

### 1.3 - fec_configs
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/fec_configs |
| type | array |

Each element allow RRC to configure the frame for FEC encoding depending on the
channel condition.

### 1.3.1 - fec_type
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/fec_configs/fec_type |
| type | enum |

Enum values:

| FEC     | Block Size | Data Size |
|-------------|------------|-----------|
| RS(255,239) | 255 | 239 |
| RS(255,223) | 255 | 223 |
| RS(255,191) | 255 | 191 |
| RS(255,127) | 255 | 127 |

When FEC is active, the `frame_payload_size` will be rounded
to the nearest block size multiple.

### 1.3.2 - threshold
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/fec_configs/threshold |
| type | unsigned |
| min | 0 |
| max | 100 |

Threshold value when FEC will be used.

### 1.4  - hwsrc
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/fec_configs/hwsrc |
| type | 24-bit unsigned |

Source ID used in ppWIFI 802.11 frame
The peer `hwdst` has to be configured with same value as `hwsrc`

### 1.5 - hwdst
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/frame_config/fec_configs/hwdst |
| type | 24-bit unsigned |

Destination ID used in ppWIFI 802.11 frame and packet filtering
The peer `hwsrc` has to be configured with same value as `hwsrc`,
otherwise the frames coming from the peer will be filtered out.

## 2 - llc_configs
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs |
| type | array |

Used to define the LLCs (Logical Link Channels)

### 2.1 - lcid
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/lcid |
| type | unsigned |
| min | 0 |
| max | 255 |

Logical Channel ID

### 2.2 - tx_mode
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/tx_mode |
| type | enum {AM,TM} |

Transmission Mode
* AM - Akcnowledge Mode
* TM - Transparent Mode

### 2.3 - scheduling_config
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/scheduling_config |
| type | sequence |

Used to configure the LLC scheduling parameters

### 2.3.1 - nd_pdu_size
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/scheduling_config/nd_pdu_size |
| type | unsigned |
| default | 64 |

Non data guaranteeed PDU size.
Guaranteed PDU is used for acks in `tx_mode=AM`.
This has no use in `tx_mode=TM`.
Setting it to `0` will disable non data guaranteed PDU,
although LLC will still allocate ACKs from scheduling slice.

### 2.3.1 - quanta
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/scheduling_config/quanta |
| type | unsigned |
| default | 255 |

Byte slice for LLC in each scheduling round.
The higher the number the higher the priority of the LLC will be.

### 2.4 - am_tx_config
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/am_tx_config |
| type | sequence |

Acknowledged Mode Transmit Configuration, only used when `tx_mode=AM`

### 2.4.3 - allow_rlf
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/am_tx_config/bool |
| type | bool |
| default | 1 |

Allow LLC to trigger RLF.

### 2.4.2 - arq_window_size
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/am_tx_config/arq_window_size |
| type | unsigned |
| default | 25 |

Automatic Repeat Request window size used to determine the next
retransmission slot for the unacknowdedged LLC PDU.

### 2.4.3 - max_retx_count
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/am_tx_config/max_retx_count |
| type | unsigned |
| default | 50 |

Maximum number of retransmission before triggering RLF.

If `allow_rlf=0`, max_retx_count PDUs will be dropped.

### 2.5.1 - common_tx_config
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/llc_configs/common_tx_config |
| type | sequence |

Common Transmit Configuration, used by both acknowledged mode and 
transparent mode.

### 2.5.1 - crc_type
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/common_tx_config/crc_type |
| type | enum |
| default | NONE |

Cyclic Redundancy Check Type used to check LLC PDU Integrity.

If `crc_type=NONE`, CRC Check will be disabled. 

**CRC Types**

| Type | Size | Polynomial |
|------|------|------------|
| CRC32_04C11DB7 | 32-bit | 04C11DB7 |

### 2.6 auto_init_on_rx
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/auto_init_on_tx |
| type | bool |
| default | false |

Auto initiate when LLC PDU is received but not active.

## 3 - pdcp_configs
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs |
| type | array |

Used to define the PDCPs (Packet Data Convergence Protocol) and endpoint 
instances.

### 3.1 - lcid
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/lcid |
| type | unsigned |
| min | 0 |
| max | 255 |

Logical Channel ID which is bound to LLC

### 3.2 - tx_config
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/tx_config |
| type | sequence |

Transmit Configuration

### 3.2.1 - allow_segmentation
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/tx_config/allow_segmentation |
| type | bool |
| default | false |

Allow end point packet segmentation.
Larger packes that doesn't fit to the scheduled bytes will be broken
down into multiple PDCP segments.

### 3.2.2 - allow_reordering
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/tx_config/allow_reordering |
| type | bool |
| type | false |

Allow endpoint packet to be received in the original sending order.

### 3.2.3 - allow_rlf
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/tx_config/allow_rlf |
| type | bool |
| type | false |

Allow PDCP triggered RLF.

### 3.2.4 - max_sn_distance
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/tx_config/max_sn_distance |
| type | unsigned |
| default | 1000 |

Maximum distance of the last completed packet to the most furthest pending
packet. If the packet distance is greater than max_sn_distance, it would be
indicative of dropped LLC packet thereby triggering RLF (Radio Link Failure).

If `allow_rlf=0`, last completed packet will be moved up to the next pending
packet with distance less than max_sn_distance.

### 3.2.5 - min_commit_size
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/pdcp_configs/tx_config/min_commit_size |
| type | unsigned |
| default | 0 |

Minimum bytes scheduled for PDCP before segment gets allocated.
If `min_commit_size=0`, will always commit scheduled bytes.

### 3.3 - endpoint_config
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/pdcp_configs/endpoint_config |
| type | sequence |

Endpoint configuration

### 3.3.1 - type
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/pdcp_configs/endpoint_config/type |
| type | enum |

**type enum values**

| type | Description |
|------|-------------|
| RRC0 | RRC Signalling |
| RRC1 | RRC Signalling |
| UDP | UDP Transport |
| TCP | TCP Transport |
| PIPE | Named Pipe |
| UART | Serial Device |

### 3.3.2 - udp_tx_address
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/pdcp_configs/endpoint_config/udp_tx_address |
| type | host:port |

Available only when `type=UDP`

### 3.3.3 - udp_rx_address
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/pdcp_configs/endpoint_config/udp_rx_address |
| type | host:port |

Available only when `type=UDP`

### 3.3.4 - tcp_address
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/pdcp_configs/endpoint_config/tcp_address |
| type | host:port |

Available only when `type=TCP`

### 3.3.5 - path
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/pdcp_configs/endpoint_config/path |
| type | string |

Filesystem path 

### 3.4 auto_init_on_tx
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/llc_configs/auto_init_on_tx |
| type | bool |
| default | false |

Auto initiate when EP packet is received but not active.

## 4 - app_config
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/app_config |
| type | sequence |

Used to define application and hardware settings. 

### 4.1 - wifi_over_udp
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/app_config/wifi_over_udp |
| type | sequence |

Used to connect over UDP instead of actual WIFI device.

### 4.1.1 - tx_address
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/app_config/wifi_over_udp/tx_address |
| type | host:port |

### 4.1.2 - rx_address
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/app_config/wifi_over_udp/rx_address |
| type | host:port |

### 4.2 - tx_device
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/app_config/tx_device |
| type | string |

WIFI device used for transmit in dual wifi configuration.

### 4.3 - rx_device
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/app_config/rx_device |
| type | string |

WIFI device used for receive in dual wifi configuration.

### 4.4 - txrx_device
|      | JSON Info                      |
|------|--------------------------------|
| path | rrc_config/app_config/txrx_device |
| type | string |

WIFI device used for transmit/receive in single wifi configuration.

### 4.1.1 - udp_console
|      | JSON Info                      |
|------|--------------------------------| 
| path | rrc_config/app_config/udp_console |
| type | host:port |

UDP console address to controll the app.
 