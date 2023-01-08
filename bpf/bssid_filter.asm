      ldb  [3]              ;  rt_len = radiotap.length_high;
      lsh  #8               ;  rt_len <<= 8;
      tax                   ;  
      or   x                ;  rt_len |= radiotap.length_low;
      tax                   ;  
      ldb  [x + 0]          ;  
      and  #0x0F            ;  // TYPE=0b10 && PROTOCOL=0b00
      jne  #0x08, die       ;  if (0b1000 != fr80211.fc.proto_type) goto die; 
      ldb  [x + 16]         ;  // FC(2) + DURATION(2) + ADDR1(6) + ADDR2(6)
      jne  #0xEF, die       ;  if (imm != fr80211.addr3[0]) goto die;
      ldb  [x + 17]         ;  
      jne  #0xBE, die       ;  if (imm != fr80211.addr3[1]) goto die;
      ldb  [x + 18]         ;  
      jne  #0xAD, die       ;  if (imm != fr80211.addr3[2]) goto die;
      ldb  [x + 19]         ;  
      jne  #0xDE, die       ;  if (imm != fr80211.addr3[3]) goto die;
      ldb  [x + 20]         ;  
      jne  #0xAF, die       ;  if (imm != fr80211.addr3[4]) goto die;
      ldb  [x + 21]         ;  
      jne  #0xFA, die       ;  if (imm != fr80211.addr3[5]) goto die;
done: ret #-1               ;  return -1;
die:  ret #0                ;  return 0;