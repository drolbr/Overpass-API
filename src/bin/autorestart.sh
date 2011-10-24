#!/bin/bash

VERSION=0.6.94

EXEC_DIR="/srv/osm-3s/v$VERSION/"
DB_DIR="/opt/osm-3s/v$VERSION/"
REPLICATE_DIR="/home/roland/osm-replicate-diffs/"
DOWNLOAD_DIFFS="http://planet.openstreetmap.org/minute-replicate/"

pushd $EXEC_DIR/bin
./dispatcher --osm-base --meta --db-dir="$DB_DIR" &>nohup.out &
./dispatcher --osm-base --meta --db-dir="$DB_DIR" &>nohup.out &
sleep 5
./apply_osc_to_db.sh "$DB_DIR" "$REPLICATE_DIR" `cat "$DB_DIR/replicate_id"` --meta &
sleep 5
./rules_loop.sh "$DB_DIR" &
popd
