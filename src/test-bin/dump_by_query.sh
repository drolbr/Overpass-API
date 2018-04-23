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

create_query()
{
  echo "<osm-script timeout=\"86400\">" >$5

  LAT=$1
  while [[ $LAT -lt $2 ]]; do
  {
    LON=$3
    while [[ $LON -lt $4 ]]; do
    {
      echo "<bbox-query s=\"$LAT\" n=\"$(($LAT+1))\" w=\"$LON\" e=\"$(($LON+1))\"/>" >>$5
      echo "<print mode=\"$6\"/>" >>$5
      echo "<recurse type=\"node-way\" into=\"ways\"/>" >>$5
      echo "<print mode=\"$6\" from=\"ways\"/>" >>$5
      echo "<union>" >>$5
      echo "  <recurse type=\"node-relation\"/>" >>$5
      echo "  <recurse type=\"way-relation\" from=\"ways\"/>" >>$5
      echo "</union>" >>$5
      echo "<print mode=\"$6\"/>" >>$5
      echo >>$5
      LON=$(($LON + 1))
    }; done
    LAT=$(($LAT+1))
  }; done

  echo "</osm-script>" >>$5
};

perform_query()
{
  create_query $1 $2 $3 $4 query_$5.xml $6
  $7/bin/osm3s_query <query_sww.xml | gzip >dump_$5.xml.gz
};

perform_query_stripe()
{
  perform_query -90 -30 $1 $2 "s$3" $4 $5
  perform_query -30 30 $1 $2 "m$3" $4 $5
  perform_query 30 90 $1 $2 "n$3" $4 $5
};

perform_global_query()
{
  perform_query_stripe -180 -120 ww $1 $2
  perform_query_stripe -120 -60 wm $1 $2
  perform_query_stripe -60 0 we $1 $2
  perform_query_stripe 0 60 ew $1 $2
  perform_query_stripe 60 120 em $1 $2
  perform_query_stripe 120 180 ee $1 $2
};

perform_global_query $1 $2
