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

#include <sys/stat.h>
#include <cstdio>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "meta_updater.h"
#include "tags_updater.h"
#include "way_updater.h"

using namespace std;


Update_Way_Logger::~Update_Way_Logger()
{
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator it = insert.begin();
      it != insert.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator it = keep.begin();
      it != keep.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator it = erase.begin();
      it != erase.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
}


Way_Updater::Way_Updater(Transaction& transaction_, bool meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_)
{}

Way_Updater::Way_Updater(string db_dir_, bool meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(true), db_dir(db_dir_), meta(meta_)
{
  partial_possible = !file_exists
      (db_dir + 
       osm_base_settings().WAYS->get_file_name_trunk() +
       osm_base_settings().WAYS->get_data_suffix() +
       osm_base_settings().WAYS->get_index_suffix());
}

namespace
{
  Way_Comparator_By_Id way_comparator_by_id;
  Way_Equal_Id way_equal_id;
}


// Geupdatete Wege separat laden, alte Fassung, und separat behalten

// Für geupdatete Wege altes Meta kopieren

// Für geupdatete Wege Tags kopieren

// Neueste Versionen aus new_data, mit Index auf Basis der neuesten Nodes
// zuästzlich neue Fassungen der verschobenen Ways

// Indexe von Skeletons. Duplizierung von Versionen ebenso.
// Wenn Meta aktiv, soll zu jedem Skeleton genau eine neue Meta-Version existieren

//   // Compute which meta data really has changed
//   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > attic_meta;
//   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_meta;
//   new_current_meta(new_data, existing_map_positions, existing_meta, attic_meta, new_meta);

// Auf Basis der Skeletons und geladenen Tags
// Neue Tags aus new_data, wenn dort mind. eine Version vorliegt, sonst aus geladenen Tags

//   std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_map_positions
//       = new_idx_positions(new_data);
//   // TODO: old code
//      ...
//   
//   if (meta == keep_attic)
//   {
//     // TODO: For compatibility with the update_logger, this doesn't happen during the tag processing itself.
//     //cancel_out_equal_tags(attic_local_tags, new_local_tags);
// 
//     // Collect all data of existing attic id indexes
//     std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_attic_map_positions
//         = get_existing_map_positions(ids_to_update_, *transaction, *attic_settings().NODES);
//     std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists
//         = get_existing_idx_lists(ids_to_update_, existing_attic_map_positions,
//                                  *transaction, *attic_settings().NODE_IDX_LIST);

// Älteste Version ist immer alte Nodes + altes Skeleton
// Neueste Version ist schon geklärt
// Dazwischen: anhand der timestamps ausrechnen!
//
// Aus exakter Zustandfolge leiten sich auch Tags und Meta her:
// Beides jeweils kopieren für indirekt geänderte Wege

//     // Compute which objects really have changed
//     std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > new_attic_skeletons;
//     std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists = existing_idx_lists;
//     compute_new_attic_skeletons(new_data, existing_map_positions, attic_skeletons,
//                                 new_attic_skeletons, new_attic_idx_lists);
//     
//     std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > new_undeleted
//         = compute_undeleted_skeletons(new_data, existing_map_positions, existing_attic_map_positions);
//     
//     strip_single_idxs(existing_idx_lists);
//     std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_attic_map_positions
//         = strip_single_idxs(new_attic_idx_lists);
// 
//     compute_new_attic_meta(new_data, existing_map_positions, attic_meta);
//     
//     // Compute tags
//     std::map< Tag_Index_Local, std::set< Attic< Node_Skeleton::Id_Type > > > new_attic_local_tags
//         = compute_new_attic_local_tags(new_data, existing_map_positions, attic_local_tags);
//     std::map< Tag_Index_Global, std::set< Attic< Node_Skeleton::Id_Type > > > new_attic_global_tags
//         = compute_attic_global_tags(new_attic_local_tags);
//     
//     // Compute changelog
//     std::map< Timestamp, std::set< Change_Entry< Node_Skeleton::Id_Type > > > changelog
//         = compute_changelog(new_skeletons, new_attic_skeletons,
//                             new_local_tags, new_attic_local_tags,
//                             new_meta, attic_meta);
//     
//     // Update id indexes
//     update_map_positions(new_attic_map_positions, *transaction, *attic_settings().NODES);
//   
//     // Update id index lists
//     update_elements(existing_idx_lists, new_attic_idx_lists,
//                     *transaction, *attic_settings().NODE_IDX_LIST);
//   
//     // Add attic elements
//     update_elements(std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >(), new_attic_skeletons,
//                     *transaction, *attic_settings().NODES);
//   
//     // Add attic elements
//     update_elements(std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > >(),
//                     new_undeleted, *transaction, *attic_settings().NODES_UNDELETED);
//   
//     // Add attic meta
//     update_elements
//         (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >(),
//          attic_meta, *transaction, *attic_settings().NODES_META);
//   
//     // Update tags
//     update_elements(std::map< Tag_Index_Local, std::set< Attic < Node_Skeleton::Id_Type > > >(),
//                     new_attic_local_tags, *transaction, *attic_settings().NODE_TAGS_LOCAL);
//     update_elements(std::map< Tag_Index_Global, std::set< Attic < Node_Skeleton::Id_Type > > >(),
//                     new_attic_global_tags, *transaction, *attic_settings().NODE_TAGS_GLOBAL);
//     
//     // Write changelog
//     update_elements(std::map< Timestamp, std::set< Change_Entry< Node_Skeleton::Id_Type > > >(), changelog,
//                     *transaction, *attic_settings().NODE_CHANGELOG);
//   }
//       
//   //TODO: old code
//   if (meta != only_data)
//   {
//     map< uint32, vector< uint32 > > idxs_by_id;
//     create_idxs_by_id(nodes_meta_to_insert, idxs_by_id);
//     process_user_data(*transaction, user_by_id, idxs_by_id);
//     
//     if (update_logger)
//     {
//       stable_sort(nodes_meta_to_delete.begin(), nodes_meta_to_delete.begin());
//       nodes_meta_to_delete.erase(unique(nodes_meta_to_delete.begin(), nodes_meta_to_delete.end()),
//                                  nodes_meta_to_delete.end());
//       update_logger->set_delete_meta_data(nodes_meta_to_delete);
//       nodes_meta_to_delete.clear();
//     }
//   }
//   callback->update_finished();


std::map< Uint31_Index, std::set< Way_Skeleton > > get_implicitly_moved_skeletons
    (const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_nodes,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& already_known_skeletons,
     Transaction& transaction, const File_Properties& file_properties)
{
  std::set< Uint31_Index > node_req;
  for (std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator
      it = attic_nodes.begin(); it != attic_nodes.end(); ++it)
    node_req.insert(it->first);
  std::set< Uint31_Index > req = calc_parents(node_req);
  
  std::vector< Node_Skeleton::Id_Type > node_ids;
  for (std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator
      it = attic_nodes.begin(); it != attic_nodes.end(); ++it)
  {
    for (std::set< Node_Skeleton >::const_iterator nit = it->second.begin(); nit != it->second.end(); ++nit)
      node_ids.push_back(nit->id);
  }
  std::sort(node_ids.begin(), node_ids.end());
  node_ids.erase(std::unique(node_ids.begin(), node_ids.end()), node_ids.end());
  
  std::vector< Way_Skeleton::Id_Type > known_way_ids;
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = already_known_skeletons.begin(); it != already_known_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator wit = it->second.begin(); wit != it->second.end(); ++wit)
      known_way_ids.push_back(wit->id);
  }
  std::sort(known_way_ids.begin(), known_way_ids.end());
  known_way_ids.erase(std::unique(known_way_ids.begin(), known_way_ids.end()), known_way_ids.end());
  
  std::map< Uint31_Index, std::set< Way_Skeleton > > result;
  
  Block_Backend< Uint31_Index, Way_Skeleton > db(transaction.data_index(&file_properties));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (binary_search(known_way_ids.begin(), known_way_ids.end(), it.object().id))
      continue;
    for (vector< Node::Id_Type >::const_iterator nit = it.object().nds.begin();
         nit != it.object().nds.end(); ++nit)
    {
      if (binary_search(node_ids.begin(), node_ids.end(), *nit))
      {
        result[it.index()].insert(it.object());
        break;
      }
    }
  }

  return result;
}


