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


// New node_updater:


bool geometrically_equal(const Node_Skeleton& a, const Node_Skeleton& b)
{
  return (a.ll_lower == b.ll_lower);
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
     const std::map< Node_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Element_Skeleton > > >&
         existing_attic_skeleton_timestamps,
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
    {
      // A later version exist also in new_data. Make this version a (short-lived) attic version.
      if (it->idx.val() != 0 && (next_it->idx.val() == 0 || !geometrically_equal(it->elem, next_it->elem)))
      {
        full_attic[it->idx].insert(Attic< Element_Skeleton >(it->elem, next_it->meta.timestamp));
        idx_lists[it->elem.id].insert(it->idx);
      }
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
    
    if (geometrically_equal(*it_attic, it->elem))
      // We don't need to store a separate attic version
      continue;
    
    typename std::map< Node_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Element_Skeleton > > >::const_iterator
        it_attic_time = existing_attic_skeleton_timestamps.find(it->elem.id);
    if (it_attic_time == existing_attic_skeleton_timestamps.end() ||
        it_attic_time->second.second.timestamp < it->meta.timestamp)
    {
      full_attic[*idx].insert(Attic< Element_Skeleton >(*it_attic, it->meta.timestamp));
      idx_lists[it_attic->id].insert(*idx);
    }
  }
}


/* Collects undeleted elements with their index and their timestamp. This is necessary to identify
 * for an undeleted object the fact that is was deleted before its recreation. */
template< typename Element_Skeleton >
std::map< Uint31_Index, std::set< Attic< typename Element_Skeleton::Id_Type > > >
    compute_undeleted_skeletons
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > >& existing_idx_lists)
{
  std::map< Uint31_Index, std::set< Attic< typename Element_Skeleton::Id_Type > > > result;

  typename Element_Skeleton::Id_Type last_id = typename Element_Skeleton::Id_Type(0ull);
  for (typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (last_id == it->elem.id)
    {
      // An earlier version exists also in new_data.
      typename std::vector< typename Data_By_Id< Element_Skeleton >::Entry >::const_iterator last_it = it;
      --last_it;
      if (!(last_it->idx == it->idx))
        result[it->idx].insert(Attic< typename Element_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
    }
    else
    {
      std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > >::const_iterator
          attic_idx_it = existing_idx_lists.find(it->elem.id);
      if (attic_idx_it != existing_idx_lists.end()
          && attic_idx_it->second.find(it->idx) != attic_idx_it->second.end())
      {
        const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
        if (!idx || !(*idx == it->idx))
          result[it->idx].insert(Attic< typename Element_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
      }
    }
    last_id = it->elem.id;
  }
  
  return result;
}


template< typename Id_Type >
void compare_and_add_different_tags(Id_Type id, Uint31_Index idx,
    const std::vector< std::pair< std::string, std::string > >& tags,
    const std::vector< std::pair< std::string, std::string > >& comparison_tags,
    std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags)
{
  std::map< std::string, std::string > tags_by_key;
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = comparison_tags.begin();
       it != comparison_tags.end(); ++it)
    tags_by_key[it->first] = it->second;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin();
       it != tags.end(); ++it)
  {
    if (tags_by_key[it->first] != it->second)
      new_local_tags[Tag_Index_Local(idx.val() & 0x7fffff00, it->first, it->second)].insert(id);
  }
}


/* Cancels out all tags that are equal in the old and the new tag data. */
template< typename Id_Type >
void cancel_out_equal_tags
    (std::map< Tag_Index_Local, std::set< Id_Type > >& attic_local_tags,
     std::map< Tag_Index_Local, std::set< Id_Type > >& new_local_tags)
{
  typename std::map< Tag_Index_Local, std::set< Id_Type > >::iterator it_idx_attic
      = attic_local_tags.begin();
  typename std::map< Tag_Index_Local, std::set< Id_Type > >::iterator it_idx_new
      = new_local_tags.begin();
  while (it_idx_attic != attic_local_tags.end() && it_idx_new != new_local_tags.end())
  {
    if (it_idx_attic->first < it_idx_new->first)
      ++it_idx_attic;
    else if (it_idx_new->first < it_idx_attic->first)
      ++it_idx_new;
    else
    {
      typename std::set< Id_Type >::iterator it_attic = it_idx_attic->second.begin();
      typename std::set< Id_Type >::iterator it_new = it_idx_new->second.begin();
      
      while (it_attic != it_idx_attic->second.end() && it_new != it_idx_new->second.end())
      {
        if (*it_attic < *it_new)
          ++it_attic;
        else if (*it_new < *it_attic)
          ++it_new;
        else
        {
          Id_Type val = *it_new;
          it_idx_attic->second.erase(it_attic);
          it_idx_new->second.erase(it_new);
          it_attic = it_idx_attic->second.upper_bound(val);
          it_new = it_idx_new->second.upper_bound(val);
        }
      }
      
      ++it_idx_attic;
      ++it_idx_new;
    }
  }
}


