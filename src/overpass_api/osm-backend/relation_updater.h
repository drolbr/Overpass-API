#ifndef DE_OSM3S__BACKEND__RELATION_UPDATER
#define DE_OSM3S__BACKEND__RELATION_UPDATER

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

struct Relation_Updater
{
  Relation_Updater()
  {
    max_role_id = 0;
    max_written_role_id = 0;
  }
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_relation
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< Relation_Entry >& members)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Relation rel;
    rel.id = id;
    rel.members = members;
    rel.tags = tags;
    rels_to_insert.push_back(rel);
  }
  
  void set_relation(const Relation& rel)
  {
    ids_to_modify.push_back(make_pair(rel.id, true));
    rels_to_insert.push_back(rel);
  }
  
  uint32 get_role_id(const string& s)
  {
    if (max_role_id == 0)
    {
      // load roles
      Block_Backend< Uint32_Index, String_Object > roles_db
      (*de_osm3s_file_ids::RELATION_ROLES, true);
      for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
	it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
      {
	cerr<<'.';
	role_ids[it.object().val()] = it.index().val();
	if (max_role_id <= it.index().val())
	  max_role_id = it.index().val()+1;
      }
      cerr<<"max_role_id on load "<<max_role_id<<'\n';
      max_written_role_id = max_role_id;
    }
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
    compute_indexes();
    update_rel_ids(to_delete);
    update_members(to_delete);

    vector< Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_rel_tags_local(tags_to_delete);
    update_rel_tags_global(tags_to_delete);
    flush_roles();

    ids_to_modify.clear();
    rels_to_insert.clear();
    
    cerr<<'R'<<' '<<time(NULL)<<' ';
    cerr<<'r'<<' '<<time(NULL)<<' ';
  }
  
  void update(vector< pair< uint32, uint32 > >& moved_relations)
  {
    show_mem_status();
    
    map< uint32, vector< uint32 > > to_delete;
    compute_indexes();
    update_rel_ids(to_delete, &moved_relations);
    update_members(to_delete);

    vector< Tag_Entry > tags_to_delete;
    prepare_delete_tags(tags_to_delete, to_delete);
    update_rel_tags_local(tags_to_delete);
    update_rel_tags_global(tags_to_delete);
    flush_roles();

    show_mem_status();
    
    ids_to_modify.clear();
    rels_to_insert.clear();
  }
  
  void update_moved_idxs(vector< pair< uint32, uint32 > >& moved_nodes,
			 vector< pair< uint32, uint32 > >& moved_ways,
			 vector< pair< uint32, uint32 > >& moved_relations)
  {
    show_mem_status();
    
    ids_to_modify.clear();
    rels_to_insert.clear();
    sort(moved_nodes.begin(), moved_nodes.end());
    sort(moved_ways.begin(), moved_ways.end());
    
    map< uint32, vector< uint32 > > to_delete;
    find_affected_relations(moved_nodes, moved_ways);
    update_rel_ids(to_delete, &moved_relations);
    update_members(to_delete);

    vector< Tag_Entry > tags_to_delete;
    prepare_tags(tags_to_delete, to_delete);
    update_rel_tags_local(tags_to_delete);
    flush_roles();

    show_mem_status();
    
    ids_to_modify.clear();
    rels_to_insert.clear();
  }
  
