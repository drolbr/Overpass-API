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
  echo "Usage: $0 base_url"
  exit 0
}; fi

BASE_URL="$1"
EXEC_DIR="`dirname $0`/"
DB_DIR="`$EXEC_DIR/dispatcher --show-dir`"
CLONE_BASE_DIR="/var/www/clone"
CLONE_DIR="$CLONE_BASE_DIR/`date '+%F'`"

# drop diffs that are older than one day
for I in "$CLONE_BASE_DIR"/????-??-??; do
{
  BASEDATE=`basename $I`
  YESTERDAY=$((`date '+%s'` - 86400))
  if [[ $BASEDATE < `date -d@$YESTERDAY '+%F'` ]]; then
  {
    echo "drop $I"
    rm -Rf $I
  }; else
  {
    echo "keep $I"
  }; fi
}; done

if [[ ! ${DB_DIR:0:1} == "/" ]]; then
{
  DB_DIR="`pwd`/$DB_DIR"
}; fi

ABS_CLONE_DIR="$CLONE_DIR"
if [[ ! ${ABS_CLONE_DIR:0:1} == "/" ]]; then
{
  ABS_CLONE_DIR="`pwd`/$ABS_CLONE_DIR"
}; fi

if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="`pwd`/$EXEC_DIR"
}; fi

pushd "$EXEC_DIR"

mkdir -p "$ABS_CLONE_DIR"
rm -f "$ABS_CLONE_DIR"/*.bin
rm -f "$ABS_CLONE_DIR"/*.map
rm -f "$ABS_CLONE_DIR"/*.idx
#cp "$DB_DIR/replicate_id" "$ABS_CLONE_DIR/replicate_id"
./osm3s_query --clone="$ABS_CLONE_DIR"

echo "$BASE_URL/`basename $CLONE_DIR`" >"$CLONE_BASE_DIR/latest_dir"

popd
