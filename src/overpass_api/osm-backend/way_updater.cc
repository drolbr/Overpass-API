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


// Älteste Version ist immer alte Nodes + altes Skeleton
// Neueste Version ist schon geklärt
// Dazwischen: anhand der timestamps ausrechnen!

// Liste der Timestamps pro Node und jeweils gültige Koordinate aus new_attic_node_skeletons erstellen
// Liste der Zeitpunkte pro Way anhand new_data-Zeitpunkte plus Node-Zeitpunkte erstellen:
// - durchlaufe alle Einträge in new_data für den jeweils aktuellen Zustand
// - durchlaufe alle Einträge in implicitly_moved_skeletons für Gesamtzustand
// An jedem Zustandswechsel wird der Änderungszeitpunkt am alten Element eingetragen
// Hat ein Way an einem Zeitpunkt seinen Index verändert,
// so muss am neuen Standort zusätzlich ein Undelete-Eintrag gesetzt werden,
// wenn es nicht zum gleichen Zeitpunkt ein Meta-Update gegeben hat.

// Aus exakter Zustandfolge leiten sich auch Tags und Meta her:
// Beides jeweils kopieren für indirekt geänderte Wege


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
    // Otherwise the node is not contained in our list - something has gone wrong seriously.
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
      nodes_by_id[it2->id].push_back(std::make_pair(it->first, *it2));
  }
  
  for (std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it = new_node_idx_by_id.begin();
       it != new_node_idx_by_id.end(); ++it)
    nodes_by_id[it->first].push_back(std::make_pair
        (it->second.ll_upper, Attic< Node_Skeleton >(Node_Skeleton(it->first, it->second.ll_lower),
             std::numeric_limits< unsigned long long >::max())));
    
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
    
    if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
      // This is the latest version of this element. Care here for changes since this element.
    {
      add_intermediate_versions(it->elem, it->meta.timestamp,
                                std::numeric_limits< unsigned long long >::max(), nodes_by_id,
                                false, Uint31_Index(0u),
                                full_attic, new_undeleted, idx_lists);
    }
  }
  
  // Add the missing elements that result from node moves only
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_intermediate_versions(*it2, 0, std::numeric_limits< unsigned long long >::max(), nodes_by_id,
                                false, it->first,
                                full_attic, new_undeleted, idx_lists);
  }
}


template< typename Id_Type >
struct Descending_By_Timestamp
{
  bool operator()(const Id_Type& lhs, const Id_Type& rhs) const
  {
    return rhs.timestamp < lhs.timestamp;
  }
};


