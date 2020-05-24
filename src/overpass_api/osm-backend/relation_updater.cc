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

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"
#include "relation_updater.h"
#include "tags_updater.h"


Relation_Updater::Relation_Updater(Transaction& transaction_, meta_modes meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true),
    max_role_id(0), max_written_role_id(0), meta(meta_), keys(*osm_base_settings().RELATION_KEYS)
{}

Relation_Updater::Relation_Updater(std::string db_dir_, meta_modes meta_)
  : update_counter(0), transaction(0),
    external_transaction(false),
    max_role_id(0), max_written_role_id(0), db_dir(db_dir_), meta(meta_),
    keys(*osm_base_settings().RELATION_KEYS)
{}


void Relation_Updater::load_roles()
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");

  Block_Backend< Uint32_Index, String_Object > roles_db
      (transaction->data_index(osm_base_settings().RELATION_ROLES));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
      it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
  {
    role_ids[it.object().val()] = it.index().val();
    if (max_role_id <= it.index().val())
      max_role_id = it.index().val()+1;
  }
  max_written_role_id = max_role_id;

  if (!external_transaction)
    delete transaction;
}


uint32 Relation_Updater::get_role_id(const std::string& s)
{
  if (max_role_id == 0)
    load_roles();
  std::map< std::string, uint32 >::const_iterator it(role_ids.find(s));
  if (it != role_ids.end())
    return it->second;
  role_ids[s] = max_role_id;
  ++max_role_id;
  return (max_role_id - 1);
}


std::map< Uint31_Index, std::set< Relation_Skeleton > > get_implicitly_moved_skeletons
    (const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_nodes,
     const std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_ways,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& already_known_skeletons,
     Transaction& transaction, const File_Properties& file_properties)
{
  std::set< Uint31_Index > member_req;
  for (std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator
      it = attic_nodes.begin(); it != attic_nodes.end(); ++it)
    member_req.insert(it->first);
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = attic_ways.begin(); it != attic_ways.end(); ++it)
    member_req.insert(it->first);
  std::set< Uint31_Index > req = calc_parents(member_req);

  std::vector< Node_Skeleton::Id_Type > node_ids;
  for (std::map< Uint31_Index, std::set< Node_Skeleton > >::const_iterator
      it = attic_nodes.begin(); it != attic_nodes.end(); ++it)
  {
    for (std::set< Node_Skeleton >::const_iterator nit = it->second.begin(); nit != it->second.end(); ++nit)
      node_ids.push_back(nit->id);
  }
  std::sort(node_ids.begin(), node_ids.end());
  node_ids.erase(std::unique(node_ids.begin(), node_ids.end()), node_ids.end());

  std::vector< Way_Skeleton::Id_Type > way_ids;
  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = attic_ways.begin(); it != attic_ways.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator nit = it->second.begin(); nit != it->second.end(); ++nit)
      way_ids.push_back(nit->id);
  }
  std::sort(way_ids.begin(), way_ids.end());
  way_ids.erase(std::unique(way_ids.begin(), way_ids.end()), way_ids.end());

  std::vector< Relation_Skeleton::Id_Type > known_relation_ids;
  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator
      it = already_known_skeletons.begin(); it != already_known_skeletons.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator wit = it->second.begin(); wit != it->second.end(); ++wit)
      known_relation_ids.push_back(wit->id);
  }
  std::sort(known_relation_ids.begin(), known_relation_ids.end());
  known_relation_ids.erase(std::unique(known_relation_ids.begin(), known_relation_ids.end()), known_relation_ids.end());

  std::map< Uint31_Index, std::set< Relation_Skeleton > > result;

  Block_Backend< Uint31_Index, Relation_Skeleton > db(transaction.data_index(&file_properties));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (binary_search(known_relation_ids.begin(), known_relation_ids.end(), it.object().id))
      continue;
    for (std::vector< Relation_Entry >::const_iterator nit = it.object().members.begin();
         nit != it.object().members.end(); ++nit)
    {
      if (nit->type == Relation_Entry::NODE)
      {
        if (binary_search(node_ids.begin(), node_ids.end(), Node_Skeleton::Id_Type(nit->ref.val())))
        {
          result[it.index()].insert(it.object());
          break;
        }
      }
      else if (nit->type == Relation_Entry::WAY)
      {
        if (binary_search(way_ids.begin(), way_ids.end(), Way_Skeleton::Id_Type(nit->ref.val())))
        {
          result[it.index()].insert(it.object());
          break;
        }
      }
    }
  }

  return result;
}


/* Adds to attic_skeletons and new_skeletons all those relations that have moved just because
   a node in these relations has moved.
   We assert that every node id that appears in a relation in existing_skeletons has its Quad_Coord
   in new_node_idx_by_id. */
void new_implicit_skeletons
    (const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& existing_skeletons,
     bool record_minuscule_moves,
     std::map< Uint31_Index, std::set< Relation_Skeleton > >& attic_skeletons,
     std::map< Uint31_Index, std::set< Relation_Skeleton > >& new_skeletons,
     std::vector< std::pair< Relation::Id_Type, Uint31_Index > >& moved_relations)
{
  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it = existing_skeletons.begin();
       it != existing_skeletons.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      attic_skeletons[it->first].insert(*it2);
  }

  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it = existing_skeletons.begin();
       it != existing_skeletons.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      std::vector< uint32 > member_idxs;
      for (std::vector< Relation_Entry >::const_iterator nit = it2->members.begin();
           nit != it2->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE)
        {
          std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2
              = new_node_idx_by_id.find(Node_Skeleton::Id_Type(nit->ref.val()));
          if (it2 != new_node_idx_by_id.end())
            member_idxs.push_back(it2->second.ll_upper);
        }
        else if (nit->type == Relation_Entry::WAY)
        {
          std::map< Way_Skeleton::Id_Type, Uint31_Index >::const_iterator it2
              = new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val()));
          if (it2 != new_way_idx_by_id.end())
            member_idxs.push_back(it2->second.val());
        }
      }

      Uint31_Index index = Relation::calc_index(member_idxs);

      Relation_Skeleton new_skeleton = *it2;
      new_skeleton.node_idxs.clear();
      new_skeleton.way_idxs.clear();

      if (Relation::indicates_geometry(index))
      {
        for (std::vector< Relation_Entry >::const_iterator nit = it2->members.begin();
             nit != it2->members.end(); ++nit)
        {
          if (nit->type == Relation_Entry::NODE)
          {
            std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2
                = new_node_idx_by_id.find(Node_Skeleton::Id_Type(nit->ref.val()));
            if (it2 != new_node_idx_by_id.end())
              new_skeleton.node_idxs.push_back(it2->second.ll_upper);
          }
          else if (nit->type == Relation_Entry::WAY)
          {
            std::map< Way_Skeleton::Id_Type, Uint31_Index >::const_iterator it2
                = new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val()));
            if (it2 != new_way_idx_by_id.end())
              new_skeleton.way_idxs.push_back(it2->second);
          }
        }

        new_skeletons[index].insert(new_skeleton);
      }
      else
        new_skeletons[index].insert(new_skeleton);

      if (!(index == it->first))
        moved_relations.push_back(std::make_pair(it2->id, it->first));
    }
  }
}


