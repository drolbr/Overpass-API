/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
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

using namespace std;


struct Update_Way_Logger
{
public:
  void insertion(const Way& way)
  {
    map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator it = insert.find(way.id);
    if (it == insert.end())
      insert.insert(make_pair(way.id, make_pair< Way, OSM_Element_Metadata* >(way, 0)));
    else
      it->second.first = way;
    
    it = keep.find(way.id);
    if (it != keep.end())
      keep.erase(it);
  }
  
  void insertion(uint32 id, const OSM_Element_Metadata& meta)
  {
    if (insert[id].second)
      delete insert[id].second;
    insert[id].second = new OSM_Element_Metadata(meta);
  }
  
  void deletion(const Uint31_Index& index, const Way_Skeleton& skel)
  {
    map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator it = erase.find(skel.id);
    if (it == erase.end())
    {
      it = erase.insert(make_pair(skel.id, make_pair< Way, OSM_Element_Metadata* >
          (Way(skel.id), 0))).first;
    }
    else
      it->second.first = Way(skel.id);
    it->second.first.index = index.val();
    it->second.first.nds = skel.nds;
    it->second.first.segment_idxs = skel.segment_idxs;
  }
  
  void keeping(const Uint31_Index& index, const Way_Skeleton& skel)
  {
    map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator it = erase.find(skel.id);
    if (it != erase.end())
      return;
    
    it = keep.find(skel.id);
    if (it == keep.end())
    {
      it = keep.insert(make_pair(skel.id, make_pair< Way, OSM_Element_Metadata* >
          (Way(skel.id), 0))).first;
    }
    else
      it->second.first = Way(skel.id);
    it->second.first.index = index.val();
    it->second.first.nds = skel.nds;
    it->second.first.segment_idxs = skel.segment_idxs;
  }
  
  void deletion(const Tag_Index_Local& index, const Uint32_Index& ref)
  {
    map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator it = erase.find(ref.val());
    if (it != erase.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
    it = keep.find(ref.val());
    if (it != keep.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
  }
  
  void deletion(const Uint31_Index& index, const OSM_Element_Metadata_Skeleton& meta_skel)
  {
    map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator it = erase.find(meta_skel.ref);
    if (it != erase.end())
    {
      if (it->second.second)
        delete it->second.second;
      OSM_Element_Metadata* meta = new OSM_Element_Metadata();
      meta->version = meta_skel.version;
      meta->timestamp = meta_skel.timestamp;
      meta->changeset = meta_skel.changeset;
      meta->user_id = meta_skel.user_id;
      it->second.second = meta;
    }
    it = keep.find(meta_skel.ref);
    if (it != keep.end())
    {
      if (it->second.second)
        delete it->second.second;
      OSM_Element_Metadata* meta = new OSM_Element_Metadata();
      meta->version = meta_skel.version;
      meta->timestamp = meta_skel.timestamp;
      meta->changeset = meta_skel.changeset;
      meta->user_id = meta_skel.user_id;
      it->second.second = meta;
    }
  }
  
  map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator insert_begin() const
  { return insert.begin(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator insert_end() const
  { return insert.end(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator keep_begin() const
  { return keep.begin(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator keep_end() const
  { return keep.end(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator erase_begin() const
  { return erase.begin(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator erase_end() const
  { return erase.end(); }
  
  map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator insert_begin()
  { return insert.begin(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator insert_end()
  { return insert.end(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator keep_begin()
  { return keep.begin(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator keep_end()
  { return keep.end(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator erase_begin()
  { return erase.begin(); }
  map< uint32, pair< Way, OSM_Element_Metadata* > >::iterator erase_end()
  { return erase.end(); }

  ~Update_Way_Logger();
  
private:
  map< uint32, pair< Way, OSM_Element_Metadata* > > insert;
  map< uint32, pair< Way, OSM_Element_Metadata* > > keep;
  map< uint32, pair< Way, OSM_Element_Metadata* > > erase;
};


struct Way_Updater
{
  Way_Updater(Transaction& transaction, bool meta);
  
  Way_Updater(string db_dir, bool meta);
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_way
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< uint32 > nds,
       const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Way way;
    way.id = id;
    way.nds = nds;
    way.tags = tags;
    ways_to_insert.push_back(way);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= way.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      ways_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
  }
  
  void set_way(const Way& way,
	       const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(way.id, true));
    ways_to_insert.push_back(way);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= way.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      ways_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
  }
  
  void update(Osm_Backend_Callback* callback, bool partial,
	      Update_Way_Logger* update_logger);
  
  void update_moved_idxs
      (Osm_Backend_Callback* callback, const vector< pair< uint32, uint32 > >& moved_nodes,
       Update_Way_Logger* update_logger);
  
  const vector< pair< uint32, uint32 > >& get_moved_ways() const
  {
    return moved_ways;
  }
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  bool partial_possible;
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Way > ways_to_insert;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  vector< pair< uint32, uint32 > > moved_ways;
  string db_dir;
  
  bool meta;
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > > ways_meta_to_insert;
  map< uint32, string > user_by_id;
  
  void find_affected_ways(const vector< pair< uint32, uint32 > >& moved_nodes,
       Update_Way_Logger* update_logger);
  
  void compute_indexes(vector< Way* >& ways_ptr);

  void update_way_ids(const vector< Way* >& ways_ptr, map< uint32, vector< uint32 > >& to_delete,
		      bool record_minuscule_moves);
  
  void update_members
      (const vector< Way* >& ways_ptr, const map< uint32, vector< uint32 > >& to_delete,
       Update_Way_Logger* update_logger);
  
  void merge_files(const vector< string >& froms, string into);
};

#endif
