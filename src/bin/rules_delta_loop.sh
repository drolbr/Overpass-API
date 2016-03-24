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
  echo Usage: $0 database_dir
  exit 0
};
fi

DB_DIR=$1

EXEC_DIR="`dirname $0`/"
if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="`pwd`/$EXEC_DIR"
};
fi

pushd "$EXEC_DIR"

while [[ true ]]; do
{
  echo "`date '+%F %T'`: update started" >>$DB_DIR/rules_loop.log
  sed "s/{{area_version}}/$(cat $DB_DIR/area_version)/g" $DB_DIR/rules/areas_delta.osm3s | ./osm3s_query --progress --rules
  echo "`date '+%F %T'`: update finished" >>$DB_DIR/rules_loop.log
  sleep 3600
}; done
