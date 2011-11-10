#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "meta_collector.h"
#include "area_query.h"
#include "around.h"
#include "bbox_query.h"
#include "item.h"
#include "newer.h"
#include "query.h"
#include "user.h"

#include <algorithm>
#include <sstream>

using namespace std;

//-----------------------------------------------------------------------------

void Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  if (attributes["type"] == "node")
    type = QUERY_NODE;
  else if (attributes["type"] == "way")
    type = QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = QUERY_RELATION;
/*  else if (attributes["type"] == "area")
    type = QUERY_AREA;*/
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
/*    temp<<"For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\", \"relation\" or \"area\".";*/
    add_static_error(temp.str());
  }
}

void Query_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  Has_Kv_Statement* has_kv(dynamic_cast<Has_Kv_Statement*>(statement));
  if (has_kv)
  {
    key_values.push_back(make_pair< string, string >
	(has_kv->get_key(), has_kv->get_value()));
    return;
  }
  Area_Query_Statement* area(dynamic_cast<Area_Query_Statement*>(statement));
  Around_Statement* around(dynamic_cast<Around_Statement*>(statement));
  Bbox_Query_Statement* bbox(dynamic_cast<Bbox_Query_Statement*>(statement));
  Item_Statement* item(dynamic_cast<Item_Statement*>(statement));
  Newer_Statement* newer(dynamic_cast<Newer_Statement*>(statement));
  User_Statement* user(dynamic_cast<User_Statement*>(statement));
  if (area != 0)
  {
    if (type != QUERY_NODE)
    {
      ostringstream temp;
      temp<<"An area-query as substatement is only allowed for queries of type \"node\".";
      add_static_error(temp.str());
      return;
    }
    constraints.push_back(statement->get_query_constraint());
  }
  else if (around != 0)
  {
    if (type != QUERY_NODE)
    {
      ostringstream temp;
      temp<<"An around as substatement is only allowed for queries of type \"node\".";
      add_static_error(temp.str());
      return;
    }
    constraints.push_back(statement->get_query_constraint());
  }
  else if ((bbox != 0) || (item != 0) || (user != 0) || (newer != 0))
    constraints.push_back(statement->get_query_constraint());
  else
    substatement_error(get_name(), statement);
}

void Query_Statement::forecast()
{
}

vector< uint32 > Query_Statement::collect_ids
  (const vector< pair< string, string > >& key_values,
   const File_Properties& file_prop, uint32 stopwatch_account,
   Resource_Manager& rman)
{
  if (key_values.empty())
    return vector< uint32 >();
 
  stopwatch.stop(Stopwatch::NO_DISK);
  Block_Backend< Tag_Index_Global, Uint32_Index > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  
  vector< uint32 > new_ids;
  vector< pair< string, string > >::const_iterator it = key_values.begin();
  if (it->second != "")
  {
    set< Tag_Index_Global > tag_req;
    Tag_Index_Global idx;
    idx.key = it->first;
    idx.value = it->second;
    tag_req.insert(idx);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
        it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
        !(it2 == tags_db.discrete_end()); ++it2)
      new_ids.push_back(it2.object().val());
  }
  else
  {
    set< pair< Tag_Index_Global, Tag_Index_Global > > range_req;
    pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
    idx_pair.first.key = it->first;
    idx_pair.first.value = "";
    idx_pair.second.key = it->first + (char)0;
    idx_pair.second.value = "";
    range_req.insert(idx_pair);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
        it2(tags_db.range_begin
          (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
        !(it2 == tags_db.range_end()); ++it2)
      new_ids.push_back(it2.object().val());
  }
  stopwatch.add(stopwatch_account, tags_db.read_count());
  stopwatch.stop(stopwatch_account);
  sort(new_ids.begin(), new_ids.end());
  stopwatch.stop(Stopwatch::NO_DISK);
  ++it;
  
  for (; it != key_values.end(); ++it)
  {
    vector< uint32 > old_ids;
    old_ids.swap(new_ids);
    if (it->second != "")
    {
      set< Tag_Index_Global > tag_req;
      Tag_Index_Global idx;
      idx.key = it->first;
      idx.value = it->second;
      tag_req.insert(idx);
      for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
          it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
          !(it2 == tags_db.discrete_end()); ++it2)
      {
        if (binary_search(old_ids.begin(), old_ids.end(), it2.object().val()))
	  new_ids.push_back(it2.object().val());
      }
    }
    else
    {
      set< pair< Tag_Index_Global, Tag_Index_Global > > range_req;
      pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
      idx_pair.first.key = it->first;
      idx_pair.first.value = "";
      idx_pair.second.key = it->first + (char)0;
      idx_pair.second.value = "";
      range_req.insert(idx_pair);
      for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
	  it2(tags_db.range_begin
	  (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
	  !(it2 == tags_db.range_end()); ++it2)
      {
	if (binary_search(old_ids.begin(), old_ids.end(), it2.object().val()))
	  new_ids.push_back(it2.object().val());
      }
    }
    stopwatch.add(stopwatch_account, tags_db.read_count());
    stopwatch.stop(stopwatch_account);
    sort(new_ids.begin(), new_ids.end());
    stopwatch.stop(Stopwatch::NO_DISK);
    
    rman.health_check(*this);
  }
  
  return new_ids;
}

