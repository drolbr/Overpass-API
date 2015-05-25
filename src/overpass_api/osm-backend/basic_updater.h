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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__BASIC_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__BASIC_UPDATER_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"


template< typename Element_Skeleton >
struct Data_By_Id
{
  struct Entry
  {
    Uint31_Index idx;
    Element_Skeleton elem;
    OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > meta;
    std::vector< std::pair< std::string, std::string > > tags;
    
    Entry(Uint31_Index idx_, Element_Skeleton elem_,
        OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > meta_,
        std::vector< std::pair< std::string, std::string > > tags_
            = std::vector< std::pair< std::string, std::string > >())
        : idx(idx_), elem(elem_), meta(meta_), tags(tags_) {}
    
    bool operator<(const Entry& e) const
    {
      if (this->elem.id < e.elem.id)
        return true;
      if (e.elem.id < this->elem.id)
        return false;
      return (this->meta.version < e.meta.version);
    }
  };
  
  std::vector< Entry > data;
};


template< typename Skeleton >
void remove_time_inconsistent_versions(Data_By_Id< Skeleton >& new_data)
{
  typename std::vector< typename Data_By_Id< Skeleton >::Entry >::iterator from_it = new_data.data.begin();
  typename std::vector< typename Data_By_Id< Skeleton >::Entry >::iterator to_it = new_data.data.begin();
  if (from_it != new_data.data.end())
    ++from_it;
  while (from_it != new_data.data.end())
  {
    if (to_it->elem.id == from_it->elem.id && from_it->meta.timestamp <= to_it->meta.timestamp)
      std::cerr<<"Version "<<to_it->meta.version<<
          " has a later or equal timestamp ("<<Timestamp(to_it->meta.timestamp).str()<<")"
	  " than version "<<from_it->meta.version<<" ("<<Timestamp(from_it->meta.timestamp).str()<<")"
	  " of "<<name_of_type< Skeleton >()<<" "<<from_it->elem.id.val()<<'\n';
    else
      ++to_it;
    *to_it = *from_it;
    ++from_it;
  }
}


// ----------------------------------------------------------------------------
// generic updater functions

template< typename Element_Skeleton >
std::vector< typename Element_Skeleton::Id_Type > ids_to_update
    (const Data_By_Id< Element_Skeleton >& new_data)
{
  std::vector< typename Element_Skeleton::Id_Type > result;
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
    result.push_back(it->elem.id);
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}


template< typename Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > get_existing_map_positions
    (const std::vector< Id_Type >& ids,
     Transaction& transaction, const File_Properties& file_properties)
{
  Random_File< Id_Type, Uint31_Index > random(transaction.random_index(&file_properties));
  
  std::vector< std::pair< Id_Type, Uint31_Index > > result;
  for (typename std::vector< Id_Type >::const_iterator it = ids.begin(); it != ids.end(); ++it)
  {
    Uint31_Index idx = random.get(it->val());
    if (idx.val() > 0)
      result.push_back(make_pair(*it, idx));
  }
  return result;
}


template< typename Id_Type >
struct Idx_Agnostic_Compare
{
  bool operator()(const std::pair< Id_Type, Uint31_Index >& a, const std::pair< Id_Type, Uint31_Index >& b)
  {
    return (a.first < b.first);
  }
};


template< typename Element_Skeleton >
std::map< Uint31_Index, std::set< Element_Skeleton > > get_existing_skeletons
    (const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& ids_with_position,
     Transaction& transaction, const File_Properties& file_properties)
{
  std::set< Uint31_Index > req;
  for (typename std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >::const_iterator
      it = ids_with_position.begin(); it != ids_with_position.end(); ++it)
    req.insert(it->second);
  
  std::map< Uint31_Index, std::set< Element_Skeleton > > result;
  Idx_Agnostic_Compare< typename Element_Skeleton::Id_Type > comp;
  
  Block_Backend< Uint31_Index, Element_Skeleton > db(transaction.data_index(&file_properties));
  for (typename Block_Backend< Uint31_Index, Element_Skeleton >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (binary_search(ids_with_position.begin(), ids_with_position.end(),
        make_pair(it.object().id, 0), comp))
      result[it.index()].insert(it.object());
  }

  return result;
}


