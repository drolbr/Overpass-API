#ifndef DE_OSM3S__BACKEND__AREA_UPDATER
#define DE_OSM3S__BACKEND__AREA_UPDATER

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../backend/block_backend.h"
#include "../backend/random_file.h"
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

struct Area_Updater
{
  Area_Updater() {}
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.insert(id);
  }
  
  void set_area
      (uint32 id, const Uint31_Index& index,
       const vector< pair< string, string > >& tags,
       const set< uint32 >& used_indices)
  {
    ids_to_modify.insert(id);
    
    Area_Location area(id, used_indices);
    area.tags = tags;
    areas_to_insert.push_back(make_pair(area, index));
  }
  
  void set_area(const Uint31_Index& index, const Area_Location& area)
  {
    ids_to_modify.insert(area.id);
    areas_to_insert.push_back(make_pair(area, index));
  }
  
  void add_blocks(const map< Uint31_Index, vector< Area_Block > >& area_blocks_)
  {
    for (map< Uint31_Index, vector< Area_Block > >::const_iterator
        it(area_blocks_.begin()); it != area_blocks_.end(); ++it)
    {
      for (vector< Area_Block >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        area_blocks[it->first].push_back(*it2);
    }
  }
  
  void update()
  {
    cerr<<'.'<<' '<<time(NULL)<<' ';
    
    map< Uint31_Index, set< Area_Skeleton > > locations_to_delete;
    map< Uint31_Index, set< Area_Block > > blocks_to_delete;
    update_area_ids(locations_to_delete, blocks_to_delete);
    update_members(locations_to_delete, blocks_to_delete);
 
    vector< Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, locations_to_delete);
    update_area_tags_local(tags_to_delete);
    update_area_tags_global(tags_to_delete);

    ids_to_modify.clear();
    areas_to_insert.clear();
    
    cerr<<'A'<<' '<<time(NULL)<<' ';
    cerr<<'a'<<' '<<time(NULL)<<' ';
  }
  
