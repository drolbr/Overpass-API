/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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
    (File_Blocks_Index_Base& tags_local, std::vector< Tag_Entry< Id_Type > >& tags_to_delete,
     const std::map< uint32, std::vector< uint32 > >& to_delete);

template< class TObject >
void prepare_tags
    (File_Blocks_Index_Base& tags_local, std::vector< TObject* >& elems_ptr,
     std::vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete,
     const std::map< uint32, std::vector< typename TObject::Id_Type > >& to_delete);

template< class TObject >
void update_tags_local
    (File_Blocks_Index_Base& tags_local, const std::vector< TObject* >& elems_ptr,
     const std::vector< std::pair< typename TObject::Id_Type, bool > >& ids_to_modify,
     const std::vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete);

template< class TObject >
void update_tags_global
    (File_Blocks_Index_Base& tags_global, const std::vector< TObject* >& elems_ptr,
     const std::vector< std::pair< typename TObject::Id_Type, bool > >& ids_to_modify,
     const std::vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete);


// make indices appropriately coarse
template< typename Id_Type >
std::map< uint32, std::set< Id_Type > > collect_coarse
    (const std::map< uint32, std::vector< Id_Type > >& elems_by_idx)
{
  std::map< uint32, std::set< Id_Type > > coarse;
  for (typename std::map< uint32, std::vector< Id_Type > >::const_iterator
      it(elems_by_idx.begin()); it != elems_by_idx.end(); ++it)
  {
    std::set< Id_Type >& handle(coarse[it->first & 0x7fffff00]);
    for (typename std::vector< Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      handle.insert(*it2);
  }
  return coarse;
}


//-----------------------------------------------------------------------------


template< class Id_Type >
void prepare_delete_tags
    (File_Blocks_Index_Base& tags_local, std::vector< Tag_Entry< Id_Type > >& tags_to_delete,
     const std::map< uint32, std::vector< Id_Type > >& to_delete)
{
  // make indices appropriately coarse
  std::map< uint32, std::set< Id_Type > > to_delete_coarse;
  for (typename std::map< uint32, std::vector< Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    std::set< Id_Type >& handle(to_delete_coarse[it->first & 0x7fffff00]);
    for (typename std::vector< Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      handle.insert(*it2);
    }
  }

  // formulate range query
  Ranges< Tag_Index_Local > ranges;
  for (auto it = to_delete_coarse.begin(); it != to_delete_coarse.end(); ++it)
    ranges.push_back({ it->first, "", "" }, { it->first + 1, "", "" });
  ranges.sort();

  // iterate over the result
  Block_Backend< Tag_Index_Local, Id_Type > rels_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry< Id_Type > tag_entry;
  current_index.index = 0xffffffff;
  for (auto it = rels_db.range_begin(ranges); !(it == rels_db.range_end()); ++it)
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

    std::set< Id_Type >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
      tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


template< class Id_Type >
void get_existing_tags
    (const std::vector< std::pair< Id_Type, Uint31_Index > >& ids_with_position,
     File_Blocks_Index_Base& tags_local, std::vector< Tag_Entry< Id_Type > >& tags_to_delete)
{
  // make indices appropriately coarse
  std::map< uint32, std::set< Id_Type > > to_delete_coarse;
  for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator
      it = ids_with_position.begin(); it != ids_with_position.end(); ++it)
    to_delete_coarse[it->second.val() & 0x7fffff00].insert(it->first);

  // formulate range query
  Ranges< Tag_Index_Local > ranges;
  for (auto it = to_delete_coarse.begin(); it != to_delete_coarse.end(); ++it)
    ranges.push_back({ it->first, "", "" }, { it->first + 1, "", "" });
  ranges.sort();

  // iterate over the result
  Block_Backend< Tag_Index_Local, Id_Type > rels_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry< Id_Type > tag_entry;
  current_index.index = 0xffffffff;
  for (auto it = rels_db.range_begin(ranges); !(it == rels_db.range_end()); ++it)
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

    std::set< Id_Type >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
      tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


template < class TObject >
void prepare_tags
    (File_Blocks_Index_Base& tags_local, std::vector< TObject* >& elems_ptr,
     std::vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete,
     const std::map< uint32, std::vector< typename TObject::Id_Type > >& to_delete)
{
  // make indices appropriately coarse
  std::map< uint32, std::set< typename TObject::Id_Type > > to_delete_coarse;
  for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    std::set< typename TObject::Id_Type >& handle(to_delete_coarse[it->first & 0x7fffff00]);
    for (typename std::vector< typename TObject::Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      handle.insert(*it2);
  }

  // formulate range query
  Ranges< Tag_Index_Local > ranges;
  for (auto it = to_delete_coarse.begin(); it != to_delete_coarse.end(); ++it)
    ranges.push_back({ it->first, "", "" }, { it->first + 1, "", "" });
  ranges.sort();

  // iterate over the result
  Block_Backend< Tag_Index_Local, Uint32_Index > elems_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry< typename TObject::Id_Type > tag_entry;
  current_index.index = 0xffffffff;
  for (auto it = elems_db.range_begin(ranges);
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

    std::set< typename TObject::Id_Type >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
    {
      TObject* elem(binary_ptr_search_for_id(elems_ptr, it.object().val()));
      if (elem != 0)
	elem->tags.push_back(std::make_pair(it.index().key, it.index().value));
      tag_entry.ids.push_back(it.object().val());
    }
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


template < class TObject >
void update_tags_local
    (File_Blocks_Index_Base& tags_local, const std::vector< TObject* >& elems_ptr,
     const std::vector< std::pair< typename TObject::Id_Type, bool > >& ids_to_modify,
     const std::vector< Tag_Entry< typename TObject::Id_Type > >& tags_to_delete)
{
  std::map< Tag_Index_Local, std::set< typename TObject::Id_Type > > db_to_delete;
  std::map< Tag_Index_Local, std::set< typename TObject::Id_Type > > db_to_insert;

  for (typename std::vector< Tag_Entry< typename TObject::Id_Type > >::const_iterator
      it(tags_to_delete.begin()); it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Local index;
    index.index = it->index;
    index.key = it->key;
    index.value = it->value;

    std::set< typename TObject::Id_Type > elem_ids;
    for (typename std::vector< typename TObject::Id_Type >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
      elem_ids.insert(*it2);

    db_to_delete[index] = elem_ids;
  }

  typename std::vector< TObject* >::const_iterator rit = elems_ptr.begin();
  for (typename std::vector< std::pair< typename TObject::Id_Type, bool > >::const_iterator
      it(ids_to_modify.begin()); it != ids_to_modify.end(); ++it)
  {
    if ((rit != elems_ptr.end()) && (it->first == (*rit)->id))
    {
      if (it->second)
      {
	Tag_Index_Local index;
	index.index = (*rit)->index & 0x7fffff00;

	for (std::vector< std::pair< std::string, std::string > >::const_iterator
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
  elem_db.update(db_to_delete, db_to_insert);
}


#endif
