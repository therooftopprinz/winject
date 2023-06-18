# WInject

## Repository
Dependencies:
* [googletest](https://github.com/google/googletest)
* [json](https://github.com/nlohmann/json)
* [bfc](https://github.com/therooftopprinz/bfc)
* [cum](https://github.com/therooftopprinz/cum)
* [logless](https://github.com/therooftopprinz/logless)
* [schifra](https://github.com/ArashPartow/schifra)

Folder structure:
* bfc - BFC classes
* bin_tests - simple packet injection (WInject, fixed pattern) send / receive
* bpf - ppWiFi paclet filter scratch pad
* cum - Common Utility Messenging for RRC
* googletest - GTest & GMock
* json - JSON C++
* Logless - Logger
* schifra - Reed-Solomon FEC C++ 
* SimpleFEC - WInject with RS Error Correction
* udp_channeler - UDP channel simulation and limiter
* udp_checker - create patterns and send over channel and verify
* winject - 802.11 headers and filters, radiotap, and wifi injector
* winject_udp - simple packet injection (WInject, udp - small packets) send / receive
* winject_udp_defrag - simple packet injection (WInject, udp - splitted) send / receive
* winject_udp_usermode - complete WInject ppWiFi implementation

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
- User Packet Framing
- Application Endpoint
- Compression
- Integrity Protection
- Ciphering

ppWIFI RRC:
- Radio Link Configuration management

## ppWiFi 802.11 Frame structure
```

      +----------------------+----------------------+
   00 | FRAME CTL (DATA)     | DURATION             |
      +----------------------+----------------------+
   02 | ADDRESS1: 0xFFFFFFFFFFFF                    |
      +----------------------+----------------------+
   08 | SRC                  | DST                  |
      +----------------------+----------------------+
   0C | ADDRESS3: 0xB16B00B5B4B3                    |
      +---------------------------------------------+
   12 | SEQ CTL                                     |
      +---------------------------------------------+
   14 | FRAME TYPE                                  |
      +---------------------------------------------+
   15 | ppWiFi LLC PDUs                             | SZ
      +---------------------------------------------+
   SZ | FCS                                         | EOF
      +---------------------------------------------+

      SRC: 24-bit Sender Identifier
      DST: 24-bit Receiver Identifier
      FRAME TYPE:
         0: None
         1-255: RRC Specified mapping
```
## ppWiFi LLC
```
      +---+---+-----------------------+
   00 | # | R | LLC SN                |
      +---+---+-------+---+-----------+
   01 | LCID          | A | SIZEH     |
      +---------------+---+-----------+
   02 | SIZEL                         |
      +-------------------------------+
   03 | CRC                           | CRCSZ
      +-------------------------------+
      | Payload                       | EOP
      +-------------------------------+

      R: Retransmit Indicator 
      LLC SN: 6-bit LLC sequence number
      LCID: 5-bit Logical Channel Identifier
      A: Acknowledgement
      SIZEH: Upper 3-bit of the LLC size
      SIZEL: Lower 8-bit of the LLC size
      CRC: Cyclic Redundancy Check
```
## LLC-ACK Payload:
```
      +---+---+-----------------------+
   00 | # | # | START SN              |
      +---+---+-----------------------+
   01 | # | # | COUNT                 | EOS
      +---+---+-----------------------+

      LLC SN: Start LLC SN
      COUNT: Number of LLC PDU acknowledged
```

## LLC-DATA Payload:
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
      | HMAC (OPTIONAL)               |
      +-------------------------------+
      | PDCP-SEGMENTs                 | EOP
      +-------------------------------+

      IV: Initialization Vector
      MAC: Message Authentication Code
```

## PDCP-SEGMENT Payload
```
      +-------------------------------+
   00 | PACKET SN                     |
      +-------------------------------+
      | SIZE                          | 02
      +-------------------------------+
   01 | OFFSET (OPTIONAL)             | 02
      +-------------------------------+
      | PAYLOAD                       | EOS
      +-------------------------------+
```

# Architecture
```
               +--------------+
               | WiFi Device  |
               +--------------+
                   ^      |
                   |      |
+------------------------------------------+
|         TX       |      |      RX        |
|          +-------+      +-------+        |
|          |                      |        |
|          |                      v        |
| conf  +-----+               +-------+    |
|   +-->| MUX |               | DEMUX |    |
|   |   +-----+               +-------+    |
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

## Unidirectional RX LLC Establishment
```
AL3                AL0                    BL0                BL3
 |                  |                      |                  |
 |                  | PullReq(LC3)         |                  |
 |                  +--------------------->|                  |
 |                  |                      |                  |
 | reconfig_rx      | PullResp(LC3)        |                  |
 |<- - - - - - - - -+<---------------------+                  |
 | enable_rx        |                      |                  |
 |                  | ActivateReq(RX,LC3)  | enable_tx        |
 |                  +--------------------->+- - - - - - - - ->|
 |                  |                      |                  |
 | enable_rx        | ActivateResp         |                  |
 |<- - - - - - - - -|<---------------------+                  |
 |                  |                      |                  |
 |                    << LLC ACTIVATED <<                     |
 |< - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |
```

## Unidirectional TX LLC Establishment
```
AL3                AL0                    BL0                BL3
 |                  |                      |                  |
 |                  | PushReq(LC3)         | reconfig_rx      |
 |                  +--------------------->+- - - - - - - - ->|
 |                  |                      | enable_rx        |
 |                  | PushResp(LC3)        |                  |
 |                  |<---------------------+                  |
 |                  |                      |                  |
 |                  | ActivateReq(TX,LC3)  |                  |
 |                  +--------------------->+ - - - - - - - - >|
 |                  |                      | enable_rx        |
 | enable_tx        | ActivateResp         |                  |
 |<- - - - - - - - -+<---------------------+                  |
 |                  |                      |                  |
 |                     >> LLC ACTIVATED >>                    |
 +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - ->|
```

## Bidirectional LLC Establishment
```
AL3                AL0                    BL0                BL3
 |                  |                      |                  |
 |                  | PullReq(LC3)         |                  |
 |                  +--------------------->|                  |
 |                  |                      |                  |
 | reconfig_rx      | PullResp(LC3)        |                  |
 |<- - - - - - - - -+<---------------------+                  |
 | enable_rx        |                      |                  |
 |                  |                      |                  |
 |                  | PushReq(LC3)         | reconfig_rx      |
 |                  +--------------------->+- - - - - - - - ->|
 |                  |                      | enable_rx        |
 |                  | PushResp(LC3)        |                  |
 |                  |<---------------------+                  |
 |                  |                      |                  |
 |                  | ActivateReq(AB,LC3)  | enable_tx        |
 |                  +--------------------->+- - - - - - - - ->|
 |                  |                      |                  |
 |                    << LLC ACTIVATED  <<                    |
 |< - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                  |                      |                  |
 | enable_tx        | ActivateResp         |                  |
 |<- - - - - - - - -+<---------------------+                  |
 |                  |                      |                  |
 |                    >> LLC ACTIVATED  >>                    |
 +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - ->|
```

## LLC Reconfiguration
```
 AL3                AL0             BL0                BL3
 |                  |               |                  |
 |                   << LLC ACTIVE <<                  |
 |< - - - - - - - - - - - - - - - - - - - - - - - - - >|
 |                  |               |                  |
 |                  | PushReq(LC3)  | reconfig_rx      |
 |                  +-------------->+- - - - - - - - ->|
 |                  |               | enable_rx        |
 |                  | PushResp(LC3) |                  |
 |                  |<--------------+                  |
 |                  |               |                  |
```

## LLC Fault
```
 AL3                AL0             BL0                BL3
 |                  |               |                  |
 | DATA 0           |               |                  |
 +-------------------------> X      |                  |
 | DATA 0 RTX0      |               |                  |
 +-------------------------> X      |                  |
 |                  |               |                  |
                          . . .
 |                  |               |                  |
 | DATA 0 MAXRTX    |               |                  |
 +-------------------------> X      |                  |
 |                  |               |                  |
 | on_rlf           | FaultRequest  | disable_rx       |
 + - - - - - - - -->+---------------+- - - - - - - - ->|
 | disable_tx       |               | reset_pdcp       |
 | reset_pdcp       | FaultResponse |                  |
 |                  |<--------------+                  |
 |                  |               |                  |
 |                   >> LLC FAULT >>                   |
 +- - - - - - - - - - - - - - -> X  |                  |
 ```