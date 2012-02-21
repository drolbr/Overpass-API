#!/usr/bin/env bash

# Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
#
# This file is part of Overpass_API.
#
# Overpass_API is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Overpass_API is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.

if [[ -z $4  ]]; then
{
  echo "Usage: $0 database_dir replicate_dir start_id --meta=(yes|no)"
  exit 0
}; fi

DB_DIR="$1"
REPLICATE_DIR="$2"
START=$3
META=

if [[ $4 == "--meta=yes" || $4 == "--meta" ]]; then
{
  META="--meta"
}
elif [[ $4 == "--meta=no" ]]; then
{
  META=
}
else
{
  echo "You must specify --meta=yes or --meta=no"
  exit 0
}; fi

EXEC_DIR="`dirname $0`/"
if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="`pwd`/$EXEC_DIR"
}; fi

get_replicate_filename()
{
  printf -v TDIGIT3 %03u $(($TARGET % 1000))
  ARG=$(($TARGET / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  REPLICATE_FILENAME=$REPLICATE_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3
};

collect_minute_diffs()
{
  TEMP_DIR=$1
  TARGET=$(($START + 1))

  get_replicate_filename

  while [[ ( -s $REPLICATE_FILENAME.state.txt ) && ( $(($START + 720)) -ge $(($TARGET)) ) ]];
  do
  {
    printf -v TARGET_FILE %09u $TARGET
    gunzip <$REPLICATE_FILENAME.osc.gz >$TEMP_DIR/$TARGET_FILE.osc
    TARGET=$(($TARGET + 1))
    get_replicate_filename
  };
  done
  TARGET=$(($TARGET - 1))
};

apply_minute_diffs()
{
  ./update_from_dir --osc-dir=$1 --version=$DATA_VERSION $META
  EXITCODE=$?
  while [[ $EXITCODE -ne 0 ]];
  do
  {
    sleep 60
    ./update_from_dir --osc-dir=$1 --version=$DATA_VERSION $META
    EXITCODE=$?
  };
  done
};

update_state()
{
  get_replicate_filename
  TIMESTAMP_LINE=`grep "^timestamp" <$REPLICATE_FILENAME.state.txt`
  while [[ -z $TIMESTAMP_LINE ]]; do
  {
    sleep 5
    TIMESTAMP_LINE=`grep "^timestamp" <$REPLICATE_FILENAME.state.txt`
  }; done
  DATA_VERSION=${TIMESTAMP_LINE:10}
};

echo >>$DB_DIR/apply_osc_to_db.log

# update_state

pushd "$EXEC_DIR"

while [[ true ]]; do
{
  echo "`date '+%F %T'`: updating from $START" >>$DB_DIR/apply_osc_to_db.log

  TEMP_DIR=`mktemp -d /tmp/osm-3s_update_XXXXXX`
  collect_minute_diffs $TEMP_DIR

  if [[ $TARGET -gt $START ]]; then
  {
    echo "`date '+%F %T'`: updating to $TARGET" >>$DB_DIR/apply_osc_to_db.log

    update_state

    apply_minute_diffs $TEMP_DIR
    echo "$TARGET" >$DB_DIR/replicate_id

    echo "`date '+%F %T'`: update complete" $TARGET >>$DB_DIR/apply_osc_to_db.log
  };
  else
  {
    sleep 5
  }; fi

  rm -f $TEMP_DIR/*
  rmdir $TEMP_DIR

  START=$TARGET
}; done
