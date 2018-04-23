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

echo "Overpass QL:"
echo "<source lang=cpp>"
echo "$1" | ./osm3s_query --dump-pretty-ql --concise
echo "</source>"
echo
echo "XML:"
echo "<source lang=xml>"
echo "$1" | ./osm3s_query --dump-xml --concise
echo "</source>"
echo
echo "Display result: [https://overpass-api.de/api/convert?data=`echo "$1" | ./tocgi`&target=openlayers&zoom=12&lat=50.72&lon=7.1 OpenLayers map] [https://overpass-api.de/api/interpreter?data=%5Bout:json%5D;`echo "$1" | ./tocgi` JSON], [https://overpass-api.de/api/interpreter?data=`echo "$1" | ./tocgi` XML]."
