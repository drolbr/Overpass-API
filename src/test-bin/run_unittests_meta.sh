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


user_test()
{
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE $1 uid=$2 >run/$3/user_$1_$2.log
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE $1 uid=$2 tags >run/$3/user_tags_$1_$2.log
  echo "\
<osm-script timeout=\"86400\">\
\
<user uid=\"$2\"/>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/$3/uid_query_$2.xml  
  echo "\
<osm-script timeout=\"86400\">\
\
<user name=\"User_$2\"/>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/$3/name_query_$2.xml  

  echo "\
<osm-script timeout=\"86400\">\
\
<query type=\"node\">\
  <user name=\"User_$2\"/>\
  <has-kv k=\"foo\" v=\"bar\"/>\
</query>\
<print mode=\"meta\"/>\
<query type=\"way\">\
  <user name=\"User_$2\"/>\
  <has-kv k=\"foo\" v=\"bar\"/>\
</query>\
<print mode=\"meta\"/>\
<query type=\"relation\">\
  <user name=\"User_$2\"/>\
  <has-kv k=\"foo\" v=\"bar\"/>\
</query>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/$3/tags_query_$2.xml  

  $BASEDIR/bin/osm3s_query --db-dir=run/$3/ --concise <run/$3/uid_query_$2.xml >run/$3/uid_$1_$2.log
  $BASEDIR/bin/osm3s_query --db-dir=run/$3/ --concise <run/$3/name_query_$2.xml >run/$3/name_$1_$2.log
  $BASEDIR/bin/osm3s_query --db-dir=run/$3/ --concise <run/$3/tags_query_$2.xml >run/$3/tags_$1_$2.log

  RES=$RES`diff -q run/$3/user_$1_$2.log run/$3/uid_$1_$2.log`
  RES=$RES`diff -q run/$3/user_$1_$2.log run/$3/name_$1_$2.log`
  RES=$RES`diff -q run/$3/user_tags_$1_$2.log run/$3/tags_$1_$2.log`
};


prepare_test()
{
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE >run/$1/stdin.log
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE diff >run/$1/diff.log
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE after >run/$1/after.log
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE after "timestamp=2004-01-01T00:00:00Z" tags >run/$1/newer.log
  echo "\
<osm-script timeout=\"86400\">\
\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"1.0\" e=\"2.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"2.0\" e=\"3.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"3.0\" e=\"4.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"4.0\" e=\"5.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"5.0\" e=\"6.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"6.0\" e=\"7.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"7.0\" e=\"8.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"8.0\" e=\"9.0\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"9.0\" e=\"10.0\"/>\
<print mode=\"meta\"/>\
\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"1.0\" e=\"7.0\"/>\
<recurse type=\"node-way\"/>\
<print mode=\"meta\"/>\
\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"1.0\" e=\"2.0\"/>\
<recurse type=\"node-way\" into=\"ways\"/>\
<union>\
  <recurse type=\"node-relation\"/>\
  <recurse type=\"way-relation\" from=\"ways\"/>\
</union>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"2.0\" e=\"3.0\"/>\
<recurse type=\"node-way\" into=\"ways\"/>\
<union>\
  <recurse type=\"node-relation\"/>\
  <recurse type=\"way-relation\" from=\"ways\"/>\
</union>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"3.0\" e=\"4.0\"/>\
<recurse type=\"node-way\" into=\"ways\"/>\
<union>\
  <recurse type=\"node-relation\"/>\
  <recurse type=\"way-relation\" from=\"ways\"/>\
</union>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"4.0\" e=\"5.0\"/>\
<recurse type=\"node-way\" into=\"ways\"/>\
<union>\
  <recurse type=\"node-relation\"/>\
  <recurse type=\"way-relation\" from=\"ways\"/>\
</union>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"5.0\" e=\"6.0\"/>\
<recurse type=\"node-way\" into=\"ways\"/>\
<union>\
  <recurse type=\"node-relation\"/>\
  <recurse type=\"way-relation\" from=\"ways\"/>\
</union>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"6.0\" e=\"7.0\"/>\
<recurse type=\"node-way\" into=\"ways\"/>\
<union>\
  <recurse type=\"node-relation\"/>\
  <recurse type=\"way-relation\" from=\"ways\"/>\
</union>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/$1/query.xml

  echo "\
<osm-script timeout=\"86400\">\
\
<query type=\"node\">\
  <newer than=\"2004-01-01T00:00:00Z\"/>
  <has-kv k=\"foo\" v=\"bar\"/>\
</query>\
<print mode=\"meta\"/>\
<query type=\"way\">\
  <newer than=\"2004-01-01T00:00:00Z\"/>
  <has-kv k=\"foo\" v=\"bar\"/>\
</query>\
<print mode=\"meta\"/>\
<query type=\"relation\">\
  <newer than=\"2004-01-01T00:00:00Z\"/>
  <has-kv k=\"foo\" v=\"bar\"/>\
</query>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/$1/newer_query.xml
};


