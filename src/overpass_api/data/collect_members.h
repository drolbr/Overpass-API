/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H
#define DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H

#include "../core/datatypes.h"
#include "../statements/statement.h"

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
     const vector< Relation::Id_Type >* children_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

map< Uint31_Index, vector< Way_Skeleton > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint31_Index, Uint31_Index > >* way_ranges = 0,
     const vector< Way::Id_Type >* way_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

map< Uint32_Index, vector< Node_Skeleton > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const vector< Node::Id_Type >* node_ids = 0, bool invert_ids = false, const uint32* role_id = 0);
 
map< Uint32_Index, vector< Node_Skeleton > > way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Way_Skeleton > >& ways,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const vector< Node::Id_Type >* node_ids = 0, bool invert_ids = false);

     
vector< Node::Id_Type > relation_node_member_ids
    (Resource_Manager& rman, const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
     const uint32* role_id = 0);
vector< Way::Id_Type > relation_way_member_ids
    (Resource_Manager& rman, const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
     const uint32* role_id = 0);
vector< Relation::Id_Type > relation_relation_member_ids
    (Resource_Manager& rman, const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
     const uint32* role_id = 0);

vector< Node::Id_Type > way_nd_ids(const map< Uint31_Index, vector< Way_Skeleton > >& ways);

const map< uint32, string >& relation_member_roles(Transaction& transaction);
uint32 determine_role_id(Transaction& transaction, const string& role);


struct Quad_Coord
{
  Quad_Coord(uint32 ll_upper_, uint32 ll_lower_) : ll_upper(ll_upper_), ll_lower(ll_lower_) {}
  
  uint32 ll_upper;
  uint32 ll_lower;
};


void add_way_to_area_blocks(const vector< Quad_Coord >& coords,
                            uint32 id, map< Uint31_Index, vector< Area_Block > >& areas);


vector< Quad_Coord > make_geometry(const Way_Skeleton& way, const vector< Node >& nodes);

#endif