inline std::map< Way_Skeleton::Id_Type, Uint31_Index > dictionary_from_skeletons
    (const std::map< Uint31_Index, std::set< Way_Skeleton > >& new_way_skeletons)
{
  std::map< Way_Skeleton::Id_Type, Uint31_Index > result;

  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator
      it = new_way_skeletons.begin(); it != new_way_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator nit = it->second.begin(); nit != it->second.end(); ++nit)
      result.insert(std::make_pair(nit->id, it->first.val()));
  }

  return result;
}


void lookup_missing_nodes
    (std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const Data_By_Id< Relation_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& known_skeletons_1,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& known_skeletons_2,
     Transaction& transaction)
{
  std::vector< Node_Skeleton::Id_Type > missing_ids;

  for (std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;

    std::vector< uint32 > nd_idxs;
    for (std::vector< Relation_Entry >::const_iterator nit = it->elem.members.begin();
         nit != it->elem.members.end(); ++nit)
    {
      if (nit->type == Relation_Entry::NODE &&
          new_node_idx_by_id.find(nit->ref) == new_node_idx_by_id.end())
        missing_ids.push_back(nit->ref);
    }
  }

  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it = known_skeletons_1.begin();
       it != known_skeletons_1.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      for (std::vector< Relation_Entry >::const_iterator nit = it2->members.begin();
           nit != it2->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE &&
            new_node_idx_by_id.find(nit->ref) == new_node_idx_by_id.end())
          missing_ids.push_back(nit->ref);
      }
    }
  }

  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it = known_skeletons_2.begin();
       it != known_skeletons_2.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      for (std::vector< Relation_Entry >::const_iterator nit = it2->members.begin();
           nit != it2->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE &&
            new_node_idx_by_id.find(nit->ref) == new_node_idx_by_id.end())
          missing_ids.push_back(nit->ref);
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
      new_node_idx_by_id.insert(std::make_pair(it2->id, Quad_Coord(it->first.val(), it2->ll_lower)));
  }
}


void lookup_missing_ways
    (std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id,
     const Data_By_Id< Relation_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& known_skeletons_1,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& known_skeletons_2,
     Transaction& transaction)
{
  std::vector< Way_Skeleton::Id_Type > missing_ids;

  for (std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;

    std::vector< uint32 > nd_idxs;
    for (std::vector< Relation_Entry >::const_iterator nit = it->elem.members.begin();
         nit != it->elem.members.end(); ++nit)
    {
      if (nit->type == Relation_Entry::WAY &&
          new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val())) == new_way_idx_by_id.end())
        missing_ids.push_back(Way_Skeleton::Id_Type(nit->ref.val()));
    }
  }

  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it = known_skeletons_1.begin();
       it != known_skeletons_1.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      for (std::vector< Relation_Entry >::const_iterator nit = it2->members.begin();
           nit != it2->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::WAY &&
            new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val())) == new_way_idx_by_id.end())
          missing_ids.push_back(Way_Skeleton::Id_Type(nit->ref.val()));
      }
    }
  }

  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it = known_skeletons_2.begin();
       it != known_skeletons_2.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      for (std::vector< Relation_Entry >::const_iterator nit = it2->members.begin();
           nit != it2->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::WAY &&
            new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val())) == new_way_idx_by_id.end())
          missing_ids.push_back(Way_Skeleton::Id_Type(nit->ref.val()));
      }
    }
  }

  std::sort(missing_ids.begin(), missing_ids.end());
  missing_ids.erase(std::unique(missing_ids.begin(), missing_ids.end()), missing_ids.end());

  // Collect all data of existing id indexes
  std::vector< std::pair< Way_Skeleton::Id_Type, Uint31_Index > > existing_map_positions
      = get_existing_map_positions(missing_ids, transaction, *osm_base_settings().WAYS);

  // Collect all data of existing skeletons
  std::map< Uint31_Index, std::set< Way_Skeleton > > existing_skeletons
      = get_existing_skeletons< Way_Skeleton >
      (existing_map_positions, transaction, *osm_base_settings().WAYS);

  for (std::map< Uint31_Index, std::set< Way_Skeleton > >::const_iterator it = existing_skeletons.begin();
       it != existing_skeletons.end(); ++it)
  {
    for (std::set< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      new_way_idx_by_id.insert(std::make_pair(it2->id, it->first.val()));
  }
}


/* We assert that every node id that appears in a relation in existing_skeletons has its Quad_Coord
   in new_node_idx_by_id. */
void compute_geometry
    (const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id,
     Data_By_Id< Relation_Skeleton >& new_data)
{
  std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator next_it = new_data.data.begin();
  for (std::vector< Data_By_Id< Relation_Skeleton >::Entry >::iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && next_it->elem.id == it->elem.id)
      // We don't care on intermediate versions
      continue;

    if (it->idx == 0u)
      // We don't touch deleted objects
      continue;

    std::vector< uint32 > member_idxs;
    for (std::vector< Relation_Entry >::const_iterator nit = it->elem.members.begin();
         nit != it->elem.members.end(); ++nit)
    {
      if (nit->type == Relation_Entry::NODE)
      {
        std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2
            = new_node_idx_by_id.find(Node_Skeleton::Id_Type(nit->ref.val()));
        if (it2 != new_node_idx_by_id.end())
          member_idxs.push_back(it2->second.ll_upper);
        else
          ;//std::cerr<<"compute_geometry: Node "<<nit->ref.val()<<" used in relation "<<it->elem.id.val()<<" not found.\n";
      }
      else if (nit->type == Relation_Entry::WAY)
      {
        std::map< Way_Skeleton::Id_Type, Uint31_Index >::const_iterator it2
            = new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val()));
        if (it2 != new_way_idx_by_id.end())
          member_idxs.push_back(it2->second.val());
        else
          ;//std::cerr<<"compute_geometry: Way "<<nit->ref.val()<<" used in relation "<<it->elem.id.val()<<" not found.\n";
      }
    }

    Uint31_Index index = Relation::calc_index(member_idxs);

    it->elem.node_idxs.clear();
    it->elem.way_idxs.clear();

    if (Relation::indicates_geometry(index))
    {
      for (std::vector< Relation_Entry >::const_iterator nit = it->elem.members.begin();
           nit != it->elem.members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE)
        {
          std::map< Node_Skeleton::Id_Type, Quad_Coord >::const_iterator it2
              = new_node_idx_by_id.find(Node_Skeleton::Id_Type(nit->ref.val()));
          if (it2 != new_node_idx_by_id.end())
            it->elem.node_idxs.push_back(it2->second.ll_upper);
        }
        else if (nit->type == Relation_Entry::WAY)
        {
          std::map< Way_Skeleton::Id_Type, Uint31_Index >::const_iterator it2
              = new_way_idx_by_id.find(Way_Skeleton::Id_Type(nit->ref.val()));
          if (it2 != new_way_idx_by_id.end())
            it->elem.way_idxs.push_back(it2->second);
        }
      }
    }

    it->idx = index;
  }
}


