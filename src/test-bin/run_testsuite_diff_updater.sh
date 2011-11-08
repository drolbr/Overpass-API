#!/usr/bin/env bash

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

# Prepare testing the statements
date +%T
mkdir -p run/diff_updater
rm -fR run/diff_updater/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >run/diff_updater/stdin.log
$BASEDIR/bin/update_database --db-dir=run/diff_updater/ --version=mock-up-init <run/diff_updater/stdin.log

# Test a differential update - prepare needed data
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
