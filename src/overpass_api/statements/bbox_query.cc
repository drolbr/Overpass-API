/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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
#include "../data/bbox_filter.h"
#include "../data/way_geometry_store.h"
#include "bbox_query.h"
#include "recurse.h"


//-----------------------------------------------------------------------------


class Bbox_Constraint : public Query_Constraint
{
  public:
    bool delivers_data(Resource_Manager& rman);

    Bbox_Constraint(Bbox_Query_Statement& bbox_) : bbox(&bbox_),
        filter_(Bbox_Double(bbox->get_south(), bbox->get_west(), bbox->get_north(), bbox->get_east())) {}
    bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Bbox_Constraint() {}

  private:
    Bbox_Query_Statement* bbox;
    Bbox_Filter filter_;
};


bool Bbox_Constraint::delivers_data(Resource_Manager& rman)
{
  const Bbox_Double& bbox_ = filter_.get_bbox();

  if (!bbox_.valid())
    return false;

  return (bbox_.north - bbox_.south) * (bbox_.east - bbox_.west) < 1.0;
}


bool Bbox_Constraint::get_ranges
    (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges = filter_.get_ranges_32();
  return true;
}


bool Bbox_Constraint::get_ranges
    (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  ranges = filter_.get_ranges_31();
  return true;
}


void Bbox_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  filter_.filter(into, timestamp);
}


void Bbox_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  filter_.filter(query, rman, into, timestamp);
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Bbox_Query_Statement > Bbox_Query_Statement::statement_maker("bbox-query");

Bbox_Query_Statement::Bbox_Query_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

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
    std::ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0) || (attributes["n"] == ""))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  if (north < south)
  {
    std::ostringstream temp;
    temp<<"The value of attribute \"n\" of the element \"bbox-query\""
    <<" must always be greater or equal than the value of attribute \"s\".";
    add_static_error(temp.str());
  }
  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0) || (attributes["w"] == ""))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0) || (attributes["e"] == ""))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
}


Bbox_Query_Statement::Bbox_Query_Statement(const Bbox_Double& bbox)
    : Output_Statement(0), south(bbox.south), north(bbox.north), west(bbox.west), east(bbox.east) {}


Bbox_Query_Statement::~Bbox_Query_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


const std::set< std::pair< Uint32_Index, Uint32_Index > >& Bbox_Query_Statement::get_ranges_32()
{
  if (ranges_32.empty())
    ::get_ranges_32(south, north, west, east).swap(ranges_32);
  return ranges_32;
}


const std::set< std::pair< Uint31_Index, Uint31_Index > >& Bbox_Query_Statement::get_ranges_31()
{
  if (ranges_31.empty())
    ranges_31 = calc_parents(get_ranges_32());
  return ranges_31;
}


void Bbox_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;

  Bbox_Constraint constraint(*this);
  std::set< std::pair< Uint32_Index, Uint32_Index > > ranges;
  constraint.get_ranges(rman, ranges);
  get_elements_by_id_from_db< Uint32_Index, Node_Skeleton >
      (into.nodes, into.attic_nodes,
       std::vector< Node::Id_Type >(), false, ranges, *this, rman,
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
