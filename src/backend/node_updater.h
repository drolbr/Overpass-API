#ifndef DE_OSM3S__BACKEND__NODE_UPDATER
#define DE_OSM3S__BACKEND__NODE_UPDATER

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../dispatch/settings.h"
#include "block_backend.h"
#include "random_file.h"

using namespace std;

struct Uint32_Index
{
  Uint32_Index(uint32 i) : value(i) {}
  Uint32_Index(void* data) : value(*(uint32*)data) {}
  
  uint32 size_of() const
  {
    return 4;
  }
  
  static uint32 max_size_of()
  {
    return 4;
  }
  
  static uint32 size_of(void* data)
  {
    return 4;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = value;
  }
  
  bool operator<(const Uint32_Index& index) const
  {
    return this->value < index.value;
  }
  
  bool operator==(const Uint32_Index& index) const
  {
    return this->value == index.value;
  }
  
  uint32 val() const
  {
    return value;
  }
  
protected:
  uint32 value;
};

struct Node
{
  uint32 id;
  uint32 ll_upper_;
  uint32 ll_lower_;
  vector< pair< string, string > > tags;
  
  Node() {}
  
  Node(uint32 id_, double lat, double lon)
    : id(id_), ll_upper_(ll_upper(lat, lon)), ll_lower_(ll_lower(lat, lon))
  {}
  
  static uint32 ll_upper(double lat, double lon)
  {
    uint32 result(0), ilat((lat + 90.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
    
    for (uint32 i(0); i < 16; ++i)
    {
      result |= ((0x1<<i+16)&ilat)>>(15-i);
      result |= ((0x1<<i+16)&(uint32)ilon)>>(16-i);
    }
    
    return result;
  }
  
  static uint32 ll_lower(double lat, double lon)
  {
    uint32 result(0), ilat((lat + 90.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
    
    for (uint32 i(0); i < 16; ++i)
    {
      result |= ((0x1<<i)&ilat)<<(i+1);
      result |= ((0x1<<i)&(uint32)ilon)<<i;
    }
    
    return result;
  }
  
  static double lat(uint32 ll_upper, uint32 ll_lower)
  {
    uint32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<31-2*i)&ll_upper)<<i;
      result |= ((0x1<<31-2*i)&ll_lower)>>16-i;
    }
    
    return ((double)result)/10000000 - 90.0;
  }
  
  static double lon(uint32 ll_upper, uint32 ll_lower)
  {
    int32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<30-2*i)&ll_upper)<<i+1;
      result |= ((0x1<<30-2*i)&ll_lower)>>15-i;
    }
    
    return ((double)result)/10000000;
  }
};

struct Node_Comparator_By_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id < b.id);
  }
};

struct Node_Skeleton
{
  uint32 id;
  uint32 ll_lower;
  
  Node_Skeleton() {}
  
  Node_Skeleton(void* data)
  {
    id = *(uint32*)data;
    ll_lower = *(uint32*)((uint8*)data+4);
  }
  
  Node_Skeleton(const Node& node)
    : id(node.id), ll_lower(node.ll_lower_) {}
  
  Node_Skeleton(uint32 id_, uint32 ll_lower_)
  : id(id_), ll_lower(ll_lower_) {}
  
  uint32 size_of() const
  {
    return 8;
  }
  
  static uint32 size_of(void* data)
  {
    return 8;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *(uint32*)((uint8*)data+4) = ll_lower;
  }
  
  bool operator<(const Node_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Node_Skeleton& a) const
  {
    return this->id == a.id;
  }
};

struct Node_Tag_Entry
{
  uint32 index;
  string key;
  string value;
  vector< uint32 > node_ids;
};

struct Node_Tag_Index_Local
{
  uint32 index;
  string key;
  string value;
  
  Node_Tag_Index_Local() {}
  
