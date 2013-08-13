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


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the deletion and insertion lists for the
 * database operation.  Also, the list of moved nodes is filled. */
template< typename Element_Skeleton >
void new_current_skeletons
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::vector< std::pair< typename Element_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& existing_skeletons,
     bool record_minuscule_moves,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Element_Skeleton > >& new_skeletons,
     vector< pair< Node::Id_Type, Uint32_Index > >& moved_nodes)
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
      moved_nodes.push_back(make_pair(it->elem.id, Uint32_Index(idx->val())));
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::map< Uint31_Index, std::set< Element_Skeleton > >::iterator it_attic_idx
        = attic_skeletons.find(it->idx);
    if (it_attic_idx == attic_skeletons.end())
    {
      // Something has gone wrong. Save at least the new node.
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    typename std::set< Element_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
    {
      // Something has gone wrong. Save at least the new node.
      new_skeletons[it->idx].insert(it->elem);
      continue;
    }
    
    // We have found an element at the same index with the same id, so this is a candidate for
    // not being moved.
    if (geometrically_equal(it->elem, *it_attic))
      // The element stays unchanged.
      it_attic_idx->second.erase(it_attic);
    else
    {
      new_skeletons[it->idx].insert(it->elem);
      if (record_minuscule_moves)
        moved_nodes.push_back(make_pair(it->elem.id, Uint32_Index(idx->val())));
    }
  }
}


template< typename Element_Skeleton, typename Update_Logger >
void update_skeletons
    (const std::map< Uint31_Index, std::set< Element_Skeleton > >& attic_skeletons,
     const std::map< Uint31_Index, std::set< Element_Skeleton > >& new_skeletons,
     Transaction& transaction, const File_Properties& file_properties,
     Update_Logger* update_logger)
{
  Block_Backend< Uint31_Index, Element_Skeleton > node_db
      (transaction.data_index(&file_properties));
  if (update_logger)
    node_db.update(attic_skeletons, new_skeletons, *update_logger);
  else
    node_db.update(attic_skeletons, new_skeletons);
}


/* Szenarien:
 * - nur Basisdaten
 * - auch aktuelle Metadaten
 * - auch Historie
 * Update-Log kann immer geschrieben werden.
 */
  
  // Wir haben vorab: Skeleton, Tags, Meta der neuen und Zwischenversionen
  // Für die Planung, welche Elemente eingefügt werden müssen, reicht diese Information aus
  // typedef vmap< (Id, version), (Idx, Skel, Meta, Tags) > Data_Dict
  
  // vmap< Id > ids_to_update(Data_Dict new_data)
  
  // vmap < Id, Idx > get_exisiting_map_positions(vmap< Id >)
  
  // map< Idx, set< Skel > > get_existing_skeletons(vmap < Id, Idx >)

  // vmap < Id, Idx > new_idx_positions(Data_Dict new_data, map< Idx, set< Skel > >)
  
  // update_map_positions(vmap < Id, Idx >)
  
  // pair< map< Idx, set< Skel > >, map< Idx, set< Skel > > >
  //     new_current_skeletons(Data_Dict new_data, map< Idx, set< Skel > >)
  
  // update_skeletons(map< Idx, set< Skel > > delete, map< Idx, set< Skel > > insert)

  // == meta ==
  
  // map< Idx, set< Meta > > get_existing_meta(vmap < Id, Idx >)

  // pair< map< Idx, set< Meta > >, map< Idx, set< Meta > > >
  //     new_current_meta(Data_Dict new_data, map< Idx, set< Meta > >)
  
  // update_meta(map< Idx, set< Meta > > delete, map< Idx, set< Meta > > insert)
  
  // == tags ==
  
  // map< (Idx, key, value), vec< Id > > get_existing_local_tags(vmap < Id, Idx >)

  // pair< map< (Idx, key, value), vec< Id > >, map< (Idx, key, value), vec< Id > > >
  //     new_current_local_tags(Data_Dict new_data, map< Idx, set< Skel > >)
  
  // update_local_tags(map< (Idx, key, value), vec< Id > > delete, map< (Idx, key, value), vec< Id > > insert)
  
  // pair< map< (key, value), vec< Id > >, map< (key, value), vec< Id > > > new_current_global_tags
  //    (map< (Idx, key, value), vec< Id > > delete, map< (Idx, key, value), vec< Id > > insert)
  
  // update_global_tags(map< (Idx, key, value), vec< Id > > delete, map< (Idx, key, value), vec< Id > > insert)
  
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
  

Node_Updater::Node_Updater(Transaction& transaction_, bool meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_)
{}

