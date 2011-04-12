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

# Test template_db
date +%T
perform_test_loop file_blocks 12
date +%T
perform_test_loop block_backend 13
date +%T
perform_test_loop random_file 4

# Test overpass_api/osm-backend
mkdir -p input/run_and_compare.sh_1/
rm -f input/run_and_compare.sh_1/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >input/run_and_compare.sh_1/stdin.log
date +%T
perform_serial_test run_and_compare.sh 1

mkdir -p input/run_and_compare.sh_2/
rm -f input/run_and_compare.sh_2/*
mv input/run_and_compare.sh_1/stdin.log input/run_and_compare.sh_2/stdin.log
date +%T
perform_serial_test run_and_compare.sh 2

mkdir -p input/run_and_compare.sh_3/
rm -f input/run_and_compare.sh_3/*
mv input/run_and_compare.sh_2/stdin.log input/run_and_compare.sh_3/stdin.log
date +%T
perform_serial_test run_and_compare.sh 3

# Prepare testing the statements
mkdir -p input/update_database/
rm -f input/update_database/*
mv input/run_and_compare.sh_3/stdin.log input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ <input/update_database/stdin.log

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

# Test the query statement
prepare_test_loop query 25 $DATA_SIZE
date +%T
perform_test_loop query 25 "$DATA_SIZE ../../input/update_database/"

# Test the foreach statement
prepare_test_loop foreach 4 $DATA_SIZE
date +%T
perform_test_loop foreach 4 "$DATA_SIZE ../../input/update_database/"

# Test the union statement
prepare_test_loop union 6 $DATA_SIZE
date +%T
perform_test_loop union 6 ../../input/update_database/

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

# Test a differential update
date +%T
rm -fR run/diff_updater
mv input/update_database run/diff_updater
date +%T
$BASEDIR/test-bin/generate_test_file $DATA_SIZE diff_do >run/diff_updater/do_stdin.log
date +%T
$BASEDIR/bin/update_database --db-dir=run/diff_updater/ <run/diff_updater/do_stdin.log
date +%T
$BASEDIR/test-bin/diff_updater --pattern_size=$DATA_SIZE --db-dir=run/diff_updater/ >run/diff_updater/diff_do.log
date +%T
$BASEDIR/test-bin/generate_test_file $DATA_SIZE diff_compare >run/diff_updater/compare_stdin.log
date +%T
rm -f run/diff_updater/*.map run/diff_updater/*.bin run/diff_updater/*.idx
$BASEDIR/bin/update_database --db-dir=run/diff_updater/ <run/diff_updater/compare_stdin.log
date +%T
$BASEDIR/test-bin/diff_updater --pattern_size=$DATA_SIZE --db-dir=run/diff_updater/ >run/diff_updater/diff_compare.log
RES=`diff -q run/diff_updater/diff_compare.log run/diff_updater/diff_do.log`
if [[ -n $RES ]]; then
{
  echo `date +%T` "Test diff 1 FAILED."
}; else
{
  echo `date +%T` "Test diff 1 succeeded."
  rm -R run/diff_updater
}; fi
