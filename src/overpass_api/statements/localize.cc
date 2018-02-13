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


class NWR_Context
{
public:
  struct Node_Context
  {
    Node_Context() : local_id(0ull), count(0) {}
    Derived_Skeleton::Id_Type local_id;
    uint count;
    double lat;
    double lon;
    std::map< Node_Skeleton::Id_Type, std::vector< Way_Skeleton::Id_Type > > link_by_origin;
  };
  
  struct Way_Section_Context
  {
    Way_Section_Context(uint pos_) : pos(pos_), local_id(0ull) {}
    uint pos;
    Derived_Skeleton::Id_Type local_id;
    std::vector< Point_Double > points;
  };
  
  NWR_Context(Resource_Manager& rman, const Statement& stmt, const Set& input_set);
  void prepare_links(Resource_Manager& rman, const Statement& stmt, const Set& input_set);
  Derived_Skeleton::Id_Type local_id_by_node_id(Node_Skeleton::Id_Type id) const;
  Node_Context& context_by_node_id(Node_Skeleton::Id_Type id) { return context_by_node_id_[id]; }
  std::vector< Way_Section_Context >& context_by_way_id(Way_Skeleton::Id_Type id) { return context_by_way_id_[id]; }
  
private:
  Set relevant_nwrs;
  std::map< Node_Skeleton::Id_Type, Node_Context > context_by_node_id_;
  std::map< Way_Skeleton::Id_Type, std::vector< Way_Section_Context > > context_by_way_id_;
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


template< typename Index, typename Object, typename Task >
void for_each_elem(const std::map< Index, std::vector< Object > >& container, Task task)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it_idx = container.begin(); it_idx != container.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
      task(it_idx->first, *it_elem);
  }
}


struct Set_Coord_In_Context
{
  void operator()(Uint32_Index idx, const Node_Skeleton& node)
  {
    NWR_Context::Node_Context& context = (*context_by_node_id)[node.id];
    context.lat = ::lat(idx.val(), node.ll_lower);
    context.lon = ::lon(idx.val(), node.ll_lower);
  }
  
  Set_Coord_In_Context(std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_)
    : context_by_node_id(&context_by_node_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
};


struct Count_Node_Use
{
  void operator()(Uint31_Index idx, const Way_Skeleton& way)
  {
    if (!way.nds.empty())
    {
      (*context_by_node_id)[way.nds.front()].count += 2;
      for (uint i = 1; i < way.nds.size()-1; ++i)
        ++(*context_by_node_id)[way.nds[i]].count;
      (*context_by_node_id)[way.nds.back()].count += 2;
    }
  }
  
  Count_Node_Use(std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_)
    : context_by_node_id(&context_by_node_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
};


struct Partition_Into_Links
{
  void operator()(Uint31_Index idx, const Way_Skeleton& way)
  {
    if (!way.nds.empty())
    {
      std::vector< NWR_Context::Way_Section_Context >& way_context = (*context_by_way_id)[way.id];
      
      uint start = 0;
      const NWR_Context::Node_Context& start_context = (*context_by_node_id)[way.nds[start]];
      way_context.push_back(NWR_Context::Way_Section_Context(start));
      way_context.back().points.push_back(Point_Double(start_context.lat, start_context.lon));
      
      for (uint i = 1; i < way.nds.size()-1; ++i)
      {
        NWR_Context::Node_Context& context = (*context_by_node_id)[way.nds[i]];
        way_context.back().points.push_back(Point_Double(context.lat, context.lon));
        
        if (context.local_id.val() || context.count > 1)
        {
          context.link_by_origin[way.nds[start]].push_back(way.id);
          start = i;
          way_context.push_back(NWR_Context::Way_Section_Context(start));
          way_context.back().points.push_back(Point_Double(context.lat, context.lon));
        }
      }
      
      NWR_Context::Node_Context& context = (*context_by_node_id)[way.nds.back()];
      way_context.back().points.push_back(Point_Double(context.lat, context.lon));
      context.link_by_origin[way.nds[start]].push_back(way.id);
    }
  }
  