template < typename TIndex, typename TObject >
void Query_Statement::get_elements_by_id_from_db
    (map< TIndex, vector< TObject > >& elements,
     const vector< uint32 >& ids, const set< pair< TIndex, TIndex > >& range_req,
     Resource_Manager& rman, File_Properties& file_prop)
{
  uint elements_count = 0;
  Block_Backend< TIndex, TObject > elements_db
      (rman.get_transaction()->data_index(&file_prop));
  for (typename Block_Backend< TIndex, TObject >::Range_Iterator
      it(elements_db.range_begin
          (Default_Range_Iterator< TIndex >(range_req.begin()),
	   Default_Range_Iterator< TIndex >(range_req.end())));
      !(it == elements_db.range_end()); ++it)
  {
    if (++elements_count >= 64*1024)
    {
      elements_count = 0;
      rman.health_check(*this);
    }
    
    if (binary_search(ids.begin(), ids.end(), it.object().id) || (ids.empty()))
      elements[it.index()].push_back(it.object());
  }    
}

template < typename TIndex >
set< pair< TIndex, TIndex > > Query_Statement::get_ranges_by_id_from_db
    (const vector< uint32 >& ids,
     Resource_Manager& rman, File_Properties& file_prop)
{
  set< pair< TIndex, TIndex > > range_req;
  {
    Random_File< TIndex > random
        (rman.get_transaction()->random_index(&file_prop));
    for (vector< uint32 >::const_iterator it(ids.begin()); it != ids.end(); ++it)
      range_req.insert(make_pair(random.get(*it), TIndex(random.get(*it).val()+1)));
  }
  return range_req;
}

template< typename TIndex, typename TObject >
void clear_empty_indices
    (map< TIndex, vector< TObject > >& modify)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end();)
  {
    if (!it->second.empty())
    {
      ++it;
      continue;
    }
    typename map< TIndex, vector< TObject > >::iterator next_it = it;
    if (++next_it == modify.end())
    {
      modify.erase(it);
      break;
    }
    else
    {
      TIndex idx = next_it->first;
      modify.erase(it);
      it = modify.find(idx);
    }
  }
}

void Query_Statement::execute(Resource_Manager& rman)
{
  enum { nothing, /*ids_collected,*/ ranges_collected, data_collected } answer_state
      = nothing;
  Set into;
  
  stopwatch.start();
  
  if (type == QUERY_NODE)
  {
    vector< uint32 > ids(collect_ids
        (key_values, *osm_base_settings().NODE_TAGS_GLOBAL,
         Stopwatch::NODE_TAGS_GLOBAL, rman));

    if (ids.empty() && !key_values.empty())
      answer_state = data_collected;
    
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < data_collected; ++it)
    {
      if ((*it)->collect(rman, into, type, ids))
        answer_state = data_collected;
    }
  
    set< pair< Uint32_Index, Uint32_Index > > range_req;
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < ranges_collected; ++it)
    {
      if ((*it)->get_ranges(rman, range_req))
	answer_state = ranges_collected;
    }
  
    if (answer_state < ranges_collected)
      range_req = get_ranges_by_id_from_db< Uint32_Index >
          (ids, rman, *osm_base_settings().NODES);
    if (answer_state < data_collected)
      get_elements_by_id_from_db< Uint32_Index, Node_Skeleton >
          (into.nodes, ids, range_req, rman, *osm_base_settings().NODES);
  }
  else if (type == QUERY_WAY)
  {
    vector< uint32 > ids(collect_ids
        (key_values, *osm_base_settings().WAY_TAGS_GLOBAL,
	 Stopwatch::WAY_TAGS_GLOBAL, rman));

    if (ids.empty() && !key_values.empty())
      answer_state = data_collected;
    
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < data_collected; ++it)
    {
      if ((*it)->collect(rman, into, type, ids))
        answer_state = data_collected;
    }
  
    set< pair< Uint31_Index, Uint31_Index > > range_req;
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < ranges_collected; ++it)
    {
      if ((*it)->get_ranges(rman, range_req))
	answer_state = ranges_collected;
    }
    
    if (answer_state < ranges_collected)
      range_req = get_ranges_by_id_from_db< Uint31_Index >
          (ids, rman, *osm_base_settings().WAYS);
    if (answer_state < data_collected)
      get_elements_by_id_from_db< Uint31_Index, Way_Skeleton >
          (into.ways, ids, range_req, rman, *osm_base_settings().WAYS);
  }
  else if (type == QUERY_RELATION)
  {
    vector< uint32 > ids(collect_ids
        (key_values, *osm_base_settings().RELATION_TAGS_GLOBAL,
	 Stopwatch::RELATION_TAGS_GLOBAL, rman));
	 
    if (ids.empty() && !key_values.empty())
      answer_state = data_collected;
    
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < data_collected; ++it)
    {
      if ((*it)->collect(rman, into, type, ids))
        answer_state = data_collected;
    }
  
    set< pair< Uint31_Index, Uint31_Index > > range_req;
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < ranges_collected; ++it)
    {
      if ((*it)->get_ranges(rman, range_req))
	answer_state = ranges_collected;
    }
    
    if (answer_state < ranges_collected)
      range_req = get_ranges_by_id_from_db< Uint31_Index >
          (ids, rman, *osm_base_settings().RELATIONS);
    if (answer_state < data_collected)
      get_elements_by_id_from_db< Uint31_Index, Relation_Skeleton >
          (into.relations, ids, range_req, rman, *osm_base_settings().RELATIONS);
  }
  
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(rman, into);
  
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(*this, rman, into);
  
  clear_empty_indices(into.nodes);
  clear_empty_indices(into.ways);
  clear_empty_indices(into.relations);
  
  into.nodes.swap(rman.sets()[output].nodes);
  into.ways.swap(rman.sets()[output].ways);
  into.relations.swap(rman.sets()[output].relations);
  rman.sets()[output].areas.clear();
  
  stopwatch.report(get_name());  
  rman.health_check(*this);
}

//-----------------------------------------------------------------------------

void Has_Kv_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["v"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  key = attributes["k"];
  value = attributes["v"];
  if (key == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
}

void Has_Kv_Statement::forecast()
{
  // will never be called
}
