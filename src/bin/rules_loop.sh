#!/bin/bash

if [[ -z $1  ]]; then
{
  echo Usage: $0 database_dir
  exit 0
};
fi

DB_DIR=$1

while [[ true ]]; do
{
  echo "`date '+%F %T'`: update started" >>$DB_DIR/rules_loop.log
  ./osm3s_query --progress --rules <$DB_DIR/rules/areas.osm3s
  echo "`date '+%F %T'`: update finished" >>$DB_DIR/rules_loop.log
  sleep 3
}; done
