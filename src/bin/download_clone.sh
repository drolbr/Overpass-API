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

set -euo pipefail

CLONE_DIR="$1"
REMOTE_DIR=
SOURCE=
META=
TEMP_FILE=/tmp/ovepass_files_list

if [[ -z $1 ]]; then
{
  echo "Usage: $0 --db-dir=database_dir --source=https://dev.overpass-api.de/api_drolbr/ --meta=(yes|no|attic)"
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

if [[ -n ${1} ]]; then process_param "${1}"; fi
if [[ -n ${2} ]]; then process_param "${2}"; fi
if [[ -n ${3} ]]; then process_param "${3}"; fi

FILES_BASE="\
nodes.bin nodes.map node_tags_local.bin node_tags_global.bin node_frequent_tags.bin node_keys.bin \
ways.bin ways.map way_tags_local.bin way_tags_global.bin way_frequent_tags.bin way_keys.bin \
relations.bin relations.map relation_roles.bin relation_tags_local.bin relation_tags_global.bin relation_frequent_tags.bin relation_keys.bin"

FILES_META="\
nodes_meta.bin \
ways_meta.bin \
relations_meta.bin \
user_data.bin user_indices.bin"

FILES_ATTIC="\
nodes_attic.bin nodes_attic.map node_attic_indexes.bin nodes_attic_undeleted.bin nodes_meta_attic.bin \
node_changelog.bin node_tags_local_attic.bin node_tags_global_attic.bin node_frequent_tags_attic.bin \
ways_attic.bin ways_attic.map way_attic_indexes.bin ways_attic_undeleted.bin ways_meta_attic.bin \
way_changelog.bin way_tags_local_attic.bin way_tags_global_attic.bin way_frequent_tags_attic.bin \
relations_attic.bin relations_attic.map relation_attic_indexes.bin relations_attic_undeleted.bin relations_meta_attic.bin \
relation_changelog.bin relation_tags_local_attic.bin relation_tags_global_attic.bin relation_frequent_tags_attic.bin"

# $1 - remote source
# $2 - local destination
fetch_file()
{
  wget -c -O "${2}" "${1}"
};

retry_fetch_file()
{
  DEADLINE=$(($(date '+%s') + 86400))
  rm -f "${2}"
  fetch_file "${1}" "${2}"
  until [[ -s "${2}" ]]; do {
    if [[ $(date '+%s' || true) -ge ${DEADLINE} ]]; then
      echo "File ${1} unavailable. Aborting."
      exit 1
    fi
    sleep 15
    fetch_file "${1}" "${2}"
  }; done
};

parallel_download ()
{
  aria2c -d "${CLONE_DIR}" -j 16 -R -x 16 -i "${TEMP_FILE}"
};

mkdir -p "${CLONE_DIR}"
fetch_file "${SOURCE}/trigger_clone" "${CLONE_DIR}/base-url"

REMOTE_DIR=$(cat < "${CLONE_DIR}/base-url")
#echo "Triggered generation of a recent clone"
#sleep 30

retry_fetch_file "${REMOTE_DIR}/replicate_id" "${CLONE_DIR}/replicate_id"

rm -f "${TEMP_FILE}}"
touch "${TEMP_FILE}"
for I in ${FILES_BASE}; do
{
  echo "${REMOTE_DIR}/${I}" >> "${TEMP_FILE}"
  echo "${REMOTE_DIR}/${I}.idx" >> "${TEMP_FILE}"
}; done
parallel_download

rm -f "${TEMP_FILE}}"
if [[ ${META} == "yes" || ${META} == "attic" ]]; then
{
  for I in ${FILES_META}; do
  {
    echo "${REMOTE_DIR}/${I}" >> "${TEMP_FILE}"
    echo "${REMOTE_DIR}/${I}.idx" >> "${TEMP_FILE}"
  }; done
  parallel_download
}; fi

rm -f "${TEMP_FILE}}"
if [[ ${META} == "attic" ]]; then
{
  for I in ${FILES_ATTIC}; do
  {
    echo "${REMOTE_DIR}/${I}" >> "${TEMP_FILE}"
    echo "${REMOTE_DIR}/${I}.idx" >> "${TEMP_FILE}"
  }; done
  parallel_download
}; fi
rm -f "${TEMP_FILE}}"

echo " database ready."
