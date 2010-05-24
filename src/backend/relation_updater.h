#ifndef DE_OSM3S__BACKEND__RELATION_UPDATER
#define DE_OSM3S__BACKEND__RELATION_UPDATER

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

struct Relation_Tag_Entry
{
  uint32 index;
  string key;
  string value;
  vector< uint32 > rel_ids;
};

struct Relation_Tag_Index_Local
{
  uint32 index;
  string key;
  string value;
  
  Relation_Tag_Index_Local() {}
  
  Relation_Tag_Index_Local(void* data)
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
  
  bool operator<(const Relation_Tag_Index_Local& a) const
  {
    if ((index & 0x7fffffff) != (a.index & 0x7fffffff))
      return ((index & 0x7fffffff) < (a.index & 0x7fffffff));
    if (index != a.index)
      return (index < a.index);
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Relation_Tag_Index_Local& a) const
  {
    if (index != a.index)
      return false;
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Relation_Tag_Index_Global
{
  string key;
  string value;
  
  Relation_Tag_Index_Global() {}
  
  Relation_Tag_Index_Global(void* data)
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
  
  bool operator<(const Relation_Tag_Index_Global& a) const
  {
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Relation_Tag_Index_Global& a) const
  {
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Relation_Updater
{
  Relation_Updater()
  {
    // load roles
    Block_Backend< Uint32_Index, String_Object > roles_db
      (de_osm3s_file_ids::RELATION_ROLES, true);
    max_role_id = 0;
    for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
        it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
    {
      role_ids[it.object().val()] = it.index().val();
      if (max_role_id <= it.index().val())
	max_role_id = it.index().val()+1;
    }
    max_written_role_id = max_role_id;
  }
  
  void set_id_deleted(uint32 id)
  {
    ids_to_delete.push_back(id);
  }
  
  void set_relation
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< Relation_Entry >& members)
  {
    ids_to_delete.push_back(id);
    
    Relation rel;
    rel.id = id;
    rel.members = members;
    rel.tags = tags;
    rels_to_insert.push_back(rel);
  }
  
  void set_relation(const Relation& rel)
  {
    ids_to_delete.push_back(rel.id);
    rels_to_insert.push_back(rel);
  }
  
  uint32 get_role_id(const string& s)
  {
    map< string, uint32 >::const_iterator it(role_ids.find(s));
    if (it != role_ids.end())
      return it->second;
    role_ids[s] = max_role_id;
    ++max_role_id;
    return (max_role_id - 1);
  }
  
  void update()
  {
    cerr<<'.'<<' '<<time(NULL)<<' ';
    
    map< uint32, vector< uint32 > > to_delete;
    update_rel_ids(to_delete);
    update_members(to_delete);

    vector< Relation_Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_rel_tags_local(tags_to_delete);
    update_rel_tags_global(tags_to_delete);
    flush_roles();

    ids_to_delete.clear();
    rels_to_insert.clear();
    
    cerr<<'R'<<' '<<time(NULL)<<' ';
    cerr<<'r'<<' '<<time(NULL)<<' ';
  }
  
private:
  map< string, uint32 > role_ids;
  uint32 max_written_role_id;
  uint32 max_role_id;
  vector< uint32 > ids_to_delete;
  vector< Relation > rels_to_insert;
  static Relation_Comparator_By_Id rel_comparator_by_id;
  
  void update_rel_ids(map< uint32, vector< uint32 > >& to_delete)
  {
    // retrieve the indices of the referred nodes and ways
    map< uint32, uint32 > used_nodes;
    map< uint32, uint32 > used_ways;
    for (vector< Relation >::const_iterator wit(rels_to_insert.begin());
	 wit != rels_to_insert.end(); ++wit)
    {
      for (vector< Relation_Entry >::const_iterator nit(wit->members.begin());
	   nit != wit->members.end(); ++nit)
      {
	if (nit->type == Relation_Entry::NODE)
	  used_nodes[nit->ref] = 0;
	else if (nit->type == Relation_Entry::WAY)
	  used_ways[nit->ref] = 0;
      }
    }
    
    Random_File< Uint32_Index > node_random(de_osm3s_file_ids::NODES, false);
    Random_File< Uint31_Index > way_random(de_osm3s_file_ids::WAYS, false);
    for (map< uint32, uint32 >::iterator it(used_nodes.begin());
	 it != used_nodes.end(); ++it)
      it->second = node_random.get(it->first).val();
    for (map< uint32, uint32 >::iterator it(used_ways.begin());
        it != used_ways.end(); ++it)
      it->second = way_random.get(it->first).val();
    for (vector< Relation >::iterator wit(rels_to_insert.begin());
	 wit != rels_to_insert.end(); ++wit)
    {
      vector< uint32 > member_idxs;
      for (vector< Relation_Entry >::const_iterator nit(wit->members.begin());
	   nit != wit->members.end(); ++nit)
      {
	if (nit->type == Relation_Entry::NODE)
	  member_idxs.push_back(used_nodes[nit->ref]);
	else if (nit->type == Relation_Entry::WAY)
	  member_idxs.push_back(used_ways[nit->ref]);
      }
      wit->index = Relation::calc_index(member_idxs);
    }
    
    // process the rels itself
    sort(ids_to_delete.begin(), ids_to_delete.end());
    sort(rels_to_insert.begin(), rels_to_insert.end(), rel_comparator_by_id);
    
    Random_File< Uint31_Index > random(de_osm3s_file_ids::RELATIONS, true);
    vector< Relation >::const_iterator wit(rels_to_insert.begin());
    for (vector< uint32 >::const_iterator it(ids_to_delete.begin());
        it != ids_to_delete.end(); ++it)
    {
      Uint31_Index index(random.get(*it));
      if (index.val() > 0)
	to_delete[index.val()].push_back(*it);
      if ((wit != rels_to_insert.end()) && (*it == wit->id))
      {
	random.put(*it, Uint31_Index(wit->index));
	++wit;
      }
    }
  }
  
  void update_members(const map< uint32, vector< uint32 > >& to_delete)
  {
    map< Uint31_Index, set< Relation_Skeleton > > db_to_delete;
    map< Uint31_Index, set< Relation_Skeleton > > db_to_insert;
    
    for (map< uint32, vector< uint32 > >::const_iterator
        it(to_delete.begin()); it != to_delete.end(); ++it)
    {
      Uint31_Index idx(it->first);
      for (vector< uint32 >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
	db_to_delete[idx].insert(Relation_Skeleton(*it2, vector< Relation_Entry >()));
    }
    for (vector< Relation >::const_iterator it(rels_to_insert.begin());
        it != rels_to_insert.end(); ++it)
    {
      Uint31_Index idx(it->index);
      db_to_insert[idx].insert(Relation_Skeleton(*it));
    }
    
    Block_Backend< Uint31_Index, Relation_Skeleton > rel_db
      (de_osm3s_file_ids::RELATIONS, true);
    rel_db.update(db_to_delete, db_to_insert);
  }
  
  void prepare_delete_tags
      (vector< Relation_Tag_Entry >& tags_to_delete,
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
    set< pair< Relation_Tag_Index_Local, Relation_Tag_Index_Local > > range_set;
    for (map< uint32, set< uint32 > >::const_iterator
	 it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
    {
      Relation_Tag_Index_Local lower, upper;
      lower.index = it->first;
      lower.key = "";
      lower.value = "";
      upper.index = it->first + 1;
      upper.key = "";
      upper.value = "";
      range_set.insert(make_pair(lower, upper));
    }
    
    // iterate over the result
    Block_Backend< Relation_Tag_Index_Local, Uint32_Index > rels_db
	(de_osm3s_file_ids::RELATION_TAGS_LOCAL, true);
    Relation_Tag_Index_Local current_index;
    Relation_Tag_Entry rel_tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Relation_Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(rels_db.range_begin
	     (Default_Range_Iterator< Relation_Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Relation_Tag_Index_Local >(range_set.end())));
	 !(it == rels_db.range_end()); ++it)
    {
      if (!(current_index == it.index()))
      {
	if ((current_index.index != 0xffffffff) && (!rel_tag_entry.rel_ids.empty()))
	  tags_to_delete.push_back(rel_tag_entry);
	current_index = it.index();
	rel_tag_entry.index = it.index().index;
	rel_tag_entry.key = it.index().key;
	rel_tag_entry.value = it.index().value;
	rel_tag_entry.rel_ids.clear();
      }
      
      set< uint32 >& handle(to_delete_coarse[it.index().index]);
      if (handle.find(it.object().val()) != handle.end())
	rel_tag_entry.rel_ids.push_back(it.object().val());
    }
    if ((current_index.index != 0xffffffff) && (!rel_tag_entry.rel_ids.empty()))
      tags_to_delete.push_back(rel_tag_entry);
  }
       
  void update_rel_tags_local(const vector< Relation_Tag_Entry >& tags_to_delete)
  {
    map< Relation_Tag_Index_Local, set< Uint32_Index > > db_to_delete;
    map< Relation_Tag_Index_Local, set< Uint32_Index > > db_to_insert;
    
    for (vector< Relation_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Relation_Tag_Index_Local index;
      index.index = it->index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > rel_ids;
      for (vector< uint32 >::const_iterator it2(it->rel_ids.begin());
	   it2 != it->rel_ids.end(); ++it2)
	rel_ids.insert(*it2);
      
      db_to_delete[index] = rel_ids;
    }
    
    for (vector< Relation >::const_iterator it(rels_to_insert.begin());
	 it != rels_to_insert.end(); ++it)
    {
      Relation_Tag_Index_Local index;
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
    
    Block_Backend< Relation_Tag_Index_Local, Uint32_Index > rel_db
	(de_osm3s_file_ids::RELATION_TAGS_LOCAL, true);
    rel_db.update(db_to_delete, db_to_insert);
  }
  
  void update_rel_tags_global(const vector< Relation_Tag_Entry >& tags_to_delete)
  {
    map< Relation_Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Relation_Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Relation_Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Relation_Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > rel_ids;
      for (vector< uint32 >::const_iterator it2(it->rel_ids.begin());
	   it2 != it->rel_ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    for (vector< Relation >::const_iterator it(rels_to_insert.begin());
	 it != rels_to_insert.end(); ++it)
    {
      Relation_Tag_Index_Global index;
      
      for (vector< pair< string, string > >::const_iterator it2(it->tags.begin());
	   it2 != it->tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(it->id);
	db_to_delete[index];
      }
    }
    
    Block_Backend< Relation_Tag_Index_Global, Uint32_Index > rel_db
      (de_osm3s_file_ids::RELATION_TAGS_GLOBAL, true);
    rel_db.update(db_to_delete, db_to_insert);
  }

  void flush_roles()
  {
    map< Uint32_Index, set< String_Object > > db_to_delete;
    map< Uint32_Index, set< String_Object > > db_to_insert;
    
    for (map< string, uint32 >::const_iterator it(role_ids.begin());
    it != role_ids.end(); ++it)
    {
      if (it->second >= max_written_role_id)
	db_to_insert[Uint32_Index(it->second)].insert
	(String_Object(it->first));
    }
    
    Block_Backend< Uint32_Index, String_Object > roles_db
    (de_osm3s_file_ids::RELATION_ROLES, true);
    roles_db.update(db_to_delete, db_to_insert);
  }
};

#endif
