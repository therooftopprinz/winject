      ldb  [3]              ;  rt_len = radiotap.length_high;
      lsh  #8               ;  rt_len <<= 8;
      tax                   ;  
      ldb  [2]              ;
      or   x                ;  rt_len |= radiotap.length_low;
      tax                   ;  
      ldb  [x + 0]          ;  
      and  #0x0F            ;  // TYPE=0b10 && PROTOCOL=0b00
      jne  #0x08, die       ;  if (0b1000 != fr80211.fc.proto_type) goto die; 
      ret #-1               ;  return -1;
die:  ret #0                ;  return 0;