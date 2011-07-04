#!/bin/bash

if [[ -z $3 ]]; then
{
  echo "Usage:  $0  Planet_File  Database_Dir  Executable_Dir"
  echo "        where"
  echo "    Planet_File is the filename and path of the compressed planet file, including .bz2,"
  echo "    Database_Dir is the directory the database should go into, and"
  echo "    Executable_Dir is the directory that contains the executable update_database."
  exit 0
}; fi

PLANET_FILE=$1
DB_DIR=$2
EXEC_DIR=$3

if [[ ! -s $PLANET_FILE ]]; then
{
  echo "File $PLANET_FILE doesn't exist."
  exit 1
}; fi

mkdir -p "$DB_DIR/"
bunzip2 <$PLANET_FILE | $EXEC_DIR/bin/update_database --db-dir=$DB_DIR/
