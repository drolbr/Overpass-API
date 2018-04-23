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

mv "$1" _

echo '/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.' >>$1
echo ' *' >>$1
echo ' * This file is part of Overpass_API.' >>$1
echo ' *' >>$1
echo ' * Overpass_API is free software: you can redistribute it and/or modify' >>$1
echo ' * it under the terms of the GNU Affero General Public License as' >>$1
echo ' * published by the Free Software Foundation, either version 3 of the' >>$1
echo ' * License, or (at your option) any later version.' >>$1
echo ' *' >>$1
echo ' * Overpass_API is distributed in the hope that it will be useful,' >>$1
echo ' * but WITHOUT ANY WARRANTY; without even the implied warranty of' >>$1
echo ' * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the' >>$1
echo ' * GNU General Public License for more details.' >>$1
echo ' *' >>$1
echo ' * You should have received a copy of the GNU Affero General Public License' >>$1
echo ' * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.' >>$1
echo ' */' >>$1

echo '' >>$1

cat <_ | awk -f trim_copyright_notice_cc.awk >>"$1"
