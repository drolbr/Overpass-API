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
  typedef Uint64 Id_Type;
  
  Id_Type id;
  uint32 index;
  uint32 ll_lower_;
  std::vector< std::pair< std::string, std::string > > tags;
  
  Node() : id(0ull) {}
  
  Node(Id_Type id_, double lat, double lon)
      : id(id_), index(ll_upper_(lat, lon)), ll_lower_(ll_lower(lat, lon))
  {}
  
  Node(Id_Type id_, uint32 ll_upper_, uint32 ll_lower__)
      : id(id_), index(ll_upper_), ll_lower_(ll_lower__)
  {}  
  
  bool operator<(const Node& a) const
  {
    return this->id.val() < a.id.val();
  }
  
  bool operator==(const Node& a) const
  {
    return this->id.val() == a.id.val();
  }
};


struct Node_Comparator_By_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id.val() < b.id.val());
  }

  bool operator() (const Node* a, const Node* b)
  {
    return (a->id.val() < b->id.val());
  }  
};


struct Node_Equal_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id.val() == b.id.val());
  }

  bool operator() (const Node* a, const Node* b)
  {
    return (a->id.val() == b->id.val());
  }  
};


struct Node_Skeleton
{
  typedef Node::Id_Type Id_Type;
  typedef Node_Skeleton Delta;

  Node::Id_Type id;
  uint32 ll_lower;
  
  Node_Skeleton() : id(0ull) {}
  
  Node_Skeleton(void* data)
    : id(*(uint64*)data), ll_lower(*(uint32*)((uint8*)data+8)) {}
  
  Node_Skeleton(const Node& node)
  : id(node.id), ll_lower(node.ll_lower_) {}
  
  Node_Skeleton(Node::Id_Type id_)
  : id(id_), ll_lower(0) {}
  
  Node_Skeleton(Node::Id_Type id_, uint32 ll_lower_)
  : id(id_), ll_lower(ll_lower_) {}
  
  uint32 size_of() const
  {
    return 12;
  }
  
  static uint32 size_of(void* data)
  {
    return 12;
  }
  
  void to_data(void* data) const
  {
    *(uint64*)data = id.val();
    *(uint32*)((uint8*)data+8) = ll_lower;
  }
  
  static Node::Id_Type get_id(const void* data)
  {
    return *(const uint64*)data;
  }

  bool operator<(const Node_Skeleton& a) const
  {
    return this->id.val() < a.id.val();
  }
  
  bool operator==(const Node_Skeleton& a) const
  {
    return this->id.val() == a.id.val();
  }
};


#endif
