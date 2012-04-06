#!/usr/bin/env bash

# Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
#
# This file is part of Overpass_API.
#
# Overpass_API is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Overpass_API is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.

EXEC_DIR="`pwd`/../"
CLONE_DIR="$1"
REMOTE_DIR=
DONE=

FILES_TO_HANDLE="nodes.bin nodes.map node_tags_local.bin node_tags_global.bin ways.bin ways.map way_tags_local.bin way_tags_global.bin relations.bin relations.map relation_roles.bin relation_tags_local.bin relation_tags_global.bin nodes_meta.bin ways_meta.bin relations_meta.bin user_data.bin user_indices.bin"

# $1 - remote source
# $2 - local destination
fetch_file()
{
  wget -O "$2" "$1"
};

retry_fetch_file()
{
  if [[ ! -s "$2" ]]; then {
    fetch_file "$1" "$2"
  }; fi
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

fetch_file "http://overpass-api.de/api/trigger_clone" "$CLONE_DIR/base-url"

REMOTE_DIR=`cat <"$CLONE_DIR/base-url"`
echo "Triggered generation of a recent clone"
sleep 30

retry_fetch_file "$REMOTE_DIR/replicate_id" "$CLONE_DIR/replicate_id"

for I in $FILES_TO_HANDLE; do
{
  download_file $I
}; done

# download_file nodes.bin
# download_file nodes.map
# download_file node_tags_local.bin
# download_file node_tags_global.bin
# 
# download_file ways.bin
# download_file ways.map
# download_file way_tags_local.bin
# download_file way_tags_global.bin
# 
# download_file relations.bin
# download_file relations.map
# download_file relation_roles.bin
# download_file relation_tags_local.bin
# download_file relation_tags_global.bin
# 
# download_file nodes_meta.bin
# download_file ways_meta.bin
# download_file relations_meta.bin
# download_file user_data.bin
# download_file user_indices.bin

echo "Waiting for all files to be uncompressed "
DONE=
while [[ $DONE != "yes" ]]; do
{
  DONE="yes"
  echo -n "."

  for I in $FILES_TO_HANDLE; do
  {
    check_gz $I
  }; done

  sleep 1
}; done

echo " database ready."
