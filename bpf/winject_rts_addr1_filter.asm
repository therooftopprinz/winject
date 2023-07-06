      ldb  [3]              ;  rt_len = radiotap.length_high;
      lsh  #8               ;  rt_len <<= 8;
      tax                   ;  
      ldb  [2]              ;
      or   x                ;  rt_len |= radiotap.length_low;
      tax                   ;  
      ldb  [x + 0]          ;  
      and  #0xFF            ;  // PROTOCOL=0b00 && TYPE=0b01 && SUBTYPE=0b1011
      jne  #0xb4, die       ;  if (1 != fr80211.fc.type && 0xb!= fr80211.fc.subtype) goto die;
      ld   [x + 10]         ;  // FC(2) + DURATION(2) + ADDR1(6)
      jne  #0xDEADBEEF, die ;      
      ldh  [x + 20]         ;  
      jne  #0xCAFE, die     ;  if (0xDEADBEEFCAFE != fr80211.addr2) goto die;
      ldh  [x + 4]          ;
      jne  #0xFFFF, die     ;
      ldb  [x + 6]          ;
      jne  #0xFF, die       ;  if (0xFFFFFF != fr80211.addr1 >> 24)
      ret #-1               ;  return -1;     
die:  ret #0                ;  return 0;