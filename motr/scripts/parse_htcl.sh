#!/bin/bash 

##Data pool:
##    # fid name
##    0x6f00000000000001:0x1d 'the pool'
##Profile:
##    # fid name: pool(s)
##    0x7000000000000001:0x34 'default': 'the pool' None None
##Services:
##    localhost  (RC)
##    [started]  hax        0x7200000000000001:0x6   192.168.100.169@tcp:12345:1:1
##    [started]  confd      0x7200000000000001:0x9   192.168.100.169@tcp:12345:2:1
##    [started]  ioservice  0x7200000000000001:0xc   192.168.100.169@tcp:12345:2:2
##    [unknown]  m0_client  0x7200000000000001:0x17  192.168.100.169@tcp:12345:4:1
##    [unknown]  m0_client  0x7200000000000001:0x1a  192.168.100.169@tcp:12345:4:2
##
##    LOCAL_ADDR = 192.168.100.169@tcp:12345:4:1
##    HA_ADDR = 192.168.100.169@tcp:12345:1:1
##    PROFILE = 0x7000000000000001:0x34
##    PROC_FID = 0x6f00000000000001:0x1d

hctl status > hctl.txt

export LOCAL_ADDR=`cat hctl.txt  | grep m0_client | head -1 | tr -s ' ' | cut -d ' ' -f 5`
export HA_ADDR=`cat hctl.txt  | grep hax | tr -s ' ' | cut -d ' ' -f 5`
export PROFILE=`cat hctl.txt | grep default | grep 'the pool' | tr -s ' ' | cut -d ' ' -f 2`
export PROC_FID=`cat hctl.txt  | grep m0_client | head -1 | tr -s ' ' | cut -d ' ' -f 4`

rm hctl.txt
