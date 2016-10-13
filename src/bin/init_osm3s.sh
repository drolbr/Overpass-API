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

if [[ -z $3 ]]; then
{
  echo "Usage:  $0  Planet_File  Database_Dir  Executable_Dir  [--meta]"
  echo "        where"
  echo "    Planet_File is the filename and path of the compressed planet file, including .bz2,"
  echo "    Database_Dir is the directory the database should go into, and"
  echo "    Executable_Dir is the directory that contains the executable update_database."
  echo "    Add --meta in the end if you want to use meta data."
  exit 0
}; fi

PLANET_FILE="$1"
DB_DIR="$2"
EXEC_DIR="$3"
META="$4"
COMPRESSION="$5"

if [[ ! -s $PLANET_FILE ]]; then
{
  echo "File $PLANET_FILE doesn't exist."
  exit 1
}; fi

mkdir -p "$DB_DIR/"
bunzip2 <$PLANET_FILE | $EXEC_DIR/bin/update_database --db-dir=$DB_DIR/ $META $COMPRESSION
