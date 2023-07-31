## LC Config
### 0 : rrc0
- RRC Channel
- TM (Transparent Mode)
- 4 GiB Quanta
### 1 : ssh1
- SSH Channel
- AM (Acknowledge Mode)
- 0.5 KiB Quanta
- 64 B Guaranteed ND-PDU
- reTX at T+30 frame slots
- 200 max reTX (6000 frame slots)
- Segmented
- Reordered (3000 max pdcp sn delta)
- TCP Endpoint
### 6 : video6
- Video Channel
- TM (Transparent Mode)
- 1.45 KB Quanta
- UDP Endpoint
### 7 : video7
- Video Channel
- FT-AM (Fault Tolerant AM)
- 1.45K Quanta
- 64 B Guaranteed ND-PDU
- reTX at T+30 frame slots
- 25 max reTX (750 frame slots)
- Segmented
- Reordered (500 max pdcp sn delta)
- UDP Endpoint
### 8. video8
- Video Channel
- FT-AM (Fault Tolerant AM)
- 1.45K Quanta
- 64 B Guaranteed ND-PDU
- reTX at T+30 frame slots
- 25 max reTX (750 frame slots)
- Segmented
- Reordered (500 max pdcp sn delta)
- UDP Endpoint