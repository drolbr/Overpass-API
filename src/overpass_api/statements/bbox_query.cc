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

#include <algorithm>
#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/collect_members.h"
#include "../data/geometry.h"
#include "bbox_query.h"
#include "recurse.h"

using namespace std;

//-----------------------------------------------------------------------------

    
class Bbox_Constraint : public Query_Constraint
{
  public:
    bool delivers_data(Resource_Manager& rman);
    
    Bbox_Constraint(Bbox_Query_Statement& bbox_) : bbox(&bbox_) {}
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Bbox_Constraint() {}
    
  private:
    Bbox_Query_Statement* bbox;
};


bool Bbox_Constraint::delivers_data(Resource_Manager& rman)
{
  if (!bbox)
    return false;
  
  return ((bbox->get_north() - bbox->get_south())*(bbox->get_east() - bbox->get_west())
      < 1.0);
}


bool Bbox_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges = bbox->get_ranges_32();
  return true;
}


bool Bbox_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  ranges = bbox->get_ranges_31();
  return true;
}


void Bbox_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  // process nodes
  uint32 south_ = ilat_(bbox->get_south());
  uint32 north_ = ilat_(bbox->get_north());
  int32 west_ = ilon_(bbox->get_west());
  int32 east_ = ilon_(bbox->get_east());
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = into.nodes.begin();
      it != into.nodes.end(); ++it)
  {
    vector< Node_Skeleton > local_into;
    for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      uint32 lat(::ilat(it->first.val(), iit->ll_lower));
      int32 lon(::ilon(it->first.val(), iit->ll_lower));
      if ((lat >= south_) && (lat <= north_) &&
          (((lon >= west_) && (lon <= east_))
            || ((east_ < west_) && ((lon >= west_) || (lon <= east_)))))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }  
  
  for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::iterator it = into.attic_nodes.begin();
      it != into.attic_nodes.end(); ++it)
  {
    vector< Attic< Node_Skeleton > > local_into;
    for (vector< Attic< Node_Skeleton > >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      uint32 lat(::ilat(it->first.val(), iit->ll_lower));
      int32 lon(::ilon(it->first.val(), iit->ll_lower));
      if ((lat >= south_) && (lat <= north_) &&
          (((lon >= west_) && (lon <= east_))
            || ((east_ < west_) && ((lon >= west_) || (lon <= east_)))))
        local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }

  
  const set< pair< Uint31_Index, Uint31_Index > >& ranges = bbox->get_ranges_31();
  
  // pre-process ways to reduce the load of the expensive filter
  filter_ways_by_ranges(into.ways, ranges);
  filter_ways_by_ranges(into.attic_ways, ranges);
  
  // pre-filter relations
  filter_relations_by_ranges(into.relations, ranges);
  filter_relations_by_ranges(into.attic_relations, ranges);
  
  //TODO: filter areas
}


bool matches_bbox(const Bbox_Query_Statement& bbox, const vector< Quad_Coord >& way_geometry)
{
  vector< Quad_Coord >::const_iterator nit = way_geometry.begin();
  if (nit == way_geometry.end())
    return false;

  // ways with single node only
  if (way_geometry.size() == 1)
  {
    double lat(::lat(nit->ll_upper, nit->ll_lower));
    double lon(::lon(nit->ll_upper, nit->ll_lower));
    if ((lat >= bbox.get_south()) && (lat <= bbox.get_north()) &&
        (((lon >= bbox.get_west()) && (lon <= bbox.get_east()))
          || ((bbox.get_east() < bbox.get_west()) &&
              ((lon >= bbox.get_west()) || (lon <= bbox.get_east())))))
      return true;

    return false;
  }

  double first_lat(::lat(nit->ll_upper, nit->ll_lower));
  double first_lon(::lon(nit->ll_upper, nit->ll_lower));
  for (++nit; nit != way_geometry.end(); ++nit)
  {
    double second_lat(::lat(nit->ll_upper, nit->ll_lower));
    double second_lon(::lon(nit->ll_upper, nit->ll_lower));
    
    if (segment_intersects_bbox
        (first_lat, first_lon, second_lat, second_lon,
         bbox.get_south(), bbox.get_north(), bbox.get_west(), bbox.get_east()))
      return true;
    
    first_lat = second_lat;
    first_lon = second_lon;
  }
  return false;
}


