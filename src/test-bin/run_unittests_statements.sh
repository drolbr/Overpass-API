#!/bin/bash

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

print_test_5()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  "$BASEDIR/test-bin/$1" "$I" $ARGS 2>stderr.log | grep "^  <" | sort >stdout.log
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

prepare_test_loop()
{
  I=1
  DATA_SIZE=$3
  while [[ $I -le $2 ]]; do
  {
    mkdir -p expected/$1_$I/
    rm -f expected/$1_$I/*
    $BASEDIR/test-bin/generate_test_file $DATA_SIZE $1_$I >expected/$1_$I/stdout.log
    touch expected/$1_$I/stderr.log
    I=$(($I + 1))
  }; done
};

# Prepare testing the statements
mkdir -p input/update_database/
rm -f input/update_database/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --version=mock-up-init <input/update_database/stdin.log

# Test the print and id_query statements
prepare_test_loop print 4 $DATA_SIZE
mkdir -p expected/print_5/
$BASEDIR/test-bin/generate_test_file $DATA_SIZE print_4 | grep "^  <" | sort >expected/print_5/stdout.log
touch expected/print_5/stderr.log

date +%T
perform_test_loop print 4 "$DATA_SIZE ../../input/update_database/"
print_test_5 print 5 "$DATA_SIZE ../../input/update_database/"

# Test the recurse statement
prepare_test_loop recurse 11 $DATA_SIZE
date +%T
perform_test_loop recurse 11 "$DATA_SIZE ../../input/update_database/"

# Test the bbox_query statement
prepare_test_loop bbox_query 8 $DATA_SIZE
date +%T
perform_test_loop bbox_query 8 "$DATA_SIZE ../../input/update_database/"

# Test the bbox_query statement
prepare_test_loop around 6 $DATA_SIZE
date +%T
perform_test_loop around 6 "$DATA_SIZE ../../input/update_database/"

# Test the query statement
prepare_test_loop query 29 $DATA_SIZE
date +%T
perform_test_loop query 29 "$DATA_SIZE ../../input/update_database/"

# Test the foreach statement
prepare_test_loop foreach 4 $DATA_SIZE
date +%T
perform_test_loop foreach 4 "$DATA_SIZE ../../input/update_database/"

# Test the union statement
prepare_test_loop union 6 $DATA_SIZE
date +%T
perform_test_loop union 6 ../../input/update_database/

rm -f input/update_database/*
