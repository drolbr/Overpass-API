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


Way_Updater::Way_Updater(Transaction& transaction_, meta_modes meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_)
{}

Way_Updater::Way_Updater(string db_dir_, meta_modes meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(true), db_dir(db_dir_), meta(meta_)
{
  partial_possible = !file_exists
      (db_dir + 
       osm_base_settings().WAYS->get_file_name_trunk() +
       osm_base_settings().WAYS->get_data_suffix() +
       osm_base_settings().WAYS->get_index_suffix());
}


// Geupdatete Wege separat laden, alte Fassung, und separat behalten

// Für geupdatete Wege altes Meta kopieren

// Für geupdatete Wege Tags kopieren

// Neueste Versionen aus new_data, mit Index auf Basis der neuesten Nodes
// zuästzlich neue Fassungen der verschobenen Ways

// Indexe von Skeletons. Duplizierung von Versionen ebenso.
// Wenn Meta aktiv, soll zu jedem Skeleton genau eine neue Meta-Version existieren

// Auf Basis der Skeletons und geladenen Tags
// Neue Tags aus new_data, wenn dort mind. eine Version vorliegt, sonst aus geladenen Tags

// ...


bool geometrically_equal(const Way_Skeleton& a, const Way_Skeleton& b)
{
  return (a.nds == b.nds);
}


void compute_idx_and_geometry
    (Uint31_Index& idx, Way_Skeleton& skeleton,
     uint64 expiration_timestamp,
     const std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >& nodes_by_id)
{
  std::vector< Quad_Coord > geometry;
  
  for (std::vector< Node_Skeleton::Id_Type >::const_iterator it = skeleton.nds.begin();
       it != skeleton.nds.end(); ++it)
  {
    std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
        ::const_iterator nit = nodes_by_id.find(*it);
    if (nit != nodes_by_id.end() && !nit->second.empty())
    {
      std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > >::const_iterator
          it2 = nit->second.begin();
      while (it2 != nit->second.end() && it2->second.timestamp < expiration_timestamp)
        ++it2;
      if (it2 != nit->second.end())
        geometry.push_back(Quad_Coord(it2->first.val(), it2->second.ll_lower));
      // Otherwise the node has expired before our way - something has gone wrong seriously.
    }
    else
      std::cerr<<"Node "<<it->val()<<" used in way "<<skeleton.id.val()<<" not found.\n";
    // Otherwise the node is not contained in our list - something has gone wrong seriously.
  }
  
  vector< uint32 > nd_idxs;
  for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    nd_idxs.push_back(it->ll_upper);
    
  idx = Way::calc_index(nd_idxs);
      
  if (Way::indicates_geometry(idx))
    skeleton.geometry.swap(geometry);
  else
    skeleton.geometry.clear();
}


/* Checks the nds of the way whether in the time window an underlying node has moved.
 * If yes, the necessary intermediate versions are generated.
 */
void add_intermediate_versions
    (const Way_Skeleton& skeleton, const uint64 old_timestamp, const uint64 new_timestamp,
     const std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >& nodes_by_id,
     bool add_last_version, Uint31_Index attic_idx,
     std::map< Uint31_Index, std::set< Attic< Way_Skeleton > > >& full_attic,
     std::map< Uint31_Index, std::set< Attic< Way_Skeleton::Id_Type > > >& new_undeleted,
     std::map< Way_Skeleton::Id_Type, std::set< Uint31_Index > >& idx_lists)
{
  std::vector< uint64 > relevant_timestamps;
  for (std::vector< Node_Skeleton::Id_Type >::const_iterator it = skeleton.nds.begin();
       it != skeleton.nds.end(); ++it)
  {
    std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
        ::const_iterator nit = nodes_by_id.find(*it);
    if (nit != nodes_by_id.end() && !nit->second.empty())
    {
      for (std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > >::const_iterator
          it2 = nit->second.begin(); it2 != nit->second.end(); ++it2)
      {
        if (old_timestamp < it2->second.timestamp && it2->second.timestamp < new_timestamp)
          relevant_timestamps.push_back(it2->second.timestamp);
      }
    }
    // Otherwise the node is not contained in our list. Could happen if it didn't change at all.
  }
  std::sort(relevant_timestamps.begin(), relevant_timestamps.end());
  relevant_timestamps.erase(std::unique(relevant_timestamps.begin(), relevant_timestamps.end()),
                            relevant_timestamps.end());
  
  // Track index for the undelete creation
  Uint31_Index last_idx = Uint31_Index(0u);
  
  for (std::vector< uint64 >::const_iterator it = relevant_timestamps.begin();
       it != relevant_timestamps.end(); ++it)
  {
    Uint31_Index idx = attic_idx;
    attic_idx = Uint31_Index(0u);
    Way_Skeleton cur_skeleton = skeleton;
    if (idx.val() == 0)
      compute_idx_and_geometry(idx, cur_skeleton, *it, nodes_by_id);
    full_attic[idx].insert(Attic< Way_Skeleton >(cur_skeleton, *it));
    idx_lists[skeleton.id].insert(idx);
    
    // Manage undelete entries
    if (it != relevant_timestamps.begin() && !(last_idx == idx))
      new_undeleted[idx].insert(Attic< Way_Skeleton::Id_Type >(skeleton.id, *(it-1)));
    last_idx = idx;
  }

  Uint31_Index idx = attic_idx;
  Way_Skeleton last_skeleton = skeleton;
  if (idx.val() == 0)
    compute_idx_and_geometry(idx, last_skeleton, new_timestamp, nodes_by_id);
    
  if (add_last_version)
  {
    full_attic[idx].insert(Attic< Way_Skeleton >(last_skeleton, new_timestamp));
    idx_lists[skeleton.id].insert(idx);
  }
    
  // Manage undelete entries
  if (!relevant_timestamps.empty() && !(last_idx == idx))
    new_undeleted[idx].insert(Attic< Way_Skeleton::Id_Type >(skeleton.id, relevant_timestamps.back()));
}


/* Checks the nds of the way whether in the time window an underlying node has moved.
 * If yes, the necessary intermediate versions are generated.
 */
void add_intermediate_changelog_entries
    (const Way_Skeleton& skeleton, const uint64 old_timestamp, const uint64 new_timestamp,
     const std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >& nodes_by_id,
     bool add_last_version, Uint31_Index attic_idx, Uint31_Index new_idx,
     std::map< Timestamp, std::set< Change_Entry< Way_Skeleton::Id_Type > > >& result)
{
  std::vector< uint64 > relevant_timestamps;
  for (std::vector< Node_Skeleton::Id_Type >::const_iterator it = skeleton.nds.begin();
       it != skeleton.nds.end(); ++it)
  {
    std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
        ::const_iterator nit = nodes_by_id.find(*it);
    if (nit != nodes_by_id.end() && !nit->second.empty())
    {
      for (std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > >::const_iterator
          it2 = nit->second.begin(); it2 != nit->second.end(); ++it2)
      {
        if (old_timestamp < it2->second.timestamp && it2->second.timestamp < new_timestamp)
          relevant_timestamps.push_back(it2->second.timestamp);
      }
    }
    // Otherwise the node is not contained in our list. Could happen if it didn't change at all.
  }
  std::sort(relevant_timestamps.begin(), relevant_timestamps.end());
  relevant_timestamps.erase(std::unique(relevant_timestamps.begin(), relevant_timestamps.end()),
                            relevant_timestamps.end());
  
  std::vector< Uint31_Index > idxs;
  
  for (std::vector< uint64 >::const_iterator it = relevant_timestamps.begin();
       it != relevant_timestamps.end(); ++it)
  {
    Uint31_Index idx = attic_idx;
    attic_idx = Uint31_Index(0u);
    Way_Skeleton cur_skeleton = skeleton;
    if (idx.val() == 0)
      compute_idx_and_geometry(idx, cur_skeleton, *it, nodes_by_id);
    idxs.push_back(idx);
  }

  Uint31_Index idx = attic_idx;
  Way_Skeleton last_skeleton = skeleton;
  if (idx.val() == 0)
    compute_idx_and_geometry(idx, last_skeleton, new_timestamp, nodes_by_id);
  idxs.push_back(idx);

  int i = 0;
  for (std::vector< uint64 >::const_iterator it = relevant_timestamps.begin();
       it != relevant_timestamps.end(); ++it)
  {
    result[Timestamp(*it)].insert(
        Change_Entry< Way_Skeleton::Id_Type >(skeleton.id, idxs[i], idxs[i+1]));
    ++i;
  }
    
  if (add_last_version)
    result[Timestamp(new_timestamp)].insert(
        Change_Entry< Way_Skeleton::Id_Type >(skeleton.id, idx, new_idx));
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the set of elements to store to attic.
 * We use that in attic_skeletons can only appear elements with ids that exist also in new_data. */
void compute_new_attic_skeletons
    (const Data_By_Id< Way_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& implicitly_moved_skeletons,
     const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& attic_map_positions,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_skeletons,
     const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
     std::map< Uint31_Index, std::set< Attic< Way_Skeleton > > >& full_attic,
     std::map< Uint31_Index, std::set< Attic< Way_Skeleton::Id_Type > > >& new_undeleted,
     std::map< Way_Skeleton::Id_Type, std::set< Uint31_Index > >& idx_lists)
{
  // Fill nodes_by_id from attic nodes as well as the current nodes in new_node_idx_by_id
  std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > > nodes_by_id;
  for (std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >::const_iterator
      it = new_attic_node_skeletons.begin(); it != new_attic_node_skeletons.end(); ++it)
  {
    for (std::set< Attic< Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      //std::cout<<it2->id.val()<<'\t'<<it2->timestamp<<'\n';
      nodes_by_id[it2->id].push_back(std::make_pair(it->first, *it2));
    }
  }
  
  for (std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it = new_node_idx_by_id.begin();
       it != new_node_idx_by_id.end(); ++it)
  {
    //std::cout<<it->first.val()<<'\n';
    nodes_by_id[it->first].push_back(std::make_pair
        (it->second.ll_upper, Attic< Node_Skeleton >(Node_Skeleton(it->first, it->second.ll_lower),
             NOW)));
  }
    
  // Create full_attic and idx_lists by going through new_data and filling the gaps
  std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Way_Skeleton::Id_Type last_id = Way_Skeleton::Id_Type(0u);
  for (std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
    {
      if (it->idx.val() != 0)
      {
        add_intermediate_versions(it->elem, it->meta.timestamp, next_it->meta.timestamp, nodes_by_id,
                                  // Add last version only if it differs from the next version
                                  (next_it->idx.val() == 0 || !geometrically_equal(it->elem, next_it->elem)),
                                  Uint31_Index(0u),
                                  full_attic, new_undeleted, idx_lists);
      }
    }
    
    if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
      // This is the latest version of this element. Care here for changes since this element.
    {
      add_intermediate_versions(it->elem, it->meta.timestamp,
                                NOW, nodes_by_id,
                                false, Uint31_Index(0u),
                                full_attic, new_undeleted, idx_lists);
    }

    if (last_id == it->elem.id)
    {
      // An earlier version exists also in new_data.
      std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator last_it = it;
      --last_it;
      if (last_it->idx == Uint31_Index(0u))
        new_undeleted[it->idx].insert(Attic< Way_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
      continue;
    }
    else
    {
      const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
      const Uint31_Index* idx_attic = binary_pair_search(attic_map_positions, it->elem.id);
      if (!idx && idx_attic)
        new_undeleted[it->idx].insert(Attic< Way_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
    }
    last_id = it->elem.id;
    
    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    if (!idx)
      // No old data exists. So there is nothing to do here.
      continue;    

    std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
      // Something has gone wrong. Skip this object.
      continue;
    
    std::set< Way_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
      // Something has gone wrong. Skip this object.
      continue;

    add_intermediate_versions(*it_attic, 0, it->meta.timestamp, nodes_by_id,
                              !geometrically_equal(*it_attic, it->elem), *idx,
                              full_attic, new_undeleted, idx_lists);
  }
  
  // Add the missing elements that result from node moves only
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_intermediate_versions(*it2, 0, NOW, nodes_by_id,
                                false, it->first,
                                full_attic, new_undeleted, idx_lists);
  }
}


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


/* Adds the implicity known Quad_Coords from the given ways for nodes not yet known in
 * new_node_idx_by_id */
void add_implicitly_known_nodes
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


/* We assert that every node id that appears in a way in existing_skeletons has its Quad_Coord
   in new_node_idx_by_id. */
void compute_geometry
    (const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     Data_By_Id< Way_Skeleton >& new_data)
{
  std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator next_it = new_data.data.begin();
  for (std::vector< Data_By_Id< Way_Skeleton >::Entry >::iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && next_it->elem.id == it->elem.id)
      // We don't care on intermediate versions
      continue;
      
    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;
    
    vector< uint32 > nd_idxs;
    for (vector< Node::Id_Type >::const_iterator nit = it->elem.nds.begin(); nit != it->elem.nds.end(); ++nit)
    {
      std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2 = new_node_idx_by_id.find(*nit);
      if (it2 != new_node_idx_by_id.end())
        nd_idxs.push_back(it2->second.ll_upper);
      else
        std::cerr<<"Node "<<nit->val()<<" used in way "<<it->elem.id.val()<<" not found.\n";
    }
    
    Uint31_Index index = Way::calc_index(nd_idxs);
      
    it->elem.geometry.clear();
    
    if (Way::indicates_geometry(index))
    {    
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
        else
          std::cerr<<"Node "<<nit->val()<<" used in way "<<it2->id.val()<<" not found.\n";
      }
    
      Uint31_Index index = Way::calc_index(nd_idxs);
      if (nd_idxs.empty())
        index = 0xff;
      
      Way_Skeleton new_skeleton = *it2;
      new_skeleton.geometry.clear();
      
      if (Way::indicates_geometry(index))
      {  
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
        new_skeletons[index].insert(new_skeleton);
      
      if (!(index == it->first))
        moved_ways.push_back(std::make_pair(it2->id, it->first));
    }
  }
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the set of elements to store to attic.
 * We use that in attic_skeletons can only appear elements with ids that exist also in new_data. */
std::map< Timestamp, std::set< Change_Entry< Way_Skeleton::Id_Type > > > compute_changelog(
    const Data_By_Id< Way_Skeleton >& new_data,
    const std::map< Uint31_Index, std::set< Way_Skeleton > >& implicitly_moved_skeletons,
    const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
    const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& attic_map_positions,
    const std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_skeletons,
    const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
    const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons)
{
  std::map< Timestamp, std::set< Change_Entry< Way_Skeleton::Id_Type > > > result;
  
  // Fill nodes_by_id from attic nodes as well as the current nodes in new_node_idx_by_id
  std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > > nodes_by_id;
  for (std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >::const_iterator
      it = new_attic_node_skeletons.begin(); it != new_attic_node_skeletons.end(); ++it)
  {
    for (std::set< Attic< Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      //std::cout<<it2->id.val()<<'\t'<<it2->timestamp<<'\n';
      nodes_by_id[it2->id].push_back(std::make_pair(it->first, *it2));
    }
  }
  
  for (std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it = new_node_idx_by_id.begin();
       it != new_node_idx_by_id.end(); ++it)
  {
    //std::cout<<it->first.val()<<'\n';
    nodes_by_id[it->first].push_back(std::make_pair
        (it->second.ll_upper, Attic< Node_Skeleton >(Node_Skeleton(it->first, it->second.ll_lower),
             NOW)));
  }
    

  std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Way_Skeleton::Id_Type last_id = Way_Skeleton::Id_Type(0u);
  for (std::vector< Data_By_Id< Way_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
    {
      Uint31_Index next_idx = next_it->idx;
      if (next_idx.val() == 0xff)
      {
        Way_Skeleton skel = next_it->elem;
        compute_idx_and_geometry(next_idx, skel, next_it->meta.timestamp + 1, nodes_by_id);
      }
      // A later version exists also in new_data.
      add_intermediate_changelog_entries(
           it->elem, it->meta.timestamp, next_it->meta.timestamp, nodes_by_id,
           true, Uint31_Index(0u), next_idx, result);
    }
    
    if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
      // This is the latest version of this element. Care here for changes since this element.
      add_intermediate_changelog_entries(it->elem, it->meta.timestamp, NOW, nodes_by_id,
                                false, 0u, 0u, result);

    if (last_id == it->elem.id)
      // An earlier version exists also in new_data. So there is nothing to do here.
      continue;
    last_id = it->elem.id;
    
    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    Uint31_Index next_idx = it->idx;
    if (next_idx.val() == 0xff)
    {
      Way_Skeleton skel = it->elem;
      compute_idx_and_geometry(next_idx, skel, it->meta.timestamp + 1, nodes_by_id);
    }
    if (!idx)
    {
      // No old data exists.
      result[it->meta.timestamp].insert(
          Change_Entry< Way_Skeleton::Id_Type >(it->elem.id, 0u, next_idx));
      continue;
    }

    std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
      // Something has gone wrong. Skip this object.
      continue;
    
    std::set< Way_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
      // Something has gone wrong. Skip this object.
      continue;

    add_intermediate_changelog_entries(*it_attic, 0, it->meta.timestamp, nodes_by_id,
                              true, *idx, next_idx, result);
  }
  
  // Add the missing elements that result from node moves only
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_intermediate_changelog_entries(*it2, 0, NOW, nodes_by_id,
                                false, it->first, 0u, result);
  }
  
  return result;
}
  

void Way_Updater::update(Osm_Backend_Callback* callback, bool partial,
              Update_Way_Logger* update_logger,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_node_skeletons,
              const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons)
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
          
  // Collect all data of existing meta elements
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way::Id_Type > > > existing_meta
      = (meta ? get_existing_meta< OSM_Element_Metadata_Skeleton< Way::Id_Type > >
             (existing_map_positions, *transaction, *meta_settings().WAYS_META) :
         std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way::Id_Type > > >());
          
  // Collect all data of existing meta elements
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > implicitly_moved_positions
      = make_id_idx_directory(implicitly_moved_skeletons);
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way::Id_Type > > > implicitly_moved_meta
      = (meta ? get_existing_meta< OSM_Element_Metadata_Skeleton< Way::Id_Type > >
             (implicitly_moved_positions, *transaction, *meta_settings().WAYS_META) :
         std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way::Id_Type > > >());

  // Collect all data of existing tags
  std::vector< Tag_Entry< Way_Skeleton::Id_Type > > existing_local_tags;
  get_existing_tags< Way_Skeleton::Id_Type >
      (existing_map_positions, *transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
       existing_local_tags);
      
  // Collect all data of existing tags for moved ways
  std::vector< Tag_Entry< Way_Skeleton::Id_Type > > implicitly_moved_local_tags;
  get_existing_tags< Way_Skeleton::Id_Type >
      (implicitly_moved_positions, *transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
       implicitly_moved_local_tags);

  // Create a node directory id to idx:
  // Evaluate first the new_node_skeletons
  std::map< Node_Skeleton::Id_Type, Quad_Coord > new_node_idx_by_id
      = dictionary_from_skeletons(new_node_skeletons);
  // Then add all nodes known from existing_skeletons geometry.
  add_implicitly_known_nodes(new_node_idx_by_id, existing_skeletons);
  // Then add all nodes known from implicitly_moved_skeletons geometry.
  add_implicitly_known_nodes(new_node_idx_by_id, implicitly_moved_skeletons);
  // Then lookup the missing nodes.
  lookup_missing_nodes(new_node_idx_by_id, new_data, implicitly_moved_skeletons, *transaction);
  
  // Compute the indices of the new ways
  compute_geometry(new_node_idx_by_id, new_data);

  // Compute which objects really have changed
  attic_skeletons.clear();
  new_skeletons.clear();
  new_current_skeletons(new_data, existing_map_positions, existing_skeletons,
      (update_logger != 0), attic_skeletons, new_skeletons, moved_ways, update_logger);
  
  // Compute and add implicitly moved ways
  new_implicit_skeletons(new_node_idx_by_id, implicitly_moved_skeletons,
      (update_logger != 0), attic_skeletons, new_skeletons, moved_ways, update_logger);

  // Compute which meta data really has changed
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_meta;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > new_meta;
  new_current_meta(new_data, existing_map_positions, existing_meta, attic_meta, new_meta);

  // Compute which meta data has moved
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > new_positions
      = make_id_idx_directory(new_skeletons);
  new_implicit_meta(implicitly_moved_meta, new_positions, attic_meta, new_meta);

  // Compute which tags really have changed
  std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > attic_local_tags;
  std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > new_local_tags;
  new_current_local_tags< Way_Skeleton, Update_Way_Logger, Way_Skeleton::Id_Type >
      (new_data, existing_map_positions, existing_local_tags, attic_local_tags, new_local_tags);
  new_implicit_local_tags(implicitly_moved_local_tags, new_positions, attic_local_tags, new_local_tags);
  std::map< Tag_Index_Global, std::set< Way_Skeleton::Id_Type > > attic_global_tags;
  std::map< Tag_Index_Global, std::set< Way_Skeleton::Id_Type > > new_global_tags;
  new_current_global_tags< Way_Skeleton::Id_Type >
      (attic_local_tags, new_local_tags, attic_global_tags, new_global_tags);
  
  add_deleted_skeletons(attic_skeletons, new_positions);

  // TODO: old code
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

  callback->update_started();
  callback->prepare_delete_tags_finished();
  
  // Update id indexes
  update_map_positions(new_positions, *transaction, *osm_base_settings().WAYS);
  callback->update_ids_finished();
  
  // Update skeletons
  update_elements(attic_skeletons, new_skeletons, *transaction, *osm_base_settings().WAYS, update_logger);
  callback->update_coords_finished();
  
  // Update meta
  if (meta)
    update_elements(attic_meta, new_meta, *transaction, *meta_settings().WAYS_META, update_logger);
  
  // Update local tags
  update_elements(attic_local_tags, new_local_tags, *transaction, *osm_base_settings().WAY_TAGS_LOCAL,
                  update_logger);
  callback->tags_local_finished();
  
  // Update global tags
  update_elements(attic_global_tags, new_global_tags, *transaction, *osm_base_settings().WAY_TAGS_GLOBAL);
  callback->tags_global_finished();
    
  if (meta == keep_attic)
  {
    // TODO: For compatibility with the update_logger, this doesn't happen during the tag processing itself.
    //cancel_out_equal_tags(attic_local_tags, new_local_tags);

    // Also include ids from the only moved ways
    enhance_ids_to_update(implicitly_moved_skeletons, ids_to_update_);
    
    // Collect all data of existing attic id indexes
    std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > existing_attic_map_positions
        = get_existing_map_positions(ids_to_update_, *transaction, *attic_settings().WAYS);
    std::map< Way_Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists
        = get_existing_idx_lists(ids_to_update_, existing_attic_map_positions,
                                 *transaction, *attic_settings().WAY_IDX_LIST);
        
    // Compute which objects really have changed
    new_attic_skeletons.clear();
    std::map< Way_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists = existing_idx_lists;
    std::map< Uint31_Index, std::set< Attic< Way_Skeleton::Id_Type > > > new_undeleted;
    compute_new_attic_skeletons(new_data, implicitly_moved_skeletons,
                                existing_map_positions, existing_attic_map_positions, attic_skeletons,
                                new_node_idx_by_id, new_attic_node_skeletons,
                                new_attic_skeletons, new_undeleted, new_attic_idx_lists);

    std::map< Way_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > > new_attic_idx_by_id_and_time =
        compute_new_attic_idx_by_id_and_time(new_data, new_skeletons, new_attic_skeletons);
        
    // Compute new meta data
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        new_attic_meta = compute_new_attic_meta(new_attic_idx_by_id_and_time,
            compute_meta_by_id_and_time(new_data, attic_meta), new_meta);
    
    // Compute tags
    std::map< Tag_Index_Local, std::set< Attic< Way_Skeleton::Id_Type > > > new_attic_local_tags
        = compute_new_attic_local_tags(new_attic_idx_by_id_and_time,
            compute_tags_by_id_and_time(new_data, attic_local_tags),
                                       existing_map_positions, existing_idx_lists);
    std::map< Tag_Index_Global, std::set< Attic< Way_Skeleton::Id_Type > > > new_attic_global_tags
        = compute_attic_global_tags(new_attic_local_tags);
    
    // Compute changelog
    std::map< Timestamp, std::set< Change_Entry< Way_Skeleton::Id_Type > > > changelog
        = compute_changelog(new_data, implicitly_moved_skeletons,
                            existing_map_positions, existing_attic_map_positions, attic_skeletons,
                            new_node_idx_by_id, new_attic_node_skeletons);
        
    strip_single_idxs(existing_idx_lists);    
    std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > new_attic_map_positions
        = strip_single_idxs(new_attic_idx_lists);
    
    // Update id indexes
    update_map_positions(new_attic_map_positions, *transaction, *attic_settings().WAYS);
  
    // Update id index lists
    update_elements(existing_idx_lists, new_attic_idx_lists,
                    *transaction, *attic_settings().WAY_IDX_LIST);
  
    // Add attic elements
    update_elements(std::map< Uint31_Index, std::set< Attic< Way_Skeleton > > >(), new_attic_skeletons,
                    *transaction, *attic_settings().WAYS);
  
    // Add attic elements
    update_elements(std::map< Uint31_Index, std::set< Attic< Way_Skeleton::Id_Type > > >(),
                    new_undeleted, *transaction, *attic_settings().WAYS_UNDELETED);
  
    // Add attic meta
    update_elements
        (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >(),
         new_attic_meta, *transaction, *attic_settings().WAYS_META);
  
    // Update tags
    update_elements(std::map< Tag_Index_Local, std::set< Attic < Way_Skeleton::Id_Type > > >(),
                    new_attic_local_tags, *transaction, *attic_settings().WAY_TAGS_LOCAL);
    update_elements(std::map< Tag_Index_Global, std::set< Attic < Way_Skeleton::Id_Type > > >(),
                    new_attic_global_tags, *transaction, *attic_settings().WAY_TAGS_GLOBAL);
    
    // Write changelog
    update_elements(std::map< Timestamp, std::set< Change_Entry< Way_Skeleton::Id_Type > > >(), changelog,
                    *transaction, *attic_settings().WAY_CHANGELOG);
  }
      
  //TODO: old code
  if (meta != only_data)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(ways_meta_to_insert, idxs_by_id);
    process_user_data(*transaction, user_by_id, idxs_by_id);
    
    if (update_logger)
    {
      stable_sort(ways_meta_to_delete.begin(), ways_meta_to_delete.begin());
      ways_meta_to_delete.erase(unique(ways_meta_to_delete.begin(), ways_meta_to_delete.end()),
                                 ways_meta_to_delete.end());
      update_logger->set_delete_meta_data(ways_meta_to_delete);
    }
  }
  callback->update_finished();
  
  new_data.data.clear();
  ways_meta_to_insert.clear();
  ways_meta_to_delete.clear();
  
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