template< typename Index, typename Element_Skeleton, typename Element_Skeleton_Delta >
std::map< typename Element_Skeleton::Id_Type, std::pair< Index, Attic< Element_Skeleton_Delta > > >
    get_existing_attic_skeleton_timestamps
    (const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& ids_with_position,
     const std::map< typename Element_Skeleton::Id_Type, std::set< Uint31_Index > >& existing_idx_lists,
     Transaction& transaction, const File_Properties& skel_file_properties,
     const File_Properties& undelete_file_properties)
{
  std::set< Uint31_Index > req;
  for (typename std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >::const_iterator
      it = ids_with_position.begin(); it != ids_with_position.end(); ++it)
    req.insert(it->second);
  
  for (typename std::map< typename Element_Skeleton::Id_Type, std::set< Uint31_Index > >::const_iterator
      it = existing_idx_lists.begin(); it != existing_idx_lists.end(); ++it)
  {
    for (std::set< Uint31_Index >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      req.insert(*it2);
  }
  
  std::map< typename Element_Skeleton::Id_Type, std::pair< Index, Attic< Element_Skeleton_Delta > > > result;
  Idx_Agnostic_Compare< typename Element_Skeleton::Id_Type > comp;
  
  Block_Backend< Uint31_Index, Attic< Element_Skeleton_Delta > > db(transaction.data_index(&skel_file_properties));
  for (typename Block_Backend< Uint31_Index, Attic< Element_Skeleton_Delta > >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (binary_search(ids_with_position.begin(), ids_with_position.end(),
        make_pair(it.object().id, 0), comp))
    {
      typename std::map< typename Element_Skeleton::Id_Type,
          std::pair< Index, Attic< Element_Skeleton_Delta > > >::iterator
          rit = result.find(it.object().id);
      if (rit == result.end())
	result.insert(std::make_pair(it.object().id, std::make_pair(it.index(), it.object())));
      else if (rit->second.second.timestamp < it.object().timestamp)
        rit->second = std::make_pair(it.index(), it.object());
    }
  }
  
  Block_Backend< Uint31_Index, Attic< typename Element_Skeleton::Id_Type > >
      undelete_db(transaction.data_index(&undelete_file_properties));
  for (typename Block_Backend< Uint31_Index, Attic< typename Element_Skeleton::Id_Type > >::Discrete_Iterator
      it(undelete_db.discrete_begin(req.begin(), req.end())); !(it == undelete_db.discrete_end()); ++it)
  {
    if (binary_search(ids_with_position.begin(), ids_with_position.end(),
        std::pair< typename Element_Skeleton::Id_Type, Uint31_Index >(it.object(), 0u), comp))
    {
      typename std::map< typename Element_Skeleton::Id_Type,
          std::pair< Index, Attic< Element_Skeleton_Delta > > >::iterator
          rit = result.find(it.object());
      if (rit == result.end())
	result.insert(std::make_pair(it.object(), std::make_pair(it.index(),
	    Attic< Element_Skeleton_Delta >(Element_Skeleton_Delta(), it.object().timestamp))));
      else if (rit->second.second.timestamp < it.object().timestamp)
        rit->second = std::make_pair(it.index(),
	    Attic< Element_Skeleton_Delta >(Element_Skeleton_Delta(), it.object().timestamp));
    }
  }

  return result;
}


template< typename Element_Skeleton >
std::map< Uint31_Index, std::set< Element_Skeleton > > get_existing_meta
    (const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& ids_with_position,
     Transaction& transaction, const File_Properties& file_properties)
{
  std::set< Uint31_Index > req;
  for (typename std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >::const_iterator
      it = ids_with_position.begin(); it != ids_with_position.end(); ++it)
    req.insert(it->second);
  
  std::map< Uint31_Index, std::set< Element_Skeleton > > result;
  Idx_Agnostic_Compare< typename Element_Skeleton::Id_Type > comp;
  
  Block_Backend< Uint31_Index, Element_Skeleton > db(transaction.data_index(&file_properties));
  for (typename Block_Backend< Uint31_Index, Element_Skeleton >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (binary_search(ids_with_position.begin(), ids_with_position.end(),
        make_pair(it.object().ref, 0), comp))
      result[it.index()].insert(it.object());
  }

  return result;
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the deletion and insertion lists for the
 * database operation.  Also, the list of moved objects is filled. */
template< typename Element_Skeleton, typename Index_Type >
void new_current_skeletons
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& existing_skeletons,
     bool record_minuscule_moves,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& new_skeletons,
     vector< pair< typename Element_Skeleton::Id_Type, Index_Type > >& moved_objects)
{
  attic_skeletons = existing_skeletons;
  
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version exist also in new_data. So there is nothing to do.
      continue;

    if (it->idx == Uint31_Index(0u))
      // There is nothing to do for elements to delete. If they exist, they are contained in the
      // attic_skeletons.
      continue;
    
    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    if (!idx)
    {
      // No old data exists. So we can add the new data and are done.
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    else if (!(*idx == it->idx))
    {
      // The old and new version have different indexes. So they are surely different.
      moved_objects.push_back(make_pair(it->elem.id, Index_Type(idx->val())));
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::map< Uint31_Index, std::set< Element_Skeleton > >::iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
    {
      // Something has gone wrong. Save at least the new object.
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::set< Element_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
    {
      // Something has gone wrong. Save at least the new object.
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    // We have found an element at the same index with the same id, so this is a candidate for
    // not being moved.
    if (false/*geometrically_equal(it->elem, *it_attic)*/) // TODO: temporary change to keep update_logger working
      // The element stays unchanged.
      it_attic_idx->second.erase(it_attic);
    else
    {
      new_skeletons[it->idx].insert(it->elem);
      if (record_minuscule_moves)
        moved_objects.push_back(make_pair(it->elem.id, Index_Type(idx->val())));
    }
  }
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the deletion and insertion lists for the
 * database operation.  Also, the list of moved nodes is filled. */
template< typename Element_Skeleton >
void new_current_meta
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >& existing_meta,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >& attic_meta,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >& new_meta)
{
  attic_meta = existing_meta;
  
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version exist also in new_data. So there is nothing to do.
      continue;

    if (it->idx == Uint31_Index(0u))
      // There is nothing to do for elements to delete. If they exist, they are contained in the
      // attic_meta.
      continue;
    
    new_meta[it->idx].insert(it->meta);    
  }
}


template< typename Id_Type >
void add_tags(Id_Type id, Uint31_Index idx,
    const std::vector< std::pair< std::string, std::string > >& tags,
    std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags)
{
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin();
       it != tags.end(); ++it)
    new_local_tags[Tag_Index_Local(idx.val() & 0x7fffff00, it->first, it->second)].insert(id);
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the deletion and insertion lists for the
 * database operation.  Also, the list of moved nodes is filled. */
template< typename Element_Skeleton, typename Id_Type >
void new_current_local_tags
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< Id_Type, Uint31_Index > >& existing_map_positions,
     const std::vector< Tag_Entry< Id_Type > >& existing_local_tags,
     std::map< Tag_Index_Local, std::set< Id_Type > >& attic_local_tags,
     std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags)
{
  //TODO: convert the data format until existing_local_tags get the new data format
  attic_local_tags.clear();
  for (typename std::vector< Tag_Entry< Id_Type > >::const_iterator it_idx = existing_local_tags.begin();
       it_idx != existing_local_tags.end(); ++it_idx)
  {
    std::set< Id_Type >& handle(attic_local_tags[*it_idx]);
    for (typename std::vector< Id_Type >::const_iterator it = it_idx->ids.begin();
         it != it_idx->ids.end(); ++it)
      handle.insert(*it);
  }
  
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version exist also in new_data. So there is nothing to do.
      continue;

    if (it->idx == Uint31_Index(0u))
      // There is nothing to do for elements to delete. If they exist, they are contained in the
      // attic_skeletons.
      continue;
    
    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    if (!idx)
    {
      // No old data exists. So we can add the new data and are done.
      add_tags(it->elem.id, it->idx, it->tags, new_local_tags);
      continue;
    }
    else if ((idx->val() & 0x7fffff00) != (it->idx.val() & 0x7fffff00))
    {
      // The old and new version have different indexes. So they are surely different.
      add_tags(it->elem.id, it->idx, it->tags, new_local_tags);
      continue;
    }
    
    // The old and new tags for this id go to the same index.
    // TODO: For compatibility with the update_logger, we add all tags
    // regardless whether they existed already before
    add_tags(it->elem.id, it->idx, it->tags, new_local_tags);
  }
}


/* Constructs the global tags from the local tags. */
template< typename Id_Type >
void new_current_global_tags
    (const std::map< Tag_Index_Local, std::set< Id_Type > >& attic_local_tags,
     const std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags,
     std::map< Tag_Index_Global, std::set< Tag_Object_Global< Id_Type > > >& attic_global_tags,
     std::map< Tag_Index_Global, std::set< Tag_Object_Global< Id_Type > > >& new_global_tags)
{
  for (typename std::map< Tag_Index_Local, std::set< Id_Type > >::const_iterator
      it_idx = attic_local_tags.begin(); it_idx != attic_local_tags.end(); ++it_idx)
  {
    std::set< Tag_Object_Global< Id_Type > >& handle(attic_global_tags[Tag_Index_Global(it_idx->first)]);
    for (typename std::set< Id_Type >::const_iterator it = it_idx->second.begin();
         it != it_idx->second.end(); ++it)
      handle.insert(Tag_Object_Global< Id_Type >(*it, it_idx->first.index));
  }
  
  for (typename std::map< Tag_Index_Local, std::set< Id_Type > >::const_iterator
      it_idx = new_local_tags.begin(); it_idx != new_local_tags.end(); ++it_idx)
  {
    std::set< Tag_Object_Global< Id_Type > >& handle(new_global_tags[Tag_Index_Global(it_idx->first)]);
    for (typename std::set< Id_Type >::const_iterator it = it_idx->second.begin();
         it != it_idx->second.end(); ++it)
      handle.insert(Tag_Object_Global< Id_Type >(*it, it_idx->first.index));
  }
}


template< typename Element_Skeleton >
std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > > new_idx_positions
    (const Data_By_Id< Element_Skeleton >& new_data)
{
  std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > > result;
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
      result.push_back(make_pair(it->elem.id, it->idx));
  }
  return result;
}


