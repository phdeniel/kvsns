#!/bin/bash

### ./m0kv.sh index create 1:6
### ./m0kv.sh index put 1:5 "clef" "valeur" -s
### ./m0kv.sh index get 1:5 "clef" -s
### ./m0kv.sh index next -s 1:5 0 10 

m0kv -l $LOCAL_ADDR -h $HA_ADDR -p $PROFILE -f $PROC_FID $@

