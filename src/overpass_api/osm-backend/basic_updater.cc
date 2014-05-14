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

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "basic_updater.h"


void Key_Storage::flush_keys(Transaction& transaction)
{
  map< Uint32_Index, set< String_Object > > db_to_delete;
  map< Uint32_Index, set< String_Object > > db_to_insert;
  
  for (std::map< std::string, uint32 >::const_iterator it = key_ids.begin(); it != key_ids.end(); ++it)
  {
    if (it->second >= max_written_key_id)
      db_to_insert[Uint32_Index(it->second)].insert(String_Object(it->first));
    if (it->second >= max_key_id)
      max_key_id = it->second + 1;
  }
  
  Block_Backend< Uint32_Index, String_Object > keys_db(transaction.data_index(file_properties));
  keys_db.update(db_to_delete, db_to_insert);
  max_written_key_id = max_key_id;
}


void Key_Storage::load_keys(Transaction& transaction)
{
  Block_Backend< Uint32_Index, String_Object > keys_db(transaction.data_index(file_properties));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
      it = keys_db.flat_begin(); !(it == keys_db.flat_end()); ++it)
  {
    key_ids[it.object().val()] = it.index().val();
    if (max_key_id <= it.index().val())
      max_key_id = it.index().val()+1;
  }
  max_written_key_id = max_key_id;
}


void Key_Storage::register_key(const string& s)
{
  map< string, uint32 >::const_iterator it(key_ids.find(s));
  if (it != key_ids.end())
    return;
  key_ids[s] = max_key_id;
  ++max_key_id;
}


template< typename Index, typename Object >
struct Ascending_By_Timestamp
{
  bool operator()(const std::pair< Index, Attic< Object > >& lhs,
                  const std::pair< Index, Attic< Object > >& rhs)
  {
    return lhs.second.timestamp < rhs.second.timestamp;
  }
};


std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
    collect_nodes_by_id(
    const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
    const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id)
{
  // Fill nodes_by_id from attic nodes as well as the current nodes in new_node_idx_by_id
  std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > > nodes_by_id;
  for (std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >::const_iterator
      it = new_attic_node_skeletons.begin(); it != new_attic_node_skeletons.end(); ++it)
  {
    for (std::set< Attic< Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      nodes_by_id[it2->id].push_back(std::make_pair(it->first, *it2));
  }
  
  for (std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it = new_node_idx_by_id.begin();
       it != new_node_idx_by_id.end(); ++it)
    nodes_by_id[it->first].push_back(std::make_pair
        (it->second.ll_upper, Attic< Node_Skeleton >(Node_Skeleton(it->first, it->second.ll_lower),
             NOW)));
  
  for (std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
      ::iterator it = nodes_by_id.begin(); it != nodes_by_id.end(); ++it)
    std::sort(it->second.begin(), it->second.end(),
	      Ascending_By_Timestamp< Uint31_Index, Node_Skeleton >());
  
  return nodes_by_id;
}


std::map< Way_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >
    collect_ways_by_id(
        const std::map< Uint31_Index, std::set< Attic< Way_Delta > > >& new_attic_way_skeletons,
        const std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id)
{
  std::map< Way_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > > ways_by_id;
         
  for (std::map< Way_Skeleton::Id_Type, Uint31_Index >::const_iterator it = new_way_idx_by_id.begin();
       it != new_way_idx_by_id.end(); ++it)
    ways_by_id[it->first].push_back(std::make_pair
        (it->second, Attic< Way_Skeleton::Id_Type >(it->first, NOW)));
    
  for (std::map< Uint31_Index, std::set< Attic< Way_Delta > > >::const_iterator
      it = new_attic_way_skeletons.begin(); it != new_attic_way_skeletons.end(); ++it)
  {
    for (std::set< Attic< Way_Delta > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      ways_by_id[it2->id].push_back(std::make_pair(it->first,
          Attic< Way_Skeleton::Id_Type >(it2->id, it2->timestamp)));
  }
  
  for (std::map< Way_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >
      ::iterator it = ways_by_id.begin(); it != ways_by_id.end(); ++it)
    std::sort(it->second.begin(), it->second.end(),
	      Ascending_By_Timestamp< Uint31_Index, Way_Skeleton::Id_Type >());
  
  return ways_by_id;
}
