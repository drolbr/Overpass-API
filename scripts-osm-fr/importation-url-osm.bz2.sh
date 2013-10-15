#!/usr/bin/env bash

. /etc/default/overpass


if [ $META == "yes" ] ; then
META_OPTION="--meta"
fi

wget -q $1 -O - | bunzip2 | $EXEC_DIR/update_database --db-dir=$DB_DIR/ $META_OPTION
