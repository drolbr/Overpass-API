#!/usr/bin/env bash

# Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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
# along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.

if [[ -z $1  ]]; then
{
  echo Usage: $0 Replicate_id Source_dir Local_dir [Sleep]
  exit 0
};
fi

REPLICATE_ID=$1
SOURCE_DIR=$2
LOCAL_DIR=$3
SLEEP_BETWEEN_DLS=${4:-15}  # How long to sleep between download attempts (sec). Default: 15

if [[ ! -d $LOCAL_DIR ]];
  then {
    mkdir $LOCAL_DIR
};
fi

# $1 - remote source
# $2 - local destination
fetch_file()
{
  wget -nv -O "$2" "$1"
};

retry_fetch_file()
{
  if [[ ! -s "$2" ]]; then {
    fetch_file "$1" "$2"
    if [[ "$3" == "gzip" ]]; then {
      gunzip -t <"$2"
      if [[ $? -ne 0 ]]; then {
        rm "$2"
      }; fi
    }; fi
  }; fi
  until [[ -s "$2" ]]; do {
    sleep $SLEEP_BETWEEN_DLS
    fetch_file "$1" "$2"
    if [[ "$3" == "gzip" ]]; then {
      gunzip -t <"$2"
      if [[ $? -ne 0 ]]; then {
        rm "$2"
      }; fi
    }; fi
  }; done
};

fetch_minute_diff()
{
  printf -v TDIGIT3 %03u $(($1 % 1000))
  ARG=$(($1 / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  
  LOCAL_PATH="$LOCAL_DIR/$TDIGIT1/$TDIGIT2"
  REMOTE_PATH="$SOURCE_DIR/$TDIGIT1/$TDIGIT2"
  mkdir -p "$LOCAL_DIR/$TDIGIT1/$TDIGIT2"

  retry_fetch_file "$REMOTE_PATH/$TDIGIT3.osc.gz" "$LOCAL_PATH/$TDIGIT3.osc.gz" "gzip"
  retry_fetch_file "$REMOTE_PATH/$TDIGIT3.state.txt" "$LOCAL_PATH/$TDIGIT3.state.txt" "text"

  TIMESTAMP_LINE=`grep timestamp $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt`
  TIMESTAMP=${TIMESTAMP_LINE:10}
};

while [[ true ]];
do
{
  REPLICATE_ID=$(($REPLICATE_ID + 1))
  fetch_minute_diff $REPLICATE_ID
  sleep 1
  echo "fetch_osc()@"`date "+%F %T"`": new_replicate_diff $REPLICATE_ID $TIMESTAMP" >>$LOCAL_DIR/fetch_osc.log
};
done
