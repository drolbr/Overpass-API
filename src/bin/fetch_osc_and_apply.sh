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
  echo "Usage: $0 diff_url"
  echo "Error : Set the URL to get diffs from (like https://planet.osm.org/replication/minute )"
  exit 0
};
fi

SOURCE_DIR="$1"

EXEC_DIR="$(dirname $0)/"
if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="$(pwd)/$EXEC_DIR"
}; fi

DB_DIR=$($EXEC_DIR/dispatcher --show-dir)
if [[ ! -s "$DB_DIR/replicate_id" ]]; then
  echo "$DB_DIR/replicate_id does not exist"
  exit 1
fi

START="auto"

get_replicate_filename()
{
  printf -v TDIGIT3 %03u $(($1 % 1000))
  ARG=$(($1 / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  REPLICATE_FILENAME=$TDIGIT1/$TDIGIT2/$TDIGIT3
  printf -v TARGET_FILE %09u $TARGET
};


fetch_file()
{
  wget -nv -O "$2" "$1"
};


collect_minute_diffs()
{
  MAX_AVAILABLE_REPLICATE_ID=$1
  TEMP_SOURCE_DIR=$2
  TEMP_TARGET_DIR=$3
  TARGET=$(($START + 1))

  while [[ ( $TARGET -le $MAX_AVAILABLE_REPLICATE_ID ) && ( $(($START + 1440)) -ge $TARGET ) && ( $(du -m "$TEMP_TARGET_DIR" | awk '{ print $1; }') -le 64 ) ]]; do
  {
    get_replicate_filename $TARGET
    fetch_file "$SOURCE_DIR/$REPLICATE_FILENAME.state.txt" "$TEMP_SOURCE_DIR/$TARGET_FILE.state.txt"
    fetch_file "$SOURCE_DIR/$REPLICATE_FILENAME.osc.gz" "$TEMP_SOURCE_DIR/$TARGET_FILE.osc.gz"

    gunzip <"$TEMP_SOURCE_DIR/$TARGET_FILE.osc.gz" >"$TEMP_TARGET_DIR/$TARGET_FILE.osc"

    TARGET=$(($TARGET + 1))
  }; done
  TARGET=$(($TARGET - 1))
};


apply_minute_diffs()
{
  ./update_from_dir --osc-dir=$1 --version=$DATA_VERSION --flush-size=0 &
  CHILD_PID=$!
  wait "$CHILD_PID"
  EXITCODE=$?
  if [[ $EXITCODE -eq 15 ]]; then  # SIGTERM
    echo "$(date -u '+%F %T'): update_from_dir terminated" >>$DB_DIR/apply_osc_to_db.log
    exit $EXITCODE
  fi
  while [[ $EXITCODE -ne 0 ]]; do
  {
    sleep 60
    ./update_from_dir --osc-dir=$1 --version=$DATA_VERSION --flush-size=0 &
    CHILD_PID=$!
    wait "$CHILD_PID"
    EXITCODE=$?
    if [[ $EXITCODE -eq 15 ]]; then  # SIGTERM
      echo "$(date -u '+%F %T'): update_from_dir terminated" >>$DB_DIR/apply_osc_to_db.log
      exit $EXITCODE
    fi
  }; done
  DIFF_COUNT=$(($DIFF_COUNT + 1))
};


update_state()
{
  get_replicate_filename $TARGET
  TIMESTAMP_LINE=$(grep "^timestamp" <$TEMP_SOURCE_DIR/$TARGET_FILE.state.txt)
  DATA_VERSION=${TIMESTAMP_LINE:10}
};


shutdown()
{
  if [[ $CHILD_PID -ge 1 ]]; then
    kill $CHILD_PID
  fi
  rm -fR $TEMP_SOURCE_DIR
  rm -fR $TEMP_TARGET_DIR
  exit 15
};


echo >>$DB_DIR/fetch_osc_and_apply.log

mkdir -p $DB_DIR/augmented_diffs/
DIFF_COUNT=0

# update_state

pushd "$EXEC_DIR"

if [[ $START == "auto" ]]; then
{
  START=$(($(cat $DB_DIR/replicate_id) + 0))
}; fi

trap shutdown SIGTERM

./migrate_database --migrate &
CHILD_PID=$!
wait "$CHILD_PID"

while [[ true ]]; do
{
  echo "$(date -u '+%F %T'): updating from $START" >>$DB_DIR/fetch_osc_and_apply.log

  TEMP_SOURCE_DIR=$(mktemp -d /tmp/osm-3s_update_XXXXXX)
  TEMP_TARGET_DIR=$(mktemp -d /tmp/osm-3s_update_XXXXXX)

  fetch_file "$SOURCE_DIR/state.txt" "$TEMP_SOURCE_DIR/state.txt"
  MAX_SEQ_NR=$(cat "$TEMP_SOURCE_DIR/state.txt" | grep -aE '^sequenceNumber')
  MAX_AVAILABLE_REPLICATE_ID=$((${MAX_SEQ_NR:15} + 0))

  collect_minute_diffs $MAX_AVAILABLE_REPLICATE_ID $TEMP_SOURCE_DIR $TEMP_TARGET_DIR

  if [[ $TARGET -gt $START ]]; then
  {
    echo "$(date -u '+%F %T'): updating to $TARGET" >>$DB_DIR/fetch_osc_and_apply.log

    update_state
    apply_minute_diffs $TEMP_TARGET_DIR
    echo "$TARGET" >$DB_DIR/replicate_id

    echo "$(date -u '+%F %T'): update complete $TARGET" >>$DB_DIR/fetch_osc_and_apply.log
  };
  else
  {
    sleep 10
  }; fi

  rm -f $TEMP_TARGET_DIR/*
  rmdir $TEMP_TARGET_DIR
  rm -f $TEMP_SOURCE_DIR/*
  rmdir $TEMP_SOURCE_DIR

  START=$TARGET
}; done
