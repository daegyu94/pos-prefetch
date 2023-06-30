#!/bin/bash 

if [ $# -ne 1 ]; then
  echo "$0 <record/report>"
  exit
fi

type=$1

app=`python3 ~/pos-prefetch/initiator/main.py`

if [ "$type" == "record" ]; then
  echo "perf: $app_pid"
  uftrace record $app
elif [ "$type" == "report" ]; then
  uftrace report
fi

