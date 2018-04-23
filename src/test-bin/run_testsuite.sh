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

if [[ -z $1  ]]; then
{
  echo "Usage: $0 test_size"
  echo
  echo "An appropriate value for a fast test is 40, a comprehensive value is 2000."
  exit 0
};
fi

# Check whether we do compile with tests.
# Otherweise we will likely not test the current code
if [[ -r ../src/Makefile.am ]]; then
{
  RES=`cat ../src/Makefile.am | grep -E '^SUBDIRS = test-bin'`
  if [[ -z $RES ]]; then
  {
    echo "According to the Makefile.am, the tests haven't compiled. Please turn on test-bin in the Makefile.am"
    exit 0
  };
  fi
};
else
{
  echo "Cannot locate ../src/Makefile.am . Please run the tests in the osm-3s_testing subdirectory."
  exit 0
};
fi
if [[ -r ../src/configure.ac ]]; then
{
  RES=`cat ../src/configure.ac | grep -E '^AC_CONFIG_FILES.* test-bin/Makefile'`
  if [[ -z $RES ]]; then
  {
    echo "According to configure.ac, the tests haven't compiled. Please turn on test-bin in configure.ac"
    exit 0
  };
  fi
};
else
{
  echo "Cannot locate ../src/configure.ac . Please run the tests in the osm-3s_testing subdirectory."
  exit 0
};
fi

# The size of the test pattern. Asymptotically, the test pattern consists of
# size^2 elements. The size must be divisible by ten. For a full featured test,
# set the value to 2000.
DATA_SIZE="$1"
BASEDIR="$(cd `dirname $0` && pwd)/.."
NOTIMES="$2"

$BASEDIR/test-bin/run_testsuite_template_db.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_testsuite_osm_backend.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_unittests_statements.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_testsuite_osm3s_query.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_testsuite_map_ql.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_testsuite_diff_updater.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_testsuite_interpreter.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_unittests_output_csv.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_unittests_areas.sh $DATA_SIZE $2

if [[ $DATA_SIZE -gt 800 ]]; then
{
  DATA_SIZE=800
}; fi

$BASEDIR/test-bin/run_unittests_meta.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_unittests_attic.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_unittests_vlt.sh $DATA_SIZE $2

$BASEDIR/test-bin/run_testsuite_translate_xapi.sh $DATA_SIZE $2
