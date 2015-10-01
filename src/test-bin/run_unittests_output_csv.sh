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

perform_test_interpreter()
{
  EXEC="output_csv"
  I="$1"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  if [[ -s "../../input/${EXEC}_$I/stdin.log" ]]; then
  {
    #echo "stdin.log found"
    "$BASEDIR/cgi-bin/interpreter" $ARGS <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "$BASEDIR/cgi-bin/interpreter" $ARGS >stdout.log 2>stderr.log
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

# Prepare testing the statements with meta
mkdir -p input/update_database/
rm -f input/update_database/*
mkdir -p input/update_database/templates/
cp -p $BASEDIR/templates/* input/update_database/templates/
$BASEDIR/test-bin/generate_test_file_meta 40 more_tags >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --meta --version=mock-up-init <input/update_database/stdin.log

# do the differential update including start/stop of dispatcher
date +%T
$BASEDIR/bin/dispatcher --osm-base --meta --db-dir=input/update_database/ &
sleep 1

II=1
while [[ $II -lt 31 ]]; do
{
  mkdir -p input/output_csv_$II/
  II=$(($II + 1))
}; done

# Check whether all special fields are operational and independend from each other
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid;true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_1/stdin.log
echo 'data=[out:csv(::id,::id;true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_2/stdin.log
echo 'data=[out:csv(::uid,::user,::changeset,::timestamp,::version,::lon,::lat,::id,::type;true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_3/stdin.log

# Check normal fields, including empty entries and mixed with functional fields
echo 'data=[out:csv(even,foo,odd;true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_4/stdin.log
echo 'data=[out:csv("even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_5/stdin.log
echo 'data=[out:csv(even,::id,foo;true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_6/stdin.log
echo 'data=[out:csv("even",::id,"foo";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_7/stdin.log

# Check separation characters and strings
echo 'data=[out:csv("even",::id,"foo";true;",")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_8/stdin.log
echo 'data=[out:csv("even",::id,"foo","odd","even";true;",")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_9/stdin.log
echo 'data=[out:csv("even",::id,::lat,::lon;true;",")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_10/stdin.log
echo 'data=[out:csv("even";true;",")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_11/stdin.log
echo 'data=[out:csv(::id;true;",")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_12/stdin.log
echo 'data=[out:csv("even",::id,"foo";true;"")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_13/stdin.log
echo 'data=[out:csv("even",::id,"foo";true;" ")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_14/stdin.log
echo 'data=[out:csv("even",::id,"foo";true;"---")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_15/stdin.log

# Check w/o headlines
echo 'data=[out:csv(even,foo,odd;false;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_16/stdin.log
echo 'data=[out:csv("even","foo","odd";false;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_17/stdin.log
echo 'data=[out:csv(even,::id,foo;false;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_18/stdin.log
echo 'data=[out:csv("even",::id,"foo";false;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_19/stdin.log

# Check with empty result and various output modes
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out meta;' >input/output_csv_20/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out ids;' >input/output_csv_21/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out skel;' >input/output_csv_22/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out;' >input/output_csv_23/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out tags;' >input/output_csv_24/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out center;' >input/output_csv_25/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out center meta;' >input/output_csv_26/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out bb;' >input/output_csv_27/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out bb meta;' >input/output_csv_28/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out geom;' >input/output_csv_29/stdin.log
echo 'data=[out:csv(::type,::id,::lat,::lon,::version,::timestamp,::changeset,::user,::uid,"even","foo","odd";true;"\t")];(node(1);node(2);node(7);node(14);way(1);way(2);way(7);way(14);rel(1);rel(2);rel(42);rel(161););out geom meta;' >input/output_csv_30/stdin.log

II=1
while [[ $II -lt 31 ]]; do
{
  perform_test_interpreter $II
  II=$(($II + 1))
}; done

$BASEDIR/bin/dispatcher --terminate

rm -fR input/update_database/*

II=1
while [[ $II -lt 31 ]]; do
{
  rm -fR input/output_csv_$II/
  II=$(($II + 1))
}; done
