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

#include "../core/datatypes.h"


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


typedef enum { only_data, keep_meta, keep_attic } meta_modes;

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
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}


template< typename Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > get_existing_map_positions
    (const std::vector< Id_Type >& ids,
     Transaction& transaction, const File_Properties& file_properties)
{
  Random_File< Uint31_Index > random(transaction.random_index(&file_properties));
  
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
template< typename Element_Skeleton, typename Update_Logger, typename Index_Type >
void new_current_skeletons
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& existing_skeletons,
     bool record_minuscule_moves,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& new_skeletons,
     vector< pair< typename Element_Skeleton::Id_Type, Index_Type > >& moved_objects,
     Update_Logger* update_logger)
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
      tell_update_logger_insertions(*it, update_logger);
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    else if (!(*idx == it->idx))
    {
      // The old and new version have different indexes. So they are surely different.
      moved_objects.push_back(make_pair(it->elem.id, Index_Type(idx->val())));
      tell_update_logger_insertions(*it, update_logger);
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::map< Uint31_Index, std::set< Element_Skeleton > >::iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
    {
      // Something has gone wrong. Save at least the new object.
      tell_update_logger_insertions(*it, update_logger);
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::set< Element_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
    {
      // Something has gone wrong. Save at least the new object.
      tell_update_logger_insertions(*it, update_logger);
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
      tell_update_logger_insertions(*it, update_logger);
      new_skeletons[it->idx].insert(it->elem);
      if (record_minuscule_moves)
        moved_objects.push_back(make_pair(it->elem.id, Index_Type(idx->val())));
    }
  }
  std::cerr<<'\n';
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
template< typename Element_Skeleton, typename Update_Logger, typename Id_Type >
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
     std::map< Tag_Index_Global, std::set< Id_Type > >& attic_global_tags,
     std::map< Tag_Index_Global, std::set< Id_Type > >& new_global_tags)
{
  for (typename std::map< Tag_Index_Local, std::set< Id_Type > >::const_iterator
      it_idx = attic_local_tags.begin(); it_idx != attic_local_tags.end(); ++it_idx)
  {
    std::set< Id_Type >& handle(attic_global_tags[Tag_Index_Global(it_idx->first)]);
    for (typename std::set< Id_Type >::const_iterator it = it_idx->second.begin();
         it != it_idx->second.end(); ++it)
      handle.insert(*it);
  }
  
  for (typename std::map< Tag_Index_Local, std::set< Id_Type > >::const_iterator
      it_idx = new_local_tags.begin(); it_idx != new_local_tags.end(); ++it_idx)
  {
    std::set< Id_Type >& handle(new_global_tags[Tag_Index_Global(it_idx->first)]);
    for (typename std::set< Id_Type >::const_iterator it = it_idx->second.begin();
         it != it_idx->second.end(); ++it)
      handle.insert(*it);
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
  Random_File< Uint31_Index > random(transaction.random_index(&file_properties));
  
  for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator
      it = new_idx_positions.begin(); it != new_idx_positions.end(); ++it)
    random.put(it->first.val(), it->second);
}


template< typename Index, typename Object, typename Update_Logger >
void update_elements
    (const std::map< Index, std::set< Object > >& attic_objects,
     const std::map< Index, std::set< Object > >& new_objects,
     Transaction& transaction, const File_Properties& file_properties,
     Update_Logger* update_logger)
{
  Block_Backend< Index, Object > db(transaction.data_index(&file_properties));
  if (update_logger)
    db.update(attic_objects, new_objects, *update_logger);
  else
    db.update(attic_objects, new_objects);
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
  

#endif