Node_Updater::Node_Updater(string db_dir_, bool meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(true), db_dir(db_dir_), meta(meta_)
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
  
  std::sort(new_data.data.begin(), new_data.data.end());  
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_idx_positions_
      = new_idx_positions(new_data);
  
  std::vector< Node_Skeleton::Id_Type > ids_to_update_ = ids_to_update(new_data);
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_map_positions
      = get_existing_map_positions(ids_to_update_, *transaction, *osm_base_settings().NODES);
  std::map< Uint31_Index, std::set< Node_Skeleton > > existing_skeletons
      = get_existing_skeletons< Node_Skeleton >
      (existing_map_positions, *transaction, *osm_base_settings().NODES);

  std::map< Uint31_Index, std::set< Node_Skeleton > > attic_skeletons;
  std::map< Uint31_Index, std::set< Node_Skeleton > > new_skeletons;
  new_current_skeletons(new_data, existing_map_positions, existing_skeletons,
      (update_logger != 0), attic_skeletons, new_skeletons, moved_nodes);
  
  map< uint32, vector< Node::Id_Type > > to_delete;
  callback->update_started();
  update_node_ids(to_delete, (update_logger != 0), new_idx_positions_);
  callback->update_ids_finished();
  update_coords(to_delete, update_logger);
  callback->update_coords_finished();

  update_map_positions(new_idx_positions_, *transaction, *osm_base_settings().NODES);
  update_skeletons(attic_skeletons, new_skeletons, *transaction, *osm_base_settings().NODES, update_logger);
  
  new_data.data.clear();
  
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
  
  Node_Comparator_By_Id node_comparator_by_id;
  Node_Equal_Id node_equal_id;
  
  vector< Tag_Entry< Node::Id_Type > > tags_to_delete;
  prepare_delete_tags(*transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL),
		      tags_to_delete, to_delete);
  callback->prepare_delete_tags_finished();
  vector< Node* > nodes_ptr = sort_elems_to_insert
      (nodes_to_insert, node_comparator_by_id, node_equal_id);
  update_tags_local(*transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL),
		    nodes_ptr, ids_to_modify, tags_to_delete, update_logger);
  callback->tags_local_finished();
  update_tags_global(*transaction->data_index(osm_base_settings().NODE_TAGS_GLOBAL),
		     nodes_ptr, ids_to_modify, tags_to_delete);
  callback->tags_global_finished();
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(nodes_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().NODES_META),
		      nodes_meta_to_insert, ids_to_modify, to_delete, update_logger);
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
  stable_sort
      (nodes_to_insert.begin(), nodes_to_insert.end(), node_comparator_by_id);
  vector< Node >::iterator nodes_begin
      (unique(nodes_to_insert.rbegin(), nodes_to_insert.rend(), node_equal_id)
       .base());
  nodes_to_insert.erase(nodes_to_insert.begin(), nodes_begin);
  
//   uint put_count = 0;
  
  Random_File< Uint32_Index > random
      (transaction->random_index(osm_base_settings().NODES));
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< Node::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    Uint32_Index index(random.get(it->first.val()));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
//         ++put_count;
//         std::cerr<<(binary_search(new_idx_positions.begin(), new_idx_positions.end(),
//             make_pair(it->first, Uint31_Index(nit->index)), Idx_Agnostic_Compare< Node_Skeleton::Id_Type >()));
//         if (put_count % 100 == 0)
//           std::cerr<<'\n';
	//random.put(it->first.val(), Uint32_Index(nit->index));
	if ((index.val() > 0) &&
	    (index.val() != nit->index || record_minuscule_moves))
	  ;//moved_nodes.push_back(make_pair(it->first, index));
      }
      ++nit;
    }
  }
  
//   std::cerr<<(new_idx_positions.size() == put_count)<<'\n';
  
  sort(moved_nodes.begin(), moved_nodes.end());
}

void Node_Updater::update_coords(const map< uint32, vector< Node::Id_Type > >& to_delete,
				 Update_Node_Logger* update_logger)
{
  map< Uint32_Index, set< Node_Skeleton > > db_to_delete;
  map< Uint32_Index, set< Node_Skeleton > > db_to_insert;
  
  for (map< uint32, vector< Node::Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint32_Index idx(it->first);
    for (vector< Node::Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    db_to_delete[idx].insert(Node_Skeleton(*it2, 0));
  }
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< Node::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	Uint32_Index idx(nit->index);
	db_to_insert[idx].insert(Node_Skeleton(*nit));
	if (update_logger)
          update_logger->insertion(*nit);	  
      }
      ++nit;
    }
  }
  
//   Block_Backend< Uint32_Index, Node_Skeleton > node_db
//       (transaction->data_index(osm_base_settings().NODES));
//   if (update_logger)
//     node_db.update(db_to_delete, db_to_insert, *update_logger);
//   else
//     node_db.update(db_to_delete, db_to_insert);
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
