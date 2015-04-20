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
      pwd >"__temp"
      if [[ "$FILE" == "test-shadow.lock" || ! -s "$FILE" ]]; then
        echo "no" >>"__temp"
      else
        echo "yes" >>"__temp"
      fi
      cat "__temp" "../../expected/$DIRNAME/$FILE" | awk -f "../../expected/set_variables.awk" >"expected_$FILE"
      RES=`diff -q "expected_$FILE" "$FILE"`
      rm "__temp" #"expected_$FILE"
      if [[ -n $RES ]]; then
      {
        echo $RES
        FAILED=YES
      }; fi
    }; fi
  }; done
};

perform_twin_test()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  "$BASEDIR/test-bin/$1" "${I}w" $ARGS >stdout.write.log 2>stderr.write.log &
  "$BASEDIR/test-bin/$1" "${I}r" $ARGS >stdout.read.log 2>stderr.read.log
  sleep 5
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
#     rm -R *
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

dispatcher_client_server()
{
  mkdir -p "run/${EXEC}_server_$1"
  pushd "run/${EXEC}_server_$1/" >/dev/null
  rm -f *
  $BASEDIR/test-bin/test_dispatcher server_3 &
  popd >/dev/null

  date +%T
  ls "run/${EXEC}_server_$1/" >before.ls
  perform_serial_test test_dispatcher $1
  ls "run/${EXEC}_server_$1/" >"run/${EXEC}_$1/after.ls"
  mv before.ls "run/${EXEC}_$1/before.ls"
  sleep 5
};

dispatcher_two_clients()
{
  mkdir -p "run/${EXEC}_server_$1"
  pushd "run/${EXEC}_server_$1/" >/dev/null
  rm -f *
  $BASEDIR/test-bin/test_dispatcher server_10 &
  popd >/dev/null

  date +%T
  ls "run/${EXEC}_server_$1/" >before.ls
  perform_twin_test test_dispatcher $1
  ls "run/${EXEC}_server_$1/" >"run/${EXEC}_$1/after.ls"
  mv before.ls "run/${EXEC}_$1/before.ls"
  sleep 5
};

# Test template_db
date +%T
perform_test_loop file_blocks 25
date +%T
perform_test_loop block_backend 13
date +%T
perform_test_loop random_file 8
date +%T
perform_test_loop test_dispatcher 20

dispatcher_client_server 21
dispatcher_client_server 22
dispatcher_client_server 23
dispatcher_client_server 24
dispatcher_two_clients 25
dispatcher_two_clients 26

# don't use that test because we cannot control the assigned pids
#dispatcher_two_clients 27
