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

if [[ -z $1  ]]; then
{
  echo "Usage: $0 test_size"
  echo
  echo "An appropriate value for a fast test is 40, a comprehensive value is 2000."
  exit 0
};
fi

BASEDIR="`pwd`/../"
INPUTDIR="../../input/vlt_model/"

mkdir -p run/vlt_model
rm -fR run/vlt_model/*

pushd run/vlt_model

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/init.osm

date '+%T'
{ cat $INPUTDIR/now.ql; echo "local; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_global.out 2>now_global.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local ll; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_global_ll.out 2>now_global_ll.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local llb; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_global_llb.out 2>now_global_llb.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local (52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_bbox.out 2>now_bbox.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local ll(52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_bbox_ll.out 2>now_bbox_ll.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local llb(52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_bbox_llb.out 2>now_bbox_llb.err
date '+%T'

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/diff_1.osc

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/killall.osc

date '+%T'
{ echo '[date:"2018-02-01T00:01:00Z"];'; cat $INPUTDIR/now.ql; echo "local; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >attic_global.out 2>attic_global.err
if [[ -z `diff now_global.out attic_global.out` ]]; then
  rm attic_global.out
fi
date '+%T'
{ echo '[date:"2018-02-01T00:01:00Z"];'; cat $INPUTDIR/now.ql; echo "local ll; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >attic_global_ll.out 2>attic_global_ll.err
if [[ -z `diff now_global_ll.out attic_global_ll.out` ]]; then
  rm attic_global_ll.out
fi
date '+%T'
{ echo '[date:"2018-02-01T00:01:00Z"];'; cat $INPUTDIR/now.ql; echo "local llb; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >attic_global_llb.out 2>attic_global_llb.err
if [[ -z `diff now_global_llb.out attic_global_llb.out` ]]; then
  rm attic_global_llb.out
fi
date '+%T'
{ echo '[date:"2018-02-01T00:01:00Z"];'; cat $INPUTDIR/now.ql; echo "local (52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >attic_bbox.out 2>attic_bbox.err
if [[ -z `diff now_bbox.out attic_bbox.out` ]]; then
  rm attic_bbox.out
fi
date '+%T'
{ echo '[date:"2018-02-01T00:01:00Z"];'; cat $INPUTDIR/now.ql; echo "local ll(52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >attic_bbox_ll.out 2>attic_bbox_ll.err
if [[ -z `diff now_bbox_ll.out attic_bbox_ll.out` ]]; then
  rm attic_bbox_ll.out
fi
date '+%T'
{ echo '[date:"2018-02-01T00:01:00Z"];'; cat $INPUTDIR/now.ql; echo "local llb(52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >attic_bbox_llb.out 2>attic_bbox_llb.err
if [[ -z `diff now_bbox_llb.out attic_bbox_llb.out` ]]; then
  rm attic_bbox_llb.out
fi
date '+%T'

date '+%T'
  cat $INPUTDIR/delta_test.ql \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >delta_test.out 2>delta_test.err
date '+%T'

for i in *.err; do
  diff -q "../../expected/vlt_model/$i" "$i"
done

for i in *.out; do
  cat <$i | sed 's/Overpass API [^ ]* [a-f0-9]*/Overpass API/g' >"_$i"
  mv "_$i" "$i"
  diff -q "../../expected/vlt_model/$i" "$i"
done

popd
