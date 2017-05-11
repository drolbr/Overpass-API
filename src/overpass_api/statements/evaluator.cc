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
}


void Requested_Context::bind(const std::string& set_name)
{
  add_usage(set_name, object_usage);
  object_usage = 0;
}

  
void Set_With_Context::prefetch(uint usage, const Set& set, Transaction& transaction)
{
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
}


Prepare_Task_Context::Prepare_Task_Context(const Requested_Context& requested, Resource_Manager& rman)
    : contexts(requested.set_usage.size()), relation_member_roles_(0)
{
  for (std::vector< Set_Usage >::const_iterator it = requested.set_usage.begin(); it != requested.set_usage.end(); ++it)
  {
    Set_With_Context& context = contexts[std::distance(requested.set_usage.begin(), it)];
    context.name = it->set_name;
    
    std::map< std::string, Set >::const_iterator mit(rman.sets().find(context.name));
    if (mit != rman.sets().end())
      context.prefetch(it->usage, mit->second, *rman.get_transaction());
  }
  
  if (requested.role_names_requested)
    relation_member_roles_ = &relation_member_roles(*rman.get_transaction());
}


const Set_With_Context* Prepare_Task_Context::get_set(const std::string& set_name) const
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
