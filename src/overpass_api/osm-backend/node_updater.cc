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
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"
#include "node_updater.h"
#include "tags_updater.h"


Update_Node_Logger::~Update_Node_Logger()
{
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = insert.begin();
      it != insert.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = keep.begin();
      it != keep.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = erase.begin();
      it != erase.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
}


// New node_updater:


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


bool geometrically_equal(const Node_Skeleton& a, const Node_Skeleton& b)
{
  return (a.ll_lower == b.ll_lower);
}


// TODO: temporary helper function for update_logger
void tell_update_logger_insertions
    (const typename Data_By_Id< Node_Skeleton >::Entry& entry, Update_Node_Logger* update_logger)
{
  if (update_logger)
  {
    Node node(entry.elem.id, entry.idx.val(), entry.elem.ll_lower);
    node.tags = entry.tags;
    update_logger->insertion(node);
  }
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the deletion and insertion lists for the
 * database operation.  Also, the list of moved nodes is filled. */
template< typename Element_Skeleton, typename Update_Logger >
void new_current_skeletons
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& existing_skeletons,
     bool record_minuscule_moves,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& new_skeletons,
     vector< pair< Node::Id_Type, Uint32_Index > >& moved_nodes,
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
      moved_nodes.push_back(make_pair(it->elem.id, Uint32_Index(idx->val())));
      tell_update_logger_insertions(*it, update_logger);
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::map< Uint31_Index, std::set< Element_Skeleton > >::iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
    {
      // Something has gone wrong. Save at least the new node.
      tell_update_logger_insertions(*it, update_logger);
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::set< Element_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
    {
      // Something has gone wrong. Save at least the new node.
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
        moved_nodes.push_back(make_pair(it->elem.id, Uint32_Index(idx->val())));
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


/* Enhance the existing attic meta by the meta entries of deleted elements. */
template< typename Element_Skeleton >
void compute_new_attic_meta
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Element_Skeleton::Id_Type > > >& new_attic_meta)
{
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Uint31_Index last_index(0u);
  typename Element_Skeleton::Id_Type last_id(0ull);
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
        
    if (it->idx == Uint31_Index(0u))
    {
      // For elements to delete we store the deletion meta data.
      
      // We don't have an earlier version of this element in new_data
      if (!(last_id == it->elem.id))
      {
        const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
        if (idx)
          // Take index of the existing element.
          last_index = *idx;
        else
          // Something has gone wrong. We neither have a deleted version in new_data
          // nor in the existing data.
          last_index = Uint31_Index(0u);
      }
      
      new_attic_meta[last_index].insert(it->meta);
      
      last_id = it->elem.id;
      continue;
    }
    last_id = it->elem.id;
    last_index = it->idx;
    
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version also exists in new_data. Store the meta data of this version directly in attic.
      new_attic_meta[it->idx].insert(it->meta);
  }
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the set of elements to store to attic.
 * We use that in attic_skeletons can only appear elements with ids that exist also in new_data. */
template< typename Element_Skeleton >
void compute_new_attic_skeletons
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Attic< Element_Skeleton > > >& full_attic,
     std::map< typename Element_Skeleton::Id_Type, std::set< Uint31_Index > >& idx_lists)
{
  typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  typename Element_Skeleton::Id_Type last_id = typename Element_Skeleton::Id_Type(0ull);
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version exist also in new_data. Make this version a (short-lived) attic version.
      if (it->idx.val() != 0 && (next_it->idx.val() == 0 || !geometrically_equal(it->elem, next_it->elem)))
      {
        full_attic[it->idx].insert(Attic< Element_Skeleton >(it->elem, next_it->meta.timestamp));
        idx_lists[it->elem.id].insert(it->idx);
      }
    
    if (last_id == it->elem.id)
      // An earlier version exists also in new_data. So there is nothing to do here.
      continue;
    last_id = it->elem.id;
    
    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    if (!idx)
      // No old data exists. So there is nothing to do here.
      continue;    

    typename std::map< Uint31_Index, std::set< Element_Skeleton > >::const_iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
      // Something has gone wrong. Skip this object.
      continue;
    
    typename std::set< Element_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
      // Something has gone wrong. Skip this object.
      continue;
    
    if (!geometrically_equal(*it_attic, it->elem))
    {
      full_attic[*idx].insert(Attic< Element_Skeleton >(*it_attic, it->meta.timestamp));
      idx_lists[it_attic->id].insert(*idx);
    }
  }
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


template< typename Id_Type >
void add_tags(Id_Type id, Uint31_Index idx,
    const std::vector< std::pair< std::string, std::string > >& tags,
    std::map< Tag_Index_Local, std::set< Id_Type > >& attic_local_tags,
    std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags)
{
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin();
       it != tags.end(); ++it)
    new_local_tags[Tag_Index_Local(idx.val() & 0xffffff00, it->first, it->second)].insert(id);
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
      add_tags(it->elem.id, it->idx, it->tags, attic_local_tags, new_local_tags);
      continue;
    }
    else if ((idx->val() & 0xffffff00) != (it->idx.val() & 0xffffff00))
    {
      // The old and new version have different indexes. So they are surely different.
      add_tags(it->elem.id, it->idx, it->tags, attic_local_tags, new_local_tags);
      continue;
    }
    
    // The old and new tags for this id go to the same index.
    // TODO: For compatibility with the update_logger, we add all tags
    // regardless whether they existed already before
    add_tags(it->elem.id, it->idx, it->tags, attic_local_tags, new_local_tags);
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


  // ...

  // == attic ==
  
  // vmap < Id, Idx > get_exisiting_attic_map_positions(vmap< Id >)
  
  // vmap < Id, Idx_List > get_exisiting_index_lists(vmap< Id >)
  
  // map< Idx, set< Attic< Skel > > > compile_new_attic_skeletons(Data_Dict new_data, map< Idx, set< Skel > >)
  
  // vmap < Id, Idx_List > create_and_update_idx_lists(vmap < Id, Idx_List >, vmap < Id, Idx >,
  //     map< Idx, set< Attic< Skel > > >)
  // Ermittele zu jeder Id die zutreffenden Versionen und sammele pro Id die Idxe
  // erstellt pro neuer Attic-Id mindestens einen Idx, ggf. mehrere
  
  // update_attic_map_positions(vmap < Id, Idx_List >)
  
  // update_index_lists(vmap < Id, Idx_List >)
  
  // add_new_attic_skeletons(map< Idx, set< Attic< Skel > > >)
  
  // == attic meta ==
  
  // map< Idx, set< Attic< Meta > > > compile_new_attic_meta(Data_Dict new_data, map< Idx, set< Meta > >)
  
  // add_new_attic_meta(map< Idx, set< Attic< Meta > > >)
  
  // == attic tags ==
  
  // map< (Idx, key, value), vec< Id > > compile_new_attic_local_tags
  //     (Data_Dict new_data, map< (Idx, key, value), vec< Id > >)
  
  // add_new_attic_local_tags(map< (Idx, key, value), vec< Id > >)
  
  // map< (key, value), vec< Id > > compile_new_attic_global_tags
  //     (Data_Dict new_data, map< (key, value), vec< Id > >)
  
  // add_new_attic_global_tags(map< (key, value), vec< Id > >)
  
  // == update trail ==
  
  // vec< (Id, geom? tags?, vec< Idx >) > compile_update_trail
  //     (Data_Dict new_data, each data to delete: skels, tags)
  
  // write_update_trail(vec< pair< Id, vec< Idx > > >)
    
  