std::map< Node_Skeleton::Id_Type, Quad_Coord > dictionary_from_skeletons
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


/* Adds the implicity known Quad_Coords from the given ways for nodes not yet known in
 * new_node_idx_by_id */
void add_implicity_known_nodes
    (std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& known_skeletons)
{
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = known_skeletons.begin();
       it != known_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (!it2->geometry.empty())
      {
        for (vector< Quad_Coord >::size_type i = 0; i < it2->geometry.size(); ++i)
          // Choose std::map::insert to only insert if the id doesn't exist yet.
          new_node_idx_by_id.insert(make_pair(it2->nds[i], it2->geometry[i]));
      }
    }
  }  
}


void lookup_missing_nodes
    (std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const Data_By_Id< Way_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& known_skeletons,
     Transaction& transaction)
{
  std::vector< Node_Skeleton::Id_Type > missing_ids;
  
  for (std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;
    
    vector< uint32 > nd_idxs;
    for (vector< Node::Id_Type >::const_iterator nit = it->elem.nds.begin(); nit != it->elem.nds.end(); ++nit)
    {
      if (new_node_idx_by_id.find(*nit) == new_node_idx_by_id.end())
        missing_ids.push_back(*nit);
    }
  }
  
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = known_skeletons.begin();
       it != known_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      for (vector< Node::Id_Type >::const_iterator nit = it2->nds.begin(); nit != it2->nds.end(); ++nit)
      {
        if (new_node_idx_by_id.find(*nit) == new_node_idx_by_id.end())
          missing_ids.push_back(*nit);
      }
    }
  }
  
  std::sort(missing_ids.begin(), missing_ids.end());
  missing_ids.erase(std::unique(missing_ids.begin(), missing_ids.end()), missing_ids.end());
  
  // Collect all data of existing id indexes
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_map_positions
      = get_existing_map_positions(missing_ids, transaction, *osm_base_settings().NODES);
  
  // Collect all data of existing skeletons
  std::map< Uint31_Index, std::set< Node_Skeleton > > existing_skeletons
      = get_existing_skeletons< Node_Skeleton >
      (existing_map_positions, transaction, *osm_base_settings().NODES);
      
  for (std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator it = existing_skeletons.begin();
       it != existing_skeletons.end(); ++it)
  {
    for (std::set< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      new_node_idx_by_id.insert(make_pair(it2->id, Quad_Coord(it->first.val(), it2->ll_lower)));
  }
}


