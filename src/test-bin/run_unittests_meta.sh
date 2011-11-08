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
mkdir -p run/meta
rm -fR run/meta/*

$BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE >run/meta/stdin.log
$BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE diff >run/meta/diff.log
$BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE after >run/meta/after.log
$BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE after "timestamp=2004-01-01T00:00:00Z" tags >run/meta/newer.log
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
<bbox-query s=\"10.0\" n=\"12.0\" w=\"1.0\" e=\"2.0\"/>\
<recurse type=\"node-way\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"2.0\" e=\"3.0\"/>\
<recurse type=\"node-way\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"3.0\" e=\"4.0\"/>\
<recurse type=\"node-way\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"4.0\" e=\"5.0\"/>\
<recurse type=\"node-way\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"5.0\" e=\"6.0\"/>\
<recurse type=\"node-way\"/>\
<print mode=\"meta\"/>\
<bbox-query s=\"10.0\" n=\"12.0\" w=\"6.0\" e=\"7.0\"/>\
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
" >run/meta/query.xml

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
" >run/meta/newer_query.xml  

user_test()
{
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE $1 uid=$2 >run/meta/user_$1_$2.log
  $BASEDIR/test-bin/generate_test_file_meta $DATA_SIZE $1 uid=$2 tags >run/meta/user_tags_$1_$2.log
  echo "\
<osm-script timeout=\"86400\">\
\
<user uid=\"$2\"/>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/meta/uid_query_$2.xml  
  echo "\
<osm-script timeout=\"86400\">\
\
<user name=\"User_$2\"/>\
<print mode=\"meta\"/>\
\
</osm-script>
" >run/meta/name_query_$2.xml  

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
" >run/meta/tags_query_$2.xml  

  $BASEDIR/bin/osm3s_query --db-dir=run/meta/ --concise <run/meta/uid_query_$2.xml >run/meta/uid_$1_$2.log
  $BASEDIR/bin/osm3s_query --db-dir=run/meta/ --concise <run/meta/name_query_$2.xml >run/meta/name_$1_$2.log
  $BASEDIR/bin/osm3s_query --db-dir=run/meta/ --concise <run/meta/tags_query_$2.xml >run/meta/tags_$1_$2.log

  RES=$RES`diff -q run/meta/user_$1_$2.log run/meta/uid_$1_$2.log`
  RES=$RES`diff -q run/meta/user_$1_$2.log run/meta/name_$1_$2.log`
  RES=$RES`diff -q run/meta/user_tags_$1_$2.log run/meta/tags_$1_$2.log`
};

RES=

# Perform the tests
date +%T
$BASEDIR/bin/update_database --db-dir=run/meta/ --version=mock-up-init --meta <run/meta/stdin.log
date +%T
$BASEDIR/bin/osm3s_query --db-dir=run/meta/ --concise <run/meta/query.xml >run/meta/initial.log
date +%T
user_test before 13
date +%T
user_test before 113
date +%T
user_test before 1013
date +%T

echo

date +%T
$BASEDIR/bin/update_database --db-dir=run/meta/ --version=mock-up-init --meta <run/meta/diff.log
date +%T
$BASEDIR/bin/osm3s_query --db-dir=run/meta/ --concise <run/meta/query.xml >run/meta/db_after.log
date +%T
user_test after 13
date +%T
user_test after 113
date +%T
user_test after 1013
date +%T
$BASEDIR/bin/osm3s_query --db-dir=run/meta/ --concise <run/meta/newer_query.xml >run/meta/db_newer.log
date +%T

echo

# compare both outcomes
RES=$RES`diff -q run/meta/stdin.log run/meta/initial.log`
RES=$RES`diff -q run/meta/after.log run/meta/db_after.log`
RES=$RES`diff -q run/meta/newer.log run/meta/db_newer.log`
if [[ -n $RES || -s run/meta/diff_stderr.log ]]; then
{
  echo `date +%T` "Test diff 1 FAILED."
  echo $RES
}; else
{
  echo `date +%T` "Test diff 1 succeeded."
  rm -R run/meta
}; fi
