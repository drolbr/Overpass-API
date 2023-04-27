#!/usr/bin/env bash

STARTED_COUNT=0
COMPLETED_COUNT=0

while [[ true ]]; do
{
  read FOO1 FOO2 FOO3 TYPE FOO5
  if [[ $TYPE == "request_read_and_idx" ]]; then
  {
    STARTED_COUNT=$((STARTED_COUNT + 1))
    if [[ $(($STARTED_COUNT % 100)) -eq 0 ]]; then
      echo "" >>/OVERPASS_DB_DIR/started_count.dot # adapt directory
    else
      echo -n "." >>/OVERPASS_DB_DIR/started_count.dot # adapt directory
    fi
  }
  elif [[ $TYPE == "read_finished" ]]; then
  {
    COMPLETED_COUNT=$((COMPLETED_COUNT + 1))
    if [[ $(($COMPLETED_COUNT % 100)) -eq 0 ]]; then
      echo "" >>/OVERPASS_DB_DIR/completed_count.dot # adapt directory
    else
      echo -n "." >>/OVERPASS_DB_DIR/completed_count.dot # adapt directory
    fi
  }; fi
}; done