  Partition_Into_Links(
      std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_,
      std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >& context_by_way_id_)
    : context_by_node_id(&context_by_node_id_), context_by_way_id(&context_by_way_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
  std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >* context_by_way_id;
};


struct Figure_Out_Local_Ids_Of_Links
{
  void operator()(Uint31_Index idx, const Way_Skeleton& way)
  {
    if (!way.nds.empty())
    {
      std::vector< NWR_Context::Way_Section_Context >& way_context = (*context_by_way_id)[way.id];
      
      for (uint i = 0; i < way_context.size(); ++i)
      {
        NWR_Context::Node_Context& context = (*context_by_node_id)[
            i+1 < way_context.size() ? way.nds[way_context[i+1].pos] : way.nds.back()];
        const std::vector< Way_Skeleton::Id_Type >& parallel_links
            = context.link_by_origin.find(way.nds[way_context[i].pos])->second;
        if (parallel_links.size() > 1)
        {
          uint parallel_links_pos = 0;
          while (parallel_links_pos < parallel_links.size()
              && !(parallel_links[parallel_links_pos] == way.id))
            ++parallel_links_pos;
          way_context[i].local_id = parallel_links_pos + 1;
        }
      }
    }
  }
  
  Figure_Out_Local_Ids_Of_Links(
      std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_,
      std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >& context_by_way_id_)
    : context_by_node_id(&context_by_node_id_), context_by_way_id(&context_by_way_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
  std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >* context_by_way_id;
};


NWR_Context::NWR_Context(Resource_Manager& rman, const Statement& stmt, const Set& input_set)
{
  // Collect all objects required for context
  
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
  
  // Build local ids for node segments
  
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
          context_by_node_id_[elem_refs[equal_since]->id].local_id = 1;
        context_by_node_id_[elem_refs[i]->id].local_id = i + 1 - equal_since;
      }
      else
        equal_since = i;
    }
  }
  
  // Store node coordinates for use by links
  for_each_elem(relevant_nwrs.nodes, Set_Coord_In_Context(context_by_node_id_));
  for_each_elem(relevant_nwrs.attic_nodes, Set_Coord_In_Context(context_by_node_id_));

  // Count node use by ways
  for_each_elem(relevant_nwrs.ways, Count_Node_Use(context_by_node_id_));
  for_each_elem(relevant_nwrs.attic_ways, Count_Node_Use(context_by_node_id_));
}


void NWR_Context::prepare_links(Resource_Manager& rman, const Statement& stmt, const Set& input_set)
{
  // Figure out links
  for_each_elem(relevant_nwrs.ways, Partition_Into_Links(context_by_node_id_, context_by_way_id_));
  for_each_elem(relevant_nwrs.attic_ways, Partition_Into_Links(context_by_node_id_, context_by_way_id_));
  
  for (std::map< Node_Skeleton::Id_Type, Node_Context >::iterator it_ctx = context_by_node_id_.begin();
      it_ctx != context_by_node_id_.end(); ++it_ctx)
  {
    for (std::map< Node_Skeleton::Id_Type, std::vector< Way_Skeleton::Id_Type > >::iterator
        it_orig = it_ctx->second.link_by_origin.begin(); it_orig != it_ctx->second.link_by_origin.end(); ++it_orig)
      std::sort(it_orig->second.begin(), it_orig->second.end());
  }
  
  // Figure out local ids of links
  for_each_elem(relevant_nwrs.ways, Figure_Out_Local_Ids_Of_Links(context_by_node_id_, context_by_way_id_));
  for_each_elem(relevant_nwrs.attic_ways, Figure_Out_Local_Ids_Of_Links(context_by_node_id_, context_by_way_id_));
}


Derived_Skeleton::Id_Type NWR_Context::local_id_by_node_id(Node_Skeleton::Id_Type id) const
{
  std::map< Node_Skeleton::Id_Type, Node_Context >::const_iterator it
      = context_by_node_id_.find(id);
  if (it != context_by_node_id_.end())
    return it->second.local_id;
  return Derived_Skeleton::Id_Type(0ull);
}


template< typename Index, typename Maybe_Attic >
void process_vertices(const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Set& into, Resource_Manager& rman, NWR_Context& nwr_context)
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
        NWR_Context::Node_Context& node_context = nwr_context.context_by_node_id(it_elem->id);
        ++node_context.count;
        