void compute_idx_and_geometry
    (Uint31_Index& idx, Relation_Skeleton& skeleton,
     uint64 expiration_timestamp,
     const std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >& nodes_by_id,
     const std::map< Way_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >& ways_by_id)
{
  std::vector< Uint31_Index > node_idxs;
  std::vector< Uint31_Index > way_idxs;

  for (std::vector< Relation_Entry >::const_iterator mit = skeleton.members.begin();
       mit != skeleton.members.end(); ++mit)
  {
      if (mit->type == Relation_Entry::NODE)
      {
        std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
            ::const_iterator nit = nodes_by_id.find(Node_Skeleton::Id_Type(mit->ref.val()));
        if (nit != nodes_by_id.end() && !nit->second.empty())
        {
          std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > >::const_iterator
              it2 = nit->second.begin();
          while (it2 != nit->second.end() && it2->second.timestamp < expiration_timestamp)
            ++it2;
          if (it2 != nit->second.end())
            node_idxs.push_back(it2->first);
          // Otherwise the node has expired before our relation - something has gone wrong seriously.
        }
        else
          ;//std::cerr<<"compute_idx_and_geometry: Node "<<mit->ref.val()<<" used in relation "<<skeleton.id.val()<<" not found.\n";
        // Otherwise the node is not contained in our list - something has gone wrong seriously.
      }
      else if (mit->type == Relation_Entry::WAY)
      {
        std::map< Way_Skeleton::Id_Type,
            std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >
            ::const_iterator nit = ways_by_id.find(Way_Skeleton::Id_Type(mit->ref.val()));
        if (nit != ways_by_id.end() && !nit->second.empty())
        {
          std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > >::const_iterator
              it2 = nit->second.begin();
          while (it2 != nit->second.end() && it2->second.timestamp < expiration_timestamp)
            ++it2;
          if (it2 != nit->second.end())
            way_idxs.push_back(it2->first);
          // Otherwise the way has expired before our relation - something has gone wrong seriously.
        }
        else
          ;//std::cerr<<"compute_idx_and_geometry: Way "<<mit->ref.val()<<" used in relation "<<skeleton.id.val()<<" not found.\n";
        // Otherwise the way is not contained in our list - something has gone wrong seriously.
      }
  }

  std::vector< uint32 > member_idxs;
  for (std::vector< Uint31_Index >::const_iterator it = node_idxs.begin(); it != node_idxs.end(); ++it)
    member_idxs.push_back(it->val());
  for (std::vector< Uint31_Index >::const_iterator it = way_idxs.begin(); it != way_idxs.end(); ++it)
    member_idxs.push_back(it->val());
  std::sort(member_idxs.begin(), member_idxs.end());
  member_idxs.erase(std::unique(member_idxs.begin(), member_idxs.end()), member_idxs.end());

  idx = Relation::calc_index(member_idxs);

  if (Relation::indicates_geometry(idx))
  {
    skeleton.node_idxs.swap(node_idxs);
    skeleton.way_idxs.swap(way_idxs);
  }
  else
  {
    skeleton.node_idxs.clear();
    skeleton.way_idxs.clear();
  }
}


/* Checks the nds of the relation whether in the time window an underlying node has moved.
 * If yes, the necessary intermediate versions are generated.
 */
