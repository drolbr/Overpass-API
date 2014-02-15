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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__TAGS_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__TAGS_UPDATER_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../../template_db/transaction.h"


template< class Id_Type >
void prepare_delete_tags
    (File_Blocks_Index_Base& tags_local, vector< Tag_Entry< Id_Type > >& tags_to_delete,
     const map< uint32, vector< uint32 > >& to_delete);

template< class TObject >
void prepare_tags
    (File_Blocks_Index_Base& tags_local, vector< TObject* >& elems_ptr,
     vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete,
     const map< uint32, vector< typename TObject::Id_Type > >& to_delete);

template< class TObject, class Update_Logger >
void update_tags_local
    (File_Blocks_Index_Base& tags_local, const vector< TObject* >& elems_ptr,
     const vector< pair< typename TObject::Id_Type, bool > >& ids_to_modify,
     const vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete,
     Update_Logger* update_logger);

template< class TObject >
void update_tags_global
    (File_Blocks_Index_Base& tags_global, const vector< TObject* >& elems_ptr,
     const vector< pair< typename TObject::Id_Type, bool > >& ids_to_modify,
     const vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete);
       
    
// make indices appropriately coarse
template< typename Id_Type >
map< uint32, set< Id_Type > > collect_coarse
    (const map< uint32, vector< Id_Type > >& elems_by_idx)
{
  map< uint32, set< Id_Type > > coarse;
  for (typename map< uint32, vector< Id_Type > >::const_iterator
      it(elems_by_idx.begin()); it != elems_by_idx.end(); ++it)
  {
    set< Id_Type >& handle(coarse[it->first & 0x7fffff00]);
    for (typename vector< Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      handle.insert(*it2);
  }
  return coarse;
}


// formulate range query
template< typename Id_Type >
set< pair< Tag_Index_Local, Tag_Index_Local > > make_range_set
    (const map< uint32, set< Id_Type > >& coarse)
{
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  for (typename map< uint32, set< Id_Type > >::const_iterator
      it(coarse.begin()); it != coarse.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->first;
    lower.key = "";
    lower.value = "";
    upper.index = it->first + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }
  return range_set;
}
    

//-----------------------------------------------------------------------------


template< class Id_Type >
void prepare_delete_tags
    (File_Blocks_Index_Base& tags_local, vector< Tag_Entry< Id_Type > >& tags_to_delete,
     const map< uint32, vector< Id_Type > >& to_delete)
{
  // make indices appropriately coarse
  map< uint32, set< Id_Type > > to_delete_coarse;
  for (typename map< uint32, vector< Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    set< Id_Type >& handle(to_delete_coarse[it->first & 0x7fffff00]);
    for (typename vector< Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      handle.insert(*it2);
    }
  }
  
  // formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  for (typename map< uint32, set< Id_Type > >::const_iterator
    it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->first;
    lower.key = "";
    lower.value = "";
    upper.index = it->first + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, Id_Type > rels_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry< Id_Type > tag_entry;
  current_index.index = 0xffffffff;
  for (typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator
    it(rels_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == rels_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	tags_to_delete.push_back(tag_entry);
      current_index = it.index();
      tag_entry.index = it.index().index;
      tag_entry.key = it.index().key;
      tag_entry.value = it.index().value;
      tag_entry.ids.clear();
    }
    
    set< Id_Type >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
      tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


template< class Id_Type >
void get_existing_tags
    (const std::vector< std::pair< Id_Type, Uint31_Index > >& ids_with_position,
     File_Blocks_Index_Base& tags_local, vector< Tag_Entry< Id_Type > >& tags_to_delete)
{
  // make indices appropriately coarse
  map< uint32, set< Id_Type > > to_delete_coarse;
  for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator
      it = ids_with_position.begin(); it != ids_with_position.end(); ++it)
    to_delete_coarse[it->second.val() & 0x7fffff00].insert(it->first);
  
  // formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  for (typename map< uint32, set< Id_Type > >::const_iterator
    it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->first;
    lower.key = "";
    lower.value = "";
    upper.index = it->first + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, Id_Type > rels_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry< Id_Type > tag_entry;
  current_index.index = 0xffffffff;
  for (typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator
    it(rels_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == rels_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
        tags_to_delete.push_back(tag_entry);
      current_index = it.index();
      tag_entry.index = it.index().index;
      tag_entry.key = it.index().key;
      tag_entry.value = it.index().value;
      tag_entry.ids.clear();
    }
    
    set< Id_Type >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
      tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


template < class TObject >
void prepare_tags
    (File_Blocks_Index_Base& tags_local, vector< TObject* >& elems_ptr,
     vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete,
     const map< uint32, vector< typename TObject::Id_Type > >& to_delete)
{
  // make indices appropriately coarse
  map< uint32, set< typename TObject::Id_Type > > to_delete_coarse;
  for (typename map< uint32, vector< typename TObject::Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    set< typename TObject::Id_Type >& handle(to_delete_coarse[it->first & 0x7fffff00]);
    for (typename vector< typename TObject::Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      handle.insert(*it2);
  }
  
  // formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  for (typename map< uint32, set< typename TObject::Id_Type > >::const_iterator
      it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->first;
    lower.key = "";
    lower.value = "";
    upper.index = it->first + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }

  // iterate over the result
  Block_Backend< Tag_Index_Local, Uint32_Index > elems_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry< typename TObject::Id_Type > tag_entry;
  current_index.index = 0xffffffff;
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    it(elems_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == elems_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	tags_to_delete.push_back(tag_entry);
      current_index = it.index();
      tag_entry.index = it.index().index;
      tag_entry.key = it.index().key;
      tag_entry.value = it.index().value;
      tag_entry.ids.clear();
    }
    
    set< typename TObject::Id_Type >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
    {
      TObject* elem(binary_ptr_search_for_id(elems_ptr, it.object().val()));
      if (elem != 0)
	elem->tags.push_back(make_pair(it.index().key, it.index().value));
      tag_entry.ids.push_back(it.object().val());
    }
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


template < class TObject, class Update_Logger >
void update_tags_local
    (File_Blocks_Index_Base& tags_local, const vector< TObject* >& elems_ptr,
     const vector< pair< typename TObject::Id_Type, bool > >& ids_to_modify,
     const vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete,
     Update_Logger* update_logger)
{
  map< Tag_Index_Local, set< typename TObject::Id_Type > > db_to_delete;
  map< Tag_Index_Local, set< typename TObject::Id_Type > > db_to_insert;
  
  for (typename vector< Tag_Entry< typename TObject::Id_Type > >::const_iterator
      it(tags_to_delete.begin()); it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Local index;
    index.index = it->index;
    index.key = it->key;
    index.value = it->value;
    
    set< typename TObject::Id_Type > elem_ids;
    for (typename vector< typename TObject::Id_Type >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
      elem_ids.insert(*it2);
    
    db_to_delete[index] = elem_ids;
  }
  
  typename vector< TObject* >::const_iterator rit = elems_ptr.begin();
  for (typename vector< pair< typename TObject::Id_Type, bool > >::const_iterator
      it(ids_to_modify.begin()); it != ids_to_modify.end(); ++it)
  {
    if ((rit != elems_ptr.end()) && (it->first == (*rit)->id))
    {
      if (it->second)
      {
	Tag_Index_Local index;
	index.index = (*rit)->index & 0x7fffff00;
	
	for (vector< pair< string, string > >::const_iterator
	  it2((*rit)->tags.begin()); it2 != (*rit)->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(it->first);
	  db_to_delete[index];
	}
      }
      ++rit;
    }
  }
  
  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > elem_db(&tags_local);
  if (update_logger)
    elem_db.update(db_to_delete, db_to_insert, *update_logger);
  else
    elem_db.update(db_to_delete, db_to_insert);
}


#endif
