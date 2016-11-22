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


void Set_With_Context::prefetch(const Set_Usage& usage, const Set& set, Transaction& transaction)
{
  base = &set;
  
  if (usage.usage & Set_Usage::TAGS)
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


std::pair< std::vector< Set_Usage >, uint > union_usage(const std::pair< std::vector< Set_Usage >, uint >& lhs,
    const std::pair< std::vector< Set_Usage >, uint >& rhs)
{
  std::vector< Set_Usage > result(lhs.first.size() + rhs.first.size(), Set_Usage("", 0u));
    
  std::vector< Set_Usage >::const_iterator it_lhs = lhs.first.begin();
  std::vector< Set_Usage >::const_iterator it_rhs = rhs.first.begin();
  std::vector< Set_Usage >::iterator it_res = result.begin();
    
  while (it_lhs != lhs.first.end() && it_rhs != rhs.first.end())
  {
    if (it_lhs->set_name < it_rhs->set_name)
    {
      *it_res = *it_lhs;
      ++it_res;
      ++it_lhs;        
    }
    else if (it_rhs->set_name < it_lhs->set_name)
    {
      *it_res = *it_rhs;
      ++it_res;
      ++it_rhs;
    }
    else
    {
      *it_res = *it_lhs;
      it_res->usage |= it_rhs->usage;
      ++it_res;
      ++it_lhs;
      ++it_rhs;
    }
  }
  
  it_res = std::copy(it_lhs, lhs.first.end(), it_res);
  it_res = std::copy(it_rhs, rhs.first.end(), it_res);  
  result.erase(it_res, result.end());
  
  return std::make_pair(result, lhs.second | rhs.second);
}
