#!/bin/bash 

ARGS=$1

echo "$ARGS"
if [ -z "$ARGS" ]; then
  pushd build && rm CMakeCache.txt; cmake .. && make && popd
else
  pushd build && rm CMakeCache.txt; cmake "-D$ARGS=on" .. && make && popd
fi