std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > make_id_idx_directory
    (const std::map< Uint31_Index, std::set< Way_Skeleton > >& implicitly_moved_skeletons)
{
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > result;
  Pair_Comparator_By_Id< Way_Skeleton::Id_Type, Uint31_Index > less;
  
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
       it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      result.push_back(make_pair(it2->id, it->first));
  }
  std::sort(result.begin(), result.end(), less);
  
  return result;
}


/* We assert that every node id that appears in a way in existing_skeletons has its Quad_Coord
   in new_node_idx_by_id. */
void compute_geometry
    (const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     Data_By_Id< Way_Skeleton >& new_data)
{
  for (std::vector< Data_By_Id< Way_Skeleton >::Entry >::iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;
    
    vector< uint32 > nd_idxs;
    for (vector< Node::Id_Type >::const_iterator nit = it->elem.nds.begin(); nit != it->elem.nds.end(); ++nit)
    {
      std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2 = new_node_idx_by_id.find(*nit);
      if (it2 != new_node_idx_by_id.end())
        nd_idxs.push_back(it2->second.ll_upper);
    }
    
    Uint31_Index index = Way::calc_index(nd_idxs);
      
    if (Way::indicates_geometry(index))
    {
      it->elem.geometry.clear();
        
      for (vector< Node::Id_Type >::const_iterator nit = it->elem.nds.begin();
           nit != it->elem.nds.end(); ++nit)
      {
        std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2 = new_node_idx_by_id.find(*nit);
        if (it2 != new_node_idx_by_id.end())
          it->elem.geometry.push_back(it2->second);
        else
          //TODO: throw an error in an appropriate form
          it->elem.geometry.push_back(Quad_Coord(0, 0));          
      }
    }
    if (!nd_idxs.empty())
      it->idx = index;
  }
}


// TODO: temporary helper function for update_logger
void tell_update_logger_insertions
    (const typename Data_By_Id< Way_Skeleton >::Entry& entry, Update_Way_Logger* update_logger)
{
  if (update_logger)
  {
    Way way(entry.elem.id.val(), entry.idx.val(), entry.elem.nds);
    way.tags = entry.tags;
    update_logger->insertion(way);
  }
}


/* Adds to attic_skeletons and new_skeletons all those ways that have moved just because
   a node in these ways has moved.
   We assert that every node id that appears in a way in existing_skeletons has its Quad_Coord
   in new_node_idx_by_id. */
void new_implicit_skeletons
    (const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& existing_skeletons,
     bool record_minuscule_moves,
     std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Way_Skeleton > >& new_skeletons,
     vector< pair< Way::Id_Type, Uint31_Index > >& moved_ways,
     Update_Way_Logger* update_logger)
{
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = existing_skeletons.begin();
       it != existing_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      attic_skeletons[it->first].insert(*it2);
  }

  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = existing_skeletons.begin();
       it != existing_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      vector< uint32 > nd_idxs;
      for (vector< Node::Id_Type >::const_iterator nit = it2->nds.begin(); nit != it2->nds.end(); ++nit)
      {
        std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it3 = new_node_idx_by_id.find(*nit);
        if (it3 != new_node_idx_by_id.end())
          nd_idxs.push_back(it3->second.ll_upper);
      }
    
      Uint31_Index index = Way::calc_index(nd_idxs);
      if (nd_idxs.empty())
        index = 0xff;
      
      if (Way::indicates_geometry(index))
      {
        Way_Skeleton new_skeleton = *it2;
        new_skeleton.geometry.clear();
        
        for (vector< Node::Id_Type >::const_iterator nit = it2->nds.begin(); nit != it2->nds.end(); ++nit)
        {
          std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it3 = new_node_idx_by_id.find(*nit);
          if (it3 != new_node_idx_by_id.end())
            new_skeleton.geometry.push_back(it3->second);
          else
            //TODO: throw an error in an appropriate form
            new_skeleton.geometry.push_back(Quad_Coord(0, 0));
        }
        
        new_skeletons[index].insert(new_skeleton);
      }
      else
        new_skeletons[index].insert(*it2);
    }
  }
}


/* Adds to attic_local_tags and new_local_tags the tags to delete resp. add from only
   implicitly moved ways. */
void new_implicit_local_tags
    (const std::vector< Tag_Entry< Way_Skeleton::Id_Type > >& existing_local_tags,
     const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& new_positions,
     std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > >& attic_local_tags,
     std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > >& new_local_tags)
{
  //TODO: convert the data format until existing_local_tags get the new data format
  for (typename std::vector< Tag_Entry< Way_Skeleton::Id_Type > >::const_iterator
      it_idx = existing_local_tags.begin(); it_idx != existing_local_tags.end(); ++it_idx)
  {
    std::set< Way_Skeleton::Id_Type >& handle(attic_local_tags[*it_idx]);
    for (typename std::vector< Way_Skeleton::Id_Type >::const_iterator it = it_idx->ids.begin();
         it != it_idx->ids.end(); ++it)
      handle.insert(*it);
  }

  for (typename std::vector< Tag_Entry< Way_Skeleton::Id_Type > >::const_iterator
      it_idx = existing_local_tags.begin(); it_idx != existing_local_tags.end(); ++it_idx)
  {
    for (typename std::vector< Way_Skeleton::Id_Type >::const_iterator it = it_idx->ids.begin();
         it != it_idx->ids.end(); ++it)
    {
      const Uint31_Index* idx = binary_pair_search(new_positions, *it);
      if (idx)
        new_local_tags[Tag_Index_Local(idx->val() & 0x7fffff00, it_idx->key, it_idx->value)].insert(*it);
    }
  }  
}


