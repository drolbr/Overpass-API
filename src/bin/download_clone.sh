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

EXEC_DIR="`pwd`/../"
CLONE_DIR="$1"
REMOTE_DIR=
SOURCE=
DONE=
META=

if [[ -z $1 ]]; then
{
  echo "Usage: $0 --db-dir=database_dir --source=http://overpass-api.de/api/ --meta=(yes|no)"
  exit 0
}; fi

process_param()
{
  if [[ "${1:0:9}" == "--db-dir=" ]]; then
  {
    CLONE_DIR="${1:9}"
  };
  elif [[ "${1:0:9}" == "--source=" ]]; then
  {
    SOURCE="${1:9}"
  };
  elif [[ "${1:0:7}" == "--meta=" ]]; then
  {
    META="${1:7}"
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

FILES_BASE="nodes.bin nodes.map node_tags_local.bin node_tags_global.bin ways.bin ways.map way_tags_local.bin way_tags_global.bin relations.bin relations.map relation_roles.bin relation_tags_local.bin relation_tags_global.bin"

FILES_META="nodes_meta.bin ways_meta.bin relations_meta.bin user_data.bin user_indices.bin"

# $1 - remote source
# $2 - local destination
fetch_file()
{
  wget -c -O "$2" "$1"
};

retry_fetch_file()
{
  fetch_file "$1" "$2"
  until [[ -s "$2" ]]; do {
    sleep 15
    fetch_file "$1" "$2"
  }; done
};

download_file()
{
  echo
  echo "Fetching $1.gz"
  retry_fetch_file "$REMOTE_DIR/$1.gz" "$CLONE_DIR/$1.gz"
  gunzip "$CLONE_DIR/$1.gz" &
  echo "Uncompressing it in the background."
  echo "Fetching $1.idx"
  retry_fetch_file "$REMOTE_DIR/$1.idx" "$CLONE_DIR/$1.idx"
}

check_gz()
{
  if [[ -s "$1.gz" ]]; then {
    DONE=
  }; fi
}

fetch_file "$SOURCE/trigger_clone" "$CLONE_DIR/base-url"

REMOTE_DIR=`cat <"$CLONE_DIR/base-url"`
echo "Triggered generation of a recent clone"
sleep 30

retry_fetch_file "$REMOTE_DIR/replicate_id" "$CLONE_DIR/replicate_id"

for I in $FILES_BASE; do
{
  download_file $I
}; done

if [[ $META == "yes" ]]; then
{
  for I in $FILES_META; do
  {
    download_file $I
  }; done
}; fi

echo "Waiting for all files to be uncompressed "
DONE=
while [[ $DONE != "yes" ]]; do
{
  DONE="yes"
  echo -n "."

  for I in $FILES_BASE; do
  {
    check_gz $I
  }; done

  if [[ $META == "yes" ]]; then
  {
    for I in $FILES_META; do
    {
      check_gz $I
    }; done
  }; fi

  sleep 1
}; done

echo " database ready."
