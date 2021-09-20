#!/bin/bash

set -o xtrace 

input="liste.txt"
baseoid="20"

while IFS= read -r line
do
  filename=`echo "$line" | sed 's/[[:space:]]//g'`
  echo "--> #$filename#"
  filesize=$(stat -c%s "$filename")
  echo "Size of $filename = $filesize bytes."
  
  ./m0store_cp_to $filename "$baseoid:1" 4096  2>/dev/null 1>/dev/null
  ./m0store_cp_to_bulk $filename "$baseoid:2" 4096 2>/dev/null 1>/dev/null

  ./m0store_cp_from "$baseoid:1" /tmp/"$baseoid"-1 4096 $filesize 2>/dev/null 1>/dev/null
  ./m0store_cp_from "$baseoid:2" /tmp/"$baseoid"-2 4096 $filesize 2>/dev/null 1>/dev/null

   sudo md5sum $filename  /tmp/"$baseoid"-1 /tmp/"$baseoid"-2 | tee -a ./log_md5.txt
   rm /tmp/"$baseoid"-1 /tmp/"$baseoid"-2
  
   baseoid=$((baseoid+1)) 
done < "$input"
