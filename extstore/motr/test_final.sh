#!/bin/bash

#set -o xtrace 

input="liste.txt"
baseoid="88"

while IFS= read -r line
do
  filename=`echo "$line" | sed 's/[[:space:]]//g'`
  echo "--> #$filename#"
  filesize=$(stat -c%s "$filename")
  echo "Size of $filename = $filesize bytes."

  echo "1"  
  ./m0store_cp_to $filename "$baseoid:1" 4096  2>/dev/null 1>/dev/null
  echo "2"  
  ./m0store_cp_to_bulk $filename "$baseoid:2" 4096 2>/dev/null 1>/dev/null

  echo "3"  
  ./m0store_cp_from "$baseoid:1" /tmp/"$baseoid"-1 4096 $filesize 2>/dev/null 1>/dev/null
  echo "4"  
  ./m0store_cp_from "$baseoid:2" /tmp/"$baseoid"-2 4096 $filesize 2>/dev/null 1>/dev/null

  echo "5"  
  ./m0store_cp_from_bulk "$baseoid:1" /tmp/"$baseoid"-1-b 4096 $filesize 2>/dev/null 1>/dev/null
  echo "6"  
  ./m0store_cp_from_bulk "$baseoid:2" /tmp/"$baseoid"-2-b 4096 $filesize 2>/dev/null 1>/dev/null

  echo "7"  
   sudo md5sum $filename  /tmp/"$baseoid"-1 /tmp/"$baseoid"-2 /tmp/"$baseoid"-1-b /tmp/"$baseoid"-2-b | tee -a ./log_md5.txt
   sudo file $filename  /tmp/"$baseoid"-1 /tmp/"$baseoid"-2 /tmp/"$baseoid"-1-b /tmp/"$baseoid"-2-b | tee -a ./log_md5.txt
  
  echo "8"  
  rm /tmp/"$baseoid"-1 /tmp/"$baseoid"-2 /tmp/"$baseoid"-1-b /tmp/"$baseoid"-2-b 
  
   baseoid=$((baseoid+1)) 
done < "$input"
