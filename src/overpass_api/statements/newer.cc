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

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "meta_collector.h"
#include "newer.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

class Newer_Constraint : public Query_Constraint
{
  public:
    Newer_Constraint(Newer_Statement& newer) : timestamp(newer.get_timestamp()) {}

    bool delivers_data(Resource_Manager& rman) { return false; }
    
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Newer_Constraint() {}
    
  private:
    uint64 timestamp;
};


template< typename TIndex, typename TObject >
void newer_filter_map
    (map< TIndex, vector< TObject > >& modify,
     Resource_Manager& rman, uint64 timestamp, File_Properties* file_properties)
{
  if (modify.empty())
    return;
  Meta_Collector< TIndex, typename TObject::Id_Type > meta_collector
      (modify, rman, file_properties, false);
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      const OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >* meta_skel
	  = meta_collector.get(it->first, iit->id);
      if ((meta_skel) && (meta_skel->timestamp >= timestamp))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename TIndex, typename TObject >
void newer_filter_map_attic
    (map< TIndex, vector< TObject > >& modify,
     Resource_Manager& rman, uint64 timestamp,
     File_Properties* current_file_properties, File_Properties* attic_file_properties)
{
  if (modify.empty())
    return;
  
  Meta_Collector< TIndex, typename TObject::Id_Type > current_meta_collector
      (modify, rman, current_file_properties, false);
  Meta_Collector< TIndex, typename TObject::Id_Type > attic_meta_collector
      (modify, rman, attic_file_properties, false);
      
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      const OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >* meta_skel
	  = current_meta_collector.get(it->first, iit->id);
      if (!meta_skel || !(meta_skel->timestamp < iit->timestamp))
        meta_skel = attic_meta_collector.get(it->first, iit->id, iit->timestamp);
      if ((meta_skel) && (meta_skel->timestamp >= timestamp))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


void Newer_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  newer_filter_map(into.nodes, rman, this->timestamp, meta_settings().NODES_META);
  newer_filter_map(into.ways, rman, this->timestamp, meta_settings().WAYS_META);
  newer_filter_map(into.relations, rman, this->timestamp, meta_settings().RELATIONS_META);
  
  if (timestamp != NOW)
  {
    newer_filter_map_attic(into.attic_nodes, rman, this->timestamp,
			   meta_settings().NODES_META, attic_settings().NODES_META);
    newer_filter_map_attic(into.attic_ways, rman, this->timestamp,
			   meta_settings().WAYS_META, attic_settings().WAYS_META);
    newer_filter_map_attic(into.attic_relations, rman, this->timestamp,
			   meta_settings().RELATIONS_META, attic_settings().RELATIONS_META);
  }
  
  into.areas.clear();
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Newer_Statement > Newer_Statement::statement_maker("newer");

Newer_Statement::Newer_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Statement(line_number_), than_timestamp(0)
{
  map< string, string > attributes;
  
  attributes["than"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  string timestamp = attributes["than"];
  
  if (timestamp.size() >= 19)
    than_timestamp = Timestamp(
        atol(timestamp.c_str()), //year
        atoi(timestamp.c_str()+5), //month
        atoi(timestamp.c_str()+8), //day
        atoi(timestamp.c_str()+11), //hour
        atoi(timestamp.c_str()+14), //minute
        atoi(timestamp.c_str()+17) //second
        ).timestamp;
  
  if (than_timestamp == 0)
  {
    ostringstream temp;
    temp<<"The attribute than must contain a timestamp exactly in the form yyyy-mm-ddThh:mm:ssZ.";
    add_static_error(temp.str());
  }
}

Newer_Statement::~Newer_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


void Newer_Statement::execute(Resource_Manager& rman) {}

Query_Constraint* Newer_Statement::get_query_constraint()
{
  constraints.push_back(new Newer_Constraint(*this));
  return constraints.back();
}
