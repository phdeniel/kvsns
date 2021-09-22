#!/bin/bash

#set -o xtrace 

input="liste.txt"
path="kvsns://dir2"

while IFS= read -r line
do
  filename=`echo "$line" | sed 's/[[:space:]]//g'`
  name=`basename $filename`
  echo "--> #$filename#  #$name#"
  filesize=$(stat -c%s "$filename")
  echo "Size of $filename = $filesize bytes."

  echo "1"  
  unset NO_BULK
  ./kvsns_cp $filename $path/$name-bulk 2>/dev/null 1>/dev/null
 
  echo "2"  
   export NO_BULK=1
  ./kvsns_cp $filename $path/$name-nobulk 2>/dev/null 1>/dev/null

  echo "3"  
  unset NO_BULK
  ./kvsns_cp $path/$name-bulk /tmp/$name-bulk-bulk 2>/dev/null 1>/dev/null

   echo "4"
   export NO_BULK=1
  ./kvsns_cp $path/$name-bulk /tmp/$name-bulk-nobulk 2>/dev/null 1>/dev/null

   echo "5"
   unset NO_BULK
   ./kvsns_cp $path/$name-nobulk /tmp/$name-nobulk-bulk 2>/dev/null 1>/dev/null
  
   echo "6"
   export NO_BULK=1
   ./kvsns_cp $path/$name-nobulk /tmp/$name-nobulk-nobulk 2>/dev/null 1>/dev/null


  echo "7"  
   sudo md5sum $filename /tmp/$name-bulk-bulk /tmp/$name-bulk-nobulk /tmp/$name-nobulk-bulk /tmp/$name-nobulk-nobulk 
   sudo file $filename /tmp/$name-bulk-bulk /tmp/$name-bulk-nobulk /tmp/$name-nobulk-bulk /tmp/$name-nobulk-nobulk 
  
  echo "8"  
  sudo rm /tmp/$name-bulk-bulk /tmp/$name-bulk-nobulk /tmp/$name-nobulk-bulk /tmp/$name-nobulk-nobulk 

done < "$input"
