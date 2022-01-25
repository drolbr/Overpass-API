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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_UPDATER_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "basic_updater.h"


struct Node_Updater
{
  Node_Updater(Transaction& transaction, Database_Meta_State::mode meta);

  Node_Updater(std::string db_dir, Database_Meta_State::mode meta);

  void set_id_deleted(Node::Id_Type id, const OSM_Element_Metadata* meta = 0)
  {
    if (meta)
      new_data.data.push_back(Data_By_Id< Node_Skeleton >::Entry
          (Uint31_Index(0u), Node_Skeleton(id, 0u),
           OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(id, *meta)));
    else
      new_data.data.push_back(Data_By_Id< Node_Skeleton >::Entry
          (Uint31_Index(0u), Node_Skeleton(id, 0u),
           OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(id)));

    ids_to_modify.push_back(std::make_pair(id, false));
    if (meta)
      user_by_id[meta->user_id] = meta->user_name;
  }


  void set_node(const Node& node, const OSM_Element_Metadata* meta = 0)
  {
    if (meta)
      new_data.data.push_back(Data_By_Id< Node_Skeleton >::Entry
          (Uint31_Index(node.index), Node_Skeleton(node),
           OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(node.id, *meta),
           node.tags));
    else
      new_data.data.push_back(Data_By_Id< Node_Skeleton >::Entry
          (Uint31_Index(node.index), Node_Skeleton(node),
           OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(node.id),
           node.tags));

    ids_to_modify.push_back(std::make_pair(node.id, true));
    nodes_to_insert.push_back(node);
    if (meta)
      user_by_id[meta->user_id] = meta->user_name;
  }

  void update(Osm_Backend_Callback* callback, Cpu_Stopwatch* cpu_stopwatch, bool partial);

  const std::vector< std::pair< Node::Id_Type, Uint32_Index > >& get_moved_nodes() const
  {
    return moved_nodes;
  }

  const std::map< Uint31_Index, std::set< Node_Skeleton > > get_new_skeletons() const
      { return new_skeletons; }
  const std::map< Uint31_Index, std::set< Node_Skeleton > > get_attic_skeletons() const
      { return attic_skeletons; }
  const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > get_new_attic_skeletons() const
      { return new_attic_skeletons; }

private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  bool partial_possible;
  static Node_Comparator_By_Id node_comparator_by_id;
  static Node_Equal_Id node_equal_id;
  std::string db_dir;

  Data_By_Id< Node_Skeleton > new_data;

  std::vector< std::pair< Node::Id_Type, bool > > ids_to_modify;
  std::vector< Node > nodes_to_insert;
  std::vector< std::pair< Node::Id_Type, Uint32_Index > > moved_nodes;

  Database_Meta_State::mode meta;
  std::map< uint32, std::string > user_by_id;

  std::map< Uint31_Index, std::set< Node_Skeleton > > new_skeletons;
  std::map< Uint31_Index, std::set< Node_Skeleton > > attic_skeletons;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > new_attic_skeletons;

  Key_Storage keys;

  void update_node_ids(std::map< uint32, std::vector< Node::Id_Type > >& to_delete, bool record_minuscule_moves,
      const std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > >& new_idx_positions);

  void merge_files(const std::vector< std::string >& froms, std::string into);
};


#endif
