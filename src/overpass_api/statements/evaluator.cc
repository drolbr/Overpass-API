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
#include "evaluator.h"


const uint Set_Usage::SKELETON = 1;
const uint Set_Usage::TAGS = 2;
const uint Set_Usage::GEOMETRY = 4;
const uint Set_Usage::META = 8;


bool assert_element_in_context(Error_Output* error_output,
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context)
{
  if (tree_context != Statement::elem_eval_possible)
  {
    const std::string* func_name = tree_it.function_name();
    if (error_output)
      error_output->add_parse_error((func_name ? *func_name + "(...)" : "Void function")
          + " must be called in a context where it can evaluate an element",
          tree_it->line_col.first);
    return false;
  }
  
  return true;
}


Requested_Context& Requested_Context::add_usage(const std::string& set_name, uint usage)
{
  for (std::vector< Set_Usage >::iterator it = set_usage.begin(); it != set_usage.end(); ++it)
  {
    if (it->set_name == set_name)
    {
      it->usage |= usage;
      return *this;
    }
  }
  
  set_usage.push_back(Set_Usage(set_name, usage));
  return *this;
}


Requested_Context& Requested_Context::add_usage(uint usage_)
{
  object_usage |= usage_;
  return *this;
}


Requested_Context& Requested_Context::add_role_names()
{ 
  role_names_requested = true;
  return *this;
}


Requested_Context& Requested_Context::add_user_names()
{ 
  user_names_requested = true;
  return *this;
}


void Requested_Context::add(const Requested_Context& rhs)
{
  for (std::vector< Set_Usage >::const_iterator rit = rhs.set_usage.begin(); rit != rhs.set_usage.end(); ++rit)
  {
    for (std::vector< Set_Usage >::iterator it = set_usage.begin(); it != set_usage.end(); ++it)
    {
      if (it->set_name == rit->set_name)
      {
        it->usage |= rit->usage;
        continue;
      }
    }
    
    set_usage.push_back(*rit);
  }
  
  object_usage |= rhs.object_usage;
  role_names_requested |= rhs.role_names_requested;
  user_names_requested |= rhs.user_names_requested;
}