/* Enhance the existing attic tags by the tags of intermediate versions.
   Also store for tags that have been created on already existing elements a non-tag, i.e.
   write explicitly that until now the key existed with empty value for this element. */
std::map< Tag_Index_Local, std::set< Attic< Node_Skeleton::Id_Type > > >
    compute_new_attic_local_tags
    (const Data_By_Id< Node_Skeleton >& new_data,
     const std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > >& existing_attic_map_positions,
     const std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > >& attic_local_tags)
{
  std::map< Tag_Index_Local, std::set< Attic< Node_Skeleton::Id_Type > > > result;  
  std::map< Node_Skeleton::Id_Type, uint64 > timestamp_of;
  std::map< Node_Skeleton::Id_Type, std::map< std::string, std::string > > unmatched_tags;
  std::map< Node_Skeleton::Id_Type, Uint31_Index > idx_by_id;
  
  std::vector< Data_By_Id< Node_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Node_Skeleton::Id_Type last_id(0ull);
  for (std::vector< Data_By_Id< Node_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;

    if (!(last_id == it->elem.id))
    {
      timestamp_of[it->elem.id] = it->meta.timestamp;
      
      // This is the oldest version of this id in this diff. If an object with this id existed
      // already before then we need to store explicit void tags for all tags that have not been
      // removed.
      const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
      if (idx && !(it->idx == Uint31_Index(0u)) && (idx->val() & 0x7fffff00) == (it->idx.val() & 0x7fffff00))
      {
        for (std::vector< std::pair< std::string, std::string > >::const_iterator tag_it = it->tags.begin();
             tag_it != it->tags.end(); ++tag_it)
          unmatched_tags[it->elem.id].insert(make_pair(tag_it->first, tag_it->second));
        idx_by_id.insert(make_pair(it->elem.id, it->idx.val() & 0x7fffff00));
      }
      else
      {
	// This is object is just going to be undeleted or moved
	// Explicitly add to undeleted all tags the object now gets
	idx = binary_pair_search(existing_attic_map_positions, it->elem.id);
	if (idx && !(it->idx == Uint31_Index(0u)))
	{
          for (std::vector< std::pair< std::string, std::string > >::const_iterator tag_it = it->tags.begin();
               tag_it != it->tags.end(); ++tag_it)
            result[Tag_Index_Local(it->idx.val() & 0x7fffff00, tag_it->first, void_tag_value())]
                .insert(Attic< Node_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
	}
      }
    }
    else if (!(it->idx == Uint31_Index(0u)))
    {
      // Compare to the tags of the preceeding version. For each unmatched tag of the new version,
      // add an explicitly void tag to the new_attic_local_tags
      std::set< std::string > old_keys;
      std::vector< Data_By_Id< Node_Skeleton >::Entry >::const_iterator last_it = it;
      --last_it;
      if ((it->idx.val() & 0x7fffff00) == (last_it->idx.val() & 0x7fffff00))
      {
        for (std::vector< std::pair< std::string, std::string > >::const_iterator
             tag_it = last_it->tags.begin(); tag_it != last_it->tags.end(); ++tag_it)
          old_keys.insert(tag_it->first);
      }
      
      for (std::vector< std::pair< std::string, std::string > >::const_iterator tag_it = it->tags.begin();
           tag_it != it->tags.end(); ++tag_it)
      {
        if (old_keys.find(tag_it->first) == old_keys.end())
          result[Tag_Index_Local(it->idx.val() & 0x7fffff00, tag_it->first, void_tag_value())]
              .insert(Attic< Node_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
      }
    }
    last_id = it->elem.id;
    
    if (it->idx == Uint31_Index(0u))
      // There is nothing to do for elements to delete. If they exist, they are contained in the
      // attic_skeletons.
      continue;
    
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version exist also in new_data. This is not a deletion.
      // So add the tags from this intermediate version.
    {
      if ((it->idx.val() & 0x7fffff00) == (next_it->idx.val() & 0x7fffff00))
        compare_and_add_different_tags
          (Attic< Node_Skeleton::Id_Type >(it->elem.id, next_it->meta.timestamp),
                 it->idx, it->tags, next_it->tags, result);
      else
        add_tags(Attic< Node_Skeleton::Id_Type >(it->elem.id, next_it->meta.timestamp),
                 it->idx, it->tags, result);
    }
  }
  
  // Copy all attic local tags to the set of all new attic local tags
  // Leave out those that have an entry in unmatched tags, because these are unchanged
  for (std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > >::const_iterator
      it_idx = attic_local_tags.begin(); it_idx != attic_local_tags.end(); ++it_idx)
  {
    std::set< Attic< Node_Skeleton::Id_Type > >& handle(result[it_idx->first]);
    for (std::set< Node_Skeleton::Id_Type >::const_iterator
        it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
    {
      std::map< std::string, std::string >::const_iterator it_unmatched
          = unmatched_tags[*it].find(it_idx->first.key);
      if (it_unmatched == unmatched_tags[*it].end() || it_unmatched->second != it_idx->first.value
          || idx_by_id.find(*it)->second.val() != it_idx->first.index)
        handle.insert(Attic< Node_Skeleton::Id_Type >(*it, timestamp_of[*it]));
    }
  }
  
  // Check which of the unmatched_keys are really unmatched.
  for (std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > >::const_iterator
      it_idx = attic_local_tags.begin(); it_idx != attic_local_tags.end(); ++it_idx)
  {
    for (std::set< Node_Skeleton::Id_Type >::const_iterator
        it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
      unmatched_tags[*it].erase(it_idx->first.key);
  }
  
  // Now copy the remaining unmatched keys to new_attic_local_tags
  for (std::map< Node_Skeleton::Id_Type, std::map< std::string, std::string > >::const_iterator
      it_id = unmatched_tags.begin(); it_id != unmatched_tags.end(); ++it_id)
  {
    for (std::map< std::string, std::string >::const_iterator it = it_id->second.begin();
         it != it_id->second.end(); ++it)
      result[Tag_Index_Local(idx_by_id.find(it_id->first)->second.val(), it->first, void_tag_value())]
          .insert(Attic< Node_Skeleton::Id_Type >(it_id->first, timestamp_of[it_id->first]));
  }
  
  return result;
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the set of elements to store to attic.
 * We use that in attic_skeletons can only appear elements with ids that exist also in new_data. */
std::map< Timestamp, std::set< Change_Entry< Node_Skeleton::Id_Type > > > compute_changelog(
    const Data_By_Id< Node_Skeleton >& new_data,
    const std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
    const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_skeletons)
{
  std::map< Timestamp, std::set< Change_Entry< Node_Skeleton::Id_Type > > > result;
  
  std::vector< Data_By_Id< Node_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Node_Skeleton::Id_Type last_id = Node_Skeleton::Id_Type(0ull);
  for (std::vector< Data_By_Id< Node_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
      // A later version exists also in new_data.
      result[next_it->meta.timestamp].insert(
          Change_Entry< Node_Skeleton::Id_Type >(it->elem.id, it->idx, next_it->idx));
    
    if (last_id == it->elem.id)
      // An earlier version exists also in new_data. So there is nothing to do here.
      continue;
    last_id = it->elem.id;
    
    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    if (!idx)
    {
      // No old data exists.
      result[it->meta.timestamp].insert(
          Change_Entry< Node_Skeleton::Id_Type >(it->elem.id, 0u, it->idx));
      continue;
    }

    std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
      // Something has gone wrong. Skip this object.
      continue;
    
    std::set< Node_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
      // Something has gone wrong. Skip this object.
      continue;
    
    result[it->meta.timestamp].insert(
        Change_Entry< Node_Skeleton::Id_Type >(it->elem.id, *idx, it->idx));
  }
  
  return result;
}
  

Node_Updater::Node_Updater(Transaction& transaction_, meta_modes meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_), keys(*osm_base_settings().NODE_KEYS)
{}

Node_Updater::Node_Updater(string db_dir_, meta_modes meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(meta_ == only_data || meta_ == keep_meta),
    db_dir(db_dir_), meta(meta_), keys(*osm_base_settings().NODE_KEYS)
{
  partial_possible = !file_exists
      (db_dir + 
       osm_base_settings().NODES->get_file_name_trunk() +
       osm_base_settings().NODES->get_data_suffix() +
       osm_base_settings().NODES->get_index_suffix());
}


void Node_Updater::update(Osm_Backend_Callback* callback, bool partial)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  // Prepare collecting all data of existing skeletons
  std::sort(new_data.data.begin(), new_data.data.end());
  remove_time_inconsistent_versions(new_data);
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
  attic_skeletons.clear();
  new_skeletons.clear();
  new_current_skeletons(new_data, existing_map_positions, existing_skeletons,
      0, attic_skeletons, new_skeletons, moved_nodes);
      
  // Compute which meta data really has changed
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > attic_meta;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_meta;
  new_current_meta(new_data, existing_map_positions, existing_meta, attic_meta, new_meta);
      
  // Compute which tags really have changed
  std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > > attic_local_tags;
  std::map< Tag_Index_Local, std::set< Node_Skeleton::Id_Type > > new_local_tags;
  new_current_local_tags< Node_Skeleton, Node_Skeleton::Id_Type >
      (new_data, existing_map_positions, existing_local_tags, attic_local_tags, new_local_tags);
  std::map< Tag_Index_Global, std::set< Tag_Object_Global< Node_Skeleton::Id_Type > > > attic_global_tags;
  std::map< Tag_Index_Global, std::set< Tag_Object_Global< Node_Skeleton::Id_Type > > > new_global_tags;
  new_current_global_tags< Node_Skeleton::Id_Type >
      (attic_local_tags, new_local_tags, attic_global_tags, new_global_tags);

  // Compute idx positions of new nodes
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_map_positions
      = new_idx_positions(new_data);
  // TODO: old code
  map< uint32, vector< Node::Id_Type > > to_delete;
  update_node_ids(to_delete, 0, new_map_positions);

  callback->update_started();
  callback->prepare_delete_tags_finished();
  
  store_new_keys(new_data, keys, *transaction);
  
  // Update id indexes
  update_map_positions(new_map_positions, *transaction, *osm_base_settings().NODES);
  callback->update_ids_finished();
  
  // Update skeletons
  update_elements(attic_skeletons, new_skeletons, *transaction, *osm_base_settings().NODES);
  callback->update_coords_finished();
  
  // Update meta
  if (meta)
    update_elements(attic_meta, new_meta, *transaction, *meta_settings().NODES_META);
  
  // Update local tags
  update_elements(attic_local_tags, new_local_tags, *transaction, *osm_base_settings().NODE_TAGS_LOCAL);
  callback->tags_local_finished();
  
  // Update global tags
  update_elements(attic_global_tags, new_global_tags, *transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
  callback->tags_global_finished();

  map< uint32, vector< uint32 > > idxs_by_id;
  
  if (meta == keep_attic)
  {
    // TODO: For compatibility with the update_logger, this doesn't happen during the tag processing itself.
    //cancel_out_equal_tags(attic_local_tags, new_local_tags);
    
    // Collect all data of existing attic id indexes
    std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_attic_map_positions
        = get_existing_map_positions(ids_to_update_, *transaction, *attic_settings().NODES);
    std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists
        = get_existing_idx_lists(ids_to_update_, existing_attic_map_positions,
                                 *transaction, *attic_settings().NODE_IDX_LIST);

    // Collect known change times of attic elements. This allows that
    // for each object no older version than the youngest known attic version can be written
    std::map< Node_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Node_Skeleton > > >
        existing_attic_skeleton_timestamps
        = get_existing_attic_skeleton_timestamps< Uint31_Index, Node_Skeleton, Node_Skeleton >
            (existing_attic_map_positions, existing_idx_lists,
	     *transaction, *attic_settings().NODES, *attic_settings().NODES_UNDELETED);
        
    // Compute which objects really have changed
    new_attic_skeletons.clear();
    std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists = existing_idx_lists;
    compute_new_attic_skeletons(new_data, existing_map_positions, attic_skeletons,
				existing_attic_skeleton_timestamps,
                                new_attic_skeletons, new_attic_idx_lists);
    
    std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > new_undeleted
        = compute_undeleted_skeletons(new_data, existing_map_positions, existing_idx_lists);
    
    strip_single_idxs(existing_idx_lists);
    std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_attic_map_positions
        = strip_single_idxs(new_attic_idx_lists);

    compute_new_attic_meta(new_data, existing_map_positions, attic_meta);
    
    // Compute tags
    std::map< Tag_Index_Local, std::set< Attic< Node_Skeleton::Id_Type > > > new_attic_local_tags
        = compute_new_attic_local_tags(new_data,
	    existing_map_positions, existing_attic_map_positions, attic_local_tags);
    std::map< Tag_Index_Global, std::set< Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > > >
        new_attic_global_tags = compute_attic_global_tags(new_attic_local_tags);
    
    // Compute changelog
    std::map< Timestamp, std::set< Change_Entry< Node_Skeleton::Id_Type > > > changelog
        = compute_changelog(new_data, existing_map_positions, attic_skeletons);
    
    // Prepare user indices
    copy_idxs_by_id(attic_meta, idxs_by_id);
    
    // Update id indexes
    update_map_positions(new_attic_map_positions, *transaction, *attic_settings().NODES);
  
    // Update id index lists
    update_elements(existing_idx_lists, new_attic_idx_lists,
                    *transaction, *attic_settings().NODE_IDX_LIST);
  
    // Add attic elements
    update_elements(std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >(), new_attic_skeletons,
                    *transaction, *attic_settings().NODES);
  
    // Add attic elements
    update_elements(std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > >(),
                    new_undeleted, *transaction, *attic_settings().NODES_UNDELETED);
  
    // Add attic meta
    update_elements
        (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >(),
         attic_meta, *transaction, *attic_settings().NODES_META);
  
    // Update tags
    update_elements(std::map< Tag_Index_Local, std::set< Attic < Node_Skeleton::Id_Type > > >(),
                    new_attic_local_tags, *transaction, *attic_settings().NODE_TAGS_LOCAL);
    update_elements(std::map< Tag_Index_Global,
                    std::set< Attic < Tag_Object_Global< Node_Skeleton::Id_Type > > > >(),
                    new_attic_global_tags, *transaction, *attic_settings().NODE_TAGS_GLOBAL);
    
    // Write changelog
    update_elements(std::map< Timestamp, std::set< Change_Entry< Node_Skeleton::Id_Type > > >(), changelog,
                    *transaction, *attic_settings().NODE_CHANGELOG);
  }
      
  if (meta != only_data)
  {
    copy_idxs_by_id(new_meta, idxs_by_id);
    process_user_data(*transaction, user_by_id, idxs_by_id);
  }
  callback->update_finished();
  
  new_data.data.clear();
  ids_to_modify.clear();
  nodes_to_insert.clear();
//   nodes_meta_to_insert.clear();
//   nodes_meta_to_delete.clear();

  if (!external_transaction)
    delete transaction;
  
  if (partial_possible)
  {
    new_skeletons.clear();
    attic_skeletons.clear();
    new_attic_skeletons.clear();
  }
  
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
  
  Random_File< Node_Skeleton::Id_Type, Uint32_Index > random
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
  ::merge_files< Tag_Index_Global, Tag_Object_Global< Node::Id_Type > >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
  if (meta)
  {
    ::merge_files< Uint31_Index, OSM_Element_Metadata_Skeleton< Node::Id_Type > >
        (from_transactions, into_transaction, *meta_settings().NODES_META);
  }
}
