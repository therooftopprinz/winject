
                         ppWiFi Protocol

Abstract
-----------------------------
    ppWiFi is an extension to the 802.xx protocols to support robust 
point-to-point communication on a lossy wireless channel. It allows 
transmission of multiple logical channels over a shared physical 
channel. It supports the acknowledged transmission mode, as well as 
forward error correction codes. It supports packet disassembly and 
reassembly. And it supports message authentication and encryption.

Problem
-----------------------------
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


Protocol Roles
-----------------------------
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

Implementation
-----------------------------


ppWiFi 802.11 Frame structure:

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

ppWiFi LLC:
      +---+---+-----------------------+
   00 | D | R | LLC SN                |
      +---+---+-------+---+-----------+
   01 | LCID          | A | SIZEH     |
      +---------------+---+-----------+
   02 | SIZEL                         |
      +-------------------------------+
   03 | Payload                       | EOP
      +-------------------------------+

      D: LLC-DATA Indicator (LLC-CTL if unset)
      R: Retransmit Indicator 
      LLC SN: 6-bit LLC sequence number
      LCID: 5-bit Logical Channel Identifier
      SIZEH: Upper 3-bit of the LLC size
      SIZEL: Lower 8-bit of the LLC size
      A: Acknowledgement

LLC-ACK Payload:
      +---+---+-----------------------+
   00 | # | # | START SN              |
      +---+---+-----------------------+
   01 | # | # | COUNT                 | EOS
      +---+---+-----------------------+

      LLC SN: Start LLC SN
      COUNT: Number of LLC PDU acknowledged

LLC-ACK Payload:
   Not Available. For future use.

LLC-DATA Payload:
      +-------------------------------+
   00 | PDCP PDU                      | EOS
      +-------------------------------+

ppWiFi PDCP
      +-------------------------------+
   00 | PDCP SN (IV)                  |
      +-------------------------------+
   01 | IV-EXT (OPTIONAL)             | IVSZ
      +-------------------------------+
      | OFFSET (OPTIONAL)             | 02
      +-------------------------------|
      | PAYLOAD                       | SZ
      +-------------------------------+
      | HMAC (OPTIONAL)               | EOP
      +-------------------------------+

      PDCP SN: PDCP sequence number
      OFFSET: Data offset (for framing PDCP only)
      MAC: Message Authentication Code

Architecture:
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