Relation_Skeleton add_intermediate_versions
    (const Relation_Skeleton& skeleton, const Relation_Skeleton& reference,
     const uint64 old_timestamp, const uint64 new_timestamp,
     const std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >& nodes_by_id,
     const std::map< Way_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >& ways_by_id,
     bool add_last_version, Uint31_Index attic_idx, Uint31_Index& last_idx,
     std::map< Uint31_Index, std::set< Attic< Relation_Delta > > >& full_attic,
     std::map< Uint31_Index, std::set< Attic< Relation_Skeleton::Id_Type > > >& new_undeleted,
     std::map< Relation_Skeleton::Id_Type, std::set< Uint31_Index > >& idx_lists)
{
  std::vector< uint64 > relevant_timestamps;
  for (std::vector< Relation_Entry >::const_iterator mit = skeleton.members.begin();
       mit != skeleton.members.end(); ++mit)
  {
    if (mit->type == Relation_Entry::NODE)
    {
      std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
          ::const_iterator nit = nodes_by_id.find(Node_Skeleton::Id_Type(mit->ref.val()));
      if (nit != nodes_by_id.end() && !nit->second.empty())
      {
        for (std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > >::const_iterator
            it2 = nit->second.begin(); it2 != nit->second.end(); ++it2)
        {
          if (old_timestamp < it2->second.timestamp && it2->second.timestamp <= new_timestamp)
            relevant_timestamps.push_back(it2->second.timestamp);
        }
      }
      // Otherwise the node is not contained in our list. Could happen if it hasn't changed at all.
    }
    else if (mit->type == Relation_Entry::WAY)
    {
      std::map< Way_Skeleton::Id_Type,
          std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >
          ::const_iterator nit = ways_by_id.find(Way_Skeleton::Id_Type(mit->ref.val()));
      if (nit != ways_by_id.end() && !nit->second.empty())
      {
        for (std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > >::const_iterator
            it2 = nit->second.begin(); it2 != nit->second.end(); ++it2)
        {
          if (old_timestamp < it2->second.timestamp && it2->second.timestamp <= new_timestamp)
            relevant_timestamps.push_back(it2->second.timestamp);
        }
      }
      // Otherwise the way is not contained in our list. Could happen if it hasn't changed at all.
    }
  }
  std::sort(relevant_timestamps.begin(), relevant_timestamps.end());
  relevant_timestamps.erase(std::unique(relevant_timestamps.begin(), relevant_timestamps.end()),
                            relevant_timestamps.end());

  // Care for latest element
  Uint31_Index idx = attic_idx;
  Relation_Skeleton cur_skeleton = skeleton;
  if (idx.val() == 0 || !relevant_timestamps.empty())
    compute_idx_and_geometry(idx, cur_skeleton, new_timestamp, nodes_by_id, ways_by_id);

  if (!relevant_timestamps.empty() && relevant_timestamps.back() == NOW)
    relevant_timestamps.pop_back();

  if ((add_last_version && old_timestamp < new_timestamp)
      || (!relevant_timestamps.empty() && relevant_timestamps.back() == new_timestamp))
  {
    Uint31_Index reference_idx;
    Relation_Skeleton reference_skel = reference;
    compute_idx_and_geometry(reference_idx, reference_skel, new_timestamp + 1, nodes_by_id, ways_by_id);
    if (idx == reference_idx)
      full_attic[idx].insert(Attic< Relation_Delta >(
          Relation_Delta(reference_skel, cur_skeleton), new_timestamp));
    else
      full_attic[idx].insert(Attic< Relation_Delta >(
          Relation_Delta(Relation_Skeleton(), cur_skeleton), new_timestamp));
    idx_lists[skeleton.id].insert(idx);

    // Manage undelete entries
    if (!(idx == reference_idx) && !(reference_idx == Uint31_Index(0xfe)))
      new_undeleted[reference_idx].insert(Attic< Relation_Skeleton::Id_Type >(skeleton.id, new_timestamp));

    if (!relevant_timestamps.empty() && relevant_timestamps.back() == new_timestamp)
      relevant_timestamps.pop_back();
  }

  // Track index for the undelete creation
  last_idx = idx;
  Relation_Skeleton last_skeleton = cur_skeleton;

  for (std::vector< uint64 >::const_iterator it = relevant_timestamps.end();
       it != relevant_timestamps.begin(); )
  {
    --it;

    Uint31_Index idx = attic_idx;
    Relation_Skeleton cur_skeleton = skeleton;
    if (idx.val() == 0 || it != relevant_timestamps.begin())
      compute_idx_and_geometry(idx, cur_skeleton, *it, nodes_by_id, ways_by_id);
    if (last_idx == idx)
      full_attic[idx].insert(Attic< Relation_Delta >(
          Relation_Delta(last_skeleton, cur_skeleton), *it));
    else
      full_attic[idx].insert(Attic< Relation_Delta >(
          Relation_Delta(Relation_Skeleton(), cur_skeleton), *it));
    idx_lists[skeleton.id].insert(idx);

    // Manage undelete entries
    if (!(last_idx == idx) && !(last_idx == Uint31_Index(0xfe)))
      new_undeleted[last_idx].insert(Attic< Relation_Skeleton::Id_Type >(skeleton.id, *it));
    last_idx = idx;
    last_skeleton = cur_skeleton;
  }

  if (last_idx == attic_idx)
    return last_skeleton;
  else
    return Relation_Skeleton();
}


/* Checks the nds of the relation whether in the time window an underlying node has moved.
 * If yes, the necessary intermediate versions are generated.
 */
void add_intermediate_changelog_entries
    (const Relation_Skeleton& skeleton, const uint64 old_timestamp, const uint64 new_timestamp,
     const std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >& nodes_by_id,
     const std::map< Way_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >& ways_by_id,
     bool add_last_version, Uint31_Index attic_idx, Uint31_Index new_idx,
     std::map< Timestamp, std::set< Change_Entry< Relation_Skeleton::Id_Type > > >& result)
{
  std::vector< uint64 > relevant_timestamps;
  for (std::vector< Relation_Entry >::const_iterator mit = skeleton.members.begin();
       mit != skeleton.members.end(); ++mit)
  {
    if (mit->type == Relation_Entry::NODE)
    {
      std::map< Node_Skeleton::Id_Type, std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > >
          ::const_iterator nit = nodes_by_id.find(Node_Skeleton::Id_Type(mit->ref.val()));
      if (nit != nodes_by_id.end() && !nit->second.empty())
      {
        for (std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > >::const_iterator
            it2 = nit->second.begin(); it2 != nit->second.end(); ++it2)
        {
          if (old_timestamp < it2->second.timestamp && it2->second.timestamp <= new_timestamp)
            relevant_timestamps.push_back(it2->second.timestamp);
        }
      }
      // Otherwise the node is not contained in our list. Could happen if it didn't changed at all.
    }
    else if (mit->type == Relation_Entry::WAY)
    {
      std::map< Way_Skeleton::Id_Type,
          std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > >
          ::const_iterator nit = ways_by_id.find(Way_Skeleton::Id_Type(mit->ref.val()));
      if (nit != ways_by_id.end() && !nit->second.empty())
      {
        for (std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > >::const_iterator
            it2 = nit->second.begin(); it2 != nit->second.end(); ++it2)
        {
          if (old_timestamp < it2->second.timestamp && it2->second.timestamp <= new_timestamp)
            relevant_timestamps.push_back(it2->second.timestamp);
        }
      }
      // Otherwise the way is not contained in our list. Could happen if it didn't changed at all.
    }
  }
  std::sort(relevant_timestamps.begin(), relevant_timestamps.end());
  relevant_timestamps.erase(std::unique(relevant_timestamps.begin(), relevant_timestamps.end()),
                            relevant_timestamps.end());

  if (!relevant_timestamps.empty() && relevant_timestamps.back() == NOW)
    relevant_timestamps.pop_back();

  std::vector< Uint31_Index > idxs;

  for (std::vector< uint64 >::const_iterator it = relevant_timestamps.begin();
       it != relevant_timestamps.end(); ++it)
  {
    Uint31_Index idx = attic_idx;
    attic_idx = Uint31_Index(0u);
    Relation_Skeleton cur_skeleton = skeleton;
    if (idx.val() == 0)
      compute_idx_and_geometry(idx, cur_skeleton, *it, nodes_by_id, ways_by_id);
    idxs.push_back(idx);
  }

  Uint31_Index idx = attic_idx;
  Relation_Skeleton last_skeleton = skeleton;
  if (idx.val() == 0)
    compute_idx_and_geometry(idx, last_skeleton, new_timestamp, nodes_by_id, ways_by_id);
  idxs.push_back(idx);

  int i = 0;
  for (std::vector< uint64 >::const_iterator it = relevant_timestamps.begin();
       it != relevant_timestamps.end(); ++it)
  {
    result[Timestamp(*it)].insert(
        Change_Entry< Relation_Skeleton::Id_Type >(skeleton.id, idxs[i], idxs[i+1]));
    ++i;
  }

  if (add_last_version)
    result[Timestamp(new_timestamp)].insert(
        Change_Entry< Relation_Skeleton::Id_Type >(skeleton.id, idx, new_idx));
}