void Way_Updater::update__(Osm_Backend_Callback* callback, bool partial,
              Update_Way_Logger* update_logger,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_node_skeletons)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");

  // Prepare collecting all data of existing skeletons
  std::sort(new_data.data.begin(), new_data.data.end());
  std::vector< Way_Skeleton::Id_Type > ids_to_update_ = ids_to_update(new_data);
  
  // Collect all data of existing id indexes
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > existing_map_positions
      = get_existing_map_positions(ids_to_update_, *transaction, *osm_base_settings().WAYS);
  
  // Collect all data of existing and explicitly changed skeletons
  std::map< Uint31_Index, std::set< Way_Skeleton > > existing_skeletons
      = get_existing_skeletons< Way_Skeleton >
      (existing_map_positions, *transaction, *osm_base_settings().WAYS);
  
  // Collect also all data of existing and implicitly changed skeletons
  std::map< Uint31_Index, std::set< Way_Skeleton > > implicitly_moved_skeletons
      = get_implicitly_moved_skeletons
          (attic_node_skeletons, existing_skeletons, *transaction, *osm_base_settings().WAYS);
          
//   // Collect all data of existing meta elements
//   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way::Id_Type > > > existing_meta
//       = (meta ? get_existing_meta< OSM_Element_Metadata_Skeleton< Way::Id_Type > >
//              (existing_map_positions, *transaction, *meta_settings().WAYS_META) :
//          std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way::Id_Type > > >());

  // Collect all data of existing tags
  std::vector< Tag_Entry< Way_Skeleton::Id_Type > > existing_local_tags;
  get_existing_tags< Way_Skeleton::Id_Type >
      (existing_map_positions, *transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
       existing_local_tags);
      
  // Collect all data of existing tags for moved ways
  std::vector< Tag_Entry< Way_Skeleton::Id_Type > > implicitly_moved_local_tags;
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > implicitly_moved_positions
      = make_id_idx_directory(implicitly_moved_skeletons);
  get_existing_tags< Way_Skeleton::Id_Type >
      (implicitly_moved_positions, *transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
       implicitly_moved_local_tags);

  // Create a node directory id to idx:
  // Evaluate first the new_node_skeletons
  std::map< Node_Skeleton::Id_Type, Quad_Coord > new_node_idx_by_id
      = dictionary_from_skeletons(new_node_skeletons);
  // Then add all nodes known from existing_skeletons geometry.
  add_implicity_known_nodes(new_node_idx_by_id, existing_skeletons);
  // Then add all nodes known from implicitly_moved_skeletons geometry.
  add_implicity_known_nodes(new_node_idx_by_id, implicitly_moved_skeletons);
  // Then lookup the missing nodes.
  lookup_missing_nodes(new_node_idx_by_id, new_data, implicitly_moved_skeletons, *transaction);
  
  // Compute the indices of the new ways
  compute_geometry(new_node_idx_by_id, new_data);

  // Compute which objects really have changed
  std::map< Uint31_Index, std::set< Way_Skeleton > > attic_skeletons;
  std::map< Uint31_Index, std::set< Way_Skeleton > > new_skeletons;
  new_current_skeletons(new_data, existing_map_positions, existing_skeletons,
      (update_logger != 0), attic_skeletons, new_skeletons, moved_ways, update_logger);
  
  // Compute and add implicitly moved ways
  new_implicit_skeletons(new_node_idx_by_id, implicitly_moved_skeletons,
      (update_logger != 0), attic_skeletons, new_skeletons, moved_ways, update_logger);

  // Compute which tags really have changed
  std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > attic_local_tags;
  std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > new_local_tags;
  new_current_local_tags< Way_Skeleton, Update_Way_Logger, Way_Skeleton::Id_Type >
      (new_data, existing_map_positions, existing_local_tags, attic_local_tags, new_local_tags);
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > new_positions
      = make_id_idx_directory(new_skeletons);
  new_implicit_local_tags(implicitly_moved_local_tags, new_positions, attic_local_tags, new_local_tags);
  std::map< Tag_Index_Global, std::set< Way_Skeleton::Id_Type > > attic_global_tags;
  std::map< Tag_Index_Global, std::set< Way_Skeleton::Id_Type > > new_global_tags;
  new_current_global_tags< Way_Skeleton::Id_Type >
      (attic_local_tags, new_local_tags, attic_global_tags, new_global_tags);
  
  std::cout<<"Attic:\n";
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = attic_skeletons.begin();
       it != attic_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      std::cout<<it2->id.val()<<'\t'<<hex<<it->first.val()<<'\t'<<dec<<it2->nds.size()<<'\t'<<it2->geometry.size()<<'\n';
  }
  
  std::cout<<"\nNew:\n";
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = new_skeletons.begin();
       it != new_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      std::cout<<it2->id.val()<<'\t'<<hex<<it->first.val()<<'\t'<<dec<<it2->nds.size()<<'\t'<<it2->geometry.size()<<'\n';
  }
  
  std::cout<<"\nAttic local tags:\n";  
  for (std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > >::const_iterator
      it = attic_local_tags.begin(); it != attic_local_tags.end(); ++it)
  {
    for (std::set< Way_Skeleton::Id_Type >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      std::cout<<dec<<it2->val()<<'\t'<<hex<<it->first.index<<'\t'<<it->first.key<<'\t'<<it->first.value<<'\n';
  }
  
  std::cout<<"\nNew local tags:\n";  
  for (std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > >::const_iterator
      it = new_local_tags.begin(); it != new_local_tags.end(); ++it)
  {
    for (std::set< Way_Skeleton::Id_Type >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      std::cout<<dec<<it2->val()<<'\t'<<hex<<it->first.index<<'\t'<<it->first.key<<'\t'<<it->first.value<<'\n';
  }
  
  std::cout<<"\nAttic global tags:\n";  
  for (std::map< Tag_Index_Global, std::set< Way_Skeleton::Id_Type > >::const_iterator
      it = attic_global_tags.begin(); it != attic_global_tags.end(); ++it)
  {
    for (std::set< Way_Skeleton::Id_Type >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      std::cout<<dec<<it2->val()<<'\t'<<it->first.key<<'\t'<<it->first.value<<'\n';
  }
  
  std::cout<<"\nNew global tags:\n";  
  for (std::map< Tag_Index_Global, std::set< Way_Skeleton::Id_Type > >::const_iterator
      it = new_global_tags.begin(); it != new_global_tags.end(); ++it)
  {
    for (std::set< Way_Skeleton::Id_Type >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      std::cout<<dec<<it2->val()<<'\t'<<it->first.key<<'\t'<<it->first.value<<'\n';
  }
  
  if (!external_transaction)
    delete transaction;  
}


//TODO: Kombination aus partial update und impliziten Wegen
void Way_Updater::update(Osm_Backend_Callback* callback, bool partial,
	      Update_Way_Logger* update_logger,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_node_skeletons)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  // --------------------------------------------------------------------------
    
  map< uint32, vector< Way::Id_Type > > to_delete;
  callback->update_started();
  vector< Way* > ways_ptr = sort_elems_to_insert
      (ways_to_insert, way_comparator_by_id, way_equal_id);
  compute_indexes(ways_ptr);
  callback->compute_indexes_finished();
  
  update_way_ids(ways_ptr, to_delete, (update_logger != 0));
  callback->update_ids_finished();
  update_members(ways_ptr, to_delete, update_logger);
  callback->update_coords_finished();
  
  if (update_logger && meta)
  {
    for (vector< pair< OSM_Element_Metadata_Skeleton< Way::Id_Type >, uint32 > >::const_iterator
        it = ways_meta_to_insert.begin(); it != ways_meta_to_insert.end(); ++it)
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
  
  vector< Tag_Entry< Way::Id_Type > > tags_to_delete;
  prepare_delete_tags(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
		      tags_to_delete, to_delete);
  callback->prepare_delete_tags_finished();
  update_tags_local(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
		    ways_ptr, ids_to_modify, tags_to_delete, update_logger);
  callback->tags_local_finished();
  update_tags_global(*transaction->data_index(osm_base_settings().WAY_TAGS_GLOBAL),
		     ways_ptr, ids_to_modify, tags_to_delete);
  callback->tags_global_finished();
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(ways_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().WAYS_META),
		      ways_meta_to_insert, ids_to_modify, to_delete, update_logger);
    process_user_data(*transaction, user_by_id, idxs_by_id);
    
    if (update_logger)
    {
      stable_sort(ways_meta_to_delete.begin(), ways_meta_to_delete.begin());
      ways_meta_to_delete.erase(unique(ways_meta_to_delete.begin(), ways_meta_to_delete.end()),
				 ways_meta_to_delete.end());
      update_logger->set_delete_meta_data(ways_meta_to_delete);
      ways_meta_to_delete.clear();
    }
  }
  callback->update_finished();
  
  ids_to_modify.clear();
  ways_to_insert.clear();
  
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
  else if (partial_possible && partial/* && !map_file_existed_before*/)
  {
    string to(".0a");
    to[2] += update_counter % 16;
    rename_referred_file(db_dir, "", to, *osm_base_settings().WAYS);
    rename_referred_file(db_dir, "", to, *osm_base_settings().WAY_TAGS_LOCAL);
    rename_referred_file(db_dir, "", to, *osm_base_settings().WAY_TAGS_GLOBAL);
    if (meta)
      rename_referred_file(db_dir, "", to, *meta_settings().WAYS_META);
    
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

void Way_Updater::update_moved_idxs
    (Osm_Backend_Callback* callback, const vector< pair< Node::Id_Type, Uint32_Index > >& moved_nodes,
     Update_Way_Logger* update_logger)
{
  vector< pair< Way::Id_Type, bool > > ids_to_modify__;
  vector< Way > ways_to_insert__;
  vector< pair< OSM_Element_Metadata_Skeleton< Way::Id_Type >, uint32 > > ways_meta_to_insert__;
  map< uint32, string > user_by_id__;
  
  ids_to_modify__.swap(ids_to_modify);
  ways_to_insert__.swap(ways_to_insert);
  ways_meta_to_insert__.swap(ways_meta_to_insert);
  user_by_id__.swap(user_by_id);
  
/*  if (!map_file_existed_before)
    return;*/
  
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  map< uint32, vector< Way::Id_Type > > to_delete;
  find_affected_ways(moved_nodes, update_logger);
  vector< Way* > ways_ptr = sort_elems_to_insert
      (ways_to_insert, way_comparator_by_id, way_equal_id);
  update_way_ids(ways_ptr, to_delete, (update_logger != 0));
  if (meta)
  {
    map< Way::Id_Type, uint32 > new_index_by_id;
    collect_new_indexes(ways_ptr, new_index_by_id);
    collect_old_meta_data(*transaction->data_index(meta_settings().WAYS_META), to_delete,
		          new_index_by_id, ways_meta_to_insert);
  }
  update_members(ways_ptr, to_delete, 0);
  
  vector< Tag_Entry< Way::Id_Type > > tags_to_delete;
  prepare_tags(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
	       ways_ptr, tags_to_delete, to_delete);
  update_tags_local(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
		    ways_ptr, ids_to_modify, tags_to_delete, update_logger);
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(ways_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().WAYS_META), ways_meta_to_insert,
		      ids_to_modify, to_delete, update_logger);
    process_user_data(*transaction, user_by_id, idxs_by_id);
  }
  
  //show_mem_status();
  
  ids_to_modify__.swap(ids_to_modify);
  ways_to_insert__.swap(ways_to_insert);
  ways_meta_to_insert__.swap(ways_meta_to_insert);
  user_by_id__.swap(user_by_id);
  
  if (!external_transaction)
    delete transaction;
}


void filter_affected_ways(Transaction& transaction, 
			  vector< pair< Way::Id_Type, bool > >& ids_to_modify,
			  vector< Way >& ways_to_insert,
			  const vector< Way >& maybe_affected_ways,
			  bool keep_all)
{
  // retrieve the indices of the referred nodes
  map< Node::Id_Type, uint32 > used_nodes;
  for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
      wit != maybe_affected_ways.end(); ++wit)
  {
    for (vector< Node::Id_Type >::const_iterator nit(wit->nds.begin());
        nit != wit->nds.end(); ++nit)
      used_nodes[*nit] = 0;
  }
  Random_File< Uint32_Index > node_random
      (transaction.random_index(osm_base_settings().NODES));
  for (map< Node::Id_Type, uint32 >::iterator it(used_nodes.begin());
      it != used_nodes.end(); ++it)
    it->second = node_random.get(it->first.val()).val();
  
  vector< Node::Id_Type > used_large_way_nodes;
  vector< Uint32_Index > used_large_way_idxs;
  
  for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
      wit != maybe_affected_ways.end(); ++wit)
  {
    vector< uint32 > nd_idxs;
    for (vector< Node::Id_Type >::const_iterator nit = wit->nds.begin();
        nit != wit->nds.end(); ++nit)
      nd_idxs.push_back(used_nodes[*nit]);
    
    Uint31_Index index = Way::calc_index(nd_idxs);
    if ((index.val() & 0x80000000) != 0 && (index.val() & 0x1) == 0) // Adapt 0x3
    {
      for (vector< Node::Id_Type >::const_iterator nit = wit->nds.begin();
          nit != wit->nds.end(); ++nit)
      {
        used_large_way_nodes.push_back(*nit);
        used_large_way_idxs.push_back(Uint32_Index(used_nodes[*nit]));
      }
    }
  }
  
  // collect referred nodes
  sort(used_large_way_nodes.begin(), used_large_way_nodes.end());
  used_large_way_nodes.erase(unique(used_large_way_nodes.begin(), used_large_way_nodes.end()),
      used_large_way_nodes.end());
  sort(used_large_way_idxs.begin(), used_large_way_idxs.end());
  used_large_way_idxs.erase(unique(used_large_way_idxs.begin(), used_large_way_idxs.end()),
      used_large_way_idxs.end());
  map< Uint31_Index, vector< Node_Skeleton > > large_way_nodes;
  collect_items_discrete(transaction, *osm_base_settings().NODES, used_large_way_idxs,
                        Id_Predicate< Node_Skeleton >(used_large_way_nodes), large_way_nodes);
  map< Node::Id_Type, Quad_Coord > node_coords_by_id;
  for (map< Uint31_Index, vector< Node_Skeleton > >::const_iterator it = large_way_nodes.begin();
       it != large_way_nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      node_coords_by_id[it2->id] = Quad_Coord(it->first.val(), it2->ll_lower);
  }
  
  for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
      wit != maybe_affected_ways.end(); ++wit)
  {
    vector< uint32 > nd_idxs;
    for (vector< Node::Id_Type >::const_iterator nit(wit->nds.begin());
        nit != wit->nds.end(); ++nit)
      nd_idxs.push_back(used_nodes[*nit]);
    
    uint32 index(Way::calc_index(nd_idxs));

    vector< Quad_Coord > geometry;
    if ((index & 0x80000000) != 0 && ((index & 0x1) == 0))
    {
      for (vector< Node::Id_Type >::const_iterator nit = wit->nds.begin();
          nit != wit->nds.end(); ++nit)
        geometry.push_back(node_coords_by_id[*nit]);
    }
    
    if (wit->index != index || wit->geometry != geometry || keep_all)
    {
      ids_to_modify.push_back(make_pair(wit->id, true));
      ways_to_insert.push_back(*wit);
      ways_to_insert.back().index = index;
      ways_to_insert.back().geometry = geometry;
    }
  }
}

