#!/usr/bin/env bash

i=1
while [[ $i -le $1 ]]; do
  ../build/dispatcher.perf client $2 $3 >>err.log
  i=$(($i + 1))
done

