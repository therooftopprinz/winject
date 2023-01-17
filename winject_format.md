# WInject 802.11
```
+-----------+-------------------+-----------------+-----+
| Radio Tap | 802.11 Data Frame | WInject Payload | FCS |
+-----------+-------------------+-----------------+-----+
```

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
  ./winject -fec 5:1 -src 000101 -dst 000202

# WInject LLC
```
      8 7 6 5   4 3 2 1
    +-------------------+
 0  |   nLC   |   FLAG  |
    +---------+---------+  -----
 1  |   LCID  |         |    ^
    +---------+         |    |  LC[0]..LC[nLC-1]
 2  |       LC sz       |    v
    +-------------------+  -----
```

# SimpleFEC LLC
```
+--------+--------+--------+
| HEADER | DATA N |  FEC M |
+--------+--------+--------+
```
## Properties
* DATA and FEC size are preconfigured
* A transmission consist of DATA N and FEC M
* DATA matches with the same FEC number

## Header
- Data number
- FEC number