bool geometrically_equal(const Relation_Skeleton& a, const Relation_Skeleton& b)
{
  return (a.members == b.members);
}


void adapt_newest_existing_attic
    (Uint31_Index old_idx, Uint31_Index new_idx,
     const Attic< Relation_Delta >& existing_delta,
     const Relation_Skeleton& existing_reference,
     const Relation_Skeleton& new_reference,
     std::map< Uint31_Index, std::set< Attic< Relation_Delta > > >& attic_skeletons_to_delete,
     std::map< Uint31_Index, std::set< Attic< Relation_Delta > > >& full_attic)
{
  Relation_Delta new_delta(old_idx == new_idx ? new_reference : Relation_Skeleton(),
			   existing_delta.expand(existing_reference));
  if (new_delta.members_added != existing_delta.members_added
      || new_delta.members_removed != existing_delta.members_removed
      || new_delta.node_idxs_added != existing_delta.node_idxs_added
      || new_delta.node_idxs_removed != existing_delta.node_idxs_removed
      || new_delta.way_idxs_added != existing_delta.way_idxs_added
      || new_delta.way_idxs_removed != existing_delta.way_idxs_removed)
  {
    attic_skeletons_to_delete[old_idx].insert(existing_delta);
    full_attic[new_idx].insert(Attic< Relation_Delta >(new_delta, existing_delta.timestamp));
    std::cerr<<"Relation "<<existing_delta.id.val()<<" has changed at timestamp "
        <<Timestamp(existing_delta.timestamp).str()<<" in two different diffs.\n";
  }
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the std::set of elements to store to attic.
 * We use that in attic_skeletons can only appear elements with ids that exist also in new_data. */
void compute_new_attic_skeletons
    (const Data_By_Id< Relation_Skeleton >& new_data,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& implicitly_moved_skeletons,
     const std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
     const std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > >& attic_map_positions,
     const std::map< Uint31_Index, std::set< Relation_Skeleton > >& attic_skeletons,
     const std::map< Relation_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Relation_Delta > > >&
         existing_attic_skeleton_timestamps,
     const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
     const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
     const std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id,
     const std::map< Uint31_Index, std::set< Attic< Way_Delta > > >& new_attic_way_skeletons,
     std::map< Uint31_Index, std::set< Attic< Relation_Delta > > >& full_attic,
     std::map< Uint31_Index, std::set< Attic< Relation_Skeleton::Id_Type > > >& new_undeleted,
     std::map< Relation_Skeleton::Id_Type, std::set< Uint31_Index > >& idx_lists,
     std::map< Uint31_Index, std::set< Attic< Relation_Delta > > >& attic_skeletons_to_delete)
{
  // Fill nodes_by_id from attic nodes as well as the current nodes in new_node_idx_by_id
  std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > > nodes_by_id
         = collect_nodes_by_id(new_attic_node_skeletons, new_node_idx_by_id);

  // Fill ways_by_id from attic ways as well as the current ways in new_way_idx_by_id
  std::map< Way_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > > ways_by_id
         = collect_ways_by_id(new_attic_way_skeletons, new_way_idx_by_id);

  // Create full_attic and idx_lists by going through new_data and filling the gaps
  std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Relation_Skeleton::Id_Type last_id = Relation_Skeleton::Id_Type(0u);
  for (std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    Uint31_Index it_idx = it->idx;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
    {
      if (it->idx.val() != 0)
      {
        add_intermediate_versions(it->elem, next_it->elem, it->meta.timestamp, next_it->meta.timestamp,
                                  nodes_by_id, ways_by_id,
                                  // Add last version only if it differs from the next version
                                  (next_it->idx.val() == 0 || !geometrically_equal(it->elem, next_it->elem)),
                                  Uint31_Index(0u), it_idx, full_attic, new_undeleted, idx_lists);
      }
    }

    if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
      // This is the latest version of this element. Care here for changes since this element.
      add_intermediate_versions(it->elem, Relation_Skeleton(), it->meta.timestamp, NOW,
                                nodes_by_id, ways_by_id, false, Uint31_Index(0u),
                                it_idx, full_attic, new_undeleted, idx_lists);

    if (last_id == it->elem.id)
    {
      // An earlier version exists also in new_data.
      std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator last_it = it;
      --last_it;
      if (last_it->idx == Uint31_Index(0u))
      {
        if (it_idx.val() == 0xff)
        {
          Relation_Skeleton skel = it->elem;
          compute_idx_and_geometry(it_idx, skel, it->meta.timestamp + 1, nodes_by_id, ways_by_id);
        }
        new_undeleted[it_idx].insert(Attic< Relation_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
      }
      continue;
    }
    else
    {
      const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
      const Uint31_Index* idx_attic = binary_pair_search(attic_map_positions, it->elem.id);
      if (!idx && idx_attic)
      {
        if (it_idx.val() == 0xff)
        {
          Relation_Skeleton skel = it->elem;
          compute_idx_and_geometry(it_idx, skel, it->meta.timestamp + 1, nodes_by_id, ways_by_id);
        }
        new_undeleted[it_idx].insert(Attic< Relation_Skeleton::Id_Type >(it->elem.id, it->meta.timestamp));
      }
    }
    last_id = it->elem.id;

    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    if (!idx)
      // No old data exists. So there is nothing to do here.
      continue;

    std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
      // Something has gone wrong. Skip this object.
      continue;

    std::set< Relation_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
      // Something has gone wrong. Skip this object.
      continue;

    std::map< Relation_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Relation_Delta > > >::const_iterator
        it_attic_time = existing_attic_skeleton_timestamps.find(it->elem.id);
    Relation_Skeleton oldest_new =
        add_intermediate_versions(*it_attic, it->elem,
			      it_attic_time == existing_attic_skeleton_timestamps.end() ?
			          uint64(0u) : it_attic_time->second.second.timestamp,
			      it->meta.timestamp, nodes_by_id, ways_by_id,
                              (it->idx.val() == 0 || !geometrically_equal(*it_attic, it->elem)),
                              *idx, it_idx, full_attic, new_undeleted, idx_lists);
    if (it_attic_time != existing_attic_skeleton_timestamps.end()
        && it_attic_time->second.second.id == it->elem.id)
      adapt_newest_existing_attic(it_attic_time->second.first, *idx, it_attic_time->second.second,
	  *it_attic, it_attic_time->second.second.timestamp < it->meta.timestamp ? oldest_new : Relation_Skeleton(),
	  attic_skeletons_to_delete, full_attic);
  }

  // Add the missing elements that result from node moves only
  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator
      it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      std::map< Relation_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Relation_Delta > > >::const_iterator
          it_attic_time = existing_attic_skeleton_timestamps.find(it2->id);
      Uint31_Index dummy;
      Relation_Skeleton oldest_new =
        add_intermediate_versions(*it2, *it2,
			        it_attic_time == existing_attic_skeleton_timestamps.end() ?
			            uint64(0u) : it_attic_time->second.second.timestamp,
				NOW, nodes_by_id, ways_by_id,
                                false, it->first,
                                dummy, full_attic, new_undeleted, idx_lists);
      if (it_attic_time != existing_attic_skeleton_timestamps.end()
          && it_attic_time->second.second.id == it2->id)
        adapt_newest_existing_attic(it_attic_time->second.first, it->first, it_attic_time->second.second,
	    *it2, oldest_new, attic_skeletons_to_delete, full_attic);
    }
  }
}


