#!/usr/bin/env bash

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
