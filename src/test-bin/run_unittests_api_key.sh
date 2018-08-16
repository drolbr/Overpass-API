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

if [[ -z $0  ]]; then
{
  echo "Usage: $0"
  exit 0
};
fi

BASEDIR="`pwd`/../"

check_osm_against()
{
  grep -vE 'generator=' >run/api_key_test_db/_
  DIFF=`diff run/api_key_test_db/_ "$1"`
  if [[ -z $DIFF ]]; then
    echo "Test successful."
  else
    echo "Test FAILED"
    diff run/api_key_test_db/_ "$1"
  fi
  rm run/api_key_test_db/_
};

mkdir -p run/api_key_test_db
rm -fR run/api_key_test_db/*

$BASEDIR/bin/dispatcher --osm-base --db-dir=run/api_key_test_db --time=400 &
$BASEDIR/bin/dispatcher --areas --db-dir=run/api_key_test_db &
sleep 1

echo "Based on IP address"
echo 'out;' | $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'out;' | REMOTE_ADDR="10.31.29.23" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'out;' | REMOTE_ADDR="3000:1234:5678:abcd::1" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'data=out;' | $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'data=out;' | REMOTE_ADDR="10.31.29.23" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'data=out;' | REMOTE_ADDR="3000:1234:5678:abcd::1" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm

echo
echo "Based on API key"
echo 'data=out;&apikey=f00ba4' | $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'data=out;&apikey=f00ba4' | REMOTE_ADDR="10.31.29.23" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'data=out;&apikey=f00ba4' | REMOTE_ADDR="3000:1234:5678:abcd::1" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'apikey=f11ba5&data=out;' | $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'apikey=f11ba5&data=out;' | REMOTE_ADDR="10.31.29.23" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'apikey=f11ba5&data=out;' | REMOTE_ADDR="3000:1234:5678:abcd::1" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo '[api_key:f22ba6];out;' | $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo '[api_key:f22ba6];out;' | REMOTE_ADDR="10.31.29.23" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo '[api_key:f22ba6];out;' | REMOTE_ADDR="3000:1234:5678:abcd::1" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo 'data=[api_key:"f22ba6"];out;&apikey=f00ba1;' | $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm

echo
echo "Using osm3s_query"
echo 'out;' | $BASEDIR/bin/osm3s_query \
    | check_osm_against input/api_key_test_db/empty.osm
echo 'out;' | $BASEDIR/bin/osm3s_query --rules \
    | check_osm_against input/api_key_test_db/empty.osm
echo 'out;' | REMOTE_ADDR="10.31.29.23" $BASEDIR/bin/osm3s_query --rules \
    | check_osm_against input/api_key_test_db/empty.osm
echo '[timeout:900];out;' | $BASEDIR/bin/osm3s_query \
    | check_osm_against input/api_key_test_db/zerobytes.txt
echo 'out;' | REMOTE_ADDR="10.31.29.23" $BASEDIR/cgi-bin/interpreter \
    | check_osm_against input/api_key_test_db/http_empty.osm
echo '[timeout:900];out;' | $BASEDIR/bin/osm3s_query --rules \
    | check_osm_against input/api_key_test_db/empty.osm

$BASEDIR/bin/dispatcher --osm-base --terminate
$BASEDIR/bin/dispatcher --areas --terminate

cat run/api_key_test_db/transactions.log | grep -E 'request_read_and_idx of process' \
    | awk '{ print $13; }' | check_osm_against input/api_key_test_db/osm_base_client_tokens.tsv