template< typename Id_Type >
void update_map_positions
    (std::vector< std::pair< Id_Type, Uint31_Index > > new_idx_positions,
     Transaction& transaction, const File_Properties& file_properties)
{
  Random_File< Id_Type, Uint31_Index > random(transaction.random_index(&file_properties));
  
  for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator
      it = new_idx_positions.begin(); it != new_idx_positions.end(); ++it)
    random.put(it->first.val(), it->second);
}


template< typename Index, typename Object >
void update_elements
    (const std::map< Index, std::set< Object > >& attic_objects,
     const std::map< Index, std::set< Object > >& new_objects,
     Transaction& transaction, const File_Properties& file_properties)
{
  Block_Backend< Index, Object > db(transaction.data_index(&file_properties));
  db.update(attic_objects, new_objects);
}


template< typename Id_Type >
std::map< Id_Type, std::set< Uint31_Index > > get_existing_idx_lists
    (const std::vector< Id_Type >& ids,
     const std::vector< std::pair< Id_Type, Uint31_Index > >& ids_with_position,
     Transaction& transaction, const File_Properties& file_properties)
{
  std::map< Id_Type, std::set< Uint31_Index > > result;
  
  std::set< Id_Type > req;
  typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator
      it_pos = ids_with_position.begin();
  for (typename std::vector< Id_Type >::const_iterator it = ids.begin(); it != ids.end(); ++it)
  {
    if (it_pos != ids_with_position.end() && *it == it_pos->first)
    {
      if (it_pos->second.val() == 0xff)
        req.insert(*it);
      else
        result[*it].insert(it_pos->second);
      ++it_pos;
    }
  }
  
  Block_Backend< Id_Type, Uint31_Index > db(transaction.data_index(&file_properties));
  for (typename Block_Backend< Id_Type, Uint31_Index >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
    result[it.index()].insert(it.object());

  return result;
}


/* Moves idx entries with only one idx to the return value and erases them from the list. */
template< typename Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > strip_single_idxs
    (std::map< Id_Type, std::set< Uint31_Index > >& idx_list)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > result;
  
  for (typename std::map< Id_Type, std::set< Uint31_Index > >::const_iterator it = idx_list.begin();
       it != idx_list.end(); ++it)
  {
    if (it->second.size() == 1)
      result.push_back(make_pair(it->first, *it->second.begin()));
    else
      result.push_back(make_pair(it->first, Uint31_Index(0xffu)));
  }
  
  for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator it = result.begin();
       it != result.end(); ++it)
  {
    if (it->second.val() != 0xff)
      idx_list.erase(it->first);
  }

  return result;
}


