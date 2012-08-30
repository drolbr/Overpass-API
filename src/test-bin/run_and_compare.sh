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

TEST_BIN_DIR="$(cd `dirname $0` && pwd)"

compare_files()
{
  FILE1="$1"
  FILE2="$2"

  I=1
  while [[ $I -le 9 ]]; do
  {
    grep "^$I" <"$1" | sort >"$1.$I"
    grep "^$I" <"$2" | sort >"$2.$I"
    diff -q "$1.$I" "$2.$I"
    rm "$1.$I" "$2.$I"
    I=$(($I + 1))
  }; done
};

node_compare_test()
{
  $TEST_BIN_DIR/node_updater
  $TEST_BIN_DIR/compare_osm_base_maps --db-dir=./
  rm *.bin *.map *.idx
  compare_files coord_source.csv coord_db.csv
  rm coord_source.csv coord_db.csv
  compare_files tags_source.csv tags_local.csv
  compare_files tags_source.csv tags_global.csv
  rm tags_source.csv tags_local.csv tags_global.csv
};

way_compare_test()
{
  $TEST_BIN_DIR/way_updater
  $TEST_BIN_DIR/compare_osm_base_maps --db-dir=./
  rm *.bin *.map *.idx
  compare_files member_source.csv member_db.csv
  rm member_source.csv member_db.csv
  compare_files tags_source.csv tags_local.csv
  compare_files tags_source.csv tags_global.csv
  rm tags_source.csv tags_local.csv tags_global.csv
};

relation_compare_test()
{
  $TEST_BIN_DIR/relation_updater
  $TEST_BIN_DIR/compare_osm_base_maps --db-dir=./
  rm *.bin *.map *.idx
  compare_files member_source.csv member_db.csv
  rm member_source.csv member_db.csv
  compare_files tags_source.csv tags_local.csv
  compare_files tags_source.csv tags_global.csv
  rm tags_source.csv tags_local.csv tags_global.csv
};

if [[ "$1" == 1 ]]; then
{
  node_compare_test
}; fi
if [[ "$1" == 2 ]]; then
{
  way_compare_test
}; fi
if [[ "$1" == 3 ]]; then
{
  relation_compare_test
}; fi
