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

pushd "$(dirname $0)"
BASEDIR="$(pwd)/.."
popd
pushd "$BASEDIR/osm-3s_testing/"

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
      cat <"$FILE" | sed 's/Overpass API [^ ]* [a-f0-9]*/Overpass API/g' >"_$FILE"
      mv "_$FILE" "$FILE"
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
mkdir -p run/area/
rm -f run/area/*
touch run/area/area_tags_local.bin
touch run/area/area_blocks.bin
cat input/area/init.osm | ../bin/update_database --db-dir=run/area/
date +%T

for i in $(ls input/area/*.ql); do
  ibase=$(basename $i .ql)
  cat $i | $BASEDIR/bin/osm3s_query --db-dir=run/area/ >"run/area/$ibase.out" 2>"run/area/$ibase.err"
  date +%T
done

cd run/area/

for i in $(ls *.err); do
  diff -q "../../expected/area/$i" "$i"
done

for i in $(ls *.out); do
  cat <$i | sed 's/Overpass API [^ ]* [a-f0-9]*/Overpass API/g' >"_$i"
  mv "_$i" "$i"
  diff -q "../../expected/area/$i" "$i"
done

popd
