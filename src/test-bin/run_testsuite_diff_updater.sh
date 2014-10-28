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

# Prepare testing the statements
date +%T
mkdir -p run/diff_updater_1
rm -fR run/diff_updater_1/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >run/diff_updater_1/stdin.log
$BASEDIR/bin/update_database --db-dir=run/diff_updater_1/ --version=mock-up-init <run/diff_updater_1/stdin.log

# Test a differential update - prepare needed data
date +%T
$BASEDIR/test-bin/generate_test_file $DATA_SIZE diff_do >run/diff_updater_1/do_stdin.log

# do the differential update including start/stop of dispatcher
date +%T
$BASEDIR/bin/dispatcher --osm-base --db-dir=run/diff_updater_1/ &
sleep 1
rm -f run/diff_updater_1/transactions.log
$BASEDIR/bin/update_database --version=mock-up-diff <run/diff_updater_1/do_stdin.log
cat run/diff_updater_1/transactions.log
$BASEDIR/bin/dispatcher --osm-base --terminate

# collect the result
date +%T
$BASEDIR/test-bin/diff_updater --pattern_size=$DATA_SIZE --db-dir=run/diff_updater_1/ >run/diff_updater_1/diff_do.log
$BASEDIR/test-bin/compare_osm_base_maps --db-dir=run/diff_updater_1/ >>run/diff_updater_1/diff_do.log 2>run/diff_updater_1/diff_stderr.log
date +%T

# run a fresh import of adapted data to compare
$BASEDIR/test-bin/generate_test_file $DATA_SIZE diff_compare >run/diff_updater_1/compare_stdin.log
date +%T
rm -f run/diff_updater_1/*.map run/diff_updater_1/*.bin run/diff_updater_1/*.idx
$BASEDIR/bin/update_database --db-dir=run/diff_updater_1/ <run/diff_updater_1/compare_stdin.log
date +%T
$BASEDIR/test-bin/diff_updater --pattern_size=$DATA_SIZE --db-dir=run/diff_updater_1/ >run/diff_updater_1/diff_compare.log
$BASEDIR/test-bin/compare_osm_base_maps --db-dir=run/diff_updater_1/ >>run/diff_updater_1/diff_compare.log

# compare both outcomes
RES=`diff -q run/diff_updater_1/diff_compare.log run/diff_updater_1/diff_do.log`
if [[ -n $RES || -s run/diff_updater_1/diff_stderr.log ]]; then
{
  echo `date +%T` "Test diff 1 FAILED."
}; else
{
  echo `date +%T` "Test diff 1 succeeded."
  rm -R run/diff_updater_1
}; fi


# Test the augmented diffs
# turned off. Augmented diffs are now generated from the ordinary database.

# Prepare testing the statements
# date +%T
# mkdir -p run/diff_updater_2
# rm -fR run/diff_updater_2/*
# $BASEDIR/bin/update_database --db-dir=run/diff_updater_2/ --version=mock-up-init <input/diff_updater_2/augmented_init.osm
# 
# # do the differential update including start/stop of dispatcher
# date +%T
# $BASEDIR/bin/dispatcher --osm-base --db-dir=run/diff_updater_2/ &
# sleep 1
# rm -f run/diff_updater_2/transactions.log
# $BASEDIR/bin/update_database --version=mock-up-diff --produce-diff <input/diff_updater_2/augmented_source.osm >run/diff_updater_2/augmented_diff.osm
# cat run/diff_updater_2/transactions.log
# $BASEDIR/bin/dispatcher --terminate
# 
# # compare outcome to expected outcome
# RES=`diff -q expected/diff_updater_2/augmented_diff.osm run/diff_updater_2/augmented_diff.osm`
# if [[ -n $RES || ! -f run/diff_updater_2/augmented_diff.osm ]]; then
# {
#   echo `date +%T` "Test diff 2 FAILED."
# }; else
# {
#   echo `date +%T` "Test diff 2 succeeded."
#   rm -R run/diff_updater_2
# }; fi
