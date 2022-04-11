#!/bin/bash

FILE=$1

echo ""
echo "Entries: $(tail -n +4000 $FILE | wc -l)  Begin: $(tail -n +4000 $FILE | head -n 1 | awk '{print $1}')  End: $(tail -n 1 $FILE | awk '{print $1}')"
echo ""
echo "Min: $(tail -n +4000 $FILE | awk '{print $3}' | sort -rn | tail -n 1)"

echo ""
echo "50%: $(tail -n +4000 $FILE | awk '{print $3}' | sort -n| awk '{arr[NR]=$1} END { print arr[int(NR*0.50 -0.5 + 1)]}')"
echo "90%: $(tail -n +4000 $FILE | awk '{print $3}' | sort -n| awk '{arr[NR]=$1} END { print arr[int(NR*0.90 -0.5 + 1)]}')"
echo "99%: $(tail -n +4000 $FILE | awk '{print $3}' | sort -n| awk '{arr[NR]=$1} END { print arr[int(NR*0.99 -0.5 + 1)]}')"

echo ""
echo "Max: $(tail -n +4000 $FILE | awk '{print $3}' | sort -n | tail -n 1)"
