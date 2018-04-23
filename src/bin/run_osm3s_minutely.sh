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
    --run=REPLICATE_ID --db-dir=DB_DIR --replicate-dir=REPLICATE_DIR\n\
    --minute-url=MINUTE_URL [--meta] \n\
where\n\
\n\
--run=REPLICATE_ID\n\
    The \$REPLICATE_ID to start the minute diffs from.\n\
\n\
--db-dir=DB_DIR\n\
    The directory where the database is stored. There need to be about 60 GB\n\
    space for the whole planet.\n\
\n\
--replicate-dir=REPLICATE_DIR\n\
    The directory where the minute diffs are stored. The minute diffs can be\n\
    deleted after they are applied.\n\
\n\
--minute-url=MINUTE_URL\n\
    Specify the URL to download the minute diffs from. The usual URL is\n\
    https://planet.openstreetmap.org/replication/minute/\n\
\n\
--meta=yes, --meta=no\n\
    Keep or keep not meta data in the database.\n\
\n\
--areas=yes, --areas=no\n\
    Create or create not derived area data.\n\
\n\
"
  exit 0
};
fi

RUN=
DB_DIR=
REPLICATE_DIR=
DOWNLOAD_DIFFS=
META=
NOMETA=

EXEC_DIR="`dirname $0`/"
if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="`pwd`/$EXEC_DIR"
};
fi

process_param()
{
  if [[ "${1:0:6}" == "--run=" ]]; then
  {
    RUN="${1:6}"
  };
  elif [[ "$1" == "--db-dir=" ]]; then
  {
    DB_DIR="${1:8}"
  };
  elif [[ "$1" == "--replicate-dir=" ]]; then
  {
    REPLICATE_DIR="${1:16}"
  };
  elif [[ "$1" == "--minute-url=" ]]; then
  {
    DOWNLOAD_DIFFS="${1:13}"
  };
  elif [[ "$1" == "--meta=yes" ]]; then
  {
    META="--meta"
  };
  elif [[ "$1" == "--meta=no" ]]; then
  {
    NOMETA="yes"
  };
  elif [[ "${1:0:8}" == "--areas=" ]]; then
  {
    AREAS="${1:8}"
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

if [[ -z $RUN ]]; then
{
  echo "Error: first_diff must be nonempty."
  exit 0
}; fi

if [[ -z $DB_DIR ]]; then
{
  echo "Error: database_dir must be nonempty."
  exit 0
}; fi

if [[ -z $REPLICATE_DIR ]]; then
{
  echo "Error: replicate_dir must be nonempty."
  exit 0
}; fi

if [[ -z $DOWNLOAD_DIFFS ]]; then
{
  echo "Error: You need to provide an URL to download the minute diffs from."
  exit 0
}; fi

if [[ -z $META || -z $NOMETA ]]; then
{
  echo "Error: Please choose if you want meta data or not."
  exit 0
}; fi

if [[ -z $AREAS ]]; then
{
  echo "Error: Please choose if you want areas or not."
  exit 0
}; fi

pushd "$EXEC_DIR"

./fetch_osc.sh $RUN $DOWNLOAD_DIFFS $REPLICATE_DIR &
./dispatcher --osm-base $META --db-dir=$DB_DIR/ &
if [[ $AREAS == "yes" ]]; then
{
  ./dispatcher --areas --db-dir=$DB_DIR/ &
};fi
sleep 5
./apply_osc_to_db.sh $DB_DIR/ $REPLICATE_DIR $RUN $META &