/* Compares the new data and the already existing skeletons to determine those that have
 * moved. This information is used to prepare the std::set of elements to store to attic.
 * We use that in attic_skeletons can only appear elements with ids that exist also in new_data. */
std::map< Timestamp, std::set< Change_Entry< Relation_Skeleton::Id_Type > > > compute_changelog(
    const Data_By_Id< Relation_Skeleton >& new_data,
    const std::map< Uint31_Index, std::set< Relation_Skeleton > >& implicitly_moved_skeletons,
    const std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > >& existing_map_positions,
    const std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > >& attic_map_positions,
    const std::map< Uint31_Index, std::set< Relation_Skeleton > >& attic_skeletons,
    const std::map< Node_Skeleton::Id_Type, Quad_Coord >& new_node_idx_by_id,
    const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
    const std::map< Way_Skeleton::Id_Type, Uint31_Index >& new_way_idx_by_id,
    const std::map< Uint31_Index, std::set< Attic< Way_Delta > > >& new_attic_way_skeletons)
{
  std::map< Timestamp, std::set< Change_Entry< Relation_Skeleton::Id_Type > > > result;

  // Fill nodes_by_id from attic nodes as well as the current nodes in new_node_idx_by_id
  std::map< Node_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Node_Skeleton > > > > nodes_by_id
         = collect_nodes_by_id(new_attic_node_skeletons, new_node_idx_by_id);

  // Fill ways_by_id from attic ways as well as the current ways in new_way_idx_by_id
  std::map< Way_Skeleton::Id_Type,
         std::vector< std::pair< Uint31_Index, Attic< Way_Skeleton::Id_Type > > > > ways_by_id
         = collect_ways_by_id(new_attic_way_skeletons, new_way_idx_by_id);

  std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator next_it
      = new_data.data.begin();
  Relation_Skeleton::Id_Type last_id = Relation_Skeleton::Id_Type(0u);
  for (std::vector< Data_By_Id< Relation_Skeleton >::Entry >::const_iterator
      it = new_data.data.begin(); it != new_data.data.end(); ++it)
  {
    ++next_it;
    if (next_it != new_data.data.end() && it->elem.id == next_it->elem.id)
    {
      Uint31_Index next_idx = next_it->idx;
      if (next_idx.val() == 0xff)
      {
        Relation_Skeleton skel = next_it->elem;
        compute_idx_and_geometry(next_idx, skel, next_it->meta.timestamp + 1, nodes_by_id, ways_by_id);
      }
      // A later version exists also in new_data.
      add_intermediate_changelog_entries(
           it->elem, it->meta.timestamp, next_it->meta.timestamp,
           nodes_by_id, ways_by_id,
           true, Uint31_Index(0u), next_idx, result);
    }

    if (next_it == new_data.data.end() || !(it->elem.id == next_it->elem.id))
      // This is the latest version of this element. Care here for changes since this element.
      add_intermediate_changelog_entries(it->elem, it->meta.timestamp, NOW, nodes_by_id, ways_by_id,
                                false, 0u, 0u, result);

    if (last_id == it->elem.id)
      // An earlier version exists also in new_data. So there is nothing to do here.
      continue;
    last_id = it->elem.id;

    const Uint31_Index* idx = binary_pair_search(existing_map_positions, it->elem.id);
    Uint31_Index next_idx = it->idx;
    if (next_idx.val() == 0xff)
    {
      Relation_Skeleton skel = it->elem;
      compute_idx_and_geometry(next_idx, skel, it->meta.timestamp + 1, nodes_by_id, ways_by_id);
    }
    if (!idx)
    {
      // No old data exists.
      result[it->meta.timestamp].insert(
          Change_Entry< Relation_Skeleton::Id_Type >(it->elem.id, 0u, next_idx));
      continue;
    }

    std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator it_attic_idx
        = attic_skeletons.find(*idx);
    if (it_attic_idx == attic_skeletons.end())
      // Something has gone wrong. Skip this object.
      continue;

    std::set< Relation_Skeleton >::iterator it_attic
        = it_attic_idx->second.find(it->elem);
    if (it_attic == it_attic_idx->second.end())
      // Something has gone wrong. Skip this object.
      continue;

    add_intermediate_changelog_entries(*it_attic, 0, it->meta.timestamp, nodes_by_id, ways_by_id,
                              true, *idx, next_idx, result);
  }

  // Add the missing elements that result from node moves only
  for (std::map< Uint31_Index, std::set< Relation_Skeleton > >::const_iterator
      it = implicitly_moved_skeletons.begin(); it != implicitly_moved_skeletons.end(); ++it)
  {
    for (std::set< Relation_Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      add_intermediate_changelog_entries(*it2, 0, NOW, nodes_by_id, ways_by_id,
                                false, it->first, 0u, result);
  }

  return result;
}


void Relation_Updater::update(Osm_Backend_Callback* callback, Cpu_Stopwatch* cpu_stopwatch,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_node_skeletons,
              const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
              const std::map< Uint31_Index, std::set< Way_Skeleton > >& new_way_skeletons,
              const std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_way_skeletons,
              const std::map< Uint31_Index, std::set< Attic< Way_Delta > > >& new_attic_way_skeletons)
{
  if (cpu_stopwatch)
    cpu_stopwatch->start_cpu_timer(3);

  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");

  // Prepare collecting all data of existing skeletons
  std::stable_sort(new_data.data.begin(), new_data.data.end());
  if (meta == keep_attic)
    remove_time_inconsistent_versions(new_data);
  else
    deduplicate_data(new_data);
  std::vector< Relation_Skeleton::Id_Type > ids_to_update_ = ids_to_update(new_data);

  // Collect all data of existing id indexes
  std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > > existing_map_positions
      = get_existing_map_positions(ids_to_update_, *transaction, *osm_base_settings().RELATIONS);

  // Collect all data of existing and explicitly changed skeletons
  std::map< Uint31_Index, std::set< Relation_Skeleton > > existing_skeletons
      = get_existing_skeletons< Relation_Skeleton >
      (existing_map_positions, *transaction, *osm_base_settings().RELATIONS);

  // Collect also all data of existing and implicitly changed skeletons
  std::map< Uint31_Index, std::set< Relation_Skeleton > > implicitly_moved_skeletons
      = get_implicitly_moved_skeletons
          (attic_node_skeletons, attic_way_skeletons,
           existing_skeletons, *transaction, *osm_base_settings().RELATIONS);

  // Collect all data of existing meta elements
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation::Id_Type > > > existing_meta
      = (meta ? get_existing_meta< OSM_Element_Metadata_Skeleton< Relation::Id_Type > >
             (existing_map_positions, *transaction, *meta_settings().RELATIONS_META) :
         std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation::Id_Type > > >());

  // Collect all data of existing meta elements
  std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > > implicitly_moved_positions
      = make_id_idx_directory(implicitly_moved_skeletons);
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation::Id_Type > > > implicitly_moved_meta
      = (meta ? get_existing_meta< OSM_Element_Metadata_Skeleton< Relation::Id_Type > >
             (implicitly_moved_positions, *transaction, *meta_settings().RELATIONS_META) :
         std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation::Id_Type > > >());

  // Collect all data of existing tags
  std::vector< Tag_Entry< Relation_Skeleton::Id_Type > > existing_local_tags;
  get_existing_tags< Relation_Skeleton::Id_Type >
      (existing_map_positions, *transaction->data_index(osm_base_settings().RELATION_TAGS_LOCAL),
       existing_local_tags);

  // Collect all data of existing tags for moved relations
  std::vector< Tag_Entry< Relation_Skeleton::Id_Type > > implicitly_moved_local_tags;
  get_existing_tags< Relation_Skeleton::Id_Type >
      (implicitly_moved_positions, *transaction->data_index(osm_base_settings().RELATION_TAGS_LOCAL),
       implicitly_moved_local_tags);

  // Create a node directory id to idx:
  // Evaluate first the new_node_skeletons
  std::map< Node_Skeleton::Id_Type, Quad_Coord > new_node_idx_by_id
      = dictionary_from_skeletons(new_node_skeletons);
  // Then lookup the missing nodes.
  lookup_missing_nodes(new_node_idx_by_id, new_data, existing_skeletons, implicitly_moved_skeletons,
                       *transaction);

  // Create a node directory id to idx:
  // Evaluate first the new_way_skeletons
  std::map< Way_Skeleton::Id_Type, Uint31_Index > new_way_idx_by_id
      = dictionary_from_skeletons(new_way_skeletons);
  // Then lookup the missing nodes.
  lookup_missing_ways(new_way_idx_by_id, new_data, existing_skeletons, implicitly_moved_skeletons,
                      *transaction);

  // Compute the indices of the new relations
  compute_geometry(new_node_idx_by_id, new_way_idx_by_id, new_data);

  // Compute which objects really have changed
  attic_skeletons.clear();
  new_skeletons.clear();
  new_current_skeletons(new_data, existing_map_positions, existing_skeletons,
      0, attic_skeletons, new_skeletons, moved_relations);

  // Compute and add implicitly moved relations
  new_implicit_skeletons(new_node_idx_by_id, new_way_idx_by_id, implicitly_moved_skeletons,
      0, attic_skeletons, new_skeletons, moved_relations);

  // Compute which meta data really has changed
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > > > attic_meta;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > > > new_meta;
  new_current_meta(new_data, existing_map_positions, existing_meta, attic_meta, new_meta);

  // Compute which meta data has moved
  std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > > new_positions
      = make_id_idx_directory(new_skeletons);
  new_implicit_meta(implicitly_moved_meta, new_positions, attic_meta, new_meta);

  // Compute which tags really have changed
  std::map< Tag_Index_Local, std::set< Relation_Skeleton::Id_Type > > attic_local_tags;
  std::map< Tag_Index_Local, std::set< Relation_Skeleton::Id_Type > > new_local_tags;
  new_current_local_tags< Relation_Skeleton, Relation_Skeleton::Id_Type >
      (new_data, existing_map_positions, existing_local_tags, attic_local_tags, new_local_tags);
  new_implicit_local_tags(implicitly_moved_local_tags, new_positions, attic_local_tags, new_local_tags);
  std::map< Tag_Index_Global, std::set< Tag_Object_Global< Relation_Skeleton::Id_Type > > > attic_global_tags;
  std::map< Tag_Index_Global, std::set< Tag_Object_Global< Relation_Skeleton::Id_Type > > > new_global_tags;
  new_current_global_tags< Relation_Skeleton::Id_Type >
      (attic_local_tags, new_local_tags, attic_global_tags, new_global_tags);

  add_deleted_skeletons(attic_skeletons, new_positions);

  callback->update_started();
  callback->prepare_delete_tags_finished();

  store_new_keys(new_data, keys, *transaction);

  // Update id indexes
  update_map_positions(new_positions, *transaction, *osm_base_settings().RELATIONS);
  callback->update_ids_finished();

  // Update skeletons
  update_elements(attic_skeletons, new_skeletons, *transaction, *osm_base_settings().RELATIONS);
  callback->update_coords_finished();

  // Update meta
  if (meta)
    update_elements(attic_meta, new_meta, *transaction, *meta_settings().RELATIONS_META);

  // Update local tags
  update_elements(attic_local_tags, new_local_tags, *transaction, *osm_base_settings().RELATION_TAGS_LOCAL);
  callback->tags_local_finished();

  // Update global tags
  update_elements(attic_global_tags, new_global_tags, *transaction, *osm_base_settings().RELATION_TAGS_GLOBAL);
  callback->tags_global_finished();

  flush_roles();
  callback->flush_roles_finished();

  std::map< uint32, std::vector< uint32 > > idxs_by_id;

  if (meta == keep_attic)
  {
    // TODO: For compatibility with the update_logger, this doesn't happen during the tag processing itself.
    //cancel_out_equal_tags(attic_local_tags, new_local_tags);

    // Also include ids from the only moved relations
    enhance_ids_to_update(implicitly_moved_skeletons, ids_to_update_);

    // Collect all data of existing attic id indexes
    std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > > existing_attic_map_positions
        = get_existing_map_positions(ids_to_update_, *transaction, *attic_settings().RELATIONS);
    std::map< Relation_Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists
        = get_existing_idx_lists(ids_to_update_, existing_attic_map_positions,
                                 *transaction, *attic_settings().RELATION_IDX_LIST);

    // Collect known change times of attic elements. This allows that
    // for each object no older version than the youngest known attic version can be written
    std::map< Relation_Skeleton::Id_Type, std::pair< Uint31_Index, Attic< Relation_Delta > > >
        existing_attic_skeleton_timestamps
        = get_existing_attic_skeleton_timestamps< Uint31_Index, Relation_Skeleton, Relation_Delta >
        (existing_attic_map_positions, existing_idx_lists,
	 *transaction, *attic_settings().RELATIONS, *attic_settings().RELATIONS_UNDELETED);

    // Compute which objects really have changed
    new_attic_skeletons.clear();
    std::map< Relation_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists = existing_idx_lists;
    std::map< Uint31_Index, std::set< Attic< Relation_Skeleton::Id_Type > > > new_undeleted;
    std::map< Uint31_Index, std::set< Attic< Relation_Delta > > > attic_skeletons_to_delete;
    compute_new_attic_skeletons(new_data, implicitly_moved_skeletons,
                                existing_map_positions, existing_attic_map_positions, attic_skeletons,
				existing_attic_skeleton_timestamps,
                                new_node_idx_by_id, new_attic_node_skeletons,
                                new_way_idx_by_id, new_attic_way_skeletons,
                                new_attic_skeletons, new_undeleted, new_attic_idx_lists, attic_skeletons_to_delete);

    std::map< Relation_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > > new_attic_idx_by_id_and_time =
        compute_new_attic_idx_by_id_and_time(new_data, new_skeletons, new_attic_skeletons);

    // Compute new meta data
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > > >
        new_attic_meta = compute_new_attic_meta(new_attic_idx_by_id_and_time,
            compute_meta_by_id_and_time(new_data, attic_meta), new_meta);

    // Compute tags
    std::map< Tag_Index_Local, std::set< Attic< Relation_Skeleton::Id_Type > > > new_attic_local_tags
        = compute_new_attic_local_tags(new_attic_idx_by_id_and_time,
            compute_tags_by_id_and_time(new_data, attic_local_tags),
                                       existing_map_positions, existing_idx_lists);
    std::map< Tag_Index_Global, std::set< Attic< Tag_Object_Global< Relation_Skeleton::Id_Type > > > >
        new_attic_global_tags = compute_attic_global_tags(new_attic_local_tags);

    // Compute changelog
    std::map< Timestamp, std::set< Change_Entry< Relation_Skeleton::Id_Type > > > changelog
        = compute_changelog(new_data, implicitly_moved_skeletons,
                            existing_map_positions, existing_attic_map_positions, attic_skeletons,
                            new_node_idx_by_id, new_attic_node_skeletons,
                            new_way_idx_by_id, new_attic_way_skeletons);

    strip_single_idxs(existing_idx_lists);
    std::vector< std::pair< Relation_Skeleton::Id_Type, Uint31_Index > > new_attic_map_positions
        = strip_single_idxs(new_attic_idx_lists);

    // Prepare user indices
    copy_idxs_by_id(new_attic_meta, idxs_by_id);

    // Update id indexes
    update_map_positions(new_attic_map_positions, *transaction, *attic_settings().RELATIONS);

    // Update id index lists
    update_elements(existing_idx_lists, new_attic_idx_lists,
                    *transaction, *attic_settings().RELATION_IDX_LIST);

    // Add attic elements
    update_elements(attic_skeletons_to_delete, new_attic_skeletons,
                    *transaction, *attic_settings().RELATIONS);

    // Add attic elements
    update_elements(std::map< Uint31_Index, std::set< Attic< Relation_Skeleton::Id_Type > > >(),
                    new_undeleted, *transaction, *attic_settings().RELATIONS_UNDELETED);

    // Add attic meta
    update_elements
        (std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > > >(),
         new_attic_meta, *transaction, *attic_settings().RELATIONS_META);

    // Update tags
    update_elements(std::map< Tag_Index_Local, std::set< Attic < Relation_Skeleton::Id_Type > > >(),
                    new_attic_local_tags, *transaction, *attic_settings().RELATION_TAGS_LOCAL);
    update_elements(std::map< Tag_Index_Global,
                        std::set< Attic < Tag_Object_Global< Relation_Skeleton::Id_Type > > > >(),
                    new_attic_global_tags, *transaction, *attic_settings().RELATION_TAGS_GLOBAL);

    // Write changelog
    update_elements(std::map< Timestamp, std::set< Change_Entry< Relation_Skeleton::Id_Type > > >(), changelog,
                    *transaction, *attic_settings().RELATION_CHANGELOG);

    flush_roles();
  }

  if (meta != only_data)
  {
    copy_idxs_by_id(new_meta, idxs_by_id);
    process_user_data(*transaction, user_by_id, idxs_by_id);
  }
  callback->update_finished();

  new_data.data.clear();
//   rels_meta_to_delete.clear();
//   rels_meta_to_insert.clear();

  new_skeletons.clear();
  attic_skeletons.clear();
  new_attic_skeletons.clear();

  if (!external_transaction)
    delete transaction;

  if (cpu_stopwatch)
    cpu_stopwatch->stop_cpu_timer(3);
}


std::vector< Uint31_Index > calc_node_idxs(const std::vector< uint32 >& node_idxs)
{
  std::vector< Uint31_Index > result;
  for (std::vector< uint32 >::size_type i = 0; i < node_idxs.size(); ++i)
    result.push_back((node_idxs[i] & 0x7fffff00) | 0x80000002);
  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());

  return result;
}