/* Constructs the global tags from the local tags. */
template< typename Id_Type >
std::map< Tag_Index_Global, std::set< Attic< Tag_Object_Global< Id_Type > > > > compute_attic_global_tags
    (const std::map< Tag_Index_Local, std::set< Attic< Id_Type > > >& new_attic_local_tags)
{
  std::map< Tag_Index_Global, std::set< Attic< Tag_Object_Global< Id_Type > > > > result;
  
  for (typename std::map< Tag_Index_Local, std::set< Attic< Id_Type > > >::const_iterator
      it_idx = new_attic_local_tags.begin(); it_idx != new_attic_local_tags.end(); ++it_idx)
  {
    if (it_idx->first.value == void_tag_value())
    {
      std::set< Attic< Tag_Object_Global< Id_Type > > >& handle(result[Tag_Index_Global(it_idx->first)]);
      for (typename std::set< Attic< Id_Type > >::const_iterator it = it_idx->second.begin();
           it != it_idx->second.end(); ++it)
        handle.insert(Attic< Tag_Object_Global< Id_Type > >(
            Tag_Object_Global< Id_Type >(*it, it_idx->first.index), it->timestamp));
    }
  }
  
  for (typename std::map< Tag_Index_Local, std::set< Attic< Id_Type > > >::const_iterator
      it_idx = new_attic_local_tags.begin(); it_idx != new_attic_local_tags.end(); ++it_idx)
  {
    if (it_idx->first.value != void_tag_value())
    {
      std::set< Attic< Tag_Object_Global< Id_Type > > >& handle(result[Tag_Index_Global(it_idx->first)]);
      std::set< Attic< Tag_Object_Global< Id_Type > > >& void_handle
          (result[Tag_Index_Global(it_idx->first.key, void_tag_value())]);
      for (typename std::set< Attic< Id_Type > >::const_iterator it = it_idx->second.begin();
           it != it_idx->second.end(); ++it)
      {
        handle.insert(Attic< Tag_Object_Global< Id_Type > >(
            Tag_Object_Global< Id_Type >(*it, it_idx->first.index), it->timestamp));
        void_handle.erase(Attic< Tag_Object_Global< Id_Type > >(
            Tag_Object_Global< Id_Type >(*it, it_idx->first.index), it->timestamp));
      }
    }
  }
  
  return result;
}


inline std::map< Node_Skeleton::Id_Type, Quad_Coord > dictionary_from_skeletons
    (const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons)
{
  std::map< Node_Skeleton::Id_Type, Quad_Coord > result;
  
  for (std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator
      it = new_node_skeletons.begin(); it != new_node_skeletons.end(); ++it)
  {
    for (std::set< Node_Skeleton >::const_iterator nit = it->second.begin(); nit != it->second.end(); ++nit)
      result.insert(make_pair(nit->id, Quad_Coord(it->first.val(), nit->ll_lower)));
  }
  
  return result;
}


