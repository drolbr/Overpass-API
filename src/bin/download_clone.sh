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
  echo "Usage: $0 --db-dir=database_dir --source=http://dev.overpass-api.de/api_drolbr/ --meta=(yes|no|attic)"
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

FILES_BASE="\
nodes.bin nodes.map node_tags_local.bin node_tags_global.bin node_keys.bin \
ways.bin ways.map way_tags_local.bin way_tags_global.bin way_keys.bin \
relations.bin relations.map relation_roles.bin relation_tags_local.bin relation_tags_global.bin relation_keys.bin"

FILES_META="\
nodes_meta.bin \
ways_meta.bin \
relations_meta.bin \
user_data.bin user_indices.bin"

FILES_ATTIC="\
nodes_attic.bin nodes_attic.map node_attic_indexes.bin nodes_attic_undeleted.bin nodes_meta_attic.bin \
node_changelog.bin node_tags_local_attic.bin node_tags_global_attic.bin \
ways_attic.bin ways_attic.map way_attic_indexes.bin ways_attic_undeleted.bin ways_meta_attic.bin \
way_changelog.bin way_tags_local_attic.bin way_tags_global_attic.bin \
relations_attic.bin relations_attic.map relation_attic_indexes.bin relations_attic_undeleted.bin relations_meta_attic.bin \
relation_changelog.bin relation_tags_local_attic.bin relation_tags_global_attic.bin"

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
  echo "Fetching $1"
  retry_fetch_file "$REMOTE_DIR/$1" "$CLONE_DIR/$1"
  echo "Fetching $1.idx"
  retry_fetch_file "$REMOTE_DIR/$1.idx" "$CLONE_DIR/$1.idx"
}

fetch_file "$SOURCE/trigger_clone" "$CLONE_DIR/base-url"

REMOTE_DIR=`cat <"$CLONE_DIR/base-url"`
#echo "Triggered generation of a recent clone"
#sleep 30

retry_fetch_file "$REMOTE_DIR/replicate_id" "$CLONE_DIR/replicate_id"

for I in $FILES_BASE; do
{
  download_file $I
}; done

if [[ $META == "yes" || $META == "attic" ]]; then
{
  for I in $FILES_META; do
  {
    download_file $I
  }; done
}; fi

if [[ $META == "attic" ]]; then
{
  for I in $FILES_ATTIC; do
  {
    download_file $I
  }; done
}; fi

echo " database ready."