template< typename Way_Skeleton >
void filter_ways_expensive(const Bbox_Query_Statement& bbox,
                           const Way_Geometry_Store& way_geometries,
                           map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  for (typename map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin();
      it != ways.end(); ++it)
  {
    vector< Way_Skeleton > local_into;
    for (typename vector< Way_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (matches_bbox(bbox, way_geometries.get_geometry(*iit)))
        local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename Relation_Skeleton >
void filter_relations_expensive(const Bbox_Query_Statement& bbox,
                           const vector< pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id,
                           const vector< pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id,
                           const Way_Geometry_Store& way_geometries,
                           map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  for (typename map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = relations.begin();
      it != relations.end(); ++it)
  {
    vector< Relation_Skeleton > local_into;
    for (typename vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      for (vector< Relation_Entry >::const_iterator nit = iit->members.begin();
          nit != iit->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE)
        {
          const pair< Uint32_Index, const Node_Skeleton* >* second_nd =
              binary_search_for_pair_id(node_members_by_id, nit->ref);
          if (!second_nd)
            continue;
          double lat(::lat(second_nd->first.val(), second_nd->second->ll_lower));
          double lon(::lon(second_nd->first.val(), second_nd->second->ll_lower));
          
          if (bbox.matches_bbox(lat, lon))
          {
            local_into.push_back(*iit);
            break;
          }
        }
        else if (nit->type == Relation_Entry::WAY)
        {
          const pair< Uint31_Index, const Way_Skeleton* >* second_nd =
              binary_search_for_pair_id(way_members_by_id, nit->ref32());
          if (!second_nd)
            continue;
          if (matches_bbox(bbox, way_geometries.get_geometry(*second_nd->second)))
          {
            local_into.push_back(*iit);
            break;
          }
        }
      }
    }
    it->second.swap(local_into);
  }
}


void Bbox_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  //Process ways
  filter_ways_expensive(*bbox, Way_Geometry_Store(into.ways, query, rman), into.ways);
  
  {
    //Process relations
    
    // Retrieve all nodes referred by the relations.
    const set< pair< Uint32_Index, Uint32_Index > >& node_ranges = bbox->get_ranges_32();
    
    map< Uint32_Index, vector< Node_Skeleton > > node_members
        = relation_node_members(&query, rman, into.relations, &node_ranges);
    vector< pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id
        = order_by_id(node_members, Order_By_Node_Id());
    
    // Retrieve all ways referred by the relations.
    const set< pair< Uint31_Index, Uint31_Index > >& way_ranges = bbox->get_ranges_31();
    
    map< Uint31_Index, vector< Way_Skeleton > > way_members_
        = relation_way_members(&query, rman, into.relations, &way_ranges);
    vector< pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id
        = order_by_id(way_members_, Order_By_Way_Id());
    
    filter_relations_expensive(*bbox, node_members_by_id, way_members_by_id,
        Way_Geometry_Store(way_members_, query, rman), into.relations);
  }  
  
  if (timestamp != NOW)
  {
    //Process attic ways
    filter_ways_expensive(*bbox, Way_Geometry_Store(into.attic_ways, timestamp, query, rman), into.attic_ways);
    
    //Process attic relations
    
    // Retrieve all nodes referred by the relations.
    const set< pair< Uint32_Index, Uint32_Index > >& node_ranges = bbox->get_ranges_32();
    
    map< Uint32_Index, vector< Attic< Node_Skeleton > > > node_members
        = relation_node_members(&query, rman, into.attic_relations, timestamp, &node_ranges);
    vector< pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id
        = order_attic_by_id(node_members, Order_By_Node_Id());
    
    // Retrieve all ways referred by the relations.
    const set< pair< Uint31_Index, Uint31_Index > >& way_ranges = bbox->get_ranges_31();
    
    map< Uint31_Index, vector< Attic< Way_Skeleton > > > way_members_
        = relation_way_members(&query, rman, into.attic_relations, timestamp, &way_ranges);
    vector< pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id
        = order_attic_by_id(way_members_, Order_By_Way_Id());
    
    filter_relations_expensive(*bbox, node_members_by_id, way_members_by_id,
        Way_Geometry_Store(way_members_, timestamp, query, rman), into.attic_relations);
  }  
  
  //TODO: filter areas
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Bbox_Query_Statement > Bbox_Query_Statement::statement_maker("bbox-query");

Bbox_Query_Statement::Bbox_Query_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["s"] = "";
  attributes["n"] = "";
  attributes["w"] = "";
  attributes["e"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  south = atof(attributes["s"].c_str());
  if ((south < -90.0) || (south > 90.0) || (attributes["s"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0) || (attributes["n"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  if (north < south)
  {
    ostringstream temp;
    temp<<"The value of attribute \"n\" of the element \"bbox-query\""
    <<" must always be greater or equal than the value of attribute \"s\".";
    add_static_error(temp.str());
  }
  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0) || (attributes["w"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0) || (attributes["e"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
}


Bbox_Query_Statement::~Bbox_Query_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


const set< pair< Uint32_Index, Uint32_Index > >& Bbox_Query_Statement::get_ranges_32()
{
  if (ranges_32.empty())
    ::get_ranges_32(south, north, west, east).swap(ranges_32);
  return ranges_32;
}


const set< pair< Uint31_Index, Uint31_Index > >& Bbox_Query_Statement::get_ranges_31()
{
  if (ranges_31.empty())
    ranges_31 = calc_parents(get_ranges_32());
  return ranges_31;
}


void Bbox_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  Bbox_Constraint constraint(*this);
  set< pair< Uint32_Index, Uint32_Index > > ranges;
  constraint.get_ranges(rman, ranges);
  get_elements_by_id_from_db< Uint32_Index, Node_Skeleton >
      (into.nodes, into.attic_nodes,
       vector< Node::Id_Type >(), false, rman.get_desired_timestamp(), ranges, *this, rman,
       *osm_base_settings().NODES, *attic_settings().NODES);  
  constraint.filter(rman, into, rman.get_desired_timestamp());
  filter_attic_elements(rman, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);

  transfer_output(rman, into);
  rman.health_check(*this);
}


Query_Constraint* Bbox_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Bbox_Constraint(*this));
  return constraints.back();
}
