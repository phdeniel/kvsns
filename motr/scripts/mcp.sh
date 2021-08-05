#!/bin/bash

### ./mcp.sh   -o 21:34 -s 1m -c 2 ./toto.2m -L 9

echo $@
echo "mcp -ep $LOCAL_ADDR -hax $HA_ADDR -prof $PROFILE -proc $PROC_FID $@"
mcp -ep $LOCAL_ADDR -hax $HA_ADDR -prof $PROFILE -proc $PROC_FID $@


