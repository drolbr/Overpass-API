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

#include "../data/collect_members.h"
#include "item.h"

using namespace std;

class Item_Constraint : public Query_Constraint
{
  public:
    Item_Constraint(Item_Statement& item_) : item(&item_) {}

    bool delivers_data(Resource_Manager& rman) { return true; }
    
    bool collect_nodes(Resource_Manager& rman, Set& into,
		 const vector< Uint64 >& ids, bool invert_ids);
    bool collect(Resource_Manager& rman, Set& into, int type,
		 const vector< Uint32_Index >& ids, bool invert_ids);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Item_Constraint() {}
    
  private:
    Item_Statement* item;
};

template< typename TIndex, typename TObject >
void collect_elements(const map< TIndex, vector< TObject > >& from,
		      map< TIndex, vector< TObject > >& into,
		      const vector< typename TObject::Id_Type >& ids, bool invert_ids)
{
  into.clear();
  for (typename map< TIndex, vector< TObject > >::const_iterator iit = from.begin();
      iit != from.end(); ++iit)
  {
    if (ids.empty())
    {
      for (typename vector< TObject >::const_iterator cit = iit->second.begin();
          cit != iit->second.end(); ++cit)
	into[iit->first].push_back(*cit);
    }
    else if (!invert_ids)
    {
      for (typename vector< TObject >::const_iterator cit = iit->second.begin();
          cit != iit->second.end(); ++cit)
      {
        if (binary_search(ids.begin(), ids.end(), cit->id))
	  into[iit->first].push_back(*cit);
      }
    }
    else
    {
      for (typename vector< TObject >::const_iterator cit = iit->second.begin();
          cit != iit->second.end(); ++cit)
      {
        if (!binary_search(ids.begin(), ids.end(), cit->id))
	  into[iit->first].push_back(*cit);
      }
    }
  }
}


bool Item_Constraint::collect_nodes(Resource_Manager& rman, Set& into,
				    const vector< Uint64 >& ids, bool invert_ids)
{
  collect_elements(rman.sets()[item->get_result_name()].nodes, into.nodes,
		   ids, invert_ids);
  collect_elements(rman.sets()[item->get_result_name()].attic_nodes, into.attic_nodes,
                   ids, invert_ids);
  return true;
}


bool Item_Constraint::collect(Resource_Manager& rman, Set& into,
			      int type, const vector< Uint32_Index >& ids, bool invert_ids)
{
  if (type == QUERY_WAY)
  {
    collect_elements(rman.sets()[item->get_result_name()].ways, into.ways,
                     ids, invert_ids);
    collect_elements(rman.sets()[item->get_result_name()].attic_ways, into.attic_ways,
		     ids, invert_ids);
  }
  if (type == QUERY_RELATION)
  {
    collect_elements(rman.sets()[item->get_result_name()].relations, into.relations,
		     ids, invert_ids);
    collect_elements(rman.sets()[item->get_result_name()].attic_relations, into.attic_relations,
                     ids, invert_ids);
  }
  if (type == QUERY_AREA)
    collect_elements(rman.sets()[item->get_result_name()].areas, into.areas,
		     ids, invert_ids);
  return true;
}


void Item_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  item_filter_map(into.nodes, rman.sets()[item->get_result_name()].nodes);
  item_filter_map(into.attic_nodes, rman.sets()[item->get_result_name()].attic_nodes);
  item_filter_map(into.ways, rman.sets()[item->get_result_name()].ways);
  item_filter_map(into.attic_ways, rman.sets()[item->get_result_name()].attic_ways);
  item_filter_map(into.relations, rman.sets()[item->get_result_name()].relations);
  item_filter_map(into.attic_relations, rman.sets()[item->get_result_name()].attic_relations);
  item_filter_map(into.areas, rman.sets()[item->get_result_name()].areas);
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Item_Statement > Item_Statement::statement_maker("item");

Item_Statement::Item_Statement(int line_number_, const map< string, string >& input_attributes,
                               Parsed_Query& global_settings)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["set"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["set"];
}

Item_Statement::~Item_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


Query_Constraint* Item_Statement::get_query_constraint()
{
  constraints.push_back(new Item_Constraint(*this));
  return constraints.back();
}
