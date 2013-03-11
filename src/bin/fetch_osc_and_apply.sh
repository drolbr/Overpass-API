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

TMP_DIFF=/tmp/diff.osc.gz
TMP_DIFF_UNCOMPRESS=/tmp/diff.osc
TMP_STATE=/tmp/state.txt
DIFF_URL=$1
EXEC_DIR="`dirname $0`/"
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
  
  REMOTE_PATH="$DIFF_URL/$TDIGIT1/$TDIGIT2/$TDIGIT3"
  REMOTE_DIFF="$REMOTE_PATH.osc.gz"
  REMOTE_STATE="$REMOTE_PATH.state.txt"

  wget -q -O - "$REMOTE_DIFF" > $TMP_DIFF
  if [[ ! $? == 0 ]] ; then # Diff failed to download (404, no answers, etc.)
    rm $TMP_DIFF
    return 1
  fi
  gunzip -c $TMP_DIFF > $TMP_DIFF_UNCOMPRESS
  if [[ ! $? == 0 ]] ; then # For unknown reasons, the diff get sometimes corrupted or empty, don't try to apply
  {
    rm $TMP_DIFF 2>/dev/null
    rm $TMP_DIFF_UNCOMPRESS 2>/dev/null
    return 2
  } ; fi
  cat $TMP_DIFF_UNCOMPRESS | $EXEC_DIR/update_database $2 >> /dev/null 2>&1
  ret=$?
  rm $TMP_DIFF 2>/dev/null
  rm $TMP_DIFF_UNCOMPRESS 2>/dev/null
  
  #Update the timestamp
  while [[ true ]];
  do
  	wget -q -O - "$REMOTE_STATE" > $TMP_STATE
  	if [[ ! $? == 0 ]] ; then # In case the state file wasn't there in time but the diff was, let'y wait until it become available
  	{
  	  rm $TMP_SATE
  	  sleep 1
  	} else
  	{
  	  grep timestamp $TMP_STATE | cut -f2 -d\= > $DB_DIR/osm_base_version
  	  cp $DB_DIR/osm_base_version $DB_DIR/osm_base_version_munin
  	  rm $TMP_STATE
  	  return $ret
  	}; fi	
  done

};

#Default is no meta
META_OPTION=

if [[ $2 == "--meta=yes" ]]; then
{
  META_OPTION="--meta"
}; fi



while [[ true ]];
do
{
  REPLICATE_ID=$(($REPLICATE_ID + 1))
  echo "`date '+%F %T'`: trying to apply $REPLICATE_ID" >>$DB_DIR/apply_osc_to_db.log
  fetch_and_apply_minute_diff $REPLICATE_ID $META_OPTION
  if [[ $? == 0 ]] ; then # Update success
  {
    echo "$REPLICATE_ID" > $DB_DIR/replicate_id
    echo "`date '+%F %T'`: update complete of $REPLICATE_ID" >>$DB_DIR/apply_osc_to_db.log
  }
  else
  {
    REPLICATE_ID=$(($REPLICATE_ID - 1))
    sleep 10 #Wait for the diff to be available
  }; fi
};
done