Node_Updater::Node_Updater(Transaction& transaction_, meta_modes meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_)
{}

Node_Updater::Node_Updater(string db_dir_, meta_modes meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(meta_ == only_data || meta_ == keep_meta),
    db_dir(db_dir_), meta(meta_)
{
  partial_possible = !file_exists
      (db_dir + 
       osm_base_settings().NODES->get_file_name_trunk() +
       osm_base_settings().NODES->get_data_suffix() +
       osm_base_settings().NODES->get_index_suffix());
}

void Node_Updater::update(Osm_Backend_Callback* callback, bool partial,
			  Update_Node_Logger* update_logger)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  // Prepare collecting all data of existing skeletons
  std::sort(new_data.data.begin(), new_data.data.end());  
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_map_positions
      = new_idx_positions(new_data);
  std::vector< Node_Skeleton::Id_Type > ids_to_update_ = ids_to_update(new_data);
  
  // Collect all data of existing id indexes
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_map_positions
      = get_existing_map_positions(ids_to_update_, *transaction, *osm_base_settings().NODES);
  
  // Collect all data of existing skeletons
  std::map< Uint31_Index, std::set< Node_Skeleton > > existing_skeletons
      = get_existing_skeletons< Node_Skeleton >
      (existing_map_positions, *transaction, *osm_base_settings().NODES);

  // Collect all data of existing meta elements
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node::Id_Type > > > existing_meta
      = (meta ? get_existing_meta< OSM_Element_Metadata_Skeleton< Node::Id_Type > >
             (existing_map_positions, *transaction, *meta_settings().NODES_META) :
         std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node::Id_Type > > >());
      
  // Collect all data of existing tags
  std::vector< Tag_Entry< Node_Skeleton::Id_Type > > existing_local_tags;
  get_existing_tags< Node_Skeleton::Id_Type >
      (existing_map_positions, *transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL),
       existing_local_tags);

  // Compute which objects really have changed
  std::map< Uint31_Index, std::set< Node_Skeleton > > attic_skeletons;
  std::map< Uint31_Index, std::set< Node_Skeleton > > new_skeletons;
  new_current_skeletons(new_data, existing_map_positions, existing_skeletons,
      (update_logger != 0), attic_skeletons, new_skeletons, moved_nodes, update_logger);
      
  // Compute which meta data really has changed
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > attic_meta;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_meta;
  new_current_meta(new_data, existing_map_positions, existing_meta, attic_meta, new_meta);
      
  // Compute which tags really have changed
  std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > > attic_local_tags;
  std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > > new_local_tags;
  new_current_local_tags< Node_Skeleton, Update_Node_Logger, Node_Skeleton::Id_Type >
      (new_data, existing_map_positions, existing_local_tags, attic_local_tags, new_local_tags);
  std::map< Tag_Index_Global, std::set< Node_Skeleton::Id_Type > > attic_global_tags;
  std::map< Tag_Index_Global, std::set< Node_Skeleton::Id_Type > > new_global_tags;
  new_current_global_tags< Node_Skeleton::Id_Type >
      (attic_local_tags, new_local_tags, attic_global_tags, new_global_tags);
  
  // TODO: old code
  map< uint32, vector< Node::Id_Type > > to_delete;
  update_node_ids(to_delete, (update_logger != 0), new_map_positions);

  // TODO: old code
  if (update_logger && meta)
  {
    for (vector< pair< OSM_Element_Metadata_Skeleton< Node::Id_Type >, uint32 > >::const_iterator
        it = nodes_meta_to_insert.begin(); it != nodes_meta_to_insert.end(); ++it)
    {
      OSM_Element_Metadata meta;
      meta.version = it->first.version;
      meta.timestamp = it->first.timestamp;
      meta.changeset = it->first.changeset;
      meta.user_id = it->first.user_id;
      meta.user_name = user_by_id[it->first.user_id];
      update_logger->insertion(it->first.ref, meta);
    }
  }

  callback->update_started();
  callback->prepare_delete_tags_finished();
  
  // Update id indexes
  update_map_positions(new_map_positions, *transaction, *osm_base_settings().NODES);
  callback->update_ids_finished();
  
  // Update skeletons
  update_elements(attic_skeletons, new_skeletons, *transaction, *osm_base_settings().NODES, update_logger);
  callback->update_coords_finished();
  
  // Update meta
  if (meta)
    update_elements(attic_meta, new_meta, *transaction, *meta_settings().NODES_META, update_logger);
  
  // Update local tags
  update_elements(attic_local_tags, new_local_tags, *transaction, *osm_base_settings().NODE_TAGS_LOCAL,
                  update_logger);
  callback->tags_local_finished();
  
  // Update global tags
  update_elements(attic_global_tags, new_global_tags, *transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
  callback->tags_global_finished();
  
  if (meta == keep_attic)
  {
    // Collect all data of existing attic id indexes
    std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_attic_map_positions
        = get_existing_map_positions(ids_to_update_, *transaction, *attic_settings().NODES);
    std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists
        = get_existing_idx_lists(ids_to_update_, existing_attic_map_positions,
                                 *transaction, *attic_settings().NODE_IDX_LIST);

    // Compute which objects really have changed
    std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > new_attic_skeletons;
    std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists = existing_idx_lists;
    compute_new_attic_skeletons(new_data, existing_map_positions, attic_skeletons,
                                new_attic_skeletons, new_attic_idx_lists);
    
    strip_single_idxs(existing_idx_lists);
    std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_attic_map_positions
        = strip_single_idxs(new_attic_idx_lists);

    compute_new_attic_meta(new_data, existing_map_positions, attic_meta);
        
    // Update id indexes
    update_map_positions(new_attic_map_positions, *transaction, *attic_settings().NODES);
  
    // Update id index lists
    update_elements(existing_idx_lists, new_attic_idx_lists,
                    *transaction, *attic_settings().NODE_IDX_LIST);
  
    // Add attic elements
    update_elements(std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >(), new_attic_skeletons,
                    *transaction, *attic_settings().NODES);
  
    // Add attic meta
    update_elements
        (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >(),
         attic_meta, *transaction, *attic_settings().NODES_META);
  }
      
  //TODO: old code
  if (meta != only_data)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(nodes_meta_to_insert, idxs_by_id);
    process_user_data(*transaction, user_by_id, idxs_by_id);
    
    if (update_logger)
    {
      stable_sort(nodes_meta_to_delete.begin(), nodes_meta_to_delete.begin());
      nodes_meta_to_delete.erase(unique(nodes_meta_to_delete.begin(), nodes_meta_to_delete.end()),
				 nodes_meta_to_delete.end());
      update_logger->set_delete_meta_data(nodes_meta_to_delete);
      nodes_meta_to_delete.clear();
    }
  }
  callback->update_finished();
  
  new_data.data.clear();
  ids_to_modify.clear();
  nodes_to_insert.clear();

  if (!external_transaction)
    delete transaction;
  
  if (partial_possible && !partial && (update_counter > 0))
  {
    callback->partial_started();

    vector< string > froms;
    for (uint i = 0; i < update_counter % 16; ++i)
    {
      string from(".0a");
      from[2] += i;
      froms.push_back(from);
    }
    merge_files(froms, "");
    
    if (update_counter >= 256)
      merge_files(vector< string >(1, ".2"), ".1");
    if (update_counter >= 16)
    {
      vector< string > froms;
      for (uint i = 0; i < update_counter/16 % 16; ++i)
      {
	string from(".1a");
	from[2] += i;
	froms.push_back(from);
      }
      merge_files(froms, ".1");
      
      merge_files(vector< string >(1, ".1"), "");
    }
    update_counter = 0;
    callback->partial_finished();
  }
  else if (partial_possible && partial)
  {
    string to(".0a");
    to[2] += update_counter % 16;
    rename_referred_file(db_dir, "", to, *osm_base_settings().NODES);
    rename_referred_file(db_dir, "", to, *osm_base_settings().NODE_TAGS_LOCAL);
    rename_referred_file(db_dir, "", to, *osm_base_settings().NODE_TAGS_GLOBAL);
    if (meta)
      rename_referred_file(db_dir, "", to, *meta_settings().NODES_META);
    
    ++update_counter;
    if (update_counter % 16 == 0)
    {
      callback->partial_started();
      
      string to(".1a");
      to[2] += (update_counter/16-1) % 16;
      
      vector< string > froms;
      for (uint i = 0; i < 16; ++i)
      {
	string from(".0a");
	from[2] += i;
	froms.push_back(from);
      }
      merge_files(froms, to);
      callback->partial_finished();
    }
    if (update_counter % 256 == 0)
    {
      callback->partial_started();
      
      vector< string > froms;
      for (uint i = 0; i < 16; ++i)
      {
	string from(".1a");
	from[2] += i;
	froms.push_back(from);
      }
      merge_files(froms, ".2");
      callback->partial_finished();
    }
  }
}

