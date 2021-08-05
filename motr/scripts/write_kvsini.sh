#!/bin/bash

### ./m0kv.sh index create 1:6
### ./m0kv.sh index put 1:5 "clef" "valeur" -s
### ./m0kv.sh index get 1:5 "clef" -s
### ./m0kv.sh index next -s 1:5 0 10 

echo "[motr]"
echo "        local_addr =$LOCAL_ADDR" 
echo "        ha_addr = $HA_ADDR"
echo "        profile = $PROFILE"
echo "        proc_fid = $PROC_FID"
echo "        layoutid = 9"
echo "        use_m0trace = false"
echo "        kvs_fid = 0x780000000000000a:1"

