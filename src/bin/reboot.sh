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

EXEC_DIR=""
DB_DIR=""
DIFF_DIR=""

if [[ -z $DB_DIR || -z $EXEC_DIR || -z $DIFF_DIR ]]; then
  echo "To use this script, you must do the following things:"
  echo "- edit the definitions of DB_DIR, EXEC_DIR, and DIFF_DIR in this file according to your local settings"
  echo "- put this file into your crontab, with time spec @reboot"
  exit 0
fi

rm -f "$DB_DIR/osm3s_v0.7.54_osm_base"
nohup "$EXEC_DIR/dispatcher" --osm-base --attic --rate-limit=2 --space=10737418240 "--db-dir=$DB_DIR" >>"$EXEC_DIR/osm_base.out" &

if [[ -s "$DB_DIR/replicate_id" ]]; then
  nohup "$EXEC_DIR/fetch_osc.sh" `cat "$DB_DIR/replicate_id"` "https://planet.openstreetmap.org/replication/minute/" "$DIFF_DIR" >>"$EXEC_DIR/fetch_osc.out" &
  nohup "$EXEC_DIR/apply_osc_to_db.sh" "$DIFF_DIR" auto --meta=attic >>"$EXEC_DIR/apply_osc_to_db.out" &
fi

if [[ -s "$DB_DIR/areas.bin" ]]; then
  rm -f "$DB_DIR/osm3s_v0.7.54_areas"
  nohup "$EXEC_DIR/dispatcher" --areas "--db-dir=$DB_DIR" >>"$EXEC_DIR/areas.out" &
fi
