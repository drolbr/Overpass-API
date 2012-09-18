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

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_NODE_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_NODE_H

#include "basic_types.h"
#include "index_computations.h"

#include <string>
#include <vector>


struct Node
{
  typedef uint32 Id_Type;
  
  Id_Type id;
  uint32 ll_upper;
  uint32 ll_lower_;
  std::vector< pair< std::string, std::string > > tags;
  
  Node() {}
  
  Node(uint32 id_, double lat, double lon)
      : id(id_), ll_upper(ll_upper_(lat, lon)), ll_lower_(ll_lower(lat, lon))
  {}
  
  Node(uint32 id_, uint32 ll_upper_, uint32 ll_lower__)
      : id(id_), ll_upper(ll_upper_), ll_lower_(ll_lower__)
  {}  
};


struct Node_Comparator_By_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id < b.id);
  }
};


struct Node_Equal_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id == b.id);
  }
};


struct Node_Skeleton
{
  Node::Id_Type id;
  uint32 ll_lower;
  
  Node_Skeleton() {}
  
  Node_Skeleton(void* data)
  {
    id = *(uint32*)data;
    ll_lower = *(uint32*)((uint8*)data+4);
  }
  
  Node_Skeleton(const Node& node)
  : id(node.id), ll_lower(node.ll_lower_) {}
  
  Node_Skeleton(Node::Id_Type id_, uint32 ll_lower_)
  : id(id_), ll_lower(ll_lower_) {}
  
  uint32 size_of() const
  {
    return 8;
  }
  
  static uint32 size_of(void* data)
  {
    return 8;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *(uint32*)((uint8*)data+4) = ll_lower;
  }
  
  bool operator<(const Node_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Node_Skeleton& a) const
  {
    return this->id == a.id;
  }
};


#endif