void Requested_Context::bind(const std::string& set_name)
{
  add_usage(set_name, object_usage);
  object_usage = 0;
}

  
void Set_With_Context::prefetch(uint usage, const Set& set, const Statement& stmt, Resource_Manager& rman)
{
  Transaction& transaction = *rman.get_transaction();
  base = &set;
  
  if (usage & Set_Usage::TAGS)
  {
    tag_store_nodes = new Tag_Store< Uint32_Index, Node_Skeleton >(transaction);
    tag_store_nodes->prefetch_all(set.nodes);
    
    if (!set.attic_nodes.empty())
    {
      tag_store_attic_nodes = new Tag_Store< Uint32_Index, Node_Skeleton >(transaction);
      tag_store_attic_nodes->prefetch_all(set.attic_nodes);
    }
    
    tag_store_ways = new Tag_Store< Uint31_Index, Way_Skeleton >(transaction);
    tag_store_ways->prefetch_all(set.ways);
    
    if (!set.attic_ways.empty())
    {
      tag_store_attic_ways = new Tag_Store< Uint31_Index, Way_Skeleton >(transaction);
      tag_store_attic_ways->prefetch_all(set.attic_ways);
    }
    
    tag_store_relations = new Tag_Store< Uint31_Index, Relation_Skeleton >(transaction);
    tag_store_relations->prefetch_all(set.relations);
    
    if (!set.attic_relations.empty())
    {
      tag_store_attic_relations = new Tag_Store< Uint31_Index, Relation_Skeleton >(transaction);
      tag_store_attic_relations->prefetch_all(set.attic_relations);
    }
    
    if (!base->areas.empty())
    {
      tag_store_areas = new Tag_Store< Uint31_Index, Area_Skeleton >(transaction);
      tag_store_areas->prefetch_all(set.areas);
    }
    
    tag_store_deriveds = new Tag_Store< Uint31_Index, Derived_Structure >(transaction);
    tag_store_deriveds->prefetch_all(set.deriveds);
  }
  
  if (usage & Set_Usage::GEOMETRY)
  {
    use_geometry = true;
    
    way_geometry_store = new Way_Geometry_Store(set.ways, stmt, rman);
    if (!set.attic_ways.empty())
      attic_way_geometry_store = new Way_Geometry_Store(set.attic_ways, stmt, rman);
    
    relation_geometry_store = new Relation_Geometry_Store(set.relations, stmt, rman);
    if (!set.attic_relations.empty())
      attic_relation_geometry_store = new Relation_Geometry_Store(set.attic_relations, stmt, rman);
  }
  
  if (usage & Set_Usage::META)
  {
    meta_collector_nodes = new Meta_Collector< Uint32_Index, Node_Skeleton::Id_Type >(
        set.nodes, transaction, current_meta_file_properties< Node_Skeleton >());
    
    if (!set.attic_nodes.empty())
    {
      meta_collector_attic_nodes = new Attic_Meta_Collector< Uint32_Index, Node_Skeleton >(
          set.attic_nodes, transaction, true);
    }
    
    meta_collector_ways = new Meta_Collector< Uint31_Index, Way_Skeleton::Id_Type >(
        set.ways, transaction, current_meta_file_properties< Way_Skeleton >());
    
    if (!set.attic_ways.empty())
    {
      meta_collector_attic_ways = new Attic_Meta_Collector< Uint31_Index, Way_Skeleton >(
          set.attic_ways, transaction, true);
    }
    
    meta_collector_relations = new Meta_Collector< Uint31_Index, Relation_Skeleton::Id_Type >(
        set.relations, transaction, current_meta_file_properties< Relation_Skeleton >());
    
    if (!set.attic_relations.empty())
    {
      meta_collector_attic_relations = new Attic_Meta_Collector< Uint31_Index, Relation_Skeleton >(
          set.attic_relations, transaction, true);
    }
  }
}


Element_With_Context< Node_Skeleton > Set_With_Context::get_context(
    const Uint32_Index& index, const Node_Skeleton& elem)
{
  if (use_geometry)
  {
    delete current_geometry;
    current_geometry = new Point_Geometry(::lat(index.val(), elem.ll_lower), ::lon(index.val(), elem.ll_lower));
  }
  const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta
      = (meta_collector_nodes ? meta_collector_nodes->get(index, elem.id) : 0);
  return Element_With_Context< Node_Skeleton >(&elem,
      tag_store_nodes ? tag_store_nodes->get(index, elem) : 0,
      current_geometry,
      meta, meta && parent ? parent->get_user_name(meta->user_id) : 0);
}


Element_With_Context< Attic< Node_Skeleton > > Set_With_Context::get_context(
    const Uint32_Index& index, const Attic< Node_Skeleton >& elem)
{
  if (use_geometry)
  {
    delete current_geometry;
    current_geometry = new Point_Geometry(::lat(index.val(), elem.ll_lower), ::lon(index.val(), elem.ll_lower));
  }
  const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta
      = (meta_collector_attic_nodes ? meta_collector_attic_nodes->get(index, elem.id, elem.timestamp) : 0);
  return Element_With_Context< Attic< Node_Skeleton > >(&elem,
      tag_store_attic_nodes ? tag_store_attic_nodes->get(index, elem) : 0,
      current_geometry,
      meta, meta && parent ? parent->get_user_name(meta->user_id) : 0);
}