private:
  map< Uint31_Index, vector< Area_Block > > area_blocks;
  map< string, uint32 > role_ids;
  uint32 max_written_role_id;
  uint32 max_role_id;
  set< uint32 > ids_to_modify;
  vector< pair< Area_Location, Uint31_Index > > areas_to_insert;
  static Area_Pair_Comparator_By_Id area_comparator_by_id;
  static Area_Pair_Equal_Id area_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  
  void update_area_ids
      (map< Uint31_Index, set< Area_Skeleton > >& locations_to_delete,
       map< Uint31_Index, set< Area_Block > >& blocks_to_delete)
  {
    set< Uint31_Index > blocks_req;
    
    // process the areas themselves
    Block_Backend< Uint31_Index, Area_Skeleton > area_locations_db
        (*de_osm3s_file_ids::AREAS, true);
    for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
        it(area_locations_db.flat_begin());
        !(it == area_locations_db.flat_end()); ++it)
    {
      if (ids_to_modify.find(it.object().id) != ids_to_modify.end())
      {
	for (set< uint32 >::const_iterator it2(it.object().used_indices.begin());
	    it2 != it.object().used_indices.end(); ++it2)
	  blocks_req.insert(*it2);
	locations_to_delete[it.index().val()].insert(it.object());
      }
    }

    Block_Backend< Uint31_Index, Area_Block > area_blocks_db
        (*de_osm3s_file_ids::AREA_BLOCKS, true);
    for (Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
        it(area_blocks_db.discrete_begin(blocks_req.begin(), blocks_req.end()));
        !(it == area_blocks_db.discrete_end()); ++it)
    {
      if (ids_to_modify.find(it.object().id) != ids_to_modify.end())
        blocks_to_delete[it.index()].insert(it.object());
    }
  }
  
  void update_members
      (const map< Uint31_Index, set< Area_Skeleton > >& locations_to_delete,
       const map< Uint31_Index, set< Area_Block > >& blocks_to_delete)
  {
    map< Uint31_Index, set< Area_Skeleton > > locations_to_insert;
    for (vector< pair< Area_Location, Uint31_Index > >::const_iterator
        it(areas_to_insert.begin()); it != areas_to_insert.end(); ++it)
      locations_to_insert[it->second].insert(Area_Skeleton(it->first));

    Block_Backend< Uint31_Index, Area_Skeleton > area_locations
        (*de_osm3s_file_ids::AREAS, true);
    area_locations.update(locations_to_delete, locations_to_insert);

    map< Uint31_Index, set< Area_Block > > blocks_to_insert;
    for (map< Uint31_Index, vector< Area_Block > >::const_iterator
        it(area_blocks.begin()); it != area_blocks.end(); ++it)
    {
      for (vector< Area_Block >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        blocks_to_insert[it->first].insert(*it2);
    }
    
    Block_Backend< Uint31_Index, Area_Block > area_blocks
        (*de_osm3s_file_ids::AREA_BLOCKS, true);
    area_blocks.update(blocks_to_delete, blocks_to_insert);
  }
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< Uint31_Index, set< Area_Skeleton > >& to_delete)
  {
    // make indices appropriately coarse
    map< uint32, set< uint32 > > to_delete_coarse;
    for (map< Uint31_Index, set< Area_Skeleton > >::const_iterator
	 it(to_delete.begin()); it != to_delete.end(); ++it)
    {
      set< uint32 >& handle(to_delete_coarse[it->first.val() & 0xffffff00]);
      for (set< Area_Skeleton >::const_iterator it2(it->second.begin());
	   it2 != it->second.end(); ++it2)
	handle.insert(it2->id);
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
    Block_Backend< Tag_Index_Local, Uint32_Index > areas_db
	(*de_osm3s_file_ids::AREA_TAGS_LOCAL, true);
    Tag_Index_Local current_index;
    Tag_Entry tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(areas_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
	 !(it == areas_db.range_end()); ++it)
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
    Block_Backend< Tag_Index_Local, Uint32_Index > areas_db
	(*de_osm3s_file_ids::AREA_TAGS_LOCAL, true);
    Tag_Index_Local current_index;
    Tag_Entry tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(areas_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
	 !(it == areas_db.range_end()); ++it)
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
	//TODO
	//Area_Location* area(binary_search_for_id(areas_to_insert, it.object().val()));
	//if (area != 0)
	//  area->tags.push_back(make_pair(it.index().key, it.index().value));
	//tag_entry.ids.push_back(it.object().val());
      }
    }
    if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
      tags_to_delete.push_back(tag_entry);
  }
       
  void update_area_tags_local(const vector< Tag_Entry >& tags_to_delete)
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
      
      set< Uint32_Index > area_ids;
      for (vector< uint32 >::const_iterator it2(it->ids.begin());
	   it2 != it->ids.end(); ++it2)
	area_ids.insert(*it2);
      
      db_to_delete[index] = area_ids;
    }
    
    vector< pair< Area_Location, Uint31_Index > >::const_iterator
        rit(areas_to_insert.begin());
    for (set< uint32 >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      if ((rit != areas_to_insert.end()) && (*it == rit->first.id))
      {
	Tag_Index_Local index;
	index.index = rit->second.val() & 0xffffff00;
	
	for (vector< pair< string, string > >::const_iterator
	  it2(rit->first.tags.begin()); it2 != rit->first.tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(rit->first.id);
	  db_to_delete[index];
	}
	++rit;
      }
    }
    
    Block_Backend< Tag_Index_Local, Uint32_Index > area_db
	(*de_osm3s_file_ids::AREA_TAGS_LOCAL, true);
    area_db.update(db_to_delete, db_to_insert);
  }
  
  void update_area_tags_global(const vector< Tag_Entry >& tags_to_delete)
  {
    map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      for (vector< uint32 >::const_iterator it2(it->ids.begin());
	   it2 != it->ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }

    vector< pair< Area_Location, Uint31_Index > >::const_iterator
        rit(areas_to_insert.begin());
    for (set< uint32 >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      if ((rit != areas_to_insert.end()) && (*it == rit->first.id))
      {
	Tag_Index_Global index;
	
	for (vector< pair< string, string > >::const_iterator
	  it2(rit->first.tags.begin()); it2 != rit->first.tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(rit->first.id);
	  db_to_delete[index];
	}
	++rit;
      }
    }

    Block_Backend< Tag_Index_Global, Uint32_Index > area_db
      (*de_osm3s_file_ids::AREA_TAGS_GLOBAL, true);
    area_db.update(db_to_delete, db_to_insert);
  }
};

#endif
