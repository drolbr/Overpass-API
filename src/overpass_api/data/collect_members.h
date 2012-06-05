/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H
#define DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H

#include "../core/datatypes.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

class Resource_Manager;
class Statement;


map< Uint31_Index, vector< Relation_Skeleton > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& parents,
     const set< pair< Uint31_Index, Uint31_Index > >* children_ranges = 0,
     const vector< uint32 >* children_ids = 0, bool invert_ids = false);

map< Uint31_Index, vector< Way_Skeleton > > relation_way_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint31_Index, Uint31_Index > >* way_ranges = 0,
     const vector< uint32 >* way_ids = 0, bool invert_ids = false);

map< Uint32_Index, vector< Node_Skeleton > > relation_node_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const vector< uint32 >* node_ids = 0, bool invert_ids = false);
 
map< Uint32_Index, vector< Node_Skeleton > > way_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Way_Skeleton > >& ways,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const vector< uint32 >* node_ids = 0, bool invert_ids = false);

     
vector< uint32 > relation_member_ids
    (const Statement& stmt, Resource_Manager& rman, uint32 type,
     const map< Uint31_Index, vector< Relation_Skeleton > >& rels);

vector< uint32 > way_nd_ids
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Way_Skeleton > >& ways);

#endif
