# WInject

## Repository
Dependencies:
* [googletest](https://github.com/google/googletest)
* [json](https://github.com/nlohmann/json)
* [bfc](https://github.com/therooftopprinz/bfc)
* [cum](https://github.com/therooftopprinz/cum)
* [logless](https://github.com/therooftopprinz/logless)

Folder structure:
* bpf - ppWiFi paclet filter scratch pad
* cpp_dependencies
  * bfc - BFC classes
  * cum - Common Utility Messenging for RRC
  * googletest - GTest & GMock
  * json - JSON C++
  * Logless - Logger
* udp_channeler - UDP channel simulation and limiter
* udp_checker - create patterns and send over channel and verify
* winject - complete WInject ppWiFi implementation

## Abstract
ppWiFi is an extension to the 802.xx protocols to support robust 
point-to-point communication on a lossy wireless channel. It allows 
transmission of multiple logical channels over a shared physical 
channel. It supports the acknowledged transmission mode, as well as 
forward error correction codes. It supports packet disassembly and 
reassembly. And it supports message authentication and encryption.

## Problem
SDR devices are expensive; if the application is just to be able to 
communicate regardless of the physical interface used, one could just 
use consumer devices implementing ZigBee, LoRa, Bluetooth, and others. 
However, these devices are slow, typically with less than 1 Mbps of 
throughput. WiFi, on the other hand, is fast but not robust enough, 
which is due to the fact that its transmission is error-free. Each of 
the packets has a checksum that is used to detect data errors in the 
transmission. If the frame check failed, the 802.11 MAC layer would 
sdrop that frame before reaching the network stack. This would require 
the packet to be retransmitted. WiFi also requires association, access 
point and station need to be associated before a data communication can 
begin. And the association would drop if the signal got too low or if 
it received too many errors.

All of these shortcomings are solved by ppWiFi. ppWiFi allows frame 
check errors to pass through the MAC layer, allowing erroneous frames 
to be handled. The erroneous frames can be corrected in the ppWiFi LLC 
layer, this is extremely useful in a radio-edge scenario where the 
signal drops and packets are dropped in traditional WiFi. 

## Protocol Roles
ppWIFI MAC:
- PHY Framing
- Device Addressing
- Forward error correction

ppWIFI LLC:
- Channel Multiplexing
- Automatic Repeat Request

ppWIFI PDCP:
- Packet Segmentation and Reassembly
- Compression
- Integrity Protection
- Ciphering
- Application Endpoint

ppWIFI RRC:
- Radio Resource Configuration Management

## ppWiFi 802.11 Frame structure

```
      +----------------------+----------------------+
   00 | FRAME_CTL (DATA)     | DURATION             |
      +----------------------+----------------------+
   02 | ADDRESS1: 0xFFFFFFFFFFFF                    |
      +----------------------+----------------------+
   08 | SRC                  | DST                  |
      +----------------------+----------------------+
   0C | ADDRESS3: 0xB16B00B5B4B3                    |
      +---------------------------------------------+
   12 | SEQ_CTL                                     |
      +---------------------------------------------+
   14 | FRAME_TYPE                                  |
      +---------------------------------------------+
   15 | ppWiFi LLC PDUs                             | SZ
      +---------------------------------------------+
   SZ | FCS                                         | EOF
      +---------------------------------------------+
```

* SRC : 24-bit Sender Identifier
* DST : 24-bit Receiver Identifier
* FRAME_TYPE:
  * 0: None
  * 1-255: RRC Specified Mapping
 
## ppWiFi LLC
```
      +-------------------------------+
   00 | LLC_SN                        |
      +---------------+---+-----------+
   01 | LCID          | A | SIZEH     |
      +---------------+---+-----------+
   02 | SIZEL                         |
      +-------------------------------+
   03 | CRC                           | CRCSZ
      +-------------------------------+
      | LLC_DATA/LLC_ACKs             | SIZE
      +-------------------------------+
```

* LLC_SN : LLC sequence number
* LCID : 5-bit Logical Channel Identifier
* A : Acknowledgement
	* If set the payload will be one or repeated **LLC_ACK**
	* If not set the payload will be an **LLC_DATA**
* SIZEH : Upper 3-bit of the LLC size
* SIZEL : Lower 8-bit of the LLC size
* CRC : Cyclic Redundancy Check
	* Determined by `RRC_LLCConfig/crcType` 

## LLC_ACK Payload:
```
      +-------------------------------+
   00 | START SN                      |
      +-------------------------------+
   01 | COUNT                         |
      +-------------------------------+
```
* START_SN : Start LLC SN to acknowledge
* COUNT : Number of LLC PDU acknowledged

## LLC_DATA Payload:
```
      +-------------------------------+
   00 | PDCP PDU                      | EOS
      +-------------------------------+
```

## ppWiFi PDCP
```
      +-------------------------------+
   00 | IV (OPTIONAL)                 | IVSZ
      +-------------------------------|
      | HMAC (OPTIONAL)               | HMACSZ
      +-------------------------------+
      | PDCP_SEGMENTs                 | EOP
      +-------------------------------+
```

* IV : Initialization Vector
	* Randomized for each PDU
	* Determined by `RRC_PDCPConfig/ivType`
* HMAC : Message Authentication Code
	* Determined by `RRC_PDCPConfig/hmacType`

## PDCP_SEGMENT Payload
```
      +---+---------------------------+
   00 | L | SIZEH                     | 01
      +---+---------------------------+
      | SIZEL                         | 01
      +---+---------------------------+
      | PACKET_SN (OPTIONAL)          | 02
      +-------------------------------+
      | OFFSET (OPTIONAL)             | 02
      +-------------------------------+
      | PAYLOAD                       | EOS
      +-------------------------------+
```
* L : Segment completed
* SIZE : Size of whole PDCP_SEGMENT entry
* PACKET_SN
  * Present when `RRC_PDCPConfig/allowReordering` 
* OFFSET
  *  Present when `RRC_PDCPConfig/allowSegmentation`
* PAYLOAD : Packet Data

# Architecture
```
              +----------------+
              | WiFi Device(s) |
              +----------------+
                   ^      |
                   |      |
+------------------------------------------+
|         TX       |      |      RX        |
|          +-------+      +-------+        |
|          |                      |        |
|          |                      v        |
| conf  +-------+               +------+   |
|   +-->| SCHED |               |  RX  |   |
|   |   +-------+               +------+   |
|   |    ^                         |       |
|   |    |                         |       |
|   |    |  +--------+-------------+       |
|   |    |  |        |             |       |
|   |    +--------+------------+   |       |
|   |    |  |     |  |         |   |       |
|   |    |  v     |  v         |   v       |
|   |  +------+ +------+     +------+      |
|   |  | LLC0 | | LLC1 | ... | LLCN |      |
|   |  +------+ +------+     +------+      |
|   |      ^        ^                      |
|   |      |        |                      |
|   |      v        v                      |
|   |  +------+  +------+                  |
|   +->| RRC  |  | PDCP |                  |
|      +------+  +------+                  |
|                   ^                      |
|                   |                      |
+------------------------------------------+
                    |
                    | UDP/TCP
                    v
                 +------+
                 | APP  |
                 +------+
```

# RRC Procedures

## AM LLC Establishment
```
AL3                AL0                    BL0                BL3
 |                  |                      |                  |
 |                  | ExchangeRequest(LC3) | reconfig_rx      |
 |                  +--------------------->|- - - - - - - - ->|
 |                  |                      |                  |
 | reconfig_rx      | ExchangeResponse(LC3)|                  |
 |<- - - - - - - - -+<---------------------+                  |
 |                  |          ^           |                  |
 |                  |          |           |                  |
 |                  |    activate_delay    |                  |
 |                  |          |           |                  |
 |                  |          v           |                  |
 | enable_tx        |                      |enable_tx         |
 | enable_rx        |                      |enable_rx         |
 |                  |                      |                  |
 |                  |                      |                  |
 |                    << LLC ACTIVATED  >>                    |
 |< - - - - - - - - - - - - traffic  - - - - - - - - - - - - >|
 |                  |                      |                  |
 |                  |                      |                  |
```

## LLC Fault
```
 AL3                AL0             BL0                BL3
 |                  |               |                  |
 |< - - - - - - - - - - traffic  - - - - - - - - - - ->|
 |                  |               |                  |
 | DATA 0           |               |                  |
 +-------------------------> X      |                  |
 | DATA 0 RTX0      |               |                  |
 +-------------------------> X      |                  |
 |                  |               |                  |
                          . . .
 |                  |               |                  |
 | DATA 0 MAX_RTX   |               |                  |
 +-------------------------> X      |                  |
 |                  |               |                  |
 | on_rlf           | FaultRequest  |                  |
 + - - - - - - - -->+---------------+- - - - - - - - ->|
 | disable_tx       |               | disable_tx       |
 | disable_rx       |               | disable_rx       |
 | reset_pdcp       | FaultResponse | reset_pdcp       |
 |                  |<--------------+                  |
 |                  |               |                  |
 |                   << LLC FAULT >>                   |
 |X<- - - - - - - - - - - - - - - - - - - - - - - - ->X|
 ```
 