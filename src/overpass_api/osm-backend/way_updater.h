#ifndef DE_OSM3S__BACKEND__WAY_UPDATER
#define DE_OSM3S__BACKEND__WAY_UPDATER

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

using namespace std;

struct Way_Updater
{
  Way_Updater() {}
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_way
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< uint32 > nds)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Way way;
    way.id = id;
    way.nds = nds;
    way.tags = tags;
    ways_to_insert.push_back(way);
  }
  
  void set_way(const Way& way)
  {
    ids_to_modify.push_back(make_pair(way.id, true));
    ways_to_insert.push_back(way);
  }
  
  void update(bool partial = false)
  {
    cerr<<'.';
    
    map< uint32, vector< uint32 > > to_delete;
    compute_indexes();
    update_way_ids(to_delete);
    update_members(to_delete);

    vector< Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_way_tags_local(tags_to_delete);
    update_way_tags_global(tags_to_delete);

    ids_to_modify.clear();
    ways_to_insert.clear();
    
    cerr<<'W';
    
    if (!partial && (update_counter > 0))
    {
      if (update_counter >= 64)
	merge_files(".1", ".0");
      if (update_counter >= 8)
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
    
    cerr<<"w ";
  }
  
  void update(vector< pair< uint32, uint32 > >& moved_ways)
  {
    show_mem_status();
    
    map< uint32, vector< uint32 > > to_delete;
    compute_indexes();
    update_way_ids(to_delete, &moved_ways);
    update_members(to_delete);
    
    vector< Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_way_tags_local(tags_to_delete);
    update_way_tags_global(tags_to_delete);
    
    show_mem_status();
    
    ids_to_modify.clear();
    ways_to_insert.clear();
  }
  
  void update_moved_idxs(vector< pair< uint32, uint32 > >& moved_nodes,
			 vector< pair< uint32, uint32 > >& moved_ways)
  {
    ids_to_modify.clear();
    ways_to_insert.clear();
    sort(moved_nodes.begin(), moved_nodes.end());
    
    map< uint32, vector< uint32 > > to_delete;
    find_affected_ways(moved_nodes);
    update_way_ids(to_delete, &moved_ways);
    update_members(to_delete);
    
    vector< Tag_Entry > tags_to_delete;
    prepare_tags(tags_to_delete, to_delete);
    update_way_tags_local(tags_to_delete);
    
    show_mem_status();
    
    ids_to_modify.clear();
    ways_to_insert.clear();
  }
  
private:
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Way > ways_to_insert;
  static Way_Comparator_By_Id way_comparator_by_id;
  static Way_Equal_Id way_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  uint32 update_counter;
  
  void filter_affected_ways(const vector< Way >& maybe_affected_ways)
  {
    // retrieve the indices of the referred nodes
    map< uint32, uint32 > used_nodes;
    for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
        wit != maybe_affected_ways.end(); ++wit)
    {
      for (vector< uint32 >::const_iterator nit(wit->nds.begin());
          nit != wit->nds.end(); ++nit)
        used_nodes[*nit] = 0;
    }
    Random_File< Uint32_Index > node_random(*de_osm3s_file_ids::NODES, false);
    for (map< uint32, uint32 >::iterator it(used_nodes.begin());
        it != used_nodes.end(); ++it)
      it->second = node_random.get(it->first).val();
    for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
        wit != maybe_affected_ways.end(); ++wit)
    {
      vector< uint32 > nd_idxs;
      for (vector< uint32 >::const_iterator nit(wit->nds.begin());
          nit != wit->nds.end(); ++nit)
        nd_idxs.push_back(used_nodes[*nit]);
      
      uint32 index(Way::calc_index(nd_idxs));
      if (wit->index != index)
      {
	ids_to_modify.push_back(make_pair(wit->id, true));
	ways_to_insert.push_back(*wit);
	ways_to_insert.back().index = index;
      }
    }
  }
  
  void find_affected_ways(const vector< pair< uint32, uint32 > >& moved_nodes)
  {
    vector< Way > maybe_affected_ways;
    
    set< Uint31_Index > req;
    for (vector< pair< uint32, uint32 > >::const_iterator
	 it(moved_nodes.begin()); it != moved_nodes.end(); ++it)
    {
      req.insert(Uint31_Index(it->second));
      req.insert(Uint31_Index((it->second & 0x7fffff00) | 0x80000010));
      req.insert(Uint31_Index((it->second & 0x7fff0000) | 0x80000020));
      req.insert(Uint31_Index((it->second & 0x7f000000) | 0x80000030));
    }
    req.insert(Uint31_Index(0x80000040));
    
    Block_Backend< Uint31_Index, Way_Skeleton > ways_db
	(*de_osm3s_file_ids::WAYS, false);
    for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
	 it(ways_db.discrete_begin(req.begin(), req.end()));
	 !(it == ways_db.discrete_end()); ++it)
    {
      const Way_Skeleton& way(it.object());
      bool is_affected(false);
      for (vector< uint32 >::const_iterator it3(way.nds.begin());
          it3 != way.nds.end(); ++it3)
      {
	if (binary_search(moved_nodes.begin(), moved_nodes.end(),
	  make_pair(*it3, 0), pair_comparator_by_id))
	{
	  is_affected = true;
	  break;
	}
      }
      if (is_affected)
	maybe_affected_ways.push_back(Way(way.id, it.index().val(), way.nds));
      if (maybe_affected_ways.size() >= 512*1024)
      {
	filter_affected_ways(maybe_affected_ways);
	maybe_affected_ways.clear();
      }
    }
    
    filter_affected_ways(maybe_affected_ways);
    maybe_affected_ways.clear();
  }
  
  void compute_indexes()
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
    Random_File< Uint32_Index > node_random(*de_osm3s_file_ids::NODES, false);
    for (map< uint32, uint32 >::iterator it(used_nodes.begin());
	 it != used_nodes.end(); ++it)
      it->second = node_random.get(it->first).val();
    vector< Way >::iterator wwit(ways_to_insert.begin());
    for (vector< Way >::iterator wit(ways_to_insert.begin());
	 wit != ways_to_insert.end(); ++wit)
    {
      vector< uint32 > nd_idxs;
      for (vector< uint32 >::const_iterator nit(wit->nds.begin());
	   nit != wit->nds.end(); ++nit)
	nd_idxs.push_back(used_nodes[*nit]);
      
      wit->index = Way::calc_index(nd_idxs);
    }
  }

  void update_way_ids(map< uint32, vector< uint32 > >& to_delete,
		      vector< pair< uint32, uint32 > >* moved_ways = 0)
  {
    // process the ways itself
    // keep always the most recent (last) element of all equal elements
    stable_sort(ids_to_modify.begin(), ids_to_modify.end(),
		pair_comparator_by_id);
    vector< pair< uint32, bool > >::iterator modi_begin
      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id)
      .base());
    ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
    stable_sort(ways_to_insert.begin(), ways_to_insert.end(), way_comparator_by_id);
    vector< Way >::iterator ways_begin
      (unique(ways_to_insert.rbegin(), ways_to_insert.rend(), way_equal_id)
      .base());
    ways_to_insert.erase(ways_to_insert.begin(), ways_begin);
    
    Random_File< Uint31_Index > random(*de_osm3s_file_ids::WAYS, true);
    vector< Way >::const_iterator wit(ways_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      Uint31_Index index(random.get(it->first));
      if (index.val() > 0)
	to_delete[index.val()].push_back(it->first);
      if ((wit != ways_to_insert.end()) && (it->first == wit->id))
      {
	if (it->second)
	{
	  random.put(it->first, Uint31_Index(wit->index));
	  if ((moved_ways != 0) && (index.val() > 0) &&
		      (index.val() != wit->index))
	    moved_ways->push_back(make_pair(it->first, index.val()));
	}
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
    vector< Way >::const_iterator wit(ways_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      if ((wit != ways_to_insert.end()) && (it->first == wit->id))
      {
	if (it->second)
	{
	  Uint31_Index idx(wit->index);
	  db_to_insert[idx].insert(Way_Skeleton(*wit));
	}
	++wit;
      }
    }
    
    Block_Backend< Uint31_Index, Way_Skeleton > way_db
      (*de_osm3s_file_ids::WAYS, true);
    way_db.update(db_to_delete, db_to_insert);
  }
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
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
    set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
    for (map< uint32, set< uint32 > >::const_iterator
	 it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
    {
      Tag_Index_Local lower, upper;
      lower.index = it->first;
      lower.key = "";
      lower.value = "";
      upper.index = it->first + 1;
      upper.key = "";
      upper.value = "";
      range_set.insert(make_pair(lower, upper));
    }
    
    // iterate over the result
    Block_Backend< Tag_Index_Local, Uint32_Index > ways_db
	(*de_osm3s_file_ids::WAY_TAGS_LOCAL, true);
    Tag_Index_Local current_index;
    Tag_Entry tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(ways_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
	 !(it == ways_db.range_end()); ++it)
    {
      if (!(current_index == it.index()))
      {
	if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	  tags_to_delete.push_back(tag_entry);
	current_index = it.index();
	tag_entry.index = it.index().index;
	tag_entry.key = it.index().key;
	tag_entry.value = it.index().value;
	tag_entry.ids.clear();
      }
      
      set< uint32 >& handle(to_delete_coarse[it.index().index]);
      if (handle.find(it.object().val()) != handle.end())
	tag_entry.ids.push_back(it.object().val());
    }
    if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
      tags_to_delete.push_back(tag_entry);
  }
       
  void prepare_tags
      (vector< Tag_Entry >& tags_to_delete,
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
    set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
    for (map< uint32, set< uint32 > >::const_iterator
	 it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
    {
      Tag_Index_Local lower, upper;
      lower.index = it->first;
      lower.key = "";
      lower.value = "";
      upper.index = it->first + 1;
      upper.key = "";
      upper.value = "";
      range_set.insert(make_pair(lower, upper));
    }
    
    // iterate over the result
    Block_Backend< Tag_Index_Local, Uint32_Index > ways_db
	(*de_osm3s_file_ids::WAY_TAGS_LOCAL, true);
    Tag_Index_Local current_index;
    Tag_Entry tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(ways_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
	 !(it == ways_db.range_end()); ++it)
    {
      if (!(current_index == it.index()))
      {
	if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	  tags_to_delete.push_back(tag_entry);
	current_index = it.index();
	tag_entry.index = it.index().index;
	tag_entry.key = it.index().key;
	tag_entry.value = it.index().value;
	tag_entry.ids.clear();
      }
      
      set< uint32 >& handle(to_delete_coarse[it.index().index]);
      if (handle.find(it.object().val()) != handle.end())
      {
	Way* way(binary_search_for_id(ways_to_insert, it.object().val()));
	if (way != 0)
	  way->tags.push_back(make_pair(it.index().key, it.index().value));
	tag_entry.ids.push_back(it.object().val());
      }
    }
    if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
      tags_to_delete.push_back(tag_entry);
  }
  
  void update_way_tags_local(const vector< Tag_Entry >& tags_to_delete)
  {
    map< Tag_Index_Local, set< Uint32_Index > > db_to_delete;
    map< Tag_Index_Local, set< Uint32_Index > > db_to_insert;
    
    for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Tag_Index_Local index;
      index.index = it->index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > way_ids;
      for (vector< uint32 >::const_iterator it2(it->ids.begin());
	   it2 != it->ids.end(); ++it2)
	way_ids.insert(*it2);
      
      db_to_delete[index] = way_ids;
    }
    
    vector< Way >::const_iterator wit(ways_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
    it != ids_to_modify.end(); ++it)
    {
      if ((wit != ways_to_insert.end()) && (it->first == wit->id))
      {
	if (it->second)
	{
	  Tag_Index_Local index;
	  index.index = wit->index & 0xffffff00;
	  
	  for (vector< pair< string, string > >::const_iterator
	      it2(wit->tags.begin()); it2 != wit->tags.end(); ++it2)
	  {
	    index.key = it2->first;
	    index.value = it2->second;
	    db_to_insert[index].insert(wit->id);
	    db_to_delete[index];
	  }
	}
	++wit;
      }
    }
    
    Block_Backend< Tag_Index_Local, Uint32_Index > way_db
	(*de_osm3s_file_ids::WAY_TAGS_LOCAL, true);
    way_db.update(db_to_delete, db_to_insert);
  }
  
  void update_way_tags_global(const vector< Tag_Entry >& tags_to_delete)
  {
    map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > way_ids;
      for (vector< uint32 >::const_iterator it2(it->ids.begin());
	   it2 != it->ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    vector< Way >::const_iterator wit(ways_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
    it != ids_to_modify.end(); ++it)
    {
      if ((wit != ways_to_insert.end()) && (it->first == wit->id))
      {
	if (it->second)
	{
	  Tag_Index_Global index;
      
	  for (vector< pair< string, string > >::const_iterator
	      it2(wit->tags.begin()); it2 != wit->tags.end(); ++it2)
	  {
	    index.key = it2->first;
	    index.value = it2->second;
	    db_to_insert[index].insert(wit->id);
	    db_to_delete[index];
	  }
	}
	++wit;
      }
    }

    Block_Backend< Tag_Index_Global, Uint32_Index > way_db
	(*de_osm3s_file_ids::WAY_TAGS_GLOBAL, true);
    way_db.update(db_to_delete, db_to_insert);
  }
  
  void merge_files(string from, string into)
  {
    {
      map< Uint31_Index, set< Way_Skeleton > > db_to_delete;
      map< Uint31_Index, set< Way_Skeleton > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Uint31_Index, Way_Skeleton > from_db
      (*de_osm3s_file_ids::WAYS, false, from);
      for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Uint31_Index, Way_Skeleton > into_db
	  (*de_osm3s_file_ids::WAYS, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Uint31_Index, Way_Skeleton > into_db
      (*de_osm3s_file_ids::WAYS, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((de_osm3s_file_ids::WAYS->get_file_base_name() + from 
    + de_osm3s_file_ids::WAYS->get_index_suffix()).c_str());
    remove((de_osm3s_file_ids::WAYS->get_file_base_name() + from 
    + de_osm3s_file_ids::WAYS->get_data_suffix()).c_str());
    {
      map< Tag_Index_Local, set< Uint32_Index > > db_to_delete;
      map< Tag_Index_Local, set< Uint32_Index > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Tag_Index_Local, Uint32_Index > from_db
      (*de_osm3s_file_ids::WAY_TAGS_LOCAL, false, from);
      for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Tag_Index_Local, Uint32_Index > into_db
	  (*de_osm3s_file_ids::WAY_TAGS_LOCAL, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Tag_Index_Local, Uint32_Index > into_db
          (*de_osm3s_file_ids::WAY_TAGS_LOCAL, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((de_osm3s_file_ids::WAY_TAGS_LOCAL->get_file_base_name() + from 
    + de_osm3s_file_ids::WAY_TAGS_LOCAL->get_index_suffix()).c_str());
    remove((de_osm3s_file_ids::WAY_TAGS_LOCAL->get_file_base_name() + from 
    + de_osm3s_file_ids::WAY_TAGS_LOCAL->get_data_suffix()).c_str());
    {
      map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
      map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
      
      uint32 item_count(0);
      Block_Backend< Tag_Index_Global, Uint32_Index > from_db
      (*de_osm3s_file_ids::WAY_TAGS_GLOBAL, false, from);
      for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
	it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
      {
	db_to_insert[it.index()].insert(it.object());
	if (++item_count >= 4*1024*1024)
	{
	  Block_Backend< Tag_Index_Global, Uint32_Index > into_db
	  (*de_osm3s_file_ids::WAY_TAGS_GLOBAL, true, into);
	  into_db.update(db_to_delete, db_to_insert);
	  db_to_insert.clear();
	  item_count = 0;
	}
      }
      
      Block_Backend< Tag_Index_Global, Uint32_Index > into_db
      (*de_osm3s_file_ids::WAY_TAGS_GLOBAL, true, into);
      into_db.update(db_to_delete, db_to_insert);
    }
    remove((de_osm3s_file_ids::WAY_TAGS_GLOBAL->get_file_base_name() + from 
    + de_osm3s_file_ids::WAY_TAGS_GLOBAL->get_index_suffix()).c_str());
    remove((de_osm3s_file_ids::WAY_TAGS_GLOBAL->get_file_base_name() + from 
    + de_osm3s_file_ids::WAY_TAGS_GLOBAL->get_data_suffix()).c_str());
  }
};

#endif
