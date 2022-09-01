#!/bin/sh

rm checkpatch_report
rm scan_files

find ApplicationLayer/ -print >> scan_files
find HardwareAbstraction/ -print >> scan_files
find Wrapper/ -print >> scan_files
find Silicon/ -print >> scan_files

while read -r line; do ./scripts/checkpatch.pl -f $line  >> checkpatch_report; done < scan_files


#while read line; do 
#  if [ -f "$line" ]
#  then 
#    uncrustify --replace --no-backup -l C -c /home/charles/Documents/zephyr/.uncrustify.cfg $line
#  fi
#done < scan_files