void Node_Updater::update_node_ids
    (map< uint32, vector< Node::Id_Type > >& to_delete, bool record_minuscule_moves,
     const std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > >& new_idx_positions)
{
  static Pair_Comparator_By_Id< Node::Id_Type, bool > pair_comparator_by_id;
  static Pair_Equal_Id< Node::Id_Type, bool > pair_equal_id;

  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ids_to_modify.begin(), ids_to_modify.end(), pair_comparator_by_id);
  vector< pair< Node::Id_Type, bool > >::iterator modi_begin
      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id).base());
  ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
  
  Random_File< Uint32_Index > random
      (transaction->random_index(osm_base_settings().NODES));
  for (vector< pair< Node::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    Uint32_Index index(random.get(it->first.val()));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
  }
}


void Node_Updater::merge_files(const vector< string >& froms, string into)
{
  Transaction_Collection from_transactions(false, false, db_dir, froms);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  ::merge_files< Uint32_Index, Node_Skeleton >
      (from_transactions, into_transaction, *osm_base_settings().NODES);
  ::merge_files< Tag_Index_Local, Node::Id_Type >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_LOCAL);
  ::merge_files< Tag_Index_Global, Node::Id_Type >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
  if (meta)
  {
    ::merge_files< Uint31_Index, OSM_Element_Metadata_Skeleton< Node::Id_Type > >
        (from_transactions, into_transaction, *meta_settings().NODES_META);
  }
}
