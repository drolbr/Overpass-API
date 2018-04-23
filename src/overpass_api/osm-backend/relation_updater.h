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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__RELATION_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__RELATION_UPDATER_H

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "basic_updater.h"


struct Relation_Updater
{
  Relation_Updater(Transaction& transaction, meta_modes meta);

  Relation_Updater(std::string db_dir, meta_modes meta);

  void set_id_deleted(Relation::Id_Type id, const OSM_Element_Metadata* meta = 0)
  {
    if (meta)
      new_data.data.push_back(Data_By_Id< Relation_Skeleton >::Entry
          (Uint31_Index(0u), Relation_Skeleton(id),
           OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(id, *meta)));
    else
      new_data.data.push_back(Data_By_Id< Relation_Skeleton >::Entry
          (Uint31_Index(0u), Relation_Skeleton(id),
           OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(id)));

    if (meta)
      user_by_id[meta->user_id] = meta->user_name;
  }

  void set_relation(const Relation& rel,
		    const OSM_Element_Metadata* meta = 0)
  {
    if (meta)
      new_data.data.push_back(Data_By_Id< Relation_Skeleton >::Entry
          (Uint31_Index(0xff), Relation_Skeleton(rel),
           OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(rel.id, *meta),
           rel.tags));
    else
      new_data.data.push_back(Data_By_Id< Relation_Skeleton >::Entry
          (Uint31_Index(0xff), Relation_Skeleton(rel),
           OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(rel.id),
           rel.tags));

    if (meta)
      user_by_id[meta->user_id] = meta->user_name;
  }

  uint32 get_role_id(const std::string& s);
  std::vector< std::string > get_roles();

  void update(Osm_Backend_Callback* callback, Cpu_Stopwatch* cpu_stopwatch,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_node_skeletons,
              const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons,
              const std::map< Uint31_Index, std::set< Way_Skeleton > >& new_way_skeletons,
              const std::map< Uint31_Index, std::set< Way_Skeleton > >& attic_way_skeletons,
              const std::map< Uint31_Index, std::set< Attic< Way_Delta > > >& new_attic_way_skeletons);

private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  std::map< std::string, uint32 > role_ids;
  uint32 max_role_id;
  uint32 max_written_role_id;
  std::vector< std::pair< Relation::Id_Type, Uint31_Index > > moved_relations;
  std::string db_dir;

  Data_By_Id< Relation_Skeleton > new_data;

  meta_modes meta;
  std::map< uint32, std::string > user_by_id;

  std::map< Uint31_Index, std::set< Relation_Skeleton > > new_skeletons;
  std::map< Uint31_Index, std::set< Relation_Skeleton > > attic_skeletons;
  std::map< Uint31_Index, std::set< Attic< Relation_Delta > > > new_attic_skeletons;

  Key_Storage keys;

  void flush_roles();
  void load_roles();
};

#endif
