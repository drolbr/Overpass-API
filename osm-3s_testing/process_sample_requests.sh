#!/usr/bin/env bash

date '+%F %T'

./process_sample_requests_node.sh 1 &
./process_sample_requests_node.sh 2 &
./process_sample_requests_node.sh 3 &
./process_sample_requests_node.sh 4 &
./process_sample_requests_node.sh 5 &
./process_sample_requests_node.sh 6 &
./process_sample_requests_node.sh 7 &
./process_sample_requests_node.sh 8 &
./process_sample_requests_node.sh 9 &
./process_sample_requests_node.sh a &
./process_sample_requests_node.sh b &
./process_sample_requests_node.sh c &
./process_sample_requests_node.sh d &
./process_sample_requests_node.sh e &
./process_sample_requests_node.sh f &

sleep 5
complete=$(cat node.?.txt | grep -E 'done' | wc -l)
while [[ $complete -lt 15 ]]; do
  echo -n "$(date '+%F %T')  $complete"
  for i in node.?.txt; do { echo -n "  $(cat $i | wc -l)"; }; done
  echo
  sleep 5
  complete=$(cat node.?.txt | grep -E 'done' | wc -l)
done
