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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__AREA_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__AREA_UPDATER_H

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

using namespace std;

struct Area_Pair_Comparator_By_Id {
  bool operator() (const pair< Area_Location, Uint31_Index >& a,
		   const pair< Area_Location, Uint31_Index >& b)
  {
    return (a.first.id < b.first.id);
  }
};

struct Area_Pair_Equal_Id {
  bool operator() (const pair< Area_Location, Uint31_Index >& a,
		   const pair< Area_Location, Uint31_Index >& b)
  {
    return (a.first.id == b.first.id);
  }
};

struct Area_Updater : public Area_Usage_Listener
{
  Area_Updater(Transaction& transaction_);
  Area_Updater(string db_dir_);
  
  ~Area_Updater() { flush(); }
  
  void set_id_deleted(uint32 id) { ids_to_modify.insert(id); }
  void set_area
      (uint32 id, const Uint31_Index& index,
       const vector< pair< string, string > >& tags,
       const set< uint32 >& used_indices);
  void set_area(const Uint31_Index& index, const Area_Location& area);
  void add_blocks(const map< Uint31_Index, vector< Area_Block > >& area_blocks_);
  void commit();  
  virtual void flush();
  
private:
  Transaction* transaction;
  bool external_transaction;
  string db_dir;
  map< Uint31_Index, vector< Area_Block > > area_blocks;
  unsigned int total_area_blocks_count;
  set< Area::Id_Type > ids_to_modify;
  vector< pair< Area_Location, Uint31_Index > > areas_to_insert;
  static Area_Pair_Comparator_By_Id area_comparator_by_id;
  static Area_Pair_Equal_Id area_equal_id;
  
  void update();
  void update_area_ids
      (map< Uint31_Index, set< Area_Skeleton > >& locations_to_delete,
       map< Uint31_Index, set< Area_Block > >& blocks_to_delete);  
  void update_members
      (const map< Uint31_Index, set< Area_Skeleton > >& locations_to_delete,
       const map< Uint31_Index, set< Area_Block > >& blocks_to_delete);
  void prepare_delete_tags
      (vector< Tag_Entry< uint32 > >& tags_to_delete,
       const map< Uint31_Index, set< Area_Skeleton > >& to_delete);
  void prepare_tags
      (vector< Tag_Entry< uint32 > >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
  void update_area_tags_local
      (const vector< Tag_Entry< uint32 > >& tags_to_delete);
  void update_area_tags_global
      (const vector< Tag_Entry< uint32 > >& tags_to_delete);
};

/** Implementation (inline functions): --------------------------------------*/

inline void Area_Updater::commit()
{
  if (total_area_blocks_count > 512*1024)
    update();
}

inline void Area_Updater::flush()
{
  try
  {
    if ((!ids_to_modify.empty()) ||
        (!areas_to_insert.empty()) ||
        (!area_blocks.empty()))
      update();
  }
  catch(File_Error e)
  {
    cerr<<"File_Error: "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

inline void Area_Updater::set_area
    (uint32 id, const Uint31_Index& index,
     const vector< pair< string, string > >& tags,
     const set< uint32 >& used_indices)
{
  ids_to_modify.insert(id);
  
  vector< uint32 > indices;
  for (set< uint32 >::const_iterator it(used_indices.begin());
      it != used_indices.end(); ++it)
    indices.push_back(*it);
    
  Area_Location area(id, indices);
  area.tags = tags;
  areas_to_insert.push_back(make_pair(area, index));
}

inline void Area_Updater::set_area(const Uint31_Index& index, const Area_Location& area)
{
  ids_to_modify.insert(area.id);
  areas_to_insert.push_back(make_pair(area, index));
}

#endif
