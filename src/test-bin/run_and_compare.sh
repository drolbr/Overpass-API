#!/bin/bash

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
  compare_files coord_source.csv coord_db.csv
  rm coord_source.csv coord_db.csv
  compare_files tags_source.csv tags_local.csv
  compare_files tags_source.csv tags_global.csv
  rm tags_source.csv tags_local.csv tags_global.csv
};

way_compare_test()
{
  $TEST_BIN_DIR/way_updater
  compare_files member_source.csv member_db.csv
  rm member_source.csv member_db.csv
  compare_files tags_source.csv tags_local.csv
  compare_files tags_source.csv tags_global.csv
  rm tags_source.csv tags_local.csv tags_global.csv
};

relation_compare_test()
{
  $TEST_BIN_DIR/relation_updater
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
