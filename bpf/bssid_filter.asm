      ldb  [3]              ;  rt_len = radiotap.length_high;
      lsh  #8               ;  rt_len <<= 8;
      tax                   ;  
      ldb  [2]              ;
      or   x                ;  rt_len |= radiotap.length_low;
      tax                   ;  
      ldb  [x + 0]          ;  
      and  #0x0F            ;  // TYPE=0b10 && PROTOCOL=0b00
      jne  #0x08, die       ;  if (0b1000 != fr80211.fc.proto_type) goto die; 
      ld   [x + 16]         ;  // FC(2) + DURATION(2) + ADDR1(6) + ADDR2(6)
      jne  #0xDEADBEEF, die ;  
      ldh  [x + 21]         ;  
      jne  #0xCAFE, die     ;  if (0xDEADBEEFCAFE != fr80211.addr3[5]) goto die;
      ld   [x + 4]          ;
      jne  #0xFFFFFFFF, die ;
      ldh  [x + 8]          ;
      jne  #0xFFFF, die     ;  if (0xFFFFFFFFFF != fr80211.addr3[5]) goto die
      ldh  [x + 14]         ;
      jne  #0xFFFF, die     ;  if (0xFFFF != fr80211.addr3.src)
      ldh  [x + 17]         ;
      jne  #0xFF, die       ;  if (0xFF != fr80211.addr3.port)
      ret #-1               ;  return -1;
die:  ret #0                ;  return 0;