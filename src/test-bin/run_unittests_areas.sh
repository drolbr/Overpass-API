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
  echo "Usage: $0 test_size"
  echo
  echo "An appropriate value for a fast test is 40, a comprehensive value is 2000."
  exit 0
};
fi

# The size of the test pattern. Asymptotically, the test pattern consists of
# size^2 elements. The size must be divisible by ten. For a full featured test,
# set the value to 2000.
DATA_SIZE="$1"
BASEDIR="$(cd `dirname $0` && pwd)/.."
NOTIMES="$2"

evaluate_test()
{
  DIRNAME="$1"

  FAILED=
  for FILE in `ls ../../expected/$DIRNAME/`; do
  {
    if [[ ! -f $FILE ]]; then
    {
      echo "In Test $EXEC $I: Expected file \"$FILE\" doesn't exist."
      FAILED=YES
    }; fi
  }; done
  for FILE in `ls`; do
  {
    if [[ ! -f "../../expected/$DIRNAME/$FILE" ]]; then
    {
      echo "In Test $EXEC $I: Unexpected file \"$FILE\" exists."
      FAILED=YES
    }; else
    {
      RES=`diff -q "../../expected/$DIRNAME/$FILE" "$FILE"`
      if [[ -n $RES ]]; then
      {
        echo $RES
        FAILED=YES
      }; fi
    }; fi
  }; done
};

perform_serial_test()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  if [[ -s "../../input/${EXEC}_$I/stdin.log" ]]; then
  {
    #echo "stdin.log found"
    "$BASEDIR/test-bin/$1" "$I" $ARGS <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "$BASEDIR/test-bin/$1" "$I" $ARGS >stdout.log 2>stderr.log
  }; fi
  evaluate_test "${EXEC}_$I"
  if [[ -n $FAILED ]]; then
  {
    echo `date +%T` "Test $EXEC $I FAILED."
  }; else
  {
    echo `date +%T` "Test $EXEC $I succeeded."
    rm -R *
  }; fi
  popd >/dev/null
};

perform_test_loop()
{
  I=1
  while [[ $I -le $2 ]]; do
  {
    perform_serial_test "$1" $I "$3"
    I=$(($I + 1))
  }; done
};

# Prepare testing the statements
date +%T
mkdir -p input/update_database/
rm -f input/update_database/*
$BASEDIR/test-bin/generate_test_file_areas $DATA_SIZE input >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --version=mock-up-init <input/update_database/stdin.log

# Test the print and id_query statements
date +%T
$BASEDIR/test-bin/make_area create $DATA_SIZE input/update_database/

date +%T
mkdir -p expected/make_area_4/
$BASEDIR/test-bin/generate_test_file_areas $DATA_SIZE result >expected/make_area_4/stdout.log
touch expected/make_area_4/stderr.log
perform_test_loop make_area 4 "$DATA_SIZE ../../input/update_database/"

rm -f input/update_database/*

# Prepare testing the statements
date +%T
mkdir -p input/update_database/
rm -f input/update_database/*
$BASEDIR/test-bin/generate_test_file_areas $DATA_SIZE >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --version=mock-up-init <input/update_database/stdin.log

# Run the area updater in parallel to a running update
date +%T
$BASEDIR/bin/dispatcher --osm-base --db-dir=input/update_database/ &
$BASEDIR/bin/dispatcher --areas --db-dir=input/update_database/ &
sleep 1
rm -f input/update_database/transactions.log
$BASEDIR/bin/osm3s_query --rules <input/rule_processor/rules.osm &
sleep 1
$BASEDIR/bin/update_database --version=mock-up-diff <input/rule_processor/deletions.osm
sleep 10
mkdir -p run/rule_processor/
rm -f run/rule_processor/*
$BASEDIR/bin/osm3s_query <input/rule_processor/check_area_query.osm >run/rule_processor/check_area_query.log
$BASEDIR/bin/osm3s_query <input/rule_processor/check_coord_query.osm >run/rule_processor/check_coord_query.log
$BASEDIR/bin/osm3s_query <input/rule_processor/check_query_tags.osm >run/rule_processor/check_query_tags.log
$BASEDIR/bin/osm3s_query <input/rule_processor/check_area_query_ways.osm >run/rule_processor/check_area_query_ways.log
$BASEDIR/bin/osm3s_query <input/rule_processor/check_area_query_relations.osm >run/rule_processor/check_area_query_relations.log
$BASEDIR/bin/osm3s_query <input/rule_processor/check_pivot.osm >run/rule_processor/check_pivot.log

echo "(area[triangle];area[shapes];area[multpoly];);out;" | $BASEDIR/bin/osm3s_query >result.osm

$BASEDIR/bin/dispatcher --terminate
$BASEDIR/bin/dispatcher --areas --terminate

pushd "run/rule_processor/" >/dev/null
EXEC=rule_processor
I=1
evaluate_test "rule_processor"
if [[ -n $FAILED ]]; then
{
  echo `date +%T` "Test rule_processor 1 FAILED."
}; else
{
  echo `date +%T` "Test rule_processor 1 succeeded."
  rm -R *
}; fi
popd >/dev/null

#rm -f input/update_database/*
