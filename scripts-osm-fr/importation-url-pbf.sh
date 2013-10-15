#!/usr/bin/env bash

. /etc/default/overpass


if [ $META == "yes" ] ; then
META_OPTION="--meta"
fi

wget -q $1 -O - | ./osmconvert - --out-osm | $EXEC_DIR/update_database --db-dir=$DB_DIR/ $META_OPTION
