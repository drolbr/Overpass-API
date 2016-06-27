#!/usr/bin/env bash

. /etc/default/overpass


if [ $META == "yes" ] ; then
META_OPTION="--meta"
fi

bunzip2 $1 -c | $EXEC_DIR/update_database --db-dir=$DB_DIR/ $META_OPTION

echo "All is in the title" | mail -s "OverpassAPI import ended on `hostname -f` `date`" your-email@here.com