void Way_Updater::find_affected_ways
    (const vector< pair< Node::Id_Type, Uint32_Index > >& moved_nodes,
       Update_Way_Logger* update_logger)
{
  static Pair_Comparator_By_Id< Node::Id_Type, Uint32_Index > pair_comparator_by_id;

  vector< Way > maybe_affected_ways;
  
  set< Uint31_Index > req;
  {
    vector< uint32 > moved_node_idxs;
    for (vector< pair< Node::Id_Type, Uint32_Index > >::const_iterator
        it(moved_nodes.begin()); it != moved_nodes.end(); ++it)
      moved_node_idxs.push_back(it->second.val());
    vector< uint32 > affected_way_idxs = calc_parents(moved_node_idxs);
    for (vector< uint32 >::const_iterator it = affected_way_idxs.begin();
        it != affected_way_idxs.end(); ++it)
      req.insert(Uint31_Index(*it));
  }
  
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (transaction->data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
      it(ways_db.discrete_begin(req.begin(), req.end()));
      !(it == ways_db.discrete_end()); ++it)
  {
    const Way_Skeleton& way(it.object());
    bool is_affected(false);
    for (vector< Node::Id_Type >::const_iterator it3(way.nds.begin());
        it3 != way.nds.end(); ++it3)
    {
      if (binary_search(moved_nodes.begin(), moved_nodes.end(),
	make_pair(*it3, 0), pair_comparator_by_id))
      {
	if (update_logger)
	  update_logger->keeping(it.index(), way);
	is_affected = true;
	break;
      }
    }
    if (is_affected)
      maybe_affected_ways.push_back(Way(way.id.val(), it.index().val(), way.nds));
    if (maybe_affected_ways.size() >= 512*1024)
    {
      filter_affected_ways(*transaction, ids_to_modify, ways_to_insert, maybe_affected_ways,
			   (update_logger != 0));
      maybe_affected_ways.clear();
    }
  }
  
  filter_affected_ways(*transaction, ids_to_modify, ways_to_insert, maybe_affected_ways,
			   (update_logger != 0));
  maybe_affected_ways.clear();
}

void Way_Updater::compute_indexes(vector< Way* >& ways_ptr)
{
  static Meta_Comparator_By_Id< Way::Id_Type > meta_comparator_by_id;
  static Meta_Equal_Id< Way::Id_Type > meta_equal_id;
  
  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ways_meta_to_insert.begin(), ways_meta_to_insert.end(), meta_comparator_by_id);
  vector< pair< OSM_Element_Metadata_Skeleton< Way::Id_Type >, uint32 > >::iterator meta_begin
      (unique(ways_meta_to_insert.rbegin(), ways_meta_to_insert.rend(), meta_equal_id).base());
  ways_meta_to_insert.erase(ways_meta_to_insert.begin(), meta_begin);
  
  // retrieve the indices of the referred nodes
  map< Node::Id_Type, uint32 > used_nodes;
  for (vector< Way* >::const_iterator wit = ways_ptr.begin();
      wit != ways_ptr.end(); ++wit)
  {
    for (vector< Node::Id_Type >::const_iterator nit = (*wit)->nds.begin();
        nit != (*wit)->nds.end(); ++nit)
      used_nodes[*nit] = 0;
  }
  Random_File< Uint32_Index > node_random
      (transaction->random_index(osm_base_settings().NODES));
  for (map< Node::Id_Type, uint32 >::iterator it(used_nodes.begin());
      it != used_nodes.end(); ++it)
    it->second = node_random.get(it->first.val()).val();

  vector< Node::Id_Type > used_large_way_nodes;
  vector< Uint32_Index > used_large_way_idxs;
  
  for (vector< Way* >::iterator wit = ways_ptr.begin(); wit != ways_ptr.end(); ++wit)
  {
    vector< uint32 > nd_idxs;
    for (vector< Node::Id_Type >::const_iterator nit = (*wit)->nds.begin();
        nit != (*wit)->nds.end(); ++nit)
      nd_idxs.push_back(used_nodes[*nit]);
    
    (*wit)->index = Way::calc_index(nd_idxs);
    if (((*wit)->index & 0x80000000) != 0 && (((*wit)->index & 0x1) == 0)) // Adapt 0x3
    {
      for (vector< Node::Id_Type >::const_iterator nit = (*wit)->nds.begin();
          nit != (*wit)->nds.end(); ++nit)
      {
        used_large_way_nodes.push_back(*nit);
        used_large_way_idxs.push_back(Uint32_Index(used_nodes[*nit]));
      }
      
      // old code
      //(*wit)->segment_idxs = calc_segment_idxs(nd_idxs);      
    }
  }
  
  // collect referred nodes
  sort(used_large_way_nodes.begin(), used_large_way_nodes.end());
  used_large_way_nodes.erase(unique(used_large_way_nodes.begin(), used_large_way_nodes.end()),
      used_large_way_nodes.end());
  sort(used_large_way_idxs.begin(), used_large_way_idxs.end());
  used_large_way_idxs.erase(unique(used_large_way_idxs.begin(), used_large_way_idxs.end()),
      used_large_way_idxs.end());
  map< Uint31_Index, vector< Node_Skeleton > > large_way_nodes;
  collect_items_discrete(*transaction, *osm_base_settings().NODES, used_large_way_idxs,
                        Id_Predicate< Node_Skeleton >(used_large_way_nodes), large_way_nodes);
  map< Node::Id_Type, Quad_Coord > node_coords_by_id;
  for (map< Uint31_Index, vector< Node_Skeleton > >::const_iterator it = large_way_nodes.begin();
       it != large_way_nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      node_coords_by_id[it2->id] = Quad_Coord(it->first.val(), it2->ll_lower);
  }
  
  // calculate for all large ways their geometry
  for (vector< Way* >::iterator wit = ways_ptr.begin(); wit != ways_ptr.end(); ++wit)
  {
    if (((*wit)->index & 0x80000000) != 0 && (((*wit)->index & 0x1) == 0))
    {
      for (vector< Node::Id_Type >::const_iterator nit = (*wit)->nds.begin();
          nit != (*wit)->nds.end(); ++nit)
        (*wit)->geometry.push_back(node_coords_by_id[*nit]);
    }
  }
  
  // Adapt meta data
  vector< Way* >::const_iterator wit = ways_ptr.begin();
  for (vector< pair< OSM_Element_Metadata_Skeleton< Way::Id_Type >, uint32 > >::iterator
      mit(ways_meta_to_insert.begin()); mit != ways_meta_to_insert.end(); ++mit)
  {
    while ((wit != ways_ptr.end()) && ((*wit)->id < mit->first.ref))
      ++wit;
    if (wit == ways_ptr.end())
      break;
    
    if ((*wit)->id == mit->first.ref)
      mit->second = (*wit)->index;
  }
}

