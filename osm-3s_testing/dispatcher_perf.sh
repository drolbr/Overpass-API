#!/usr/bin/env bash

/usr/bin/time -v ../build/dispatcher.perf server 1000000 &

sleep 1
rm -f err.log

for i in `seq 1 600`; do
  ../osm-3s_testing/dispatcher_client.sh 40 500 $(($i * 1001)) &
done
for i in `seq 1 600`; do
  ../osm-3s_testing/dispatcher_client.sh 10 2000 $(($i * 1001)) &
done

sleep 20
while [[ `ps -ef | grep dispatcher_client | wc -l` -ge 2 ]]; do
  ps -ef | grep dispatcher_client | wc -l
  date '+%T'
  sleep 5
done
../build/dispatcher.perf status
../build/dispatcher.perf terminate
sleep 1

