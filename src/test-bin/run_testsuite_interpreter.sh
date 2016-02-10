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
  EXEC="interpreter"
  I="$1"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  if [[ -s "../../input/${EXEC}_$I/stdin.log" ]]; then
  {
    #echo "stdin.log found"
    "$BASEDIR/cgi-bin/$EXEC" $ARGS <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "$BASEDIR/cgi-bin/$EXEC" $ARGS >stdout.log 2>stderr.log
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

prepare_test_interpreter()
{
  EXEC="interpreter"
  I="$1"

  mkdir -p expected/${EXEC}_$I/
  rm -f expected/${EXEC}_$I/*
  $BASEDIR/test-bin/generate_test_file_interpreter $DATA_SIZE ${EXEC}_$I >expected/${EXEC}_$I/stdout.log
  touch expected/${EXEC}_$I/stderr.log
};

# Prepare testing the statements
mkdir -p input/update_database/
rm -f input/update_database/*
mkdir -p input/update_database/templates/
cp -p $BASEDIR/templates/* input/update_database/templates/
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --meta --version=mock-up-init <input/update_database/stdin.log

# do the differential update including start/stop of dispatcher
date +%T
$BASEDIR/bin/dispatcher --osm-base --db-dir=input/update_database/ &
sleep 1

II=1
while [[ $II -lt 45 ]]; do
{
  mkdir -p input/interpreter_$II/
  II=$(($II + 1))
}; done

echo "data=out;" >input/interpreter_1/stdin.log
echo "data=<print/>" >input/interpreter_2/stdin.log
echo "data=foo;" >input/interpreter_3/stdin.log
echo "data=<foo/>" >input/interpreter_4/stdin.log
echo -e "data=foo;\n\
out;" >input/interpreter_5/stdin.log
echo -e "data=out;\n\
foo;" >input/interpreter_6/stdin.log
echo -e "data=foo;\n\
foo;" >input/interpreter_7/stdin.log
echo -e "data=<foo/>\n\
<print/>" >input/interpreter_8/stdin.log
echo -e "data=<print/>\n\
<foo/>" >input/interpreter_9/stdin.log
echo -e "data=<foo/>\n\
<foo/>" >input/interpreter_10/stdin.log

echo "data=[out:json];node(1);out;" >input/interpreter_11/stdin.log
echo "data=[out:json];(node(1);node($(($DATA_SIZE * 3 + 3))););out;" >input/interpreter_12/stdin.log
echo "data=[out:json];way(1);out;" >input/interpreter_13/stdin.log
echo "data=[out:json];(way(1);way(2););out;" >input/interpreter_14/stdin.log
echo "data=[out:json];rel(1);out;" >input/interpreter_15/stdin.log
echo "data=[out:json];(rel(1);rel(9););out;" >input/interpreter_16/stdin.log
echo "data=[out:json];(node(1);way(1););out;" >input/interpreter_17/stdin.log
echo "data=[out:json];(node(1);rel(1););out;" >input/interpreter_18/stdin.log
echo "data=[out:json];(way(1);rel(1););out;" >input/interpreter_19/stdin.log
echo "data=[out:json];(node(1);way(1);rel(1););out;" >input/interpreter_20/stdin.log
echo "data=[out:json];(node(1);way(1);rel(1););out ids;" >input/interpreter_21/stdin.log
echo "data=[out:json];(node(1);way(1);rel(1););out skel;" >input/interpreter_22/stdin.log
echo "data=[out:json];(node(1);way(1);rel(1););out body;" >input/interpreter_23/stdin.log

echo "data=[out:custom];out;" >input/interpreter_24/stdin.log
echo "data=[out:custom];node(1);out;&redirect=no" >input/interpreter_25/stdin.log
echo "data=[out:custom];(node(1);node($(($DATA_SIZE * 3 + 3))););out;" >input/interpreter_26/stdin.log
echo "data=[out:custom];way(1);out;&redirect=no" >input/interpreter_27/stdin.log
echo "data=[out:custom];(way(1);way(2););out;" >input/interpreter_28/stdin.log
echo "data=[out:custom];rel(1);out;&redirect=no" >input/interpreter_29/stdin.log
echo "data=[out:custom];(rel(1);rel(9););out;" >input/interpreter_30/stdin.log
echo "data=[out:custom];(node(1);way(1););out;" >input/interpreter_31/stdin.log
echo "data=[out:custom];(node(1);rel(1););out;" >input/interpreter_32/stdin.log
echo "data=[out:custom];(way(1);rel(1););out;" >input/interpreter_33/stdin.log
echo "data=[out:custom];(node(1);way(1);rel(1););out;" >input/interpreter_34/stdin.log
echo "data=[out:custom];(node(1);way(1);rel(1););out ids;" >input/interpreter_35/stdin.log
echo "data=[out:custom];(node(1);way(1);rel(1););out skel;" >input/interpreter_36/stdin.log
echo "data=[out:custom];(node(1);way(1);rel(1););out body;" >input/interpreter_37/stdin.log

echo "data=[out:json];node(1);out;&jsonp=foo" >input/interpreter_38/stdin.log
echo "data=[out:custom];way(1);out;" >input/interpreter_39/stdin.log
echo "data=[out:custom];way(1);out;&url=http://www.openstreetmap.org/?{{{type}}}={{{id}}}" >input/interpreter_40/stdin.log
echo "data=[out:custom];way(1);out;&url=http%3A%2F%2Fwww%2Eopenstreetmap%2Eorg%2F%3F%7B%7B%7Btype%7D%7D%7D%3D%7B%7B%7Bid%7D%7D%7D" >input/interpreter_41/stdin.log
echo "data=[out:custom];node(1);out;&redirect=no&template=base.wiki" >input/interpreter_42/stdin.log
echo "data=[out:custom];(node(1);way(1);rel(1););out;&template=base%2Ewiki" >input/interpreter_43/stdin.log
echo "data=[out:custom];(node(1);node($(($DATA_SIZE * 3 + 3)));way(1);way(2);rel(1);rel(9););out;&template=base.wiki" >input/interpreter_44/stdin.log

II=1
while [[ $II -lt 45 ]]; do
{
  prepare_test_interpreter $II
  perform_test_interpreter $II
  II=$(($II + 1))
}; done

$BASEDIR/bin/dispatcher --terminate

rm -fR input/update_database/*
rm -fR input/interpreter_*


# Prepare testing the statements with meta
mkdir -p input/update_database/
rm -f input/update_database/*
mkdir -p input/update_database/templates/
cp -p $BASEDIR/templates/* input/update_database/templates/
$BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --meta --version=mock-up-init <input/update_database/stdin.log

# do the differential update including start/stop of dispatcher
date +%T
$BASEDIR/bin/dispatcher --osm-base --meta --db-dir=input/update_database/ &
sleep 1

II=45
while [[ $II -lt 48 ]]; do
{
  mkdir -p input/interpreter_$II/
  II=$(($II + 1))
}; done

echo "data=[out:json];(node(7);way(1);rel(1););out meta;" >input/interpreter_45/stdin.log
echo "data=[out:custom];(node(7);way(1);rel(1););out meta;" >input/interpreter_46/stdin.log

II=45
while [[ $II -lt 47 ]]; do
{
  prepare_test_interpreter $II
  perform_test_interpreter $II
  II=$(($II + 1))
}; done

$BASEDIR/bin/dispatcher --terminate

rm -fR input/update_database/*
rm -fR input/interpreter_*