private:
  map< string, uint32 > role_ids;
  uint32 max_written_role_id;
  uint32 max_role_id;
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Relation > rels_to_insert;
  static Relation_Comparator_By_Id rel_comparator_by_id;
  static Relation_Equal_Id rel_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  
  void find_affected_relations(const vector< pair< uint32, uint32 > >& moved_nodes,
			       const vector< pair< uint32, uint32 > >& moved_ways)
  {
    set< Uint31_Index > req;
    for (vector< pair< uint32, uint32 > >::const_iterator
      it(moved_nodes.begin()); it != moved_nodes.end(); ++it)
    {
      req.insert(Uint31_Index(it->second));
      req.insert(Uint31_Index((it->second & 0x7fffff00) | 0x80000010));
      req.insert(Uint31_Index((it->second & 0x7fff0000) | 0x80000020));
      req.insert(Uint31_Index((it->second & 0x7f000000) | 0x80000030));
    }
    for (vector< pair< uint32, uint32 > >::const_iterator
      it(moved_ways.begin()); it != moved_ways.end(); ++it)
    {
      req.insert(Uint31_Index(it->second));
      req.insert(Uint31_Index((it->second & 0x7fffff00) | 0x80000010));
      req.insert(Uint31_Index((it->second & 0x7fff0000) | 0x80000020));
      req.insert(Uint31_Index((it->second & 0x7f000000) | 0x80000030));
    }
    req.insert(Uint31_Index(0x80000040));
    
    Block_Backend< Uint31_Index, Relation_Skeleton > rels_db
        (*de_osm3s_file_ids::RELATIONS, false);
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Discrete_Iterator
        it(rels_db.discrete_begin(req.begin(), req.end()));
        !(it == rels_db.discrete_end()); ++it)
    {
      const Relation_Skeleton& relation(it.object());
      bool is_affected(false);
      for (vector< Relation_Entry >::const_iterator it3(relation.members.begin());
          it3 != relation.members.end(); ++it3)
      {
	if (it3->type == Relation_Entry::NODE)
	{
	  if (binary_search(moved_nodes.begin(), moved_nodes.end(),
	      make_pair(it3->ref, 0), pair_comparator_by_id))
	  {
	    is_affected = true;
	    break;
	  }
	}
	else if (it3->type == Relation_Entry::WAY)
	{
	  if (binary_search(moved_ways.begin(), moved_ways.end(),
	      make_pair(it3->ref, 0), pair_comparator_by_id))
	  {
	    is_affected = true;
	    break;
	  }
	}
      }
      if (is_affected)
	rels_to_insert.push_back(Relation(relation.id, it.index().val(), relation.members));
    }
    
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
    
    Random_File< Uint32_Index > node_random(*de_osm3s_file_ids::NODES, false);
    Random_File< Uint31_Index > way_random(*de_osm3s_file_ids::WAYS, false);
    for (map< uint32, uint32 >::iterator it(used_nodes.begin());
        it != used_nodes.end(); ++it)
      it->second = node_random.get(it->first).val();
    for (map< uint32, uint32 >::iterator it(used_ways.begin());
        it != used_ways.end(); ++it)
      it->second = way_random.get(it->first).val();
    vector< Relation >::iterator writ(rels_to_insert.begin());
    for (vector< Relation >::iterator rit(rels_to_insert.begin());
        rit != rels_to_insert.end(); ++rit)
    {
      vector< uint32 > member_idxs;
      for (vector< Relation_Entry >::const_iterator nit(rit->members.begin());
          nit != rit->members.end(); ++nit)
      {
	if (nit->type == Relation_Entry::NODE)
	  member_idxs.push_back(used_nodes[nit->ref]);
	else if (nit->type == Relation_Entry::WAY)
	  member_idxs.push_back(used_ways[nit->ref]);
      }
      
      uint32 index(Relation::calc_index(member_idxs));
      if (rit != writ)
	*writ = *rit;
      if (writ->index != index)
      {
	ids_to_modify.push_back(make_pair(writ->id, true));
	writ->index = index;
	++writ;
      }
    }
    rels_to_insert.erase(writ, rels_to_insert.end());
  }
  
  void compute_indexes()
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
    
    Random_File< Uint32_Index > node_random(*de_osm3s_file_ids::NODES, false);
    Random_File< Uint31_Index > way_random(*de_osm3s_file_ids::WAYS, false);
    for (map< uint32, uint32 >::iterator it(used_nodes.begin());
	 it != used_nodes.end(); ++it)
      it->second = node_random.get(it->first).val();
    for (map< uint32, uint32 >::iterator it(used_ways.begin());
        it != used_ways.end(); ++it)
      it->second = way_random.get(it->first).val();
    for (vector< Relation >::iterator rit(rels_to_insert.begin());
	 rit != rels_to_insert.end(); ++rit)
    {
      vector< uint32 > member_idxs;
      for (vector< Relation_Entry >::const_iterator nit(rit->members.begin());
	   nit != rit->members.end(); ++nit)
      {
	if (nit->type == Relation_Entry::NODE)
	  member_idxs.push_back(used_nodes[nit->ref]);
	else if (nit->type == Relation_Entry::WAY)
	  member_idxs.push_back(used_ways[nit->ref]);
      }
      rit->index = Relation::calc_index(member_idxs);
    }
  }
  
  void update_rel_ids(map< uint32, vector< uint32 > >& to_delete,
		      vector< pair< uint32, uint32 > >* moved_relations = 0)
  {
    // keep always the most recent (last) element of all equal elements
    stable_sort(ids_to_modify.begin(), ids_to_modify.end(), pair_comparator_by_id);
    vector< pair< uint32, bool > >::iterator modi_begin
      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id)
      .base());
    ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
    stable_sort(rels_to_insert.begin(), rels_to_insert.end(), rel_comparator_by_id);
    vector< Relation >::iterator relations_begin
      (unique(rels_to_insert.rbegin(), rels_to_insert.rend(),
	      rel_equal_id).base());
    rels_to_insert.erase(rels_to_insert.begin(), relations_begin);
    
    // process the relations themselves
    Random_File< Uint31_Index > random(*de_osm3s_file_ids::RELATIONS, true);
    vector< Relation >::const_iterator rit(rels_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      Uint31_Index index(random.get(it->first));
      if (index.val() > 0)
	to_delete[index.val()].push_back(it->first);
      if ((rit != rels_to_insert.end()) && (it->first == rit->id))
      {
	if (it->second)
	{
	  random.put(it->first, Uint31_Index(rit->index));
	  if ((moved_relations != 0) && (index.val() > 0) &&
		      (index.val() != rit->index))
	    moved_relations->push_back(make_pair(it->first, index.val()));
	}
	++rit;
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
    vector< Relation >::const_iterator rit(rels_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      if ((rit != rels_to_insert.end()) && (it->first == rit->id))
      {
	if (it->second)
	{
	  Uint31_Index idx(rit->index);
	  db_to_insert[idx].insert(Relation_Skeleton(*rit));
	}
	++rit;
      }
    }
    
    Block_Backend< Uint31_Index, Relation_Skeleton > rel_db
      (*de_osm3s_file_ids::RELATIONS, true);
    rel_db.update(db_to_delete, db_to_insert);
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
    Block_Backend< Tag_Index_Local, Uint32_Index > rels_db
	(*de_osm3s_file_ids::RELATION_TAGS_LOCAL, true);
    Tag_Index_Local current_index;
    Tag_Entry tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(rels_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
	 !(it == rels_db.range_end()); ++it)
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
    Block_Backend< Tag_Index_Local, Uint32_Index > rels_db
	(*de_osm3s_file_ids::RELATION_TAGS_LOCAL, true);
    Tag_Index_Local current_index;
    Tag_Entry tag_entry;
    current_index.index = 0xffffffff;
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
	 it(rels_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
	 !(it == rels_db.range_end()); ++it)
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
	Relation* relation(binary_search_for_id(rels_to_insert, it.object().val()));
	if (relation != 0)
	  relation->tags.push_back(make_pair(it.index().key, it.index().value));
	tag_entry.ids.push_back(it.object().val());
      }
    }
    if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
      tags_to_delete.push_back(tag_entry);
  }
       
  void update_rel_tags_local(const vector< Tag_Entry >& tags_to_delete)
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
      
      set< Uint32_Index > rel_ids;
      for (vector< uint32 >::const_iterator it2(it->ids.begin());
	   it2 != it->ids.end(); ++it2)
	rel_ids.insert(*it2);
      
      db_to_delete[index] = rel_ids;
    }
    
    vector< Relation >::const_iterator rit(rels_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
        it != ids_to_modify.end(); ++it)
    {
      if ((rit != rels_to_insert.end()) && (it->first == rit->id))
      {
	if (it->second)
	{
	  Tag_Index_Local index;
	  index.index = rit->index & 0xffffff00;
	  
	  for (vector< pair< string, string > >::const_iterator
	      it2(rit->tags.begin()); it2 != rit->tags.end(); ++it2)
	  {
	    index.key = it2->first;
	    index.value = it2->second;
	    db_to_insert[index].insert(rit->id);
	    db_to_delete[index];
	  }
	}
	++rit;
      }
    }
	
    Block_Backend< Tag_Index_Local, Uint32_Index > rel_db
	(*de_osm3s_file_ids::RELATION_TAGS_LOCAL, true);
    rel_db.update(db_to_delete, db_to_insert);
  }
  
  void update_rel_tags_global(const vector< Tag_Entry >& tags_to_delete)
  {
    map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
	 it != tags_to_delete.end(); ++it)
    {
      Tag_Index_Global index;
      index.key = it->key;
      index.value = it->value;
      
      set< Uint32_Index > rel_ids;
      for (vector< uint32 >::const_iterator it2(it->ids.begin());
	   it2 != it->ids.end(); ++it2)
	db_to_delete[index].insert(*it2);
    }
    
    vector< Relation >::const_iterator rit(rels_to_insert.begin());
    for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
    it != ids_to_modify.end(); ++it)
    {
      if ((rit != rels_to_insert.end()) && (it->first == rit->id))
      {
	if (it->second)
	{
	  Tag_Index_Global index;
	  
	  for (vector< pair< string, string > >::const_iterator
	      it2(rit->tags.begin()); it2 != rit->tags.end(); ++it2)
	  {
	    index.key = it2->first;
	    index.value = it2->second;
	    db_to_insert[index].insert(rit->id);
	    db_to_delete[index];
	  }
	}
	++rit;
      }
    }
	
    Block_Backend< Tag_Index_Global, Uint32_Index > rel_db
      (*de_osm3s_file_ids::RELATION_TAGS_GLOBAL, true);
    rel_db.update(db_to_delete, db_to_insert);
  }

  void flush_roles()
  {
    cerr<<"max_role_id "<<max_role_id<<'\n'
        <<"max_written_role_id "<<max_written_role_id<<'\n';
    map< Uint32_Index, set< String_Object > > db_to_delete;
    map< Uint32_Index, set< String_Object > > db_to_insert;
    
    for (map< string, uint32 >::const_iterator it(role_ids.begin());
        it != role_ids.end(); ++it)
    {
      if (it->second >= max_written_role_id)
	db_to_insert[Uint32_Index(it->second)].insert
	  (String_Object(it->first));
      if (it->second >= max_role_id)
	max_role_id = it->second + 1;
    }
    
    Block_Backend< Uint32_Index, String_Object > roles_db
        (*de_osm3s_file_ids::RELATION_ROLES, true);
    roles_db.update(db_to_delete, db_to_insert);
    max_written_role_id = max_role_id;
  }
};

#endif