void Way_Updater::update_way_ids
    (const vector< Way* >& ways_ptr, map< uint32, vector< Way::Id_Type > >& to_delete,
     bool record_minuscule_moves)
{
  static Pair_Comparator_By_Id< Way::Id_Type, bool > pair_comparator_by_id;
  static Pair_Equal_Id< Way::Id_Type, bool > pair_equal_id;

  // process the ways itself
  // keep always the most recent (last) element of all equal elements
  stable_sort(ids_to_modify.begin(), ids_to_modify.end(),
	      pair_comparator_by_id);
  vector< pair< Way::Id_Type, bool > >::iterator modi_begin
	      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id)
	      .base());
  ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
  
  Random_File< Uint31_Index > random
      (transaction->random_index(osm_base_settings().WAYS));
  vector< Way* >::const_iterator wit = ways_ptr.begin();
  for (vector< pair< Way::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    Uint31_Index index(random.get(it->first.val()));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
    if ((wit != ways_ptr.end()) && (it->first == (*wit)->id))
    {
      if (it->second)
      {
	random.put(it->first.val(), Uint31_Index((*wit)->index));
	if ((index.val() > 0) && (index.val() != (*wit)->index || record_minuscule_moves))
	  moved_ways.push_back(make_pair(it->first, index.val()));
      }
      ++wit;
    }
  }
  sort(moved_ways.begin(), moved_ways.end());
}

