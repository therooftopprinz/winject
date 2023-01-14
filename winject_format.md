+-----------+-------------------+-----------------+-----+
| Radio Tap | 802.11 Data Frame | WInject Payload | FCS |
+-----------+-------------------+-----------------+-----+

802.11 Data Frame Special Meaning:
* Address 1 should be: 0xFFFFFFFFFFFF
* Address 3 should be: 0xDEADBEEFCAFE
* Address 2 formatting
  * [2] SRC source address
  * [1] SRC_PORT source port
  * [2] DST destination address
    * 0xFFFF - Broadcast
  * [1] DST_PORT destination port

  using the command utility:
  ./winject -fec 5:1 -src 0001:01 -dst 0002:02

  # Winject Frames
## Plain
    * type=0
    * payload
## FEC Data
    * type=1
    * payload
## FEC Parity
    * type=2
    * payload

# Winject Frames Payloads
## Stream
    * type=0
    * payload
## Framed Chained
    * type=1
    * framenumber
    * islastchunk
    * offset
    * size
    * payload
## Framed
    * type=2
    * framenumber
    * islastchunk
    * offset
    * size
    * payload