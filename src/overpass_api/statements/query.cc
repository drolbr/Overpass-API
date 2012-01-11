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

#include "sys/types.h"
#include "regex.h"

#include <algorithm>
#include <sstream>

using namespace std;

//-----------------------------------------------------------------------------

class Regular_Expression
{
  public:
    Regular_Expression(const string& regex);
    ~Regular_Expression();
    
    bool matches(const string& line);
    
  private:
    regex_t preg;
};

Regular_Expression::Regular_Expression(const string& regex)
{
  regcomp(&preg, regex.c_str(), REG_EXTENDED|REG_NOSUB);
}

Regular_Expression::~Regular_Expression()
{
  regfree(&preg);
}

bool Regular_Expression::matches(const string& line)
{
  return (regexec(&preg, line.c_str(), 0, 0, 0) == 0);
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Query_Statement > Query_Statement::statement_maker("query");

Query_Statement::Query_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
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
    if (has_kv->get_value() != "")
      key_values.push_back(make_pair< string, string >
	  (has_kv->get_key(), has_kv->get_value()));
    else if (has_kv->get_regex())
      key_regexes.push_back(make_pair< string, Regular_Expression* >
          (has_kv->get_key(), has_kv->get_regex()));
    else
      keys.push_back(has_kv->get_key());
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

set< Tag_Index_Global > get_kv_req(const string& key, const string& value)
{
  set< Tag_Index_Global > result;
  Tag_Index_Global idx;
  idx.key = key;
  idx.value = value;
  result.insert(idx);
  return result;
}

set< pair< Tag_Index_Global, Tag_Index_Global > > get_k_req(const string& key)
{
  set< pair< Tag_Index_Global, Tag_Index_Global > > result;
  pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
  idx_pair.first.key = key;
  idx_pair.first.value = "";
  idx_pair.second.key = key + (char)0;
  idx_pair.second.value = "";
  result.insert(idx_pair);
  return result;
}

vector< uint32 > Query_Statement::collect_ids
  (const File_Properties& file_prop, uint32 stopwatch_account,
   Resource_Manager& rman)
{
  if (key_values.empty() && keys.empty() && key_regexes.empty())
    return vector< uint32 >();
 
  Block_Backend< Tag_Index_Global, Uint32_Index > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  
  vector< uint32 > new_ids;
  vector< pair< string, string > >::const_iterator kvit = key_values.begin();

  if (kvit != key_values.end())
  {
    set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
        it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
        !(it2 == tags_db.discrete_end()); ++it2)
      new_ids.push_back(it2.object().val());

    sort(new_ids.begin(), new_ids.end());
    rman.health_check(*this);
    ++kvit;
  }
  
  for (; kvit != key_values.end(); ++kvit)
  {
    vector< uint32 > old_ids;
    old_ids.swap(new_ids);

    set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
        it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
        !(it2 == tags_db.discrete_end()); ++it2)
    {
      if (binary_search(old_ids.begin(), old_ids.end(), it2.object().val()))
	new_ids.push_back(it2.object().val());
    }
    sort(new_ids.begin(), new_ids.end());    
    rman.health_check(*this);
  }

  vector< string >::const_iterator kit = keys.begin();
  if (key_values.empty() && kit != keys.end())
  {
    set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(*kit);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
        it2(tags_db.range_begin
          (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
        !(it2 == tags_db.range_end()); ++it2)
      new_ids.push_back(it2.object().val());

    sort(new_ids.begin(), new_ids.end());
    rman.health_check(*this);
    ++kit;
  }
  
  for (; kit != keys.end(); ++kit)
  {
    vector< uint32 > old_ids;
    old_ids.swap(new_ids);
    {
      set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(*kit);
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
    sort(new_ids.begin(), new_ids.end());    
    rman.health_check(*this);
  }

  vector< pair< string, Regular_Expression* > >::const_iterator krit = key_regexes.begin();
  if (key_values.empty() && keys.empty() && krit != key_regexes.end())
  {
    set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(krit->first);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
        it2(tags_db.range_begin
          (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
        !(it2 == tags_db.range_end()); ++it2)
    {
      if (krit->second->matches(it2.index().value))
        new_ids.push_back(it2.object().val());
    }

    sort(new_ids.begin(), new_ids.end());
    rman.health_check(*this);
    ++krit;
  }
  
  for (; krit != key_regexes.end(); ++krit)
  {
    vector< uint32 > old_ids;
    old_ids.swap(new_ids);
    {
      set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(krit->first);
      for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
	  it2(tags_db.range_begin
	  (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
	  !(it2 == tags_db.range_end()); ++it2)
      {
	if (binary_search(old_ids.begin(), old_ids.end(), it2.object().val())
	    && krit->second->matches(it2.index().value))
	  new_ids.push_back(it2.object().val());
      }
    }
    sort(new_ids.begin(), new_ids.end());    
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
  set_progress(1);
  rman.health_check(*this);
  
  if (type == QUERY_NODE)
  {
    vector< uint32 > ids(collect_ids
        (*osm_base_settings().NODE_TAGS_GLOBAL, Stopwatch::NODE_TAGS_GLOBAL, rman));

    if (ids.empty() && !key_values.empty())
      answer_state = data_collected;
    
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < data_collected; ++it)
    {
      if ((*it)->collect(rman, into, type, ids))
        answer_state = data_collected;
    }

    set_progress(2);
    rman.health_check(*this);

    set< pair< Uint32_Index, Uint32_Index > > range_req;
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < ranges_collected; ++it)
    {
      if ((*it)->get_ranges(rman, range_req))
	answer_state = ranges_collected;
    }
  
    set_progress(3);
    rman.health_check(*this);
    
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
        (*osm_base_settings().WAY_TAGS_GLOBAL, Stopwatch::WAY_TAGS_GLOBAL, rman));

    if (ids.empty() && !key_values.empty())
      answer_state = data_collected;
    
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < data_collected; ++it)
    {
      if ((*it)->collect(rman, into, type, ids))
        answer_state = data_collected;
    }
  
    set_progress(2);
    rman.health_check(*this);
    
    set< pair< Uint31_Index, Uint31_Index > > range_req;
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < ranges_collected; ++it)
    {
      if ((*it)->get_ranges(rman, range_req))
	answer_state = ranges_collected;
    }
    
    set_progress(3);
    rman.health_check(*this);
    
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
        (*osm_base_settings().RELATION_TAGS_GLOBAL, Stopwatch::RELATION_TAGS_GLOBAL, rman));
	 
    if (ids.empty() && !key_values.empty())
      answer_state = data_collected;
    
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < data_collected; ++it)
    {
      if ((*it)->collect(rman, into, type, ids))
        answer_state = data_collected;
    }
  
    set_progress(2);
    rman.health_check(*this);
    
    set< pair< Uint31_Index, Uint31_Index > > range_req;
    for (vector< Query_Constraint* >::iterator it = constraints.begin();
        it != constraints.end() && answer_state < ranges_collected; ++it)
    {
      if ((*it)->get_ranges(rman, range_req))
	answer_state = ranges_collected;
    }
    
    set_progress(3);
    rman.health_check(*this);
    
    if (answer_state < ranges_collected)
      range_req = get_ranges_by_id_from_db< Uint31_Index >
          (ids, rman, *osm_base_settings().RELATIONS);
    if (answer_state < data_collected)
      get_elements_by_id_from_db< Uint31_Index, Relation_Skeleton >
          (into.relations, ids, range_req, rman, *osm_base_settings().RELATIONS);
  }
  
  set_progress(4);
  rman.health_check(*this);
  
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(rman, into);
  
  set_progress(5);
  rman.health_check(*this);
  
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(*this, rman, into);
  
  set_progress(6);
  rman.health_check(*this);
  
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

Generic_Statement_Maker< Has_Kv_Statement > Has_Kv_Statement::statement_maker("has-kv");

Has_Kv_Statement::Has_Kv_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_), regex(0)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["v"] = "";
  attributes["regv"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
  value = attributes["v"];
  if (key == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
  if (attributes["regv"] != "")
  {
    if (value != "")
    {
      ostringstream temp("");
      temp<<"In the element \"has-kv\" only one of the attributes \"v\" and \"regv\""
            " can be nonempty.";
      add_static_error(temp.str());
    }
    
    regex = new Regular_Expression(attributes["regv"]);
  }
}

Has_Kv_Statement::~Has_Kv_Statement()
{
  if (regex)
    delete regex;
}

void Has_Kv_Statement::forecast()
{
  // will never be called
}