void Way_Updater::update_members
    (const vector< Way* >& ways_ptr, const map< uint32, vector< Way::Id_Type > >& to_delete,
       Update_Way_Logger* update_logger)
{
  map< Uint31_Index, set< Way_Skeleton > > db_to_delete;
  map< Uint31_Index, set< Way_Skeleton > > db_to_insert;
  
  for (map< uint32, vector< Way::Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint31_Index idx(it->first);
    for (vector< Way::Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      db_to_delete[idx].insert(Way_Skeleton(it2->val(), vector< Node::Id_Type >(), vector< Quad_Coord >()));
  }
  vector< Way* >::const_iterator wit = ways_ptr.begin();
  for (vector< pair< Way::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((wit != ways_ptr.end()) && (it->first == (*wit)->id))
    {
      if (it->second)
      {
	Uint31_Index idx = (*wit)->index;
	db_to_insert[idx].insert(Way_Skeleton(**wit));
	if (update_logger)
          update_logger->insertion(**wit);
      }
      ++wit;
    }
  }
  
  Block_Backend< Uint31_Index, Way_Skeleton > way_db
      (transaction->data_index(osm_base_settings().WAYS));
  if (update_logger)
    way_db.update(db_to_delete, db_to_insert, *update_logger);
  else
    way_db.update(db_to_delete, db_to_insert);
}

void Way_Updater::merge_files(const vector< string >& froms, string into)
{
  Transaction_Collection from_transactions(false, false, db_dir, froms);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  ::merge_files< Uint31_Index, Way_Skeleton >
      (from_transactions, into_transaction, *osm_base_settings().WAYS);
  ::merge_files< Tag_Index_Local, Way::Id_Type >
      (from_transactions, into_transaction, *osm_base_settings().WAY_TAGS_LOCAL);
  ::merge_files< Tag_Index_Global, Way::Id_Type >
      (from_transactions, into_transaction, *osm_base_settings().WAY_TAGS_GLOBAL);
  if (meta)
  {
    ::merge_files< Uint31_Index, OSM_Element_Metadata_Skeleton< Way::Id_Type > >
        (from_transactions, into_transaction, *meta_settings().WAYS_META);
  }
}