Opaque_Geometry* new_opaque_geometry(const std::vector< Quad_Coord >& geometry)
{
  bool is_complete = true;
  for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    is_complete &= (it->ll_upper != 0 || it->ll_lower != 0);

  if (is_complete)
  {
    std::vector< Point_Double > coords;
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
      coords.push_back(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
    return new Linestring_Geometry(coords);
  }
  else
  {
    Partial_Way_Geometry* pw_geom = new Partial_Way_Geometry();
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    {
      if (it->ll_upper != 0 || it->ll_lower != 0)
        pw_geom->add_point(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
      else
        pw_geom->add_point(Point_Double(100., 200.));
    }
    return pw_geom;
  }
  
  return new Null_Geometry();
}


Opaque_Geometry* new_opaque_geometry(const std::vector< std::vector< Quad_Coord > >& geometry)
{
  bool is_complete = true;
  for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
      it != geometry.end(); ++it)
  {
    if (it->empty())
      is_complete = false;
    else if (it->size() == 1)
      is_complete &= ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0);
    else
    {
      for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        is_complete &= (it2->ll_upper != 0 || it2->ll_lower != 0);
    }
  }

  if (is_complete)
  {
    Compound_Geometry* cp_geom = new Compound_Geometry();
    
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->empty())
        cp_geom->add_component(new Null_Geometry());
      else if (it->size() == 1)
        cp_geom->add_component(new Point_Geometry(
            ::lat(it->front().ll_upper, it->front().ll_lower),
            ::lon(it->front().ll_upper, it->front().ll_lower)));
      else
      {
        std::vector< Point_Double > coords;
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          coords.push_back(Point_Double(
              ::lat(it2->ll_upper, it2->ll_lower),
              ::lon(it2->ll_upper, it2->ll_lower)));
          
        cp_geom->add_component(new Linestring_Geometry(coords));
      }
    }
    
    return cp_geom;
  }
  else if (!geometry.empty())
  {
    Partial_Relation_Geometry* pr_geom = new Partial_Relation_Geometry();

    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->empty())
        pr_geom->add_placeholder();
      else if (it->size() == 1 && ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0))
        pr_geom->add_point(Point_Double(
            ::lat(it->front().ll_upper, it->front().ll_lower),
            ::lon(it->front().ll_upper, it->front().ll_lower)));
      else
      {
        pr_geom->start_way();
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        {
          if (it2->ll_upper != 0 || it2->ll_lower != 0)
            pr_geom->add_way_point(Point_Double(
                ::lat(it2->ll_upper, it2->ll_lower),
                ::lon(it2->ll_upper, it2->ll_lower)));
          else
            pr_geom->add_way_placeholder();
        }
      }
    }
    
    return pr_geom;
  }
    
  return new Null_Geometry();
}


Element_With_Context< Way_Skeleton > Set_With_Context::get_context(
    const Uint31_Index& index, const Way_Skeleton& elem)
{
  if (use_geometry)
  {
    delete current_geometry;
    current_geometry = new_opaque_geometry(way_geometry_store->get_geometry(elem));
  }
  const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta
      = (meta_collector_ways ? meta_collector_ways->get(index, elem.id) : 0);
  return Element_With_Context< Way_Skeleton >(&elem,
      tag_store_ways ? tag_store_ways->get(index, elem) : 0,
      current_geometry,
      meta, meta && parent ? parent->get_user_name(meta->user_id) : 0);
}


Element_With_Context< Attic< Way_Skeleton > > Set_With_Context::get_context(
    const Uint31_Index& index, const Attic< Way_Skeleton >& elem)
{
  if (use_geometry)
  {
    delete current_geometry;
    current_geometry = new_opaque_geometry(attic_way_geometry_store->get_geometry(elem));
  }
  const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta
      = (meta_collector_attic_ways ? meta_collector_attic_ways->get(index, elem.id, elem.timestamp) : 0);
  return Element_With_Context< Attic< Way_Skeleton > >(&elem,
      tag_store_attic_ways ? tag_store_attic_ways->get(index, elem) : 0,
      current_geometry,
      meta, meta && parent ? parent->get_user_name(meta->user_id) : 0);
}


