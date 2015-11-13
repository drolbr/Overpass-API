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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__META_COLLECTOR_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__META_COLLECTOR_H

#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "statement.h"
#include "user_data_cache.h"


template< typename Index, typename Id_Type >
struct Meta_Collector
{
  public:
    template< typename Object >
    Meta_Collector(const map< Index, vector< Object > >& items,
        Resource_Manager& rman, const File_Properties* meta_file_prop = 0,
	bool user_data = true);
    
    Meta_Collector(const set< pair< Index, Index > >& used_ranges,
        Resource_Manager& rman, const File_Properties* meta_file_prop = 0);
    
    void reset();
    const OSM_Element_Metadata_Skeleton< Id_Type >* get
        (const Index& index, Id_Type ref);    
    const OSM_Element_Metadata_Skeleton< Id_Type >* get
        (const Index& index, Id_Type ref, uint64 timestamp);    
    const map< uint32, string >& users() const { return (user_data_cache != 0 ?
         user_data_cache->users() : empty_users_); }
    
    ~Meta_Collector()
    {
      if (meta_db)
      {
	delete db_it;
	delete meta_db;
      }
    }
  
  private:
    set< Index > used_indices;
    set< pair< Index, Index > > used_ranges;
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >* meta_db;
    typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
        ::Discrete_Iterator* db_it;
    typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
        ::Range_Iterator* range_it;
    Index* current_index;
    set< OSM_Element_Metadata_Skeleton< Id_Type > > current_objects;
    User_Data_Cache* user_data_cache;
    map< uint32, string > empty_users_;
    
    void update_current_objects(const Index&);
};



/** Implementation --------------------------------------------------------- */

template< typename Index, typename Object >
void generate_index_query
  (set< Index >& indices,
   const map< Index, vector< Object > >& items)
{
  for (typename map< Index, vector< Object > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
    indices.insert(it->first);
}


template< typename Index, typename Id_Type >
template< typename Object >
Meta_Collector< Index, Id_Type >::Meta_Collector
    (const map< Index, vector< Object > >& items,
        Resource_Manager& rman, const File_Properties* meta_file_prop, bool user_data)
  : meta_db(0), db_it(0), range_it(0), current_index(0), user_data_cache(0)
{
  if (!meta_file_prop)
    return;
  
  generate_index_query(used_indices, items);
  meta_db = new Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
      ((rman.get_transaction())->data_index(meta_file_prop));
	  
  reset();
  
  if (user_data)
  {
    if (!rman.has_user_data_cache()) {
      User_Data_Cache * udc = new User_Data_Cache(*rman.get_transaction());
      rman.set_user_data_cache(*udc);
    }
    user_data_cache = rman.get_user_data_cache();
  }
}


template< typename Index, typename Id_Type >
Meta_Collector< Index, Id_Type >::Meta_Collector
    (const set< pair< Index, Index > >& used_ranges_,
        Resource_Manager& rman, const File_Properties* meta_file_prop)
  : used_ranges(used_ranges_), meta_db(0), db_it(0), range_it(0), current_index(0), user_data_cache(0)
{
  if (!meta_file_prop)
    return;
  
  meta_db = new Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
      ((rman.get_transaction())->data_index(meta_file_prop));
      
  reset();
}


template< typename Index, typename Id_Type >
void Meta_Collector< Index, Id_Type >::reset()
{
  if (!meta_db)
    return;
      
  if (db_it)
    delete db_it;
  if (range_it)
    delete range_it;
  if (current_index)
  {
    delete current_index;
    current_index = 0;
  }
  
  if (used_ranges.empty())
  {
    db_it = new typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
        ::Discrete_Iterator(meta_db->discrete_begin(used_indices.begin(), used_indices.end()));
	
    if (!(*db_it == meta_db->discrete_end()))
      current_index = new Index(db_it->index());
    while (!(*db_it == meta_db->discrete_end()) && (*current_index == db_it->index()))
    {
      current_objects.insert(db_it->object());
      ++(*db_it);
    }
  }
  else
  {
    range_it = new typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
        ::Range_Iterator(meta_db->range_begin(
	    Default_Range_Iterator< Index >(used_ranges.begin()),
	    Default_Range_Iterator< Index >(used_ranges.end())));
	
    if (!(*range_it == meta_db->range_end()))
      current_index = new Index(range_it->index());
    while (!(*range_it == meta_db->range_end()) && (*current_index == range_it->index()))
    {
      current_objects.insert(range_it->object());
      ++(*range_it);
    }
  }
}


template< typename Index, typename Id_Type >
void Meta_Collector< Index, Id_Type >::update_current_objects(const Index& index)
{
  current_objects.clear();
    
  if (db_it)
  {
    while (!(*db_it == meta_db->discrete_end()) && (db_it->index() < index))
      ++(*db_it);
    if (!(*db_it == meta_db->discrete_end()))
      *current_index = db_it->index();
    while (!(*db_it == meta_db->discrete_end()) && (*current_index == db_it->index()))
    {
      current_objects.insert(db_it->object());
      ++(*db_it);
    }
  }
  else if (range_it)
  {
    while (!(*range_it == meta_db->range_end()) && (range_it->index() < index))
      ++(*range_it);
    if (!(*range_it == meta_db->range_end()))
      *current_index = range_it->index();
    while (!(*range_it == meta_db->range_end()) && (*current_index == range_it->index()))
    {
      current_objects.insert(range_it->object());
      ++(*range_it);
    }
  }
}


template< typename Index, typename Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Meta_Collector< Index, Id_Type >::get
    (const Index& index, Id_Type ref)
{
  if (!meta_db)
    return 0;

  if ((current_index) && (*current_index < index))
    update_current_objects(index);
  
  typename set< OSM_Element_Metadata_Skeleton< Id_Type > >::iterator it
      = current_objects.lower_bound(OSM_Element_Metadata_Skeleton< Id_Type >(ref));
  if (it != current_objects.end() && it->ref == ref)
    return &*it;
  else
    return 0;
}


template< typename Index, typename Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Meta_Collector< Index, Id_Type >::get
    (const Index& index, Id_Type ref, uint64 timestamp)
{
  if (!meta_db)
    return 0;

  if ((current_index) && (*current_index < index))
    update_current_objects(index);
  
  typename set< OSM_Element_Metadata_Skeleton< Id_Type > >::iterator it
      = current_objects.lower_bound(OSM_Element_Metadata_Skeleton< Id_Type >(ref, timestamp));
  if (it == current_objects.begin())
    return 0;
  --it;
  if (it->ref == ref)
    return &*it;
  else
    return 0;
}

#endif
