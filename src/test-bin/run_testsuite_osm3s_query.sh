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

perform_test()
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
    "$BASEDIR/bin/$1" $ARGS <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "$BASEDIR/bin/$1" $ARGS >stdout.log 2>stderr.log
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

perform_test_stdout_null()
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
    "$BASEDIR/bin/$1" $ARGS <"../../input/${EXEC}_$I/stdin.log" >/dev/null 2>stderr.log
  }; else
  {
    "$BASEDIR/bin/$1" $ARGS >/dev/null 2>stderr.log
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

# Prepare testing the statements
mkdir -p input/update_database/
rm -f input/update_database/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --version=mock-up-init <input/update_database/stdin.log

# Test osm3s_query
date +%T
perform_test osm3s_query 1
if [[ -z $NOTIMES  ]]; then
  perform_test osm3s_query 2 "--db-dir=../../input/update_database/ --verbose"
fi
perform_test osm3s_query 3 "--db-dir=../../input/update_database/"
perform_test osm3s_query 4 "--db-dir=../../input/update_database/ --concise"
perform_test osm3s_query 5 "--db-dir=../../input/update_database/ --quiet"
if [[ -z $NOTIMES  ]]; then
  perform_test osm3s_query 6 "--db-dir=../../input/update_database/ --verbose"
fi
perform_test osm3s_query 7 "--db-dir=../../input/update_database/"
perform_test osm3s_query 8 "--db-dir=../../input/update_database/ --concise"
perform_test osm3s_query 9 "--db-dir=../../input/update_database/ --quiet"
perform_test osm3s_query 10 "--db-dir=../../input/update_database/ --verbose"
perform_test osm3s_query 11 "--db-dir=../../input/update_database/"
perform_test osm3s_query 12 "--db-dir=../../input/update_database/ --concise"
perform_test osm3s_query 13 "--db-dir=../../input/update_database/ --quiet"
perform_test osm3s_query 14 "--db-dir=../../input/update_database/ --verbose"
perform_test osm3s_query 15 "--db-dir=../../input/update_database/"
perform_test osm3s_query 16 "--db-dir=../../input/update_database/ --concise"
perform_test osm3s_query 17 "--db-dir=../../input/update_database/ --quiet"

$BASEDIR/bin/dispatcher --osm-base --db-dir=input/update_database/ &
sleep 1
rm -f input/update_database/transactions.log
perform_test osm3s_query 18
cat input/update_database/transactions.log
$BASEDIR/bin/dispatcher --terminate

II=19
while [[ $II -lt 58 ]]; do
{
  perform_test osm3s_query $II "--db-dir=../../input/update_database/"
  II=$(($II + 1))
}; done

perform_test_stdout_null osm3s_query 58 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 59 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 60 "--db-dir=../../input/update_database/"
perform_test osm3s_query 61 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 62 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 63 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 64 "--db-dir=../../input/update_database/"
perform_test osm3s_query 65 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 66 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 67 "--db-dir=../../input/update_database/"
perform_test_stdout_null osm3s_query 68 "--db-dir=../../input/update_database/"
perform_test osm3s_query 69 "--db-dir=../../input/update_database/"

rm -f input/update_database/*