std::vector< Uint31_Index > calc_way_idxs(const std::vector< uint32 >& way_idxs)
{
  std::vector< Uint31_Index > result;
  for (std::vector< uint32 >::size_type i = 0; i < way_idxs.size(); ++i)
    result.push_back(way_idxs[i]);
  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());

  return result;
}


void Relation_Updater::flush_roles()
{
  std::map< Uint32_Index, std::set< String_Object > > db_to_delete;
  std::map< Uint32_Index, std::set< String_Object > > db_to_insert;

  for (std::map< std::string, uint32 >::const_iterator it(role_ids.begin());
      it != role_ids.end(); ++it)
  {
    if (it->second >= max_written_role_id)
      db_to_insert[Uint32_Index(it->second)].insert
      (String_Object(it->first));
    if (it->second >= max_role_id)
      max_role_id = it->second + 1;
  }

  Block_Backend< Uint32_Index, String_Object > roles_db
      (transaction->data_index(osm_base_settings().RELATION_ROLES));
  roles_db.update(db_to_delete, db_to_insert);
  max_written_role_id = max_role_id;
}


std::vector< std::string > Relation_Updater::get_roles()
{
  if (max_role_id == 0)
    load_roles();

  std::vector< std::string > roles(max_role_id);
  for (std::map< std::string, uint32 >::const_iterator it(role_ids.begin());
      it != role_ids.end(); ++it)
    roles[it->second] = it->first;
  return roles;
}
