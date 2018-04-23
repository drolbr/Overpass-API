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

SOURCE_DIR="$1"
LOCAL_DIR="$2"
TARGET_TIME="$3"
LOWER="$4"
UPPER="$5"
TARGET=

get_replicate_filename()
{
  printf -v TDIGIT3 %03u $(($TARGET % 1000))
  ARG=$(($TARGET / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  LOCAL_PATH="$LOCAL_DIR/$TDIGIT1/$TDIGIT2"
  REPLICATE_FILENAME="$LOCAL_PATH/$TDIGIT3"
  REMOTE_PATH="$SOURCE_DIR/$TDIGIT1/$TDIGIT2"
  REMOTE_FILE="$REMOTE_PATH/$TDIGIT3"
};

# $1 - remote source
# $2 - local destination
fetch_file()
{
  wget -nv -O "$2" "$1"
};

update_state()
{
  get_replicate_filename
  if [[ ! -s "$REPLICATE_FILENAME.state.txt" ]]; then {
    mkdir -p "$LOCAL_PATH"
    fetch_file "$REMOTE_FILE.state.txt" "$REPLICATE_FILENAME.state.txt"
  }; fi
  if [[ -s "$REPLICATE_FILENAME.state.txt" ]]; then {
    TIMESTAMP_LINE=`grep "^timestamp" <"$REPLICATE_FILENAME.state.txt"`
    DATA_VERSION=${TIMESTAMP_LINE:10}
  }; fi
};

while [[ $(($LOWER + 1)) -lt $UPPER ]]; do
{
  TARGET=$((($LOWER + $UPPER) / 2))
  update_state
  #echo "$TARGET - $TIMESTAMP_LINE"
  if [[ -s "$REPLICATE_FILENAME.state.txt" && "$DATA_VERSION" < "$TARGET_TIME" ]]; then
  {
    LOWER=$TARGET
  }; else
  {
    UPPER=$TARGET
  }; fi
}; done

echo "$LOWER"
