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
  echo "`date '+%F %T'`: updating started" >>$DB_DIR/rules_loop.log

  ./osm3s_query --concise --rules <$DB_DIR/rules/areas.osm3s

  echo "`date '+%F %T'`: updating finished" >>$DB_DIR/rules_loop.log

}; done