template< typename Element_Skeleton >
std::map< typename Element_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > >
    compute_new_attic_idx_by_id_and_time
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& new_skeletons,
     const std::map< Uint31_Index, std::set< Attic< Element_Skeleton > > >& full_attic)
{
  std::map< typename Element_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > > result;
  
  for (typename std::map< Uint31_Index, std::set< Element_Skeleton > >::const_iterator
      it = new_skeletons.begin(); it != new_skeletons.end(); ++it)
  {
    for (typename std::set< Element_Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      result[it2->id].push_back(Attic< Uint31_Index >
          (it->first, std::numeric_limits< unsigned long long >::max()));
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
        result[it->elem.id].push_back(Attic< Uint31_Index >(it->idx,
            std::numeric_limits< unsigned long long >::max()));
      else 
        result[it->elem.id].push_back(Attic< Uint31_Index >(it->idx, next_it->meta.timestamp));
    }
    ++next_it;
  }
        
  for (typename std::map< Uint31_Index, std::set< Attic< Element_Skeleton > > >::const_iterator
      it = full_attic.begin(); it != full_attic.end(); ++it)
  {
    for (typename std::set< Attic< Element_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      result[it2->id].push_back(Attic< Uint31_Index >(it->first, it2->timestamp));
  }
  
  for (typename std::map< typename Element_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > >::iterator
      it = result.begin(); it != result.end(); ++it)
    std::sort(it->second.begin(), it->second.end(),
              Descending_By_Timestamp< Attic< Uint31_Index > >());
    
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


/* Enhance the existing attic meta by the meta entries of deleted elements.
   Assert: the sequence in the vector are ordered from recent to old. */
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


template< typename Element_Skeleton >
std::map< std::pair< typename Element_Skeleton::Id_Type, std::string >, std::vector< Attic< std::string > > >
    compute_tags_by_id_and_time
    (const Data_By_Id< Element_Skeleton >& new_data,
     const std::map< Tag_Index_Local, std::set< typename Element_Skeleton::Id_Type > >& attic_local_tags)
{
  std::map< std::pair< typename Element_Skeleton::Id_Type, std::string >,
      std::vector< Attic< std::string > > > result;
      
  std::map< typename Element_Skeleton::Id_Type, uint64 > timestamp_per_id;

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
    uint64 next_timestamp = std::numeric_limits< unsigned long long >::max();
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
      if (result_ref.empty() || result_ref.back().timestamp != it->meta.timestamp)
        result_ref.push_back(Attic< std::string >("", it->meta.timestamp));
      result_ref.push_back(Attic< std::string >(it2->second, next_timestamp));
    }
  }
  
  for (typename std::map< std::pair< typename Element_Skeleton::Id_Type, std::string >,
      std::vector< Attic< std::string > > >::iterator it = result.begin(); it != result.end(); ++it)
    std::sort(it->second.begin(), it->second.end(), Descending_By_Timestamp< Attic< std::string > >());

  for (typename std::map< Tag_Index_Local, std::set< typename Element_Skeleton::Id_Type > >::const_iterator
      it = attic_local_tags.begin(); it != attic_local_tags.end(); ++it)
  {
    for (typename std::set< typename Element_Skeleton::Id_Type >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      std::vector< Attic< std::string > >& result_ref = result[std::make_pair(*it2, it->first.key)];
      uint64 timestamp = (timestamp_per_id[*it2] == 0 ?
          std::numeric_limits< unsigned long long >::max() : timestamp_per_id[*it2]);
      if (result_ref.empty() || result_ref.back().timestamp != timestamp_per_id[*it2])
        result_ref.push_back(Attic< std::string >(it->first.value, timestamp));
      else
        result_ref.back() = Attic< std::string >(it->first.value, timestamp);
    }
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
    while (tit->first.first == it->first)
    {
      // Use that one cannot insert the same value twice in a set
      
      typename std::vector< Attic< std::string > >::const_iterator tit2 = tit->second.begin();
      std::vector< Attic< Uint31_Index > >::const_iterator it2 = it->second.begin();
      if (it2 == it->second.end())
        // Assert: Can't happen
        continue;
      
      std::set< Uint31_Index > existing_attic_idxs;
      std::map< Way_Skeleton::Id_Type, std::set< Uint31_Index > >::const_iterator iit
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
      std::string last_value = "";
      ++it2;
      if (tit2 != tit->second.end() && tit2->timestamp == std::numeric_limits< unsigned long long >::max())
      {
        last_value = *tit2;
        ++tit2;
      }
      while (it2 != it->second.end() || tit2 != tit->second.end())
      {
        if (it2 == it->second.end() || it2->timestamp < tit2->timestamp)
        {
          if (last_idx.val() != 0u && last_value != *tit2 && (it2 != it->second.end() || *tit2 != "" ||
              existing_attic_idxs.find(Uint31_Index(last_idx.val() & 0x7fffff00))
                  != existing_attic_idxs.end()))
            result[Tag_Index_Local(last_idx, tit->first.second, *tit2)]
                .insert(Attic< Id_Type >(it->first, tit2->timestamp));
          last_value = *tit2;
          ++tit2;
        }
        else if (tit2 == tit->second.end() || tit2->timestamp < it2->timestamp)
        {
          if (!(last_idx == *it2) && last_value != "")
          {
            if (it2->val() != 0u)
              result[Tag_Index_Local(*it2, tit->first.second, last_value)]
                .insert(Attic< Id_Type >(it->first, it2->timestamp));
            if (last_idx.val() != 0u)
              result[Tag_Index_Local(last_idx, tit->first.second, "")]
                  .insert(Attic< Id_Type >(it->first, it2->timestamp));
          }
          last_idx = *it2;
          ++it2;
        }
        else
        {
          if (it2->val() != 0u && (!(last_idx == *it2) || last_value != *tit2))
            result[Tag_Index_Local(*it2, tit->first.second, *tit2)]
                .insert(Attic< Id_Type >(it->first, tit2->timestamp));
          if (!(last_idx == *it2) && last_idx.val() != 0u)
            result[Tag_Index_Local(last_idx, tit->first.second, "")]
                .insert(Attic< Id_Type >(it->first, tit2->timestamp));
                
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
    }
  }
}


/* Adds to attic_meta and new_meta the meta elements to delete resp. add from only
   implicitly moved ways. */
void new_implicit_meta
    (const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >&
         existing_meta,
     const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& new_positions,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >& attic_meta,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >& new_meta)
{
  for (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
          ::const_iterator it_idx = existing_meta.begin(); it_idx != existing_meta.end(); ++it_idx)
  {
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& handle(attic_meta[it_idx->first]);
    for (std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >::const_iterator
        it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
      handle.insert(*it);
  }

  for (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
          ::const_iterator it_idx = existing_meta.begin(); it_idx != existing_meta.end(); ++it_idx)
  {
    for (std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >::const_iterator
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


void add_deleted_skeletons
    (const std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_skeletons,
     std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& new_positions)
{
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = attic_skeletons.begin();
       it != attic_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      new_positions.push_back(std::make_pair(it2->id, Uint31_Index(0u)));
    }
  }
  
  std::stable_sort(new_positions.begin(), new_positions.end(),
                   Pair_Comparator_By_Id< Way_Skeleton::Id_Type, Uint31_Index >());
  new_positions.erase(std::unique(new_positions.begin(), new_positions.end(),
                      Pair_Equal_Id< Way_Skeleton::Id_Type, Uint31_Index >()), new_positions.end());
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

  // Compute idx positions of new nodes
  // TODO: old code
  map< uint32, vector< Way::Id_Type > > to_delete;
  update_way_ids(to_delete, (update_logger != 0), new_positions);

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
    std::map< Uint31_Index, std::set< Attic< Way_Skeleton > > > new_attic_skeletons;
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
        = compute_changelog(new_skeletons, new_attic_skeletons,
                            new_local_tags, new_attic_local_tags,
                            new_meta, attic_meta);
        
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
      ways_meta_to_delete.clear();
    }
  }
  callback->update_finished();
  
  new_data.data.clear();
  ids_to_modify.clear();
  ways_to_insert.clear();
  
  if (!external_transaction)
    delete transaction;
  
  if (partial_possible)
  {
    new_skeletons.clear();
    attic_skeletons.clear();
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
    (map< uint32, vector< Way::Id_Type > >& to_delete, bool record_minuscule_moves,
     const std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > >& new_idx_positions)
{
  static Pair_Comparator_By_Id< Way::Id_Type, bool > pair_comparator_by_id;
  static Pair_Equal_Id< Way::Id_Type, bool > pair_equal_id;

  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ids_to_modify.begin(), ids_to_modify.end(), pair_comparator_by_id);
  vector< pair< Way::Id_Type, bool > >::iterator modi_begin
      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id).base());
  ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
  
  Random_File< Uint32_Index > random
      (transaction->random_index(osm_base_settings().NODES));
  for (vector< pair< Way::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    Uint32_Index index(random.get(it->first.val()));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
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
