/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_UPDATER_H

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


struct Way_Updater
{
  Way_Updater(Transaction& transaction, meta_modes meta);
  
  Way_Updater(string db_dir, meta_modes meta);
  
  void set_id_deleted(Way::Id_Type id, const OSM_Element_Metadata* meta = 0)
  {
    if (meta)
      new_data.data.push_back(Data_By_Id< Way_Skeleton >::Entry
          (Uint31_Index(0u), Way_Skeleton(id),
           OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, *meta)));
    else
      new_data.data.push_back(Data_By_Id< Way_Skeleton >::Entry
          (Uint31_Index(0u), Way_Skeleton(id),
           OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id)));
    
    if (meta)
      user_by_id[meta->user_id] = meta->user_name;
  }
  
  void set_way(const Way& way,
	       const OSM_Element_Metadata* meta = 0)
  {
    if (meta)
      new_data.data.push_back(Data_By_Id< Way_Skeleton >::Entry
          (Uint31_Index(0xff), Way_Skeleton(way),
           OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(way.id, *meta),
           way.tags));
    else
      new_data.data.push_back(Data_By_Id< Way_Skeleton >::Entry
          (Uint31_Index(0xff), Way_Skeleton(way),
           OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(way.id),
           way.tags));
    
    if (meta)
      user_by_id[meta->user_id] = meta->user_name;
  }
  
  void update(Osm_Backend_Callback* callback, bool partial,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& new_node_skeletons,
              const std::map< Uint31_Index, std::set< Node_Skeleton > >& attic_node_skeletons,
              const std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > >& new_attic_node_skeletons);
  
  const vector< pair< Way::Id_Type, Uint31_Index > >& get_moved_ways() const
  {
    return moved_ways;
  }
  
  const std::map< Uint31_Index, std::set< Way_Skeleton > > get_new_skeletons() const
      { return new_skeletons; }
  const std::map< Uint31_Index, std::set< Way_Skeleton > > get_attic_skeletons() const
      { return attic_skeletons; }
  const std::map< Uint31_Index, std::set< Attic< Way_Delta > > > get_new_attic_skeletons() const
      { return new_attic_skeletons; }
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  bool partial_possible;
  vector< pair< Way::Id_Type, Uint31_Index > > moved_ways;
  string db_dir;

  Data_By_Id< Way_Skeleton > new_data;
  
  meta_modes meta;
  map< uint32, string > user_by_id;

  std::map< Uint31_Index, std::set< Way_Skeleton > > new_skeletons;
  std::map< Uint31_Index, std::set< Way_Skeleton > > attic_skeletons;
  std::map< Uint31_Index, std::set< Attic< Way_Delta > > > new_attic_skeletons;
  
  Key_Storage keys;
  
  void merge_files(const vector< string >& froms, string into);
};

#endif
