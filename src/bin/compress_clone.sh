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

EXEC_DIR="`pwd`/../"
DB_DIR=`../bin/dispatcher --show-dir`
CLONE_DIR="$1"

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

FILES_TO_HANDLE="$FILES_BASE $FILES_META $FILES_ATTIC"

compress_file()
{
  while [[ ! -s "$CLONE_DIR/$1.idx" ]]; do
  {
    sleep 5
  }; done
  gzip <"$CLONE_DIR/$1" >"$CLONE_DIR/temp.gz"
  mv "$CLONE_DIR/temp.gz" "$CLONE_DIR/$1.gz"
  rm "$CLONE_DIR/$1"
}

mkdir -p "$CLONE_DIR"
chmod 777 "$CLONE_DIR"

while [[ true ]]; do
{
  while [[ ! -e "$CLONE_DIR/trigger" ]]; do
  {
    sleep 15
  }; done

  rm -f "$CLONE_DIR/replicate_id"
  for I in $FILES_TO_HANDLE; do
  {
    rm -f "$CLONE_DIR/$I.gz"
    rm -f "$CLONE_DIR/$I.idx"
  }; done

  "$EXEC_DIR/bin/clone.sh" "$DB_DIR" "$CLONE_DIR" &

  for I in $FILES_TO_HANDLE; do
  {
    compress_file $I
  }; done

#   compress_file nodes.bin
#   compress_file nodes.map
#   compress_file node_tags_local.bin
#   compress_file node_tags_global.bin
#
#   compress_file ways.bin
#   compress_file ways.map
#   compress_file way_tags_local.bin
#   compress_file way_tags_global.bin
#
#   compress_file relations.bin
#   compress_file relations.map
#   compress_file relation_roles.bin
#   compress_file relation_tags_local.bin
#   compress_file relation_tags_global.bin
#
#   compress_file nodes_meta.bin
#   compress_file ways_meta.bin
#   compress_file relations_meta.bin
#   compress_file user_data.bin
#   compress_file user_indices.bin

  while [[ -e "$CLONE_DIR/trigger" ]]; do
  {
    rm -f "$CLONE_DIR/trigger"
    sleep 28800
  }; done

}; done
