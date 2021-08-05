#!/bin/bash

### ./m0cp.sh   -o 21:34 -s 1m -c 2 ./toto.2m -L 9

echo "m0cp -l $LOCAL_ADDR -H $HA_ADDR -p $PROFILE -P $PROC_FID $@"

m0cp -l $LOCAL_ADDR -H $HA_ADDR -p $PROFILE -P $PROC_FID $@
