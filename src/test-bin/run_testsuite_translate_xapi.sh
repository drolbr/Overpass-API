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
  I="$1"
  ARGS="$2"

  mkdir -p "run/translate_xapi_$I"
  pushd "run/translate_xapi_$I/" >/dev/null
  rm -f *

  "$BASEDIR/bin/translate_xapi" "$ARGS" >stdout.log 2>stderr.log
  evaluate_test "translate_xapi_$I"
  if [[ -n $FAILED ]]; then
  {
    echo `date +%T` "Test translate_xapi $I FAILED."
  }; else
  {
    echo `date +%T` "Test translate_xapi $I succeeded."
    rm -R *
  }; fi
  popd >/dev/null
};

# Test translate_xapi
date +%T
perform_test 1 "node[key=*]"
perform_test 2 "node[key=value1|value2|value3]"
perform_test 3 "node[key1=value1][key2=value2]"
perform_test 4 "node[bbox=7.0,51.0,8.0,52.0]"
perform_test 5 "node[bbox=7.0,51.0,8.0,52.0][key=value1|value2]"
perform_test 6 "node[key1=value1][bbox=7.0,51.0,8.0,52.0][key2=value2]"
perform_test 7 "node[key=value][@meta]"
perform_test 8 "node[key=value][@timeout=300]"
perform_test 9 "node[key=value][@user=Username]"
perform_test 10 "node[key=value1|value2|value3][@uid=496]"
perform_test 11 "node[key=value][@newer=2011-01-01T02:02:02Z]"
perform_test 12 "node[key=value1|value2|value3][@newer=2011-01-01T02:02:02Z]"
perform_test 21 "way[key=*]"
perform_test 22 "way[key=value1|value2|value3]"
perform_test 23 "way[key1=value1][key2=value2]"
perform_test 24 "way[bbox=7.0,51.0,8.0,52.0]"
perform_test 25 "way[bbox=7.0,51.0,8.0,52.0][key=value1|value2]"
perform_test 26 "way[key1=value1][bbox=7.0,51.0,8.0,52.0][key2=value2]"
perform_test 27 "way[key=value][@meta]"
perform_test 28 "way[key=value][@timeout=300]"
perform_test 29 "way[key=value][@user=Username]"
perform_test 30 "way[key=value1|value2|value3][@uid=496]"
perform_test 31 "way[key=value][@newer=2011-01-01T02:02:02Z]"
perform_test 32 "way[key=value1|value2|value3][@newer=2011-01-01T02:02:02Z]"
perform_test 41 "relation[key=*]"
perform_test 42 "relation[key=value1|value2|value3]"
perform_test 43 "relation[key1=value1][key2=value2]"
perform_test 44 "relation[bbox=7.0,51.0,8.0,52.0]"
perform_test 45 "relation[bbox=7.0,51.0,8.0,52.0][key=value1|value2]"
perform_test 46 "relation[key1=value1][bbox=7.0,51.0,8.0,52.0][key2=value2]"
perform_test 47 "relation[key=value][@meta]"
perform_test 48 "relation[key=value][@timeout=300]"
perform_test 49 "relation[key=value][@user=Username]"
perform_test 50 "relation[key=value1|value2|value3][@uid=496]"
perform_test 51 "relation[key=value][@newer=2011-01-01T02:02:02Z]"
perform_test 52 "relation[key=value1|value2|value3][@newer=2011-01-01T02:02:02Z]"
perform_test 61 "*[key=*]"
perform_test 62 "*[key=value1|value2|value3]"
perform_test 63 "*[key1=value1][key2=value2]"
perform_test 64 "*[bbox=7.0,51.0,8.0,52.0]"
perform_test 65 "*[bbox=7.0,51.0,8.0,52.0][key=value1|value2]"
perform_test 66 "*[key1=value1][bbox=7.0,51.0,8.0,52.0][key2=value2]"
perform_test 67 "*[key=value][@meta]"
perform_test 68 "*[key=value][@timeout=300]"
perform_test 69 "*[key=value][@user=Username]"
perform_test 70 "*[key=value1|value2|value3][@uid=496]"
perform_test 71 "*[key=value][@newer=2011-01-01T02:02:02Z]"
perform_test 72 "*[key=value1|value2|value3][@newer=2011-01-01T02:02:02Z]"