Element_With_Context< Relation_Skeleton > Set_With_Context::get_context(
    const Uint31_Index& index, const Relation_Skeleton& elem)
{
  if (use_geometry)
  {
    delete current_geometry;
    current_geometry = new_opaque_geometry(relation_geometry_store->get_geometry(elem));
  }
  const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta
      = (meta_collector_relations ? meta_collector_relations->get(index, elem.id) : 0);
  return Element_With_Context< Relation_Skeleton >(&elem,
      tag_store_relations ? tag_store_relations->get(index, elem) : 0,
      current_geometry,
      meta, meta && parent ? parent->get_user_name(meta->user_id) : 0);
}


Element_With_Context< Attic< Relation_Skeleton > > Set_With_Context::get_context(
    const Uint31_Index& index, const Attic< Relation_Skeleton >& elem)
{
  if (use_geometry)
  {
    delete current_geometry;
    current_geometry = new_opaque_geometry(attic_relation_geometry_store->get_geometry(elem));
  }
  const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta
      = (meta_collector_attic_relations ? meta_collector_attic_relations->get(index, elem.id, elem.timestamp) : 0);
  return Element_With_Context< Attic< Relation_Skeleton > >(&elem,
      tag_store_attic_relations ? tag_store_attic_relations->get(index, elem) : 0,
      current_geometry,
      meta, meta && parent ? parent->get_user_name(meta->user_id) : 0);
}


Element_With_Context< Area_Skeleton > Set_With_Context::get_context(
    const Uint31_Index& index, const Area_Skeleton& elem)
{
  return Element_With_Context< Area_Skeleton >(&elem,
      tag_store_areas ? tag_store_areas->get(index, elem) : 0, 0, 0, 0);
}


Element_With_Context< Derived_Skeleton > Set_With_Context::get_context(
    const Uint31_Index& index, const Derived_Structure& elem)
{
  return Element_With_Context< Derived_Skeleton >(&elem,
      tag_store_deriveds ? tag_store_deriveds->get(index, elem) : 0, 0, 0, 0);
}


Prepare_Task_Context::Prepare_Task_Context(
    const Requested_Context& requested, const Statement& stmt, Resource_Manager& rman)
    : contexts(requested.set_usage.size()), relation_member_roles_(0), users(0)
{
  for (std::vector< Set_Usage >::const_iterator it = requested.set_usage.begin(); it != requested.set_usage.end(); ++it)
  {
    Set_With_Context& context = contexts[std::distance(requested.set_usage.begin(), it)];
    context.name = it->set_name;
    context.parent = this;
    
    const Set* input = rman.get_set(context.name);
    if (input)
      context.prefetch(it->usage, *input, stmt, rman);
  }
  
  if (requested.role_names_requested)
    relation_member_roles_ = &relation_member_roles(*rman.get_transaction());
  
  if (requested.user_names_requested)
    users = &rman.users();
}


Set_With_Context* Prepare_Task_Context::get_set(const std::string& set_name)
{
  for (uint i = 0; i < contexts.size(); ++i)
  {
    if (contexts[i].name == set_name)
      return &contexts[i];
  }
  return 0;
}


uint32 Prepare_Task_Context::get_role_id(const std::string& role) const
{
  if (!relation_member_roles_)
    return std::numeric_limits< uint32 >::max();
    
  for (std::map< uint32, std::string >::const_iterator it = relation_member_roles_->begin();
      it != relation_member_roles_->end(); ++it)
  {
    if (it->second == role)
      return it->first;
  }
  return std::numeric_limits< uint32 >::max();
}


const std::string* Prepare_Task_Context::get_user_name(uint32 user_id) const
{
  if (!users)
    return 0;
  std::map< uint32, std::string >::const_iterator it = users->find(user_id);
  if (it == users->end())
    return 0;
  return &it->second;
}
