#!/bin/bash 

perf_bin="/usr/src/linux-5.4/tools/perf/perf"
perf_output="/root/pos-prefetch/perf.data"
flamegraph_dir="/root/FlameGraph"

app_pid=`pgrep python3`

if [ $# -ne 1 ]; then
  echo "$0 <record/report/flamegraph>"
  exit
fi

type=$1

if [ "$type" == "record" ]; then
  echo "perf: $app_pid"
  $perf_bin record -o $perf_output -g -p $app_pid 
elif [ "$type" == "report" ]; then
  $perf_bin report -i $perf_output
elif [ "$type" == "flamegraph" ]; then
  $perf_bin script -i $perf_output | $flamegraph_dir/stackcollapse-perf.pl | $flamegraph_dir/flamegraph.pl > /root/pos-prefetch/flamegraph.svg
fi
