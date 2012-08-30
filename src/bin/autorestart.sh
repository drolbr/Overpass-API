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

VERSION=0.6.96

EXEC_DIR="/srv/osm-3s/v$VERSION/"
DB_DIR="/opt/osm-3s/v$VERSION/"
REPLICATE_DIR="/home/roland/osm-replicate-diffs/"
DOWNLOAD_DIFFS="http://planet.openstreetmap.org/minute-replicate/"
META="--meta"
AREAS="yes"

./fetch_osc.sh `cat "$DB_DIR/replicate_id"` "$DOWNLOAD_DIFFS" "$REPLICATE_DIR" &
pushd $EXEC_DIR/bin
./dispatcher --osm-base $META --db-dir="$DB_DIR" &>nohup.out &
if [[ -n $AREAS ]]; then
{
  ./dispatcher --areas --db-dir="$DB_DIR" &>nohup.out &
}; fi
sleep 5
./apply_osc_to_db.sh "$DB_DIR" "$REPLICATE_DIR" `cat "$DB_DIR/replicate_id"` $META &
sleep 5
./rules_loop.sh "$DB_DIR" &
popd
