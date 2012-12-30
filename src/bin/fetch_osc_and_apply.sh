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
  echo "Usage: $0 diff_url --meta=(yes|no)"
  echo "Error : Set the URL to get diffs from (like http://planet.osm.org/replication/minute )"
  exit 0
};
fi

DIFF_URL=$1

if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="`pwd`/$EXEC_DIR"
}; fi

DB_DIR=`$EXEC_DIR/dispatcher --show-dir`

if [[ ! ${DB_DIR:0:1} == "/" ]]; then
{
  echo "Is dispatcher running ? cannot get the database directory path. Answer : $DB_DIR"
  exit 0
}; fi

EXEC_DIR="`dirname $0`/"
REPLICATE_ID=`cat $DB_DIR/replicate_id`
if [[ -z $REPLICATE_ID ]] ;  then
{
  echo "Unable to get the last replicated ID of the Database"
  echo "Please update $DB_DIR/replicate_id with a replication sequence number matching database age"
  exit 0
}; fi


fetch_and_apply_minute_diff()
{
  printf -v TDIGIT3 %03u $(($1 % 1000))
  ARG=$(($1 / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  
  REMOTE_PATH="$DIFF_URL/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz"
  #echo $REMOTE_PATH
  wget -q -O - "$REMOTE_PATH" > /tmp/diff.osc.gz 
  if [[ ! $? == 0 ]] ; then
    return 77
  fi
  
  gunzip -c /tmp/diff.osc.gz | $EXEC_DIR/update_database $2 > /dev/null
  return $?

};

#Default is no meta
META=

if [[ $2 == "--meta=yes" ]]; then
{
  META="--meta"
}; fi



while [[ true ]];
do
{
  REPLICATE_ID=$(($REPLICATE_ID + 1))
  echo "`date '+%F %T'`: updating to $REPLICATE_ID" >>$DB_DIR/apply_osc_to_db.log
  fetch_and_apply_minute_diff $REPLICATE_ID $META
  if [[ $? == 0 ]] ; then # Update success
  {
    echo "$REPLICATE_ID" > $DB_DIR/replicate_id
    echo "`date '+%F %T'`: update complete" $REPLICATE_ID >>$DB_DIR/apply_osc_to_db.log
  }
  else
  {
    REPLICATE_ID=$(($REPLICATE_ID - 1))
    sleep 5 #Wait 5 seconds for the diff to be uvailable
  }; fi
};
done

