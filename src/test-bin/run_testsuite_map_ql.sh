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

perform_test_map_ql()
{
  I="$1"

  mkdir -p "run/osm3s_query_$I"
  pushd "run/osm3s_query_$I/" >/dev/null
  rm -f *
  if [[ -s "../../input/osm3s_query_$I/stdin.log" ]]; then
  {
    "$BASEDIR/bin/osm3s_query" "--dump-xml" <"../../input/osm3s_query_$I/stdin.log" >xml.out.log 2>xml.err.log
    "$BASEDIR/bin/osm3s_query" "--dump-pretty-map-ql" <"../../input/osm3s_query_$I/stdin.log" >pretty.out.log 2>pretty.err.log
    "$BASEDIR/bin/osm3s_query" "--dump-compact-map-ql" <"../../input/osm3s_query_$I/stdin.log" >compact.out.log 2>compact.err.log
  }; else
  {
    echo "../../input/osm3s_query_$I/stdin.log missing"
  }; fi
  evaluate_test "osm3s_query_$I"
  if [[ -n $FAILED ]]; then
  {
    echo `date +%T` "Test osm3s_query $I FAILED."
  }; else
  {
    echo `date +%T` "Test osm3s_query $I succeeded."
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
II=70
while [[ $II -lt 134 ]]; do
{
  perform_test_map_ql $II
  II=$(($II + 1))
}; done

rm -f input/update_database/*