        Derived_Structure result("vertex", 0ull);
        result.id = rman.get_global_settings().dispense_derived_id();
        result.acquire_geometry(new Point_Geometry(
            ::lat(it_idx->first.val(), it_elem->ll_lower), ::lon(it_idx->first.val(), it_elem->ll_lower)));
      
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_tag = data.tags->begin();
            it_tag != data.tags->end(); ++it_tag)
          result.tags.push_back(std::make_pair(
              it_tag->first.empty() ? "" : it_tag->first[0] == '_' ? "_" + it_tag->first : it_tag->first,
              it_tag->second));
          
        Derived_Skeleton::Id_Type local_id = nwr_context.local_id_by_node_id(it_elem->id);
        if (local_id.val())
          result.tags.push_back(std::make_pair("_local_id", to_string(local_id.val())));
      
        into.deriveds[Uint31_Index(0u)].push_back(result);
      }
    }
  }
}


template< typename Index, typename Maybe_Attic >
void generate_links(const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Set& into, Resource_Manager& rman, NWR_Context& nwr_context)
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
        uint start = 0;
        const NWR_Context::Node_Context* start_context = &nwr_context.context_by_node_id(it_elem->nds[start]);
        std::vector< NWR_Context::Way_Section_Context >& way_context = nwr_context.context_by_way_id(it_elem->id);
        std::vector< NWR_Context::Way_Section_Context >::const_iterator it_way_ctx = way_context.begin();
        
        for (uint i = 1; i < it_elem->nds.size(); ++i)
        {
          const NWR_Context::Node_Context& context = nwr_context.context_by_node_id(it_elem->nds[i]);
          if (context.local_id.val() || context.count > 1)
          {
            Derived_Structure result("link", 0ull);
            result.id = rman.get_global_settings().dispense_derived_id();
            result.acquire_geometry(new Linestring_Geometry(it_way_ctx->points));
      
            for (std::vector< std::pair< std::string, std::string > >::const_iterator it_tag = data.tags->begin();
                it_tag != data.tags->end(); ++it_tag)
              result.tags.push_back(std::make_pair(
                  it_tag->first.empty() ? "" : it_tag->first[0] == '_' ? "_" + it_tag->first : it_tag->first,
                  it_tag->second));
          
            if (start_context->local_id.val())
              result.tags.push_back(std::make_pair("_local_from", to_string(start_context->local_id.val())));
            if (context.local_id.val())
              result.tags.push_back(std::make_pair("_local_to", to_string(context.local_id.val())));
            
            if (it_way_ctx->local_id.val())
              result.tags.push_back(std::make_pair("_local_id", to_string(it_way_ctx->local_id.val())));
            
            into.deriveds[Uint31_Index(0u)].push_back(result);
            
            start_context = &context;
            start = i;
            ++it_way_ctx;
          }
        }
      }
    }
  }
}


void Localize_Statement::execute(Resource_Manager& rman)
{
  /*
  - Way-Struktur in (Pos,local-id),... ablegen
  - Trigraphen: (Node-Id)~>(Point,ggf.local_id), (Pos,local-id)~>(Lstr,local-id)
  */
  
  Set into;

  const Set* input_set = rman.get_set(input);
  if (!input_set)
  {
    transfer_output(rman, into);
    return;
  }
  
  Requested_Context requested_context;
  requested_context.add_usage(input, Set_Usage::TAGS);
  Prepare_Task_Context context(requested_context, *this, rman);
  Set_With_Context* context_from = context.get_set(input);
  
  if (context_from && context_from->base)
  {
    NWR_Context nwr_context(rman, *this, *input_set);
    process_vertices(context_from->base->nodes, *context_from, into, rman, nwr_context);
    if (!context_from->base->attic_nodes.empty())
      process_vertices(context_from->base->attic_nodes, *context_from, into, rman, nwr_context);
    
    nwr_context.prepare_links(rman, *this, *input_set);
    generate_links(context_from->base->ways, *context_from, into, rman, nwr_context);
    if (!context_from->base->attic_ways.empty())
      generate_links(context_from->base->attic_ways, *context_from, into, rman, nwr_context);
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}