# Test 1
# Prepare testing the statements
date +%T
mkdir -p run/meta_1
rm -fR run/meta_1/*

prepare_test meta_1

RES=

# Perform the tests
date +%T
$BASEDIR/bin/update_database --db-dir=run/meta_1/ --version=mock-up-init --meta <run/meta_1/stdin.log
date +%T
$BASEDIR/bin/osm3s_query --db-dir=run/meta_1/ --concise <run/meta_1/query.xml >run/meta_1/initial.log
date +%T
user_test before 13 meta_1
date +%T
user_test before 113 meta_1
date +%T
user_test before 1013 meta_1
date +%T

echo

date +%T
$BASEDIR/bin/update_database --db-dir=run/meta_1/ --version=mock-up-init --meta <run/meta_1/diff.log
date +%T
$BASEDIR/bin/osm3s_query --db-dir=run/meta_1/ --concise <run/meta_1/query.xml >run/meta_1/db_after.log
date +%T
user_test after 13 meta_1
date +%T
user_test after 113 meta_1
date +%T
user_test after 1013 meta_1
date +%T
$BASEDIR/bin/osm3s_query --db-dir=run/meta_1/ --concise <run/meta_1/newer_query.xml >run/meta_1/db_newer.log
date +%T

echo

# compare both outcomes
RES=$RES`diff -q run/meta_1/stdin.log run/meta_1/initial.log`
RES=$RES`diff -q run/meta_1/after.log run/meta_1/db_after.log`
RES=$RES`diff -q run/meta_1/newer.log run/meta_1/db_newer.log`
if [[ -n $RES || -s run/meta_1/diff_stderr.log ]]; then
{
  echo `date +%T` "Test meta 1 FAILED."
  echo $RES
}; else
{
  echo `date +%T` "Test meta 1 succeeded."
  rm -R run/meta_1
}; fi

exit 0


# Test the augmented diffs

# Prepare testing the statements
date +%T
mkdir -p run/meta_2
rm -fR run/meta_2/*
$BASEDIR/bin/update_database --db-dir=run/meta_2/ --version=mock-up-init --meta <input/meta_2/augmented_init.osm

# do the differential update including start/stop of dispatcher
date +%T
$BASEDIR/bin/dispatcher --osm-base --meta --db-dir=run/meta_2/ &
sleep 1
rm -f run/meta_2/transactions.log
$BASEDIR/bin/update_database --version=mock-up-meta --produce-diff --meta <input/meta_2/augmented_source.osm >run/meta_2/augmented_diff.osm
cat run/meta_2/transactions.log
$BASEDIR/bin/dispatcher --terminate

# compare outcome to expected outcome
RES=`diff -q expected/meta_2/augmented_diff.osm run/meta_2/augmented_diff.osm`
if [[ -n $RES || ! -f run/meta_2/augmented_diff.osm ]]; then
{
  echo `date +%T` "Test meta 2 FAILED."
}; else
{
  echo `date +%T` "Test meta 2 succeeded."
  rm -R run/meta_2
}; fi