  Node_Tag_Index_Local(void* data)
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
    memcpy(((uint8*)data + 7), key.c_str(), key.length());
    memcpy(((uint8*)data + 7 + key.length()), value.c_str(),
	     value.length());
  }
  
  bool operator<(const Node_Tag_Index_Local& a) const
  {
    if (index != a.index)
      return (index < a.index);
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Node_Tag_Index_Local& a) const
  {
    if (index != a.index)
      return false;
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Node_Tag_Index_Global
{
  string key;
  string value;
  
  Node_Tag_Index_Global() {}
  
  Node_Tag_Index_Global(void* data)
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
    memcpy(((uint8*)data + 4), key.c_str(), key.length());
    memcpy(((uint8*)data + 4 + key.length()), value.c_str(),
	     value.length());
  }
  
  bool operator<(const Node_Tag_Index_Global& a) const
  {
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Node_Tag_Index_Global& a) const
  {
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Node_Updater
{
  Node_Updater() : update_counter(0) {}
  
  void set_id_deleted(uint32 id)
  {
    ids_to_delete.push_back(id);
  }
  
  void set_node
      (uint32 id, uint32 lat, uint32 lon,
       const vector< pair< string, string > >& tags)
  {
    ids_to_delete.push_back(id);
    
    Node node;
    node.id = id;
    node.ll_upper_ = Node::ll_upper(lat, lon);
    node.ll_lower_ = Node::ll_lower(lat, lon);
    node.tags = tags;
    nodes_to_insert.push_back(node);
  }
  
  void set_node(const Node& node)
  {
    ids_to_delete.push_back(node.id);
    nodes_to_insert.push_back(node);
  }
  
  void update(bool partial = false)
  {
    cerr<<'.'<<' '<<time(NULL)<<' ';
    
    map< uint32, vector< uint32 > > to_delete;
    update_node_ids(to_delete);
    update_coords(to_delete);
    
    vector< Node_Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_node_tags_local(tags_to_delete);
    update_node_tags_global(tags_to_delete);
    
    ids_to_delete.clear();
    nodes_to_insert.clear();
    
    cerr<<'N'<<' '<<time(NULL)<<' ';
    
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
    
    cerr<<'n'<<' '<<time(NULL)<<' ';
  }
  
  void update_DEBUG(bool partial = false)
  {
    cerr<<'.'<<' '<<time(NULL)<<' ';
    
    map< uint32, vector< uint32 > > to_delete;
    update_node_ids(to_delete);
    update_coords(to_delete);
    
    vector< Node_Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_node_tags_local(tags_to_delete);
    
    if (update_counter < 127)
      update_node_tags_global(tags_to_delete);
    else
    {
      update_node_tags_global_DEBUG(tags_to_delete);
      return;
    }
    
    ids_to_delete.clear();
    nodes_to_insert.clear();
    
    cerr<<'N'<<' '<<time(NULL)<<' ';
    
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
    
    cerr<<'n'<<' '<<time(NULL)<<' ';
  }

  void update_node_tags_global_DEBUG_2()
  {
    map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Node_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Node_Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > node_ids;
      for (vector< uint32 >::const_iterator it2(it->node_ids.begin());
	   it2 != it->node_ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    for (vector< Node >::const_iterator it(nodes_to_insert.begin());
	 it != nodes_to_insert.end(); ++it)
    {
      Node_Tag_Index_Global index;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }

    Block_Backend< Node_Tag_Index_Global, Uint32_Index > node_db
	(de_osm3s_file_ids::NODE_TAGS_GLOBAL, true, ".128");
    node_db.update(db_to_delete, db_to_insert);
  }
  
private:
  vector< uint32 > ids_to_delete;
  vector< Node > nodes_to_insert;
  static Node_Comparator_By_Id node_comparator_by_id;
  uint32 update_counter;
  
  void update_node_ids(map< uint32, vector< uint32 > >& to_delete)
  {
    sort(ids_to_delete.begin(), ids_to_delete.end());
    sort(nodes_to_insert.begin(), nodes_to_insert.end(), node_comparator_by_id);
    
    Random_File< Uint32_Index > random(de_osm3s_file_ids::NODES, true);
    vector< Node >::const_iterator nit(nodes_to_insert.begin());
    for (vector< uint32 >::const_iterator it(ids_to_delete.begin());
        it != ids_to_delete.end(); ++it)
    {
      Uint32_Index index(random.get(*it));
      if (index.val() > 0)
	to_delete[index.val()].push_back(*it);
      if ((nit != nodes_to_insert.end()) && (*it == nit->id))
      {
	random.put(*it, Uint32_Index(nit->ll_upper_));
	++nit;
      }
    }
  }
  
  void update_coords(const map< uint32, vector< uint32 > >& to_delete)
  {
    map< Uint32_Index, set< Node_Skeleton > > db_to_delete;
    map< Uint32_Index, set< Node_Skeleton > > db_to_insert;
    
    for (map< uint32, vector< uint32 > >::const_iterator
        it(to_delete.begin()); it != to_delete.end(); ++it)
    {
      Uint32_Index idx(it->first);
      for (vector< uint32 >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
	db_to_delete[idx].insert(Node_Skeleton(*it2, 0));
    }
    for (vector< Node >::const_iterator it(nodes_to_insert.begin());
        it != nodes_to_insert.end(); ++it)
    {
      Uint32_Index idx(it->ll_upper_);
      db_to_insert[idx].insert(Node_Skeleton(*it));
    }
    
    Block_Backend< Uint32_Index, Node_Skeleton > node_db
      (de_osm3s_file_ids::NODES, true);
    node_db.update(db_to_delete, db_to_insert);
  }
  
  void prepare_delete_tags
      (vector< Node_Tag_Entry >& tags_to_delete,
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
    set< pair< Node_Tag_Index_Local, Node_Tag_Index_Local > > range_set;
    for (map< uint32, set< uint32 > >::const_iterator
	 it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
    {
      Node_Tag_Index_Local lower, upper;
      lower.index = it->first;
      lower.key = "";
      lower.value = "";
      upper.index = it->first + 1;
      upper.key = "";
      upper.value = "";
      range_set.insert(make_pair(lower, upper));
    }
    
    // iterate over the result
    Block_Backend< Node_Tag_Index_Local, Uint32_Index > nodes_db
	(de_osm3s_file_ids::NODE_TAGS_LOCAL, true);
    Node_Tag_Index_Local current_index;
    Node_Tag_Entry node_tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Node_Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(nodes_db.range_begin
	     (Default_Range_Iterator< Node_Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Node_Tag_Index_Local >(range_set.end())));
	 !(it == nodes_db.range_end()); ++it)
    {
      if (!(current_index == it.index()))
      {
	if (current_index.index != 0xffffffff)
	  tags_to_delete.push_back(node_tag_entry);
	current_index = it.index();
	node_tag_entry.index = it.index().index;
	node_tag_entry.key = it.index().key;
	node_tag_entry.value = it.index().value;
	node_tag_entry.node_ids.clear();
      }
      
      set< uint32 >& handle(to_delete_coarse[it.index().index]);
      if (handle.find(it.object().val()) != handle.end())
	node_tag_entry.node_ids.push_back(it.object().val());
    }
    if (current_index.index != 0xffffffff)
      tags_to_delete.push_back(node_tag_entry);
  }
       
  void update_node_tags_local(const vector< Node_Tag_Entry >& tags_to_delete)
  {
    map< Node_Tag_Index_Local, set< Uint32_Index > > db_to_delete;
    map< Node_Tag_Index_Local, set< Uint32_Index > > db_to_insert;
    
    for (vector< Node_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Node_Tag_Index_Local index;
      index.index = it->index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > node_ids;
      for (vector< uint32 >::const_iterator it2(it->node_ids.begin());
	   it2 != it->node_ids.end(); ++it2)
	node_ids.insert(*it2);
      
      db_to_delete[index] = node_ids;
    }
    
    for (vector< Node >::const_iterator it(nodes_to_insert.begin());
	 it != nodes_to_insert.end(); ++it)
    {
      Node_Tag_Index_Local index;
      index.index = it->ll_upper_ & 0xffffff00;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }
    
    Block_Backend< Node_Tag_Index_Local, Uint32_Index > node_db
	(de_osm3s_file_ids::NODE_TAGS_LOCAL, true);
    node_db.update(db_to_delete, db_to_insert);
  }
  
  void update_node_tags_global(const vector< Node_Tag_Entry >& tags_to_delete)
  {
    map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Node_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Node_Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > node_ids;
      for (vector< uint32 >::const_iterator it2(it->node_ids.begin());
	   it2 != it->node_ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    for (vector< Node >::const_iterator it(nodes_to_insert.begin());
	 it != nodes_to_insert.end(); ++it)
    {
      Node_Tag_Index_Global index;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }

    Block_Backend< Node_Tag_Index_Global, Uint32_Index > node_db
	(de_osm3s_file_ids::NODE_TAGS_GLOBAL, true);
    node_db.update(db_to_delete, db_to_insert);
  }
  
  void update_node_tags_global_DEBUG(const vector< Node_Tag_Entry >& tags_to_delete)
  {
    map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Node_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Node_Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > node_ids;
      for (vector< uint32 >::const_iterator it2(it->node_ids.begin());
	   it2 != it->node_ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    for (vector< Node >::const_iterator it(nodes_to_insert.begin());
	 it != nodes_to_insert.end(); ++it)
    {
      Node_Tag_Index_Global index;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }

    Block_Backend< Node_Tag_Index_Global, Uint32_Index > node_db
	(de_osm3s_file_ids::NODE_TAGS_GLOBAL, true, ".128");
    node_db.update(db_to_delete, db_to_insert);
  }
  
  void merge_files(string from, string into)
  {
    {
      map< Uint32_Index, set< Node_Skeleton > > db_to_delete;
      map< Uint32_Index, set< Node_Skeleton > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Uint32_Index, Node_Skeleton > from_db
          (de_osm3s_file_ids::NODES, false, from);
      for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
	  it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Uint32_Index, Node_Skeleton > into_db
	      (de_osm3s_file_ids::NODES, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Uint32_Index, Node_Skeleton > into_db
          (de_osm3s_file_ids::NODES, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((get_file_base_name(de_osm3s_file_ids::NODES) + from 
      + get_index_suffix(de_osm3s_file_ids::NODES)).c_str());
    remove((get_file_base_name(de_osm3s_file_ids::NODES) + from 
      + get_data_suffix(de_osm3s_file_ids::NODES)).c_str());
    {
      map< Node_Tag_Index_Local, set< Uint32_Index > > db_to_delete;
      map< Node_Tag_Index_Local, set< Uint32_Index > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Node_Tag_Index_Local, Uint32_Index > from_db
        (de_osm3s_file_ids::NODE_TAGS_LOCAL, false, from);
      for (Block_Backend< Node_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Node_Tag_Index_Local, Uint32_Index > into_db
	    (de_osm3s_file_ids::NODE_TAGS_LOCAL, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Node_Tag_Index_Local, Uint32_Index > into_db
      (de_osm3s_file_ids::NODE_TAGS_LOCAL, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((get_file_base_name(de_osm3s_file_ids::NODE_TAGS_LOCAL) + from 
      + get_index_suffix(de_osm3s_file_ids::NODE_TAGS_LOCAL)).c_str());
    remove((get_file_base_name(de_osm3s_file_ids::NODE_TAGS_LOCAL) + from 
      + get_data_suffix(de_osm3s_file_ids::NODE_TAGS_LOCAL)).c_str());
    {
      map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
      map< Node_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Node_Tag_Index_Global, Uint32_Index > from_db
        (de_osm3s_file_ids::NODE_TAGS_GLOBAL, false, from);
      for (Block_Backend< Node_Tag_Index_Global, Uint32_Index >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Node_Tag_Index_Global, Uint32_Index > into_db
	    (de_osm3s_file_ids::NODE_TAGS_GLOBAL, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Node_Tag_Index_Global, Uint32_Index > into_db
        (de_osm3s_file_ids::NODE_TAGS_GLOBAL, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((get_file_base_name(de_osm3s_file_ids::NODE_TAGS_GLOBAL) + from 
      + get_index_suffix(de_osm3s_file_ids::NODE_TAGS_GLOBAL)).c_str());
    remove((get_file_base_name(de_osm3s_file_ids::NODE_TAGS_GLOBAL) + from 
      + get_data_suffix(de_osm3s_file_ids::NODE_TAGS_GLOBAL)).c_str());
  }
};

#endif