template< typename Skeleton >
std::vector< std::pair< typename Skeleton::Id_Type, Uint31_Index > > make_id_idx_directory
    (const std::map< Uint31_Index, std::set< Skeleton > >& implicitly_moved_skeletons)
{
  std::vector< std::pair< typename Skeleton::Id_Type, Uint31_Index > > result;
  Pair_Comparator_By_Id< typename Skeleton::Id_Type, Uint31_Index > less;
  
  for (typename std::map< Uint31_Index, std::set< Skeleton > >::const_iterator
       it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (typename std::set< Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      result.push_back(make_pair(it2->id, it->first));
  }
  std::sort(result.begin(), result.end(), less);
  
  return result;
}


/* Adds to attic_meta and new_meta the meta elements to delete resp. add from only
   implicitly moved ways. */
template< typename Id_Type >
void new_implicit_meta
    (const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >&
         existing_meta,
     const std::vector< std::pair< Id_Type, Uint31_Index > >& new_positions,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& attic_meta,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& new_meta)
{
  for (typename std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >
          ::const_iterator it_idx = existing_meta.begin(); it_idx != existing_meta.end(); ++it_idx)
  {
    std::set< OSM_Element_Metadata_Skeleton< Id_Type > >& handle(attic_meta[it_idx->first]);
    for (typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::const_iterator
        it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
      handle.insert(*it);
  }

  for (typename std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >
          ::const_iterator it_idx = existing_meta.begin(); it_idx != existing_meta.end(); ++it_idx)
  {
    for (typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::const_iterator
        it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
    {
      const Uint31_Index* idx = binary_pair_search(new_positions, it->ref);
      if (idx)
        new_meta[*idx].insert(*it);
    }
  }
}


/* Adds to attic_local_tags and new_local_tags the tags to delete resp. add from only
   implicitly moved ways. */
template< typename Id_Type >
void new_implicit_local_tags
    (const std::vector< Tag_Entry< Id_Type > >& existing_local_tags,
     const std::vector< std::pair< Id_Type, Uint31_Index > >& new_positions,
     std::map< Tag_Index_Local, std::set< Id_Type > >& attic_local_tags,
     std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags)
{
  //TODO: convert the data format until existing_local_tags get the new data format
  for (typename std::vector< Tag_Entry< Id_Type > >::const_iterator
      it_idx = existing_local_tags.begin(); it_idx != existing_local_tags.end(); ++it_idx)
  {
    std::set< Id_Type >& handle(attic_local_tags[*it_idx]);
    for (typename std::vector< Id_Type >::const_iterator it = it_idx->ids.begin();
         it != it_idx->ids.end(); ++it)
      handle.insert(*it);
  }

  for (typename std::vector< Tag_Entry< Id_Type > >::const_iterator
      it_idx = existing_local_tags.begin(); it_idx != existing_local_tags.end(); ++it_idx)
  {
    for (typename std::vector< Id_Type >::const_iterator it = it_idx->ids.begin();
         it != it_idx->ids.end(); ++it)
    {
      const Uint31_Index* idx = binary_pair_search(new_positions, *it);
      if (idx)
        new_local_tags[Tag_Index_Local(idx->val() & 0x7fffff00, it_idx->key, it_idx->value)].insert(*it);
    }
  }  
}


template< typename Skeleton >
void add_deleted_skeletons
    (const std::map< Uint31_Index, std::set< Skeleton > >& attic_skeletons,
     std::vector< std::pair< typename Skeleton::Id_Type, Uint31_Index > >& new_positions)
{
  for (typename std::map< Uint31_Index, std::set< Skeleton > >::const_iterator it = attic_skeletons.begin();
       it != attic_skeletons.end(); ++it)
  {
    for (typename std::set< Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      new_positions.push_back(std::make_pair(it2->id, Uint31_Index(0u)));
  }
  
  std::stable_sort(new_positions.begin(), new_positions.end(),
                   Pair_Comparator_By_Id< typename Skeleton::Id_Type, Uint31_Index >());
  new_positions.erase(std::unique(new_positions.begin(), new_positions.end(),
                      Pair_Equal_Id< typename Skeleton::Id_Type, Uint31_Index >()), new_positions.end());
}


template< typename Element_Skeleton >
std::vector< typename Element_Skeleton::Id_Type > enhance_ids_to_update
    (const std::map< Uint31_Index, std::set< Element_Skeleton > >& implicitly_moved_skeletons,
     std::vector< typename Element_Skeleton::Id_Type >& ids_to_update)
{
  for (typename std::map< Uint31_Index, std::set< Element_Skeleton > >::const_iterator
     it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (typename std::set< Element_Skeleton >::const_iterator
        it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      ids_to_update.push_back(it2->id);
  }
  std::sort(ids_to_update.begin(), ids_to_update.end());
  ids_to_update.erase(std::unique(ids_to_update.begin(), ids_to_update.end()), ids_to_update.end());
  return ids_to_update;
}


template< typename Id_Type >
struct Descending_By_Timestamp
{
  bool operator()(const Id_Type& lhs, const Id_Type& rhs) const
  {
    return rhs.timestamp < lhs.timestamp;
  }
};


template< typename Element_Skeleton, typename Attic_Skeleton >
std::map< typename Element_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > >
    compute_new_attic_idx_by_id_and_time
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& new_skeletons,
     const std::map< Uint31_Index, std::set< Attic_Skeleton > >& full_attic)
{
  std::map< typename Element_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > > result;
  
  for (typename std::map< Uint31_Index, std::set< Element_Skeleton > >::const_iterator
      it = new_skeletons.begin(); it != new_skeletons.end(); ++it)
  {
    for (typename std::set< Element_Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      result[it2->id].push_back(Attic< Uint31_Index >
          (it->first, NOW));
  }
  
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      next_it = new_data.data.begin();
  if (next_it != new_data.data.end())
    ++next_it;
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (it->idx.val() == 0)
    {
      if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
        result[it->elem.id].push_back(Attic< Uint31_Index >(it->idx, NOW));
      else 
        result[it->elem.id].push_back(Attic< Uint31_Index >(it->idx, next_it->meta.timestamp));
    }
    ++next_it;
  }
        
  for (typename std::map< Uint31_Index, std::set< Attic_Skeleton > >::const_iterator
      it = full_attic.begin(); it != full_attic.end(); ++it)
  {
    for (typename std::set< Attic_Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      result[it2->id].push_back(Attic< Uint31_Index >(it->first, it2->timestamp));
  }
  
  for (typename std::map< typename Element_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > >::iterator
      it = result.begin(); it != result.end(); ++it)
    std::sort(it->second.begin(), it->second.end(),
              Descending_By_Timestamp< Attic< Uint31_Index > >());
    
  return result;
}


/* Enhance the existing attic meta by the meta entries of deleted elements.
   Assert: the sequence in the vector is ordered from recent to old. */
template< typename Id_Type >
std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >
    compute_new_attic_meta
    (const std::map< Id_Type, std::vector< Attic< Uint31_Index > > >& new_attic_idx_by_id_and_time,
     const std::map< Id_Type, std::vector< OSM_Element_Metadata_Skeleton< Id_Type > > >& meta_by_id_and_time,
     const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& new_meta)
{
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > > result;
  
  for (typename std::map< Id_Type, std::vector< Attic< Uint31_Index > > >::const_iterator
      it = new_attic_idx_by_id_and_time.begin(); it != new_attic_idx_by_id_and_time.end(); ++it)
  {
    typename std::map< Id_Type, std::vector< OSM_Element_Metadata_Skeleton< Id_Type > > >
        ::const_iterator mit = meta_by_id_and_time.find(it->first);
        
    if (mit == meta_by_id_and_time.end())
      // Something has gone wrong seriously. We anyway cannot then copy any meta information here
      continue;
    
    // Use that one cannot insert the same value twice in a set
      
    typename std::vector< OSM_Element_Metadata_Skeleton< Id_Type > >::const_iterator
        mit2 = mit->second.begin();
    std::vector< Attic< Uint31_Index > >::const_iterator it2 = it->second.begin();
    if (it2 == it->second.end())
      // Assert: Can't happen
      continue;
    
    Uint31_Index last_idx(*it2);
    ++it2;
            
    while (mit2 != mit->second.end())
    {
      if (it2 == it->second.end())
      {
        result[last_idx].insert(*mit2);
        ++mit2;
      }
      else if (it2->timestamp < mit2->timestamp)
      {
        // Assert: last_idx != 0
        result[last_idx].insert(*mit2);
        ++mit2;
      }
      else if (it2->timestamp == mit2->timestamp)
      {
        if (last_idx.val() == 0)
          result[*it2].insert(*mit2);
        else
          result[last_idx].insert(*mit2);
        ++mit2;
        last_idx = *it2;
        ++it2;
      }
      else
      {
        if (!(it2->val() == 0) && !(last_idx == *it2))
          result[last_idx].insert(*mit2);
        last_idx = *it2;
        ++it2;
      }
    }
  }
  
  // Remove current meta from attic if it were still at the right place
  for (typename std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >
        ::const_iterator it = new_meta.begin(); it != new_meta.end(); ++it)
  {
    for (typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::const_iterator
        it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      result[it->first].erase(*it2);
  }
  
  return result;
}


template< typename Id_Type >
std::map< Tag_Index_Local, std::set< Attic< Id_Type > > > compute_new_attic_local_tags
    (const std::map< Id_Type, std::vector< Attic< Uint31_Index > > >& new_attic_idx_by_id_and_time,
     const std::map< std::pair< Id_Type, std::string >, std::vector< Attic< std::string > > >&
         tags_by_id_and_time,
     const std::vector< std::pair< Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Id_Type, std::set< Uint31_Index > >& existing_idx_lists)
{
  std::map< Tag_Index_Local, std::set< Attic< Id_Type > > > result;
  
  typename std::map< std::pair< Id_Type, std::string >, std::vector< Attic< std::string > > >
      ::const_iterator tit = tags_by_id_and_time.begin();
  for (typename std::map< Id_Type, std::vector< Attic< Uint31_Index > > >::const_iterator
      it = new_attic_idx_by_id_and_time.begin(); it != new_attic_idx_by_id_and_time.end(); ++it)
  {
    while (tit != tags_by_id_and_time.end() && tit->first.first == it->first)
    {
      // We handle in this loop body a single combination of an object (by its id) and a key.
      // The indices used by the object come from it->second.
      // The values the tag takes come from tit->second.
      
      // Use that one cannot insert the same value twice in a set
      
      typename std::vector< Attic< std::string > >::const_iterator tit2 = tit->second.begin();
      std::vector< Attic< Uint31_Index > >::const_iterator it2 = it->second.begin();
      if (it2 == it->second.end())
        // Assert: Can't happen
        continue;
      
      std::set< Uint31_Index > existing_attic_idxs;
      typename std::map< Id_Type, std::set< Uint31_Index > >::const_iterator iit
          = existing_idx_lists.find(it->first);
      if (iit != existing_idx_lists.end())
      {
        for (std::set< Uint31_Index >::const_iterator iit2 = iit->second.begin(); iit2 != iit->second.end();
             ++iit2)
          existing_attic_idxs.insert(Uint31_Index(iit2->val() & 0x7fffff00));
      }
      
      const Uint31_Index* idx_ptr = binary_pair_search(existing_map_positions, it->first);
      if (idx_ptr != 0)
        existing_attic_idxs.insert(Uint31_Index(idx_ptr->val() & 0x7fffff00));
      
      Uint31_Index last_idx = *it2;
      std::string last_value = void_tag_value() + " ";
      ++it2;
      if (tit2 != tit->second.end() && tit2->timestamp == NOW)
      {
        last_value = *tit2;
        ++tit2;
      }
      while (it2 != it->second.end() || tit2 != tit->second.end())
      {
        if (it2 == it->second.end() || (tit2 != tit->second.end() && it2->timestamp < tit2->timestamp))
        {
	  // The more recent (thus relevant) timestamp is here tit2->timestamp.
	  // In particular, the index of the object doesn't change at this time.
	  
	  // We generate an entry for the older tag situation here. This means:
	  // - If the tag was deleted at this point in time (last_index == 0u) then the tag is invalid anyway.
	  // - If the value doesn't differ from the previous value then also no entry is necessary.
	  // We then write the old tag value (maybe empty) if
	  // - the older value is not the oldest known state
	  // - or there is an even older version in this update process
	  
          if (last_idx.val() != 0u && last_value != *tit2 && (*tit2 != void_tag_value() + " "
	      || it2 != it->second.end()
              || existing_attic_idxs.find(Uint31_Index(last_idx.val() & 0x7fffff00)) != existing_attic_idxs.end()))
            result[Tag_Index_Local(Uint31_Index(last_idx.val() & 0x7fffff00), tit->first.second,
		    *tit2 != void_tag_value() + " " ? *tit2 : void_tag_value())]
                .insert(Attic< Id_Type >(it->first, tit2->timestamp));
		
          last_value = *tit2;
          ++tit2;
        }
        else if (tit2 == tit->second.end() || tit2->timestamp < it2->timestamp)
        {
	  // The more recent (thus relevant) timestamp is here it2->timestamp.
	  // In particular, the tag has before and after the value last_value.
	  
	  // We only need to write something if
	  // - the index really changes
	  // - and the tag is really set
	  // In this case we write an entry at the old index that the tag has expired
	  // and an entry at the new index that the tag wasn't set here before.
	  
          if (!((last_idx.val() & 0x7fffff00) == (it2->val() & 0x7fffff00))
	      && last_value != void_tag_value() && last_value != void_tag_value() + " ")
          {
            if (it2->val() != 0u)
              result[Tag_Index_Local(Uint31_Index(it2->val() & 0x7fffff00), tit->first.second, last_value)]
                .insert(Attic< Id_Type >(it->first, it2->timestamp));
		
            if (last_idx.val() != 0u)
              result[Tag_Index_Local(Uint31_Index(last_idx.val() & 0x7fffff00),
                                     tit->first.second, void_tag_value())]
                  .insert(Attic< Id_Type >(it->first, it2->timestamp));
          }
          last_idx = *it2;
          ++it2;
        }
        else
        {
	  // Both timestamps are equal. We need to do different things if the effective index has changed or not.
	  
	  if (!((last_idx.val() & 0x7fffff00) == (it2->val() & 0x7fffff00)))
	  {
	    // This is similar to the case that only the index changes.
	    
	    // If the younger index is non-void then we store for it a delimiter to the past
            if (last_idx.val() != 0u && last_value != void_tag_value() && last_value != void_tag_value() + " ")
              result[Tag_Index_Local(Uint31_Index(last_idx.val() & 0x7fffff00),
                                     tit->first.second, void_tag_value())]
                  .insert(Attic< Id_Type >(it->first, it2->timestamp));
	    
	    // If the older index is non-void then we write an entry for it
            if (it2->val() != 0u)
              result[Tag_Index_Local(Uint31_Index(it2->val() & 0x7fffff00), tit->first.second, *tit2)]
                .insert(Attic< Id_Type >(it->first, it2->timestamp));
	  }
	  else
	  {
	    // This is similar to the case that only the tag value changes.
            if (last_idx.val() != 0u && last_value != *tit2)
              result[Tag_Index_Local(Uint31_Index(last_idx.val() & 0x7fffff00), tit->first.second,
		    *tit2 != void_tag_value() + " " ? *tit2 : void_tag_value())]
                  .insert(Attic< Id_Type >(it->first, tit2->timestamp));
	  }
	  
          last_value = *tit2;
          ++tit2;
          last_idx = *it2;
          ++it2;
        }
      }
      
      ++tit;
    }
  }
  
  return result;
}


template< typename Element_Skeleton >
std::map< typename Element_Skeleton::Id_Type,
    std::vector< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >
    compute_meta_by_id_and_time
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::map< Uint31_Index,
         std::set< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >& attic_meta)
{
  std::map< typename Element_Skeleton::Id_Type, std::vector<
      OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > > result;

  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
    result[it->elem.id].push_back(it->meta);
      
  for (typename std::map< Uint31_Index, std::set<
        OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >::const_iterator
      it = attic_meta.begin(); it != attic_meta.end(); ++it)
  {
    for (typename std::set<
          OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > >::const_iterator
        it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      result[it2->ref].push_back(*it2);
  }
  
  for (typename std::map< typename Element_Skeleton::Id_Type, std::vector<
          OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >::iterator
      it = result.begin(); it != result.end(); ++it)
    std::sort(it->second.begin(), it->second.end(),
        Descending_By_Timestamp< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > >());
  
  return result;
}


template< typename Element_Skeleton >
std::map< std::pair< typename Element_Skeleton::Id_Type, std::string >, std::vector< Attic< std::string > > >
    compute_tags_by_id_and_time
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::map< Tag_Index_Local, std::set< typename Element_Skeleton::Id_Type > >& attic_local_tags)
{
  std::map< std::pair< typename Element_Skeleton::Id_Type, std::string >,
      std::vector< Attic< std::string > > > result;
    
  // Contains for each OSM object its oldest appearing timestamp.
  std::map< typename Element_Skeleton::Id_Type, uint64 > timestamp_per_id;

  // Convert new_data into a list of pairs of tag values and their expiration date.
  
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      next_it = new_data.data.begin();
  if (next_it != new_data.data.end())
  {
    timestamp_per_id[next_it->elem.id] = next_it->meta.timestamp;
    ++next_it;
  }
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    // The expiration date of this version.
    uint64 next_timestamp = NOW;
    
    if (next_it != new_data.data.end())
    {
      if (next_it->elem.id == it->elem.id)
        next_timestamp = next_it->meta.timestamp;
      else
        timestamp_per_id[next_it->elem.id] = next_it->meta.timestamp;
      ++next_it;
    }

    for (std::vector< std::pair< std::string, std::string > >::const_iterator it2 = it->tags.begin();
         it2 != it->tags.end(); ++it2)
    {
      std::vector< Attic< std::string > >& result_ref = result[std::make_pair(it->elem.id, it2->first)];
      if (result_ref.empty())
      {
	if (it->meta.timestamp == timestamp_per_id[it->elem.id])
          result_ref.push_back(Attic< std::string >(void_tag_value() + " ", it->meta.timestamp));
	else
	  result_ref.push_back(Attic< std::string >(void_tag_value(), it->meta.timestamp));
      }
      else if (result_ref.back().timestamp < it->meta.timestamp)
        result_ref.push_back(Attic< std::string >(void_tag_value(), it->meta.timestamp));
      result_ref.push_back(Attic< std::string >(it2->second, next_timestamp));
    }
  }
  
  for (typename std::map< std::pair< typename Element_Skeleton::Id_Type, std::string >,
      std::vector< Attic< std::string > > >::iterator it = result.begin(); it != result.end(); ++it)
    std::stable_sort(it->second.begin(), it->second.end(), Descending_By_Timestamp< Attic< std::string > >());

  for (typename std::map< Tag_Index_Local, std::set< typename Element_Skeleton::Id_Type > >::const_iterator
      it = attic_local_tags.begin(); it != attic_local_tags.end(); ++it)
  {
    for (typename std::set< typename Element_Skeleton::Id_Type >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      std::vector< Attic< std::string > >& result_ref = result[std::make_pair(*it2, it->first.key)];
      uint64 timestamp = (timestamp_per_id[*it2] == 0 ? NOW : timestamp_per_id[*it2]);
      if (result_ref.empty() || result_ref.back().timestamp < timestamp_per_id[*it2]
          || result_ref.back() != void_tag_value() + " ")
        result_ref.push_back(Attic< std::string >(it->first.value, timestamp));
      else
        result_ref.back() = Attic< std::string >(it->first.value, timestamp);
    }
  }
  
  return result;
}


//-----------------------------------------------------------------------------


struct Key_Storage
{
  Key_Storage(const File_Properties& file_properties_)
    : file_properties(&file_properties_), max_key_id(0), max_written_key_id(0) {}
  
  void load_keys(Transaction& transaction);
  void flush_keys(Transaction& transaction);
  
  void register_key(const std::string& key);

private:
  const File_Properties* file_properties;
  std::map< std::string, uint32 > key_ids;
  uint32 max_key_id;
  uint32 max_written_key_id;
};


template< typename Skeleton >
void store_new_keys(const Data_By_Id< Skeleton >& new_data,
                    Key_Storage& keys, Transaction& transaction)
{
  keys.load_keys(transaction);
  
  for (typename std::vector< typename Data_By_Id< Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;
    
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it2 = it->tags.begin();
         it2 != it->tags.end(); ++it2)
      keys.register_key(it2->first);
  }
  
  keys.flush_keys(transaction);
}


std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
    collect_nodes_by_id(
    const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
    const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id);


std::map< Way_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >
    collect_ways_by_id(
        const std::map< Uint31_Index, std::set< Attic< Way_Delta > > >& new_attic_way_skeletons,
        const std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id);


#endif
