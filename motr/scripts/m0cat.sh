#!/bin/bash

### ./m0cat.sh   -o 21:34 -s 1m -c 2 ./toto.2m -L 9

echo "m0cat -l $LOCAL_ADDR -H $HA_ADDR -p $PROFILE -P $PROC_FID $@"

m0cat -l $LOCAL_ADDR -H $HA_ADDR -p $PROFILE -P $PROC_FID $@
