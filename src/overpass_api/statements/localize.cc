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


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "localize.h"
#include "set_prop.h"


Generic_Statement_Maker< Localize_Statement > Localize_Statement::statement_maker("localize");


Localize_Statement::Localize_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), type(data)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "l";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  input = attributes["from"];
  
  if (attributes["type"] == "ll")
    type = also_loose;
  else if (attributes["type"] == "llb")
    type = all;
  else if (attributes["type"] != "l")
    add_static_error("Localize must have one of the values \"l\", \"ll\", or \"llb\". \"l\" is the default value.");
}


class Local_Id_Dispenser
{
public:
  Local_Id_Dispenser(Resource_Manager& rman, const Statement& stmt, const Set& input_set);
  Derived_Skeleton::Id_Type local_id_by_node_id(Node_Skeleton::Id_Type id) const;
  
private:
  Set relevant_nwrs;
  std::map< Node_Skeleton::Id_Type, Derived_Skeleton::Id_Type > local_id_by_node_id_;
};


struct Node_Skeleton_By_Coord_Then_Id
{
  bool operator()(const Node_Skeleton* lhs, const Node_Skeleton* rhs)
  {
    if (lhs->ll_lower != rhs->ll_lower)
      return lhs->ll_lower < rhs->ll_lower;
    return lhs->id < rhs->id;
  }
};


Local_Id_Dispenser::Local_Id_Dispenser(Resource_Manager& rman, const Statement& stmt, const Set& input_set)
{
  add_nw_member_objects(rman, &stmt, input_set, relevant_nwrs);
  
  Set extra_ways;
  if (rman.get_desired_timestamp() == NOW)
    collect_ways(stmt, rman, relevant_nwrs.nodes, extra_ways.ways);
  else
  {
    collect_ways(stmt, rman, relevant_nwrs.nodes, relevant_nwrs.attic_nodes,
                 extra_ways.ways, extra_ways.attic_ways);
    indexed_set_union(relevant_nwrs.attic_ways, extra_ways.attic_ways);
  }
  indexed_set_union(relevant_nwrs.ways, extra_ways.ways);
  
  indexed_set_union(relevant_nwrs.nodes, input_set.nodes);
  indexed_set_union(relevant_nwrs.attic_nodes, input_set.attic_nodes);
  
  std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator it_now_idx
      = relevant_nwrs.nodes.begin();
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator it_attic_idx
      = relevant_nwrs.attic_nodes.begin();
  
  while (it_now_idx != relevant_nwrs.nodes.end() || it_attic_idx != relevant_nwrs.attic_nodes.end())
  {
    std::vector< const Node_Skeleton* > elem_refs;
    
    bool now_relevant = false;
    bool attic_relevant = false;
    if (it_attic_idx == relevant_nwrs.attic_nodes.end())
      now_relevant = true;
    else if (it_now_idx == relevant_nwrs.nodes.end())
      attic_relevant = true;
    else if (it_now_idx->first < it_attic_idx->first)
      now_relevant = true;
    else if (it_attic_idx->first < it_now_idx->first)
      attic_relevant = true;
    else
    {
      now_relevant = true;
      attic_relevant = true;
    }
    
    if (now_relevant)
    {
      for (std::vector< Node_Skeleton >::const_iterator it_elem = it_now_idx->second.begin();
          it_elem != it_now_idx->second.end(); ++it_elem)
        elem_refs.push_back(&*it_elem);
  
      ++it_now_idx;
    }
    
    if (attic_relevant)
    {
      for (std::vector< Attic< Node_Skeleton > >::const_iterator it_elem = it_attic_idx->second.begin();
          it_elem != it_attic_idx->second.end(); ++it_elem)
        elem_refs.push_back(&*it_elem);
  
      ++it_attic_idx;
    }
    
    std::sort(elem_refs.begin(), elem_refs.end(), Node_Skeleton_By_Coord_Then_Id());
    
    uint equal_since = 0;
    for (uint i = 1; i < elem_refs.size(); ++i)
    {
      if (elem_refs[equal_since]->ll_lower == elem_refs[i]->ll_lower)
      {
        if (i - equal_since == 1)
          local_id_by_node_id_[elem_refs[equal_since]->id] = 1;
        local_id_by_node_id_[elem_refs[i]->id] = i + 1 - equal_since;
      }
      else
        equal_since = i;
    }
  }
}


Derived_Skeleton::Id_Type Local_Id_Dispenser::local_id_by_node_id(Node_Skeleton::Id_Type id) const
{
  std::map< Node_Skeleton::Id_Type, Derived_Skeleton::Id_Type >::const_iterator it
      = local_id_by_node_id_.find(id);
  if (it != local_id_by_node_id_.end())
    return it->second;
  return Derived_Skeleton::Id_Type(0ull);
}


template< typename Index, typename Maybe_Attic >
void generate_vertices(const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Set& into, Resource_Manager& rman, const Local_Id_Dispenser& local_id_dispenser)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const Element_With_Context< Maybe_Attic >& data = context_from.get_context(it_idx->first, *it_elem);
      
      if (data.tags && !data.tags->empty())
      {
        Derived_Structure result("vertex", 0ull);
        result.id = rman.get_global_settings().dispense_derived_id();
        result.acquire_geometry(new Point_Geometry(
            ::lat(it_idx->first.val(), it_elem->ll_lower), ::lon(it_idx->first.val(), it_elem->ll_lower)));
      
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_tag = data.tags->begin();
            it_tag != data.tags->end(); ++it_tag)
          result.tags.push_back(std::make_pair(
              it_tag->first.empty() ? "" : it_tag->first[0] == '_' ? "_" + it_tag->first : it_tag->first,
              it_tag->second));
          
        Derived_Skeleton::Id_Type local_id = local_id_dispenser.local_id_by_node_id(it_elem->id);
        if (local_id.val())
          result.tags.push_back(std::make_pair("_local_id", to_string(local_id.val())));
      
        into.deriveds[Uint31_Index(0u)].push_back(result);
      }
    }
  }
}


void Localize_Statement::execute(Resource_Manager& rman)
{
  /*
  - aus Relationen Liste der betroffenen Ways ermitteln
  - zu Ways alle beteiligten Nodes ermitteln
  - zu Nodes alle adjazenten Ways bestimmen
  
  - wegen Ways und Relationen relevante Nodes bestimmen
  - wegen Tags relevante Nodes bestimmen  
  - Vertex aus Context
  
  - Links anhand der Zuordnung Node-Id nach Vertex aufbauen
  - Links aus Context (Tags und Geometry)
  
  - Trigraphen aus der Zuordnung Way-Id nach Link und Node-Id nach Vertex aufbauen
  - Trigraphen aus Context
  */
  
  Set into;

  const Set* input_set = rman.get_set(input);
  if (!input_set)
  {
    transfer_output(rman, into);
    return;
  }
  
  Local_Id_Dispenser local_id_dispenser(rman, *this, *input_set);
  
  Requested_Context requested_context;
  requested_context.add_usage(input, Set_Usage::TAGS);
  Prepare_Task_Context context(requested_context, *this, rman);
  Set_With_Context* context_from = context.get_set(input);
  
  if (context_from && context_from->base)
  {
    generate_vertices(context_from->base->nodes, *context_from, into, rman, local_id_dispenser);
    if (!context_from->base->attic_nodes.empty())
      generate_vertices(context_from->base->attic_nodes, *context_from, into, rman, local_id_dispenser);
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}
