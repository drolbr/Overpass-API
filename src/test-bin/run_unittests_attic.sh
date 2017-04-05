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

if [[ -z $1  ]]; then
{
  echo "Usage: $0 test_size"
  echo
  echo "An appropriate value for a fast test is 40, a comprehensive value is 2000."
  exit 0
};
fi

BASEDIR="`pwd`/../"
INPUTDIR="../../input/attic_updater/"

mkdir -p run/attic_updater
rm -fR run/attic_updater/*

pushd run/attic_updater

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/init.osm

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query.ql >init_query.out 2>init_query.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0901.ql >init_query_0901.out 2>init_query_0901.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0902.ql >init_query_0902.out 2>init_query_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0903.ql >init_query_0903.out 2>init_query_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0904.ql >init_query_0904.out 2>init_query_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0905.ql >init_query_0905.out 2>init_query_0905.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/diff_0902.ql >init_diff_0902.out 2>init_diff_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/adiff_0902.ql >init_adiff_0902.out 2>init_adiff_0902.err

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/diff_1.osc

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query.ql >diff_1_query.out 2>diff_1_query.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0901.ql >diff_1_query_0901.out 2>diff_1_query_0901.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0902.ql >diff_1_query_0902.out 2>diff_1_query_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0903.ql >diff_1_query_0903.out 2>diff_1_query_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0904.ql >diff_1_query_0904.out 2>diff_1_query_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0905.ql >diff_1_query_0905.out 2>diff_1_query_0905.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/diff_0902.ql >diff_1_diff_0902.out 2>diff_1_diff_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/adiff_0902.ql >diff_1_adiff_0902.out 2>diff_1_adiff_0902.err

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/diff_2.osc

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query.ql >diff_2_query.out 2>diff_2_query.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0901.ql >diff_2_query_0901.out 2>diff_2_query_0901.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0902.ql >diff_2_query_0902.out 2>diff_2_query_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0903.ql >diff_2_query_0903.out 2>diff_2_query_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0904.ql >diff_2_query_0904.out 2>diff_2_query_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0905.ql >diff_2_query_0905.out 2>diff_2_query_0905.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/diff_0902.ql >diff_2_diff_0902.out 2>diff_2_diff_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/adiff_0902.ql >diff_2_adiff_0902.out 2>diff_2_adiff_0902.err

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/diff_3.osc

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query.ql >diff_3_query.out 2>diff_3_query.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0901.ql >diff_3_query_0901.out 2>diff_3_query_0901.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0902.ql >diff_3_query_0902.out 2>diff_3_query_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0903.ql >diff_3_query_0903.out 2>diff_3_query_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0904.ql >diff_3_query_0904.out 2>diff_3_query_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/query_0905.ql >diff_3_query_0905.out 2>diff_3_query_0905.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/diff_0902.ql >diff_3_diff_0902.out 2>diff_3_diff_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/adiff_0902.ql >diff_3_adiff_0902.out 2>diff_3_adiff_0902.err

date '+%T'

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_0902.ql >changed_0902.out 2>changed_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_0903.ql >changed_0903.out 2>changed_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_0904.ql >changed_0904.out 2>changed_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_0905.ql >changed_0905.out 2>changed_0905.err

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_fwd_0902.ql >changed_fwd_0902.out 2>changed_fwd_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_fwd_0903.ql >changed_fwd_0903.out 2>changed_fwd_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_fwd_0904.ql >changed_fwd_0904.out 2>changed_fwd_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=./ <$INPUTDIR/changed_fwd_0905.ql >changed_fwd_0905.out 2>changed_fwd_0905.err

date '+%T'

mkdir -p clone
rm -fR clone/*
date '+%T'; $BASEDIR/bin/osm3s_query --db-dir=./ --clone="clone/" >clone.out 2>clone.err

date '+%T'

date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/query.ql >diff_clone_query.out 2>diff_clone_query.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/query_0901.ql >diff_clone_query_0901.out 2>diff_clone_query_0901.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/query_0902.ql >diff_clone_query_0902.out 2>diff_clone_query_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/query_0903.ql >diff_clone_query_0903.out 2>diff_clone_query_0903.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/query_0904.ql >diff_clone_query_0904.out 2>diff_clone_query_0904.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/query_0905.ql >diff_clone_query_0905.out 2>diff_clone_query_0905.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/diff_0902.ql >diff_clone_diff_0902.out 2>diff_clone_diff_0902.err
date '+%T'; $BASEDIR/bin/osm3s_query --concise --db-dir=clone/ <$INPUTDIR/adiff_0902.ql >diff_clone_adiff_0902.out 2>diff_clone_adiff_0902.err

date '+%T'

for i in *.err; do
  diff -q "../../expected/attic_updater/$i" "$i"
done

for i in *.out; do
  diff -q "../../expected/attic_updater/$i" "$i"
done

popd
