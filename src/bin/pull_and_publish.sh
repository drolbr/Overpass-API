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
  echo -e "\n\
Usage: $0\n\
    [--publish] [--install] [--test] [--init=PLANET_DATE]\n\
    [--run=REPLICATE_ID] --version==Version\n\
where\n\
\n\
--publish=PATH\n\
    Tar-gz the automaked version to \$PATH/osm-3s_v\$VERSION.tar.gz\n\
\n\
--install\n\
    Install the software to /srv/osm-3s/v\$VERSION\n\
\n\
--test\n\
    Test the installed software with the standard testbed, size 2000.\n\
    Break if a test fails.\n\
\n\
--init=PLANET_DATE\n\
    Download planet file of the given date and init the database from\n\
    that planet file.\n\
\n\
--run=REPLICATE_ID\n\
    Startup dispatcher and apply_osc_to_db, from the given \$REPLICATE_ID.\n\
\n\
--fetch=REPLICATE_DIR\n\
    Startup fetch_osc from the \$REPLICATE_ID given for run with replicate dir
    set to \$REPLICATE_DIR.\n\
\n\
--run-areas\n\
    Startup the dispatcher for areas and rules_loop.\n\
\n\
--meta\n\
    Also process OSM meta data.\n\
"
  exit 0
};
fi

PUBLISH=
INSTALL=
TEST=
INIT=
RUN=
FETCH=
VERSION=
RUN_AREAS=
META=

EXEC_DIR=/srv/osm-3s
DB_DIR=/opt/osm-3s
PLANET_DIR=/home/roland/osm-planet
REPLICATE_DIR=/home/roland/osm-replicate-diffs/
DOWNLOAD_PLANET=https://ftp.heanet.ie/mirrors/openstreetmap.org/
DOWNLOAD_DIFFS=https://planet.openstreetmap.org/replication/minute/

process_param()
{
  if [[ "${1:0:10}" == "--publish=" ]]; then
  {
    PUBLISH="${1:10}"
  };
  elif [[ "$1" == "--install" ]]; then
  {
    INSTALL="yes"
  };
  elif [[ "$1" == "--test" ]]; then
  {
    TEST="yes"
  };
  elif [[ "${1:0:7}" == "--init=" ]]; then
  {
    INIT="${1:7}"
  };
  elif [[ "${1:0:6}" == "--run=" ]]; then
  {
    RUN="${1:6}"
  };
  elif [[ "${1:0:8}" == "--fetch=" ]]; then
  {
    FETCH="yes"
  };
  elif [[ "${1:0:10}" == "--version=" ]]; then
  {
    VERSION="${1:10}"
  };
  elif [[ "$1" == "--run-areas" ]]; then
  {
    RUN_AREAS="yes"
  };
  elif [[ "$1" == "--meta" ]]; then
  {
    META="--meta"
  };
  else
  {
    echo "Unknown argument: $1"
    exit 0
  };
  fi
};

if [[ -n $1  ]]; then process_param $1; fi
if [[ -n $2  ]]; then process_param $2; fi
if [[ -n $3  ]]; then process_param $3; fi
if [[ -n $4  ]]; then process_param $4; fi
if [[ -n $5  ]]; then process_param $5; fi
if [[ -n $6  ]]; then process_param $6; fi
if [[ -n $7  ]]; then process_param $7; fi
if [[ -n $8  ]]; then process_param $8; fi

if [[ -z $VERSION ]]; then
{
  echo "Error: Version must be nonempty"
  exit 0
};
fi
TARGET_DIR=$EXEC_DIR/v$VERSION

if [[ -n $PUBLISH || -n $INSTALL ]]; then
{
  git clone git://github.com/drolbr/Overpass-API.git

  pushd Overpass-API/
  yes | rm -R .git

  pushd src/
  autoscan
  libtoolize
  aclocal
  autoheader
  automake --add-missing
  autoconf
  rm -R autom4te.cache
  popd

  popd

  rm -fR "osm-3s_v$VERSION"
  mv Overpass-API "osm-3s_v$VERSION"
};
fi

if [[ -n $PUBLISH ]]; then
{
  #echo "SUBDIRS = bin cgi-bin" >"osm-3s_v$VERSION/src/Makefile.am"
  tar chvf - --exclude=osm-3s_testing "osm-3s_v$VERSION/" | gzip >"$PUBLISH/osm-3s_v$VERSION.tar.gz"
  #echo "SUBDIRS = bin cgi-bin test-bin" >"osm-3s_v$VERSION/src/Makefile.am"
};
fi

if [[ -n $INSTALL ]]; then
{
  if [[ -z $TEST ]]; then
  {
    echo "SUBDIRS = bin cgi-bin" >"osm-3s_v$VERSION/src/Makefile.am"
  }; fi
  pushd "osm-3s_v$VERSION/build/"
  ../src/configure --prefix="$TARGET_DIR/"
  make install
  popd
};
fi

if [[ -n $TEST ]]; then
{
  mkdir -p "osm-3s_v$VERSION/osm-3s_testing/"
  pushd "osm-3s_v$VERSION/osm-3s_testing/"
  $TARGET_DIR/test-bin/run_testsuite.sh 2000 notimes >test.stdout.log 2>test.stderr.log
  FAILED=`grep FAILED test.stdout.log`
  popd

  if [[ -n $FAILED ]]; then
  {
    cat "osm-3s_v$VERSION/osm-3s_testing/test.stdout.log"
    exit 0
  };
  fi

  echo "All tests successful."
};
fi

if [[ -n $INIT ]]; then
{
  while [[ ! -s $PLANET_DIR/planet-$INIT.osm.bz2 ]]; do
  {
    wget -nv -O $PLANET_DIR/planet-$INIT.osm.bz2 ${DOWNLOAD_PLANET}planet-$INIT.osm.bz2
    if [[ ! -s $PLANET_DIR/planet-$INIT.osm.bz2 ]]; then
      sleep 300
    fi
  };
  done

  mkdir -p "$DB_DIR/v$VERSION"
  bunzip2 <$PLANET_DIR/planet-$INIT.osm.bz2 | $EXEC_DIR/v$VERSION/bin/update_database --db-dir=$DB_DIR/v$VERSION/ $META
};
fi

if [[ -n $FETCH ]]; then
  $EXEC_DIR/v$VERSION/bin/fetch_osc.sh $RUN $DOWNLOAD_DIFFS $REPLICATE_DIR &
fi

if [[ -n $RUN ]]; then
{
  pushd $EXEC_DIR/v$VERSION/bin
  ./dispatcher --osm-base $META --db-dir=$DB_DIR/v$VERSION/ &
  sleep 5
  ./apply_osc_to_db.sh $DB_DIR/v$VERSION/ $REPLICATE_DIR $RUN $META &
  popd
};
fi

if [[ -n $RUN_AREAS ]]; then
{
  pushd $EXEC_DIR/v$VERSION/bin
  mkdir -p $DB_DIR/v$VERSION/rules/
  cp ../rules/areas.osm3s $DB_DIR/v$VERSION/rules/

  ./dispatcher --areas --db-dir=$DB_DIR/v$VERSION/ &
  sleep 5
  ./rules_loop.sh $DB_DIR/v$VERSION/ &
  popd
};
fi
