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

$BASEDIR/test-bin/run_testsuite_template_db.sh $1 $2

$BASEDIR/test-bin/run_testsuite_osm_backend.sh $1 $2

# Prepare testing the statements
mkdir -p input/update_database/
rm -f input/update_database/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >input/update_database/stdin.log
$BASEDIR/bin/update_database --db-dir=input/update_database/ --version=mock-up-init <input/update_database/stdin.log

$BASEDIR/test-bin/run_unittests_statements.sh $1 $2

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

# Test a differential update - prepare needed data
date +%T
rm -fR run/diff_updater
mv input/update_database run/diff_updater
date +%T
$BASEDIR/test-bin/generate_test_file $DATA_SIZE diff_do >run/diff_updater/do_stdin.log

# do the differential update including start/stop of dispatcher
date +%T
$BASEDIR/bin/dispatcher --osm-base --db-dir=run/diff_updater/ &
sleep 1
rm -f run/diff_updater/transactions.log
$BASEDIR/bin/update_database --version=mock-up-diff <run/diff_updater/do_stdin.log
cat run/diff_updater/transactions.log
$BASEDIR/bin/dispatcher --terminate

# collect the result
date +%T
$BASEDIR/test-bin/diff_updater --pattern_size=$DATA_SIZE --db-dir=run/diff_updater/ >run/diff_updater/diff_do.log
$BASEDIR/test-bin/compare_osm_base_maps --db-dir=run/diff_updater/ >>run/diff_updater/diff_do.log 2>run/diff_updater/diff_stderr.log
date +%T

# run a fresh import of adapted data to compare
$BASEDIR/test-bin/generate_test_file $DATA_SIZE diff_compare >run/diff_updater/compare_stdin.log
date +%T
rm -f run/diff_updater/*.map run/diff_updater/*.bin run/diff_updater/*.idx
$BASEDIR/bin/update_database --db-dir=run/diff_updater/ <run/diff_updater/compare_stdin.log
date +%T
$BASEDIR/test-bin/diff_updater --pattern_size=$DATA_SIZE --db-dir=run/diff_updater/ >run/diff_updater/diff_compare.log
$BASEDIR/test-bin/compare_osm_base_maps --db-dir=run/diff_updater/ >>run/diff_updater/diff_compare.log

# compare both outcomes
RES=`diff -q run/diff_updater/diff_compare.log run/diff_updater/diff_do.log`
if [[ -n $RES || -s run/diff_updater/diff_stderr.log ]]; then
{
  echo `date +%T` "Test diff 1 FAILED."
}; else
{
  echo `date +%T` "Test diff 1 succeeded."
  rm -R run/diff_updater
}; fi
