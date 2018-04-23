#!/usr/bin/env bash

# Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
#
# This file is part of Overpass_API.
#
# Overpass_API is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Overpass_API is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Overpass_API. If not, see <https://www.gnu.org/licenses/>.

if [[ -z $1  ]]; then
{
  echo "Usage: $0 start_diff"
  exit 0
}; fi


DIFFS_TO_KEEP=60
CURRENT_DIFF="$1"
DB_DIR="`./dispatcher --show-dir`"
CACHE_DIR="/tmp/osm3s_augmented_diffs_cache/"


mkdir -p $CACHE_DIR
while [[ true ]]; do

  # Delete outdated files in CACHE_DIR
  for FILE in /tmp/osm3s_augmented_diffs_cache/*.osc.gz; do
    if [[ `basename $FILE .osc.gz` -lt $(($CURRENT_DIFF - $DIFFS_TO_KEEP)) ]]; then
      echo "Purged $FILE"
      rm $FILE
    fi
  done

  EPOCHSECS=$(($CURRENT_DIFF * 60 + 1347432900))
  SINCE=`date --utc --date="@$EPOCHSECS" '+%FT%H:%M:%SZ'`
  UNTIL=`date --utc --date="@$(($EPOCHSECS + 60))" '+%FT%H:%M:%SZ'`

  while [[ `../cgi-bin/timestamp | tail -n 1` < $UNTIL ]]; do
    echo "`../cgi-bin/timestamp | tail -n 1` vs. $UNTIL"
    sleep 5
  done

  FILE="$CACHE_DIR/$CURRENT_DIFF.osc.gz"
  echo -n "Create $FILE ..."

  QUERY_STRING='[adiff:"'$SINCE'","'$UNTIL'"];(node(changed:"'$SINCE'","'$UNTIL'");way(changed:"'$SINCE'","'$UNTIL'");rel(changed:"'$SINCE'","'$UNTIL'"););out meta geom;'
  echo $QUERY_STRING | ./osm3s_query | gzip >"$FILE"
  echo " done."

  echo $CURRENT_DIFF >"$CACHE_DIR/newest"
  CURRENT_DIFF=$(($CURRENT_DIFF + 1))

done
