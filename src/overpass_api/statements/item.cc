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


class Item_Constraint : public Query_Constraint
{
  public:
    Item_Constraint(Item_Statement& item_) : item(&item_) {}

    bool delivers_data(Resource_Manager& rman) { return true; }

    bool collect_nodes(Resource_Manager& rman, Set& into,
		 const std::vector< Uint64 >& ids, bool invert_ids);
    bool collect(Resource_Manager& rman, Set& into, int type,
		 const std::vector< Uint32_Index >& ids, bool invert_ids);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Item_Constraint() {}

  private:
    Item_Statement* item;
};


template< typename TIndex, typename TObject >
void collect_elements(const std::map< TIndex, std::vector< TObject > >& from,
		      std::map< TIndex, std::vector< TObject > >& into,
		      const std::vector< typename TObject::Id_Type >& ids, bool invert_ids)
{
  into.clear();
  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator iit = from.begin();
      iit != from.end(); ++iit)
  {
    if (ids.empty())
    {
      for (typename std::vector< TObject >::const_iterator cit = iit->second.begin();
          cit != iit->second.end(); ++cit)
	into[iit->first].push_back(*cit);
    }
    else if (!invert_ids)
    {
      for (typename std::vector< TObject >::const_iterator cit = iit->second.begin();
          cit != iit->second.end(); ++cit)
      {
        if (binary_search(ids.begin(), ids.end(), cit->id))
	  into[iit->first].push_back(*cit);
      }
    }
    else
    {
      for (typename std::vector< TObject >::const_iterator cit = iit->second.begin();
          cit != iit->second.end(); ++cit)
      {
        if (!binary_search(ids.begin(), ids.end(), cit->id))
	  into[iit->first].push_back(*cit);
      }
    }
  }
}


bool Item_Constraint::collect_nodes(Resource_Manager& rman, Set& into,
				    const std::vector< Uint64 >& ids, bool invert_ids)
{
  Set* input = rman.get_set(item->get_input_name());
  if (input)
  {
    collect_elements(input->nodes, into.nodes, ids, invert_ids);
    collect_elements(input->attic_nodes, into.attic_nodes, ids, invert_ids);
  }
  else
  {
    into.nodes.clear();
    into.attic_nodes.clear();
  }
  return true;
}


bool Item_Constraint::collect(Resource_Manager& rman, Set& into,
			      int type, const std::vector< Uint32_Index >& ids, bool invert_ids)
{
  Set* input = rman.get_set(item->get_input_name());
  if (input)
  {
    if (type == QUERY_WAY)
    {
      collect_elements(input->ways, into.ways, ids, invert_ids);
      collect_elements(input->attic_ways, into.attic_ways, ids, invert_ids);
    }
    else if (type == QUERY_RELATION)
    {
      collect_elements(input->relations, into.relations, ids, invert_ids);
      collect_elements(input->attic_relations, into.attic_relations, ids, invert_ids);
    }
    else if (type == QUERY_AREA)
      collect_elements(input->areas, into.areas, ids, invert_ids);
  }
  else
  {
    into.ways.clear();
    into.attic_ways.clear();
    into.relations.clear();
    into.attic_relations.clear();
    into.areas.clear();
    into.deriveds.clear();
  }
  return true;
}


void Item_Constraint::filter(Resource_Manager& rman, Set& into)
{
  Set* input = rman.get_set(item->get_input_name());
  if (input)
  {
    item_filter_map(into.nodes, input->nodes);
    item_filter_map(into.attic_nodes, input->attic_nodes);
    item_filter_map(into.ways, input->ways);
    item_filter_map(into.attic_ways, input->attic_ways);
    item_filter_map(into.relations, input->relations);
    item_filter_map(into.attic_relations, input->attic_relations);
    item_filter_map(into.areas, input->areas);
    item_filter_map(into.deriveds, input->deriveds);
  }
  else
  {
    into.nodes.clear();
    into.attic_nodes.clear();
    into.ways.clear();
    into.attic_ways.clear();
    into.relations.clear();
    into.attic_relations.clear();
    into.areas.clear();
    into.deriveds.clear();
  }
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Item_Statement > Item_Statement::statement_maker("item");

Item_Statement::Item_Statement(int line_number_, const std::map< std::string, std::string >& input_attributes,
                               Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["set"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  if (attributes["set"] == "_")
  {
    input = attributes["from"];
    set_output(attributes["into"]);
  }
  else if (attributes["from"] == "_" && attributes["into"] == "_")
  {
    input = attributes["set"];
    set_output(attributes["set"]);
  }
  else
    add_static_error("The attribute \"set\" of the element \"item\" can only be set "
      "if the attributes \"from\" and \"into\" are both empty.");
}


void Item_Statement::execute(Resource_Manager& rman)
{
  if (input != get_result_name())
  {
    Set into;
    
    Set* input_set = rman.get_set(input);
    if (input_set)
      into = *input_set;
    
    transfer_output(rman, into);
  }
}


Item_Statement::~Item_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


Query_Constraint* Item_Statement::get_query_constraint()
{
  constraints.push_back(new Item_Constraint(*this));
  return constraints.back();
}
