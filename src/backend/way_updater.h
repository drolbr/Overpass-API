#ifndef DE_OSM3S__BACKEND__WAY_UPDATER
#define DE_OSM3S__BACKEND__WAY_UPDATER

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../core/datatypes.h"
#include "../dispatch/settings.h"
#include "block_backend.h"
#include "random_file.h"

using namespace std;

struct Way_Tag_Entry
{
  uint32 index;
  string key;
  string value;
  vector< uint32 > way_ids;
};

struct Way_Tag_Index_Local
{
  uint32 index;
  string key;
  string value;
  
  Way_Tag_Index_Local() {}
  
  Way_Tag_Index_Local(void* data)
  {
    index = (*((uint32*)data + 1))<<8;
    key = string(((int8*)data + 7), *(uint16*)data);
    value = string(((int8*)data + 7 + key.length()),
			   *((uint16*)data + 1));
  }
  
  uint32 size_of() const
  {
    return 7 + key.length() + value.length();
  }
  
  static uint32 size_of(void* data)
  {
    return (*((uint16*)data) + *((uint16*)data + 1) + 7);
  }
  
  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    *((uint32*)data + 1) = index>>8;
    memcpy(((uint8*)data + 7), key.data(), key.length());
    memcpy(((uint8*)data + 7 + key.length()), value.data(),
	     value.length());
  }
  
  bool operator<(const Way_Tag_Index_Local& a) const
  {
    if ((index & 0x7fffffff) != (a.index & 0x7fffffff))
      return ((index & 0x7fffffff) < (a.index & 0x7fffffff));
    if (index != a.index)
      return (index < a.index);
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Way_Tag_Index_Local& a) const
  {
    if (index != a.index)
      return false;
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Way_Tag_Index_Global
{
  string key;
  string value;
  
  Way_Tag_Index_Global() {}
  
  Way_Tag_Index_Global(void* data)
  {
    key = string(((int8*)data + 4), *(uint16*)data);
    value = string(((int8*)data + 4 + key.length()),
		     *((uint16*)data + 1));
  }
  
  uint32 size_of() const
  {
    return 4 + key.length() + value.length();
  }
  
  static uint32 size_of(void* data)
  {
    return (*((uint16*)data) + *((uint16*)data + 1) + 4);
  }
  
  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    memcpy(((uint8*)data + 4), key.data(), key.length());
    memcpy(((uint8*)data + 4 + key.length()), value.data(),
	     value.length());
  }
  
  bool operator<(const Way_Tag_Index_Global& a) const
  {
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Way_Tag_Index_Global& a) const
  {
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Way_Updater
{
  Way_Updater() {}
  
  void set_id_deleted(uint32 id)
  {
    ids_to_delete.push_back(id);
  }
  
  void set_way
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< uint32 > nds)
  {
    ids_to_delete.push_back(id);
    
    Way way;
    way.id = id;
    way.nds = nds;
    way.tags = tags;
    ways_to_insert.push_back(way);
  }
  
  void set_way(const Way& way)
  {
    ids_to_delete.push_back(way.id);
    ways_to_insert.push_back(way);
  }
  
  void update(bool partial = false)
  {
    cerr<<'.'<<' '<<time(NULL)<<' ';
    
    map< uint32, vector< uint32 > > to_delete;
    update_way_ids(to_delete);
    update_members(to_delete);

    vector< Way_Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_way_tags_local(tags_to_delete);
    update_way_tags_global(tags_to_delete);

    ids_to_delete.clear();
    ways_to_insert.clear();
    
    cerr<<'W'<<' '<<time(NULL)<<' ';
    
    if (!partial && (update_counter > 0))
    {
      if (update_counter > 64)
	merge_files(".1", ".0");
      if (update_counter > 8)
	merge_files(".0", "");
      update_counter = 0;
    }
    else if (partial)
    {
      if (++update_counter % 8 == 0)
	merge_files("", ".0");
      if (update_counter % 64 == 0)
	merge_files(".0", ".1");
    }
    
    cerr<<'w'<<' '<<time(NULL)<<' ';
  }
  
private:
  vector< uint32 > ids_to_delete;
  vector< Way > ways_to_insert;
  static Way_Comparator_By_Id way_comparator_by_id;
  uint32 update_counter;
  
  void update_way_ids(map< uint32, vector< uint32 > >& to_delete)
  {
    // retrieve the indices of the referred nodes
    map< uint32, uint32 > used_nodes;
    for (vector< Way >::const_iterator wit(ways_to_insert.begin());
	 wit != ways_to_insert.end(); ++wit)
    {
      for (vector< uint32 >::const_iterator nit(wit->nds.begin());
	   nit != wit->nds.end(); ++nit)
	used_nodes[*nit] = 0;
    }
    Random_File< Uint32_Index > node_random(de_osm3s_file_ids::NODES, false);
    for (map< uint32, uint32 >::iterator it(used_nodes.begin());
	 it != used_nodes.end(); ++it)
      it->second = node_random.get(it->first).val();
    for (vector< Way >::iterator wit(ways_to_insert.begin());
	 wit != ways_to_insert.end(); ++wit)
    {
      vector< uint32 > nd_idxs;
      for (vector< uint32 >::const_iterator nit(wit->nds.begin());
	   nit != wit->nds.end(); ++nit)
	nd_idxs.push_back(used_nodes[*nit]);
      wit->index = Way::calc_index(nd_idxs);
    }
    
    // process the ways itself
    sort(ids_to_delete.begin(), ids_to_delete.end());
    sort(ways_to_insert.begin(), ways_to_insert.end(), way_comparator_by_id);
    
    Random_File< Uint31_Index > random(de_osm3s_file_ids::WAYS, true);
    vector< Way >::const_iterator wit(ways_to_insert.begin());
    for (vector< uint32 >::const_iterator it(ids_to_delete.begin());
        it != ids_to_delete.end(); ++it)
    {
      Uint31_Index index(random.get(*it));
      if (index.val() > 0)
	to_delete[index.val()].push_back(*it);
      if ((wit != ways_to_insert.end()) && (*it == wit->id))
      {
	random.put(*it, Uint31_Index(wit->index));
	++wit;
      }
    }
  }
  
  void update_members(const map< uint32, vector< uint32 > >& to_delete)
  {
    map< Uint31_Index, set< Way_Skeleton > > db_to_delete;
    map< Uint31_Index, set< Way_Skeleton > > db_to_insert;
    
    for (map< uint32, vector< uint32 > >::const_iterator
        it(to_delete.begin()); it != to_delete.end(); ++it)
    {
      Uint31_Index idx(it->first);
      for (vector< uint32 >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
	db_to_delete[idx].insert(Way_Skeleton(*it2, vector< uint32 >()));
    }
    for (vector< Way >::const_iterator it(ways_to_insert.begin());
        it != ways_to_insert.end(); ++it)
    {
      Uint31_Index idx(it->index);
      db_to_insert[idx].insert(Way_Skeleton(*it));
    }
    
    Block_Backend< Uint31_Index, Way_Skeleton > way_db
      (de_osm3s_file_ids::WAYS, true);
    way_db.update(db_to_delete, db_to_insert);
  }
  
  void prepare_delete_tags
      (vector< Way_Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete)
  {
    // make indices appropriately coarse
    map< uint32, set< uint32 > > to_delete_coarse;
    for (map< uint32, vector< uint32 > >::const_iterator
	 it(to_delete.begin()); it != to_delete.end(); ++it)
    {
      set< uint32 >& handle(to_delete_coarse[it->first & 0xffffff00]);
      for (vector< uint32 >::const_iterator it2(it->second.begin());
	   it2 != it->second.end(); ++it2)
      {
	handle.insert(*it2);
      }
    }
    
    // formulate range query
    set< pair< Way_Tag_Index_Local, Way_Tag_Index_Local > > range_set;
    for (map< uint32, set< uint32 > >::const_iterator
	 it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
    {
      Way_Tag_Index_Local lower, upper;
      lower.index = it->first;
      lower.key = "";
      lower.value = "";
      upper.index = it->first + 1;
      upper.key = "";
      upper.value = "";
      range_set.insert(make_pair(lower, upper));
    }
    
    // iterate over the result
    Block_Backend< Way_Tag_Index_Local, Uint32_Index > ways_db
	(de_osm3s_file_ids::WAY_TAGS_LOCAL, true);
    Way_Tag_Index_Local current_index;
    Way_Tag_Entry way_tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Way_Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(ways_db.range_begin
	     (Default_Range_Iterator< Way_Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Way_Tag_Index_Local >(range_set.end())));
	 !(it == ways_db.range_end()); ++it)
    {
      if (!(current_index == it.index()))
      {
	if ((current_index.index != 0xffffffff) && (!way_tag_entry.way_ids.empty()))
	  tags_to_delete.push_back(way_tag_entry);
	current_index = it.index();
	way_tag_entry.index = it.index().index;
	way_tag_entry.key = it.index().key;
	way_tag_entry.value = it.index().value;
	way_tag_entry.way_ids.clear();
      }
      
      set< uint32 >& handle(to_delete_coarse[it.index().index]);
      if (handle.find(it.object().val()) != handle.end())
	way_tag_entry.way_ids.push_back(it.object().val());
    }
    if ((current_index.index != 0xffffffff) && (!way_tag_entry.way_ids.empty()))
      tags_to_delete.push_back(way_tag_entry);
  }
       
  void update_way_tags_local(const vector< Way_Tag_Entry >& tags_to_delete)
  {
    map< Way_Tag_Index_Local, set< Uint32_Index > > db_to_delete;
    map< Way_Tag_Index_Local, set< Uint32_Index > > db_to_insert;
    
    for (vector< Way_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Way_Tag_Index_Local index;
      index.index = it->index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > way_ids;
      for (vector< uint32 >::const_iterator it2(it->way_ids.begin());
	   it2 != it->way_ids.end(); ++it2)
	way_ids.insert(*it2);
      
      db_to_delete[index] = way_ids;
    }
    
    for (vector< Way >::const_iterator it(ways_to_insert.begin());
	 it != ways_to_insert.end(); ++it)
    {
      Way_Tag_Index_Local index;
      index.index = it->index & 0xffffff00;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }
    
    Block_Backend< Way_Tag_Index_Local, Uint32_Index > way_db
	(de_osm3s_file_ids::WAY_TAGS_LOCAL, true);
    way_db.update(db_to_delete, db_to_insert);
  }
  
  void update_way_tags_global(const vector< Way_Tag_Entry >& tags_to_delete)
  {
    map< Way_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Way_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Way_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Way_Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > way_ids;
      for (vector< uint32 >::const_iterator it2(it->way_ids.begin());
	   it2 != it->way_ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    for (vector< Way >::const_iterator it(ways_to_insert.begin());
	 it != ways_to_insert.end(); ++it)
    {
      Way_Tag_Index_Global index;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }

    Block_Backend< Way_Tag_Index_Global, Uint32_Index > way_db
	(de_osm3s_file_ids::WAY_TAGS_GLOBAL, true);
    way_db.update(db_to_delete, db_to_insert);
  }
  
  void merge_files(string from, string into)
  {
    {
      map< Uint31_Index, set< Way_Skeleton > > db_to_delete;
      map< Uint31_Index, set< Way_Skeleton > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Uint31_Index, Way_Skeleton > from_db
      (de_osm3s_file_ids::WAYS, false, from);
      for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Uint31_Index, Way_Skeleton > into_db
	  (de_osm3s_file_ids::WAYS, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Uint31_Index, Way_Skeleton > into_db
      (de_osm3s_file_ids::WAYS, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((get_file_base_name(de_osm3s_file_ids::WAYS) + from 
    + get_index_suffix(de_osm3s_file_ids::WAYS)).c_str());
    remove((get_file_base_name(de_osm3s_file_ids::WAYS) + from 
    + get_data_suffix(de_osm3s_file_ids::WAYS)).c_str());
    {
      map< Way_Tag_Index_Local, set< Uint32_Index > > db_to_delete;
      map< Way_Tag_Index_Local, set< Uint32_Index > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Way_Tag_Index_Local, Uint32_Index > from_db
      (de_osm3s_file_ids::WAY_TAGS_LOCAL, false, from);
      for (Block_Backend< Way_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Way_Tag_Index_Local, Uint32_Index > into_db
	  (de_osm3s_file_ids::WAY_TAGS_LOCAL, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Way_Tag_Index_Local, Uint32_Index > into_db
      (de_osm3s_file_ids::WAY_TAGS_LOCAL, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((get_file_base_name(de_osm3s_file_ids::WAY_TAGS_LOCAL) + from 
    + get_index_suffix(de_osm3s_file_ids::WAY_TAGS_LOCAL)).c_str());
    remove((get_file_base_name(de_osm3s_file_ids::WAY_TAGS_LOCAL) + from 
    + get_data_suffix(de_osm3s_file_ids::WAY_TAGS_LOCAL)).c_str());
    {
      map< Way_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
      map< Way_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Way_Tag_Index_Global, Uint32_Index > from_db
      (de_osm3s_file_ids::WAY_TAGS_GLOBAL, false, from);
      for (Block_Backend< Way_Tag_Index_Global, Uint32_Index >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Way_Tag_Index_Global, Uint32_Index > into_db
	  (de_osm3s_file_ids::WAY_TAGS_GLOBAL, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Way_Tag_Index_Global, Uint32_Index > into_db
      (de_osm3s_file_ids::WAY_TAGS_GLOBAL, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((get_file_base_name(de_osm3s_file_ids::WAY_TAGS_GLOBAL) + from 
    + get_index_suffix(de_osm3s_file_ids::WAY_TAGS_GLOBAL)).c_str());
    remove((get_file_base_name(de_osm3s_file_ids::WAY_TAGS_GLOBAL) + from 
    + get_data_suffix(de_osm3s_file_ids::WAY_TAGS_GLOBAL)).c_str());
  }
};

#endif
