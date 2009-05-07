#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "script_datatypes.h"
#include "script_queries.h"
#include "user_interface.h"
#include "vigilance_control.h"
#include "backend/file_types.h"
#include "backend/node_strings_file_io.h"
#include "backend/relation_strings_file_io.h"
#include "backend/way_strings_file_io.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <mysql.h>

using namespace std;

MYSQL_RES* mysql_query_wrapper(MYSQL* mysql, string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    ostringstream temp;
    temp<<"Error during SQL query ";
    temp<<'('<<query_status<<"):\n";
    temp<<"Query: "<<query<<'\n';
    temp<<"Error: "<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }

  MYSQL_RES* result(mysql_store_result(mysql));
  if (!result)
  {
    if (is_timed_out())
      runtime_error("Your query timed out.", cout);
    ostringstream temp;
    temp<<"Error during SQL query (result is null pointer)\n";
    temp<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }
  
  return result;
}

MYSQL_RES* mysql_query_use_wrapper(MYSQL* mysql, string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    ostringstream temp;
    temp<<"Error during SQL query ";
    temp<<'('<<query_status<<"):\n";
    temp<<"Query: "<<query<<'\n';
    temp<<"Error: "<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }

  MYSQL_RES* result(mysql_use_result(mysql));
  if (!result)
  {
    if (is_timed_out())
      runtime_error("Your query timed out.", cout);
    ostringstream temp;
    temp<<"Error during SQL query (result is null pointer)\n";
    temp<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }
  
  return result;
}

void mysql_query_null_wrapper(MYSQL* mysql, string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    ostringstream temp;
    temp<<"Error during SQL query ";
    temp<<'('<<query_status<<"):\n";
    temp<<"Query: "<<query<<'\n';
    temp<<"Error: "<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }
}

int int_query(MYSQL* mysql, string query)
{
  int result_val(0);
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return 0;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  if ((row) && (row[0]))
    result_val = atoi(row[0]);
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_val;
}

pair< int, int > intint_query(MYSQL* mysql, string query)
{
  pair< int, int > result_val(make_pair< int, int >(0, 0));
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_val;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  if ((row) && (row[0]) && (row[1]))
    result_val = make_pair< int, int >(atoi(row[0]), atoi(row[1]));
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_val;
}

set< uint32 >& multiint_query(MYSQL* mysql, string query, set< uint32 >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]))
  {
    result_set.insert(atoi(row[0]));
    row = mysql_fetch_row(result);
  }
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_set;
}

set< Area >& multiArea_query(MYSQL* mysql, string query, int lat, int lon, set< Area >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
	
  map< uint32, bool > area_cands;
  set< uint32 > area_definitives;
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]) && (row[3]) && (row[4]))
  {
    uint32 id(atoll(row[0]));
    int min_lon(atoi(row[2]));
    int max_lon(atoi(row[4]));
    if (max_lon > lon)
    {
      if (min_lon < lon)
      {
	int min_lat(atoi(row[1]));
	int max_lat(atoi(row[3]));
	if ((min_lat < lat) && (max_lat < lat))
	  area_cands[id] = !area_cands[id];
	else if ((min_lat < lat) || (max_lat < lat))
	{
	  int rel_lat(((long long)(max_lat - min_lat))*(lon - min_lon)/(max_lon - min_lon) + min_lat);
	  if (rel_lat < lat)
	    area_cands[id] = !area_cands[id];
	  else if (rel_lat == lat)
	    //We are on a border segment.
	    area_definitives.insert(id);
	}
	else if ((min_lat == lat) && (max_lat == lat))
	  //We are on a horizontal border segment.
	  area_definitives.insert(id);
      }
      else if (min_lon == lon)
	//We are north of a node of the border.
	//We can safely count such a segment if and only if the node is
	//on its western end.
      {
	int min_lat(atoi(row[1]));
	if (min_lat < lat)
	  area_cands[id] = !area_cands[id];
	else if (min_lat == lat)
	  //We have hit a node of the border.
	  area_definitives.insert(id);
      }
    }
    else if (max_lon == lon)
    {
      int max_lat(atoi(row[3]));
      if (max_lat == lat)
	//We have hit a node of the border.
	area_definitives.insert(id);
      else if (min_lon == max_lon)
	//We are on a vertical border segment.
      {
	int min_lat(atoi(row[1]));
	if ((min_lat <= lat) && (lat <= max_lat))
	  area_definitives.insert(id);
      }
    }
    row = mysql_fetch_row(result);
  }
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  for (set< uint32 >::const_iterator it(area_definitives.begin());
       it != area_definitives.end(); ++it)
    result_set.insert(Area(*it));
  for (map< uint32, bool >::const_iterator it(area_cands.begin());
       it != area_cands.end(); ++it)
  {
    if (it->second)
      result_set.insert(Area(it->first));
  }
  return result_set;
}

map< uint32, set< Line_Segment > >& singleArea_query
    (MYSQL* mysql, string query, map< uint32, set< Line_Segment > >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
  
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]) && (row[3]) && (row[4]))
  {
    result_set[(uint32)atoi(row[0])].insert(Line_Segment
	(atoi(row[1]), atoi(row[2]), atoi(row[3]), atoi(row[4])));
    row = mysql_fetch_row(result);
  }
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  
  return result_set;
}

set< uint32 >& multiint_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< uint32 >& source, set< uint32 >& result_set)
{
  for (set< uint32 >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

void multiint_to_null_query
    (MYSQL* mysql, string prefix, string suffix, const set< uint32 >& source)
{
  for (set< uint32 >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    mysql_query_null_wrapper(mysql, temp.str());
  }
  return;
}

set< Relation >& multiint_to_multiRelation_query
    (MYSQL* mysql, 
     string prefix1, string suffix1, string prefix2, string suffix2, string prefix3, string suffix3,
     const set< uint32 >& source, set< Relation >& result_set)
{
  map< int, set< pair< int, int > > > node_members;
  map< int, set< pair< int, int > > > way_members;
  map< int, set< pair< int, int > > > relation_members;
  
  for (set< uint32 >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix1;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix1;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > nodes;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    nodes.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    nodes.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      node_members[id] = nodes;
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  
  for (set< uint32 >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix2;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix2;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > ways;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    ways.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    ways.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      way_members[id] = ways;
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  
  for (set< uint32 >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix3;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix3;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > relations;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    relations.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    relations.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      relation_members[id] = relations;
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  
  for (set< uint32 >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    Relation relation(*it);
    relation.node_members = node_members[*it];
    relation.way_members = way_members[*it];
    relation.relation_members = relation_members[*it];
    result_set.insert(relation);
  }
  
  return result_set;
}

set< int >& multiNode_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Node >& source, set< int >& result_set)
{
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< int >& multiWay_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Way >& source, set< int >& result_set)
{
  for (set< Way >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< int >& multiRelation_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Relation_ >& source, set< int >& result_set)
{
  for (set< Relation_ >::const_iterator it(source.begin());
       it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->head;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->head;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

//-----------------------------------------------------------------------------

typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

set< Node >& multiint_to_multiNode_query(const set< uint32 >& source, set< Node >& result_set)
{
  Node_Id_Node_By_Id_Reader reader(source, result_set);
  select_by_id< Node_Id_Node_By_Id_Reader >(reader);
  return result_set;
}

void multiRange_to_multiNode_query
    (const set< pair< int, int > >& in_inside, const set< pair< int, int > >& in_border,
     set< Node >& res_inside, set< Node >& res_border)
{
  Node_Id_Node_Range_Reader reader(in_inside, in_border, res_inside, res_border);
  select_with_idx< Node_Id_Node_Range_Reader >(reader);
}

int multiRange_to_count_query
    (const set< pair< int, int > >& in_inside, const set< pair< int, int > >& in_border)
{
  Node_Id_Node_Range_Count reader(in_inside, in_border);
  count_with_idx< Node_Id_Node_Range_Count >(reader);
  return reader.get_result();
}

//-----------------------------------------------------------------------------

set< Way >& multiint_to_multiWay_query(const set< uint32 >& source, set< Way >& result_set)
{
  set< Way_ > result;
  Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< uint32 >, set< Way_ > >
      reader(source, result);
  select_by_id(reader);
  for (set< Way_ >::const_iterator it(result.begin()); it != result.end(); ++it)
  {
    Way way(it->head.second);
    way.members = ((*it).data);
    result_set.insert(way);
  }
  return result_set;
}

set< Way_ >& multiint_to_multiWay_query(const set< uint32 >& source, set< Way_ >& result)
{
  Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< uint32 >, set< Way_ > >
      reader(source, result);
  select_by_id(reader);
  return result;
}

set< Way >& multiNode_to_multiWay_query(const set< Node >& source, set< Way >& result_set)
{
  set< Way_Storage::Index > ll_idxs;
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    Way_Storage::Index ll_idx_(ll_idx(it->lat, it->lon));
    ll_idxs.insert(ll_idx_);
    ll_idxs.insert(0x88000000 | (ll_idx_>>8));
    ll_idxs.insert(0x88880000 | (ll_idx_>>16));
    ll_idxs.insert(0x88888800 | (ll_idx_>>24));
  }
  ll_idxs.insert(0x88888888);
  
  set< Way_ > result;
  Indexed_Ordered_Id_To_Many_Index_Reader< Way_Storage, set< Way_Storage::Index >, set< Way_ > >
      reader(ll_idxs, result);
  select_with_idx(reader);
  
  for (set< Way_ >::const_iterator it(result.begin()); it != result.end(); ++it)
  {
    bool is_referred(false);
    for (vector< Way_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
    {
      if (source.find(Node(*it2, 0, 0)) != source.end())
      {
	is_referred = true;
	break;
      }
    }
    if (!is_referred)
      continue;
    Way way(it->head.second);
    way.members = ((*it).data);
    result_set.insert(way);
  }
  return result_set;
}

//-----------------------------------------------------------------------------

set< Relation_ >& multiint_to_multiRelation_query
    (const set< uint32 >& source, set< Relation_ >& result_set)
{
  Indexed_Ordered_Id_To_Many_By_Id_Reader< Relation_Storage, set< uint32 >, set< Relation_ > >
      reader(source, result_set);
  select_by_id(reader);
  return result_set;
}

set< Relation >& multiint_to_multiRelation_query
    (const set< uint32 >& source, set< Relation >& result_set)
{
  set< Relation_ > result;
  Indexed_Ordered_Id_To_Many_By_Id_Reader< Relation_Storage, set< uint32 >, set< Relation_ > >
      reader(source, result);
  select_by_id(reader);
  
  for (set< Relation_ >::const_iterator it(result.begin()); it != result.end(); ++it)
  {
    Relation relation(it->head);
    for (vector< Relation_::Data >::const_iterator it2(it->data.begin());
	 it2 != it->data.end(); ++it2)
    {
      if (it2->type == Relation_Member::NODE)
	relation.node_members.insert(make_pair< int, int >(it2->id, it2->role+1));
      else if (it2->type == Relation_Member::WAY)
	relation.way_members.insert(make_pair< int, int >(it2->id, it2->role+1));
      else if (it2->type == Relation_Member::RELATION)
	relation.relation_members.insert(make_pair< int, int >(it2->id, it2->role+1));
    }
    result_set.insert(relation);
  }
  
  return result_set;
}

set< Relation_ >& multiNode_to_multiRelation_query(const set< Node >& source, set< Relation_ >& result_set)
{
  set< Relation_ > all_relations;
  Relation_Relation_Dump reader(all_relations);
  select_all(reader);
  
  for (set< Relation_ >::const_iterator it(all_relations.begin()); it != all_relations.end(); ++it)
  {
    bool is_referred(false);
    for (vector< Relation_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
    {
      if ((it2->type == Relation_Member::NODE) && (source.find(Node(it2->id, 0, 0)) != source.end()))
      {
	is_referred = true;
	break;
      }
    }
    if (is_referred)
      result_set.insert(*it);
  }
  return result_set;
}

set< Relation_ >& multiWay_to_multiRelation_query(const set< Way >& source, set< Relation_ >& result_set)
{
  set< Relation_ > all_relations;
  Relation_Relation_Dump reader(all_relations);
  select_all(reader);
  
  for (set< Relation_ >::const_iterator it(all_relations.begin()); it != all_relations.end(); ++it)
  {
    bool is_referred(false);
    for (vector< Relation_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
    {
      if ((it2->type == Relation_Member::WAY) && (source.find(Way(it2->id)) != source.end()))
      {
	is_referred = true;
	break;
      }
    }
    if (is_referred)
      result_set.insert(*it);
  }
  return result_set;
}

set< Relation_ >& multiRelation_backwards_query
    (const set< Relation_ >& source, set< Relation_ >& result_set)
{
  set< Relation_ > all_relations;
  Relation_Relation_Dump reader(all_relations);
  select_all(reader);
  
  for (set< Relation_ >::const_iterator it(all_relations.begin()); it != all_relations.end(); ++it)
  {
    bool is_referred(false);
    for (vector< Relation_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
    {
      if ((it2->type == Relation_Member::RELATION) && (source.find(Relation_(it2->id)) != source.end()))
      {
	is_referred = true;
	break;
      }
    }
    if (is_referred)
      result_set.insert(*it);
  }
  return result_set;
}

//-----------------------------------------------------------------------------

set< uint32 >& node_kv_to_multiint_query(string key, string value, set< uint32 >& result_set)
{
  set< uint32 > string_ids_global;
  set< uint32 > string_ids_local;
  set< uint32 > string_idxs_local;
  select_node_kv_to_ids(key, value, string_ids_global, string_ids_local, string_idxs_local);
  
  Tag_Id_Node_Local_Reader local_reader(string_ids_local, string_idxs_local, result_set);
  select_with_idx< Tag_Id_Node_Local_Reader >(local_reader);
  Tag_Id_Node_Global_Reader global_reader(string_ids_global, result_set);
  select_with_idx< Tag_Id_Node_Global_Reader >(global_reader);
  
  return result_set;
}

uint32 node_kv_to_count_query(string key, string value)
{
  set< uint32 > string_ids;
  set< uint32 > string_idxs_local;
  select_node_kv_to_ids(key, value, string_ids, string_ids, string_idxs_local);
  
  int source_fd = open64((DATADIR + db_subdir + NODE_TAG_ID_STATS).c_str(),
                         O_RDONLY);
  if (source_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '
        <<DATADIR + db_subdir + NODE_TAG_ID_STATS<<" kv_to_count_query:1";
    runtime_error(temp.str(), cout);
    return 0;
  }
  
  uint32 result(0);
  for (set< uint32 >::const_iterator it(string_ids.begin()); it != string_ids.end(); ++it)
  {
    uint32 summand(0);
    lseek64(source_fd, (*it - 1)*sizeof(uint32), SEEK_SET);
    read (source_fd, &summand, sizeof(uint32));
    result += summand;
  }
  
  close(source_fd);
  
  return result;
}

set< uint32 >& node_kv_multiint_to_multiint_query
    (string key, string value, const set< uint32 >& source, set< uint32 >& result_set)
{
  set< uint32 > string_ids_global;
  set< uint32 > string_ids_local;
  set< uint32 > string_idxs_local;
  select_node_kv_to_ids(key, value, string_ids_global, string_ids_local, string_idxs_local);
  
  Tag_Id_Node_Local_Multiint_Reader local_reader
      (string_ids_local, string_idxs_local, source, result_set);
  select_with_idx< Tag_Id_Node_Local_Multiint_Reader >(local_reader);
  Tag_Id_Node_Global_Multiint_Reader global_reader
      (string_ids_global, source, result_set);
  select_with_idx< Tag_Id_Node_Global_Multiint_Reader >(global_reader);
  
  return result_set;
}

set< Node >& kvs_multiNode_to_multiNode_query
    (vector< pair< string, string > >::const_iterator kvs_begin,
     vector< pair< string, string > >::const_iterator kvs_end,
     const set< Node >& source, set< Node >& result_set)
{
  vector< set< uint32 > > string_ids_global;
  vector< set< uint32 > > string_ids_local;
  set< uint32 > string_idxs_local;
  for (vector< pair< string, string > >::const_iterator it(kvs_begin);
       it != kvs_end; ++it)
  {
    string_ids_global.push_back(set< uint32 >());
    string_ids_local.push_back(set< uint32 >());
    select_node_kv_to_ids
	(it->first, it->second, string_ids_global.back(),
	 string_ids_local.back(), string_idxs_local);
  }
  
  set< uint32 > node_idxs;
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); ++it)
    node_idxs.insert(ll_idx(it->lat, it->lon) & 0xffffff00);
  set< uint32 > filtered_idxs_local;
  set_intersection
      (string_idxs_local.begin(), string_idxs_local.end(), node_idxs.begin(), node_idxs.end(),
       inserter(filtered_idxs_local, filtered_idxs_local.begin()));
  string_idxs_local.clear();
  
  map< uint32, set< uint32 > > local_node_to_id;
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); ++it)
    local_node_to_id[it->id] = set< uint32 >();
  
  map< uint32, set< uint32 > > global_node_to_id;
  set< uint32 > global_node_idxs;
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    global_node_to_id[it->id] = set< uint32 >();
    global_node_idxs.insert(ll_idx(it->lat, it->lon));
  }
  
  Tag_MultiNode_Id_Local_Reader local_reader(local_node_to_id, filtered_idxs_local);
  select_with_idx< Tag_MultiNode_Id_Local_Reader >(local_reader);
  Tag_Node_Id_Reader global_reader(global_node_to_id, global_node_idxs);
  select_with_idx< Tag_Node_Id_Reader >(global_reader);
  
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    const set< uint32 >& local_ids(local_node_to_id[it->id]);
    const set< uint32 >& global_ids(global_node_to_id[it->id]);
    bool can_be_result(true);
    for (uint32 i(0); i < string_ids_local.size(); ++i)
    {
      set< uint32 > ids_local;
      set_intersection
	  (string_ids_local[i].begin(), string_ids_local[i].end(),
	   local_ids.begin(), local_ids.end(), inserter(ids_local, ids_local.begin()));
      set< uint32 > ids_global;
      set_intersection
	  (string_ids_global[i].begin(), string_ids_global[i].end(),
	   global_ids.begin(), global_ids.end(), inserter(ids_global, ids_global.begin()));
      can_be_result &= (!((ids_local.empty()) && (ids_global.empty())));
    }
    if (can_be_result)
      result_set.insert(*it);
  }
  
  return result_set;
}

vector< vector< pair< string, string > > >& multiNode_to_kvs_query
    (const set< Node >& source, set< Node >::const_iterator& pos,
     vector< vector< pair< string, string > > >& result)
{
  set< Node >::const_iterator endpos(pos);
  uint32 i(0);
  while ((endpos != source.end()) && (i < 64*1024))
  {
    ++endpos;
    ++i;
  }
  
  set< uint32 > node_idxs;
  for (set< Node >::const_iterator it(pos); it != endpos; ++it)
    node_idxs.insert(ll_idx(it->lat, it->lon) & 0xffffff00);
  
  map< uint32, set< uint32 > > local_node_to_id;
  for (set< Node >::const_iterator it(pos); it != endpos; ++it)
    local_node_to_id[it->id] = set< uint32 >();
  
  map< uint32, set< uint32 > > global_node_to_id;
  set< uint32 > global_node_idxs;
  for (set< Node >::const_iterator it(pos); it != endpos; ++it)
  {
    global_node_to_id[it->id] = set< uint32 >();
    global_node_idxs.insert(ll_idx(it->lat, it->lon));
  }
  
  Tag_MultiNode_Id_Local_Reader local_reader(local_node_to_id, node_idxs);
  select_with_idx< Tag_MultiNode_Id_Local_Reader >(local_reader);
  Tag_Node_Id_Reader global_reader(global_node_to_id, global_node_idxs);
  select_with_idx< Tag_Node_Id_Reader >(global_reader);
  
  set< uint32 > ids_global;
  for (map< uint32, set< uint32 > >::const_iterator it(global_node_to_id.begin());
       it != global_node_to_id.end(); ++it)
  {
    for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      ids_global.insert(*it2);
  }
  map< uint32, uint32 > ids_local;
  for (map< uint32, set< uint32 > >::const_iterator it(local_node_to_id.begin());
       it != local_node_to_id.end(); ++it)
  {
    const Node& cur_nd(*(source.find(Node(it->first, 0, 0))));
    uint32 ll_idx_(ll_idx(cur_nd.lat, cur_nd.lon) & 0xffffff00);
    for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      ids_local[*it2] = ll_idx_;
  }
  map< uint32, pair< string, string > > kvs;
  
  select_node_ids_to_kvs(ids_local, ids_global, kvs);
  
  result.clear();
  result.resize(source.size());
  vector< vector< pair< string, string > > >::iterator rit(result.begin());
  for (set< Node >::const_iterator it(pos); it != endpos; ++it)
  {
    for (set< uint32 >::const_iterator it2(local_node_to_id[it->id].begin());
         it2 != local_node_to_id[it->id].end(); ++it2)
      rit->push_back(kvs[*it2]);
    for (set< uint32 >::const_iterator it2(global_node_to_id[it->id].begin());
         it2 != global_node_to_id[it->id].end(); ++it2)
      rit->push_back(kvs[*it2]);
    ++rit;
  }
  
  pos = endpos;
  return result;
}

//-----------------------------------------------------------------------------

set< Way_::Id >& way_kv_to_multiint_query(string key, string value, set< Way_::Id >& result_set)
{
  set< Way_::Id > string_ids_global;
  set< Way_::Id > string_ids_local;
  set< Way_::Id > string_idxs_local;
  select_way_kv_to_ids(key, value, string_ids_global, string_ids_local, string_idxs_local);
  
  Tag_Id_Way_Local_Reader local_reader(string_ids_local, string_idxs_local, result_set);
  select_with_idx< Tag_Id_Way_Local_Reader >(local_reader);
  Tag_Id_Way_Global_Reader global_reader(string_ids_global, result_set);
  select_with_idx< Tag_Id_Way_Global_Reader >(global_reader);
  
  return result_set;
}

uint32 way_kv_to_count_query(string key, string value)
{
  set< uint32 > string_ids;
  set< uint32 > string_idxs_local;
  select_way_kv_to_ids(key, value, string_ids, string_ids, string_idxs_local);
  
  int source_fd = open64((DATADIR + db_subdir + WAY_TAG_ID_STATS).c_str(),
                         O_RDONLY);
  if (source_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '
      <<DATADIR + db_subdir + WAY_TAG_ID_STATS<<" way_kv_to_count_query:1";
    runtime_error(temp.str(), cout);
    return 0;
  }
  
  uint32 result(0);
  for (set< uint32 >::const_iterator it(string_ids.begin()); it != string_ids.end(); ++it)
  {
    uint32 summand(0);
    lseek64(source_fd, (*it - 1)*sizeof(uint32), SEEK_SET);
    read(source_fd, &summand, sizeof(uint32));
    result += summand;
  }
  
  close(source_fd);
  
  return result;
}

vector< vector< pair< string, string > > >& multiWay_to_kvs_query
    (const set< Way >& source, set< Way >::const_iterator& pos,
     vector< vector< pair< string, string > > >& result)
{
  set< Way >::const_iterator endpos(pos);
  uint32 i(0);
  while ((endpos != source.end()) && (i < 64*1024))
  {
    ++endpos;
    ++i;
  }
  
  set< Way_::Index > source_ids;
  for (set< Way >::const_iterator it(pos); it != endpos; ++it)
    source_ids.insert(it->id);
  set< Way_ > way_coords_set;
  Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< Way_::Index >, set< Way_ > >
      way_coord_reader(source_ids, way_coords_set);
  select_by_id(way_coord_reader);
  vector< Way_ > way_coords;
  way_coords.reserve(way_coords_set.size());
  for (set< Way_ >::const_iterator it(way_coords_set.begin()); it != way_coords_set.end(); ++it)
    way_coords.push_back(*it);
  
  set< uint32 > way_idxs;
  for (vector< Way_ >::const_iterator it(way_coords.begin()); it != way_coords.end(); ++it)
    way_idxs.insert(it->head.first & 0xffffff00);
  
  map< uint32, set< uint32 > > local_way_to_id;
  for (set< Way >::const_iterator it(pos); it != endpos; ++it)
    local_way_to_id[it->id] = set< uint32 >();
  
  map< uint32, set< uint32 > > global_way_to_id;
  set< uint32 > global_way_idxs;
  for (vector< Way_ >::const_iterator it(way_coords.begin()); it != way_coords.end(); ++it)
  {
    global_way_to_id[it->head.second] = set< uint32 >();
    global_way_idxs.insert(it->head.first);
  }
  
  Tag_MultiWay_Id_Local_Reader local_reader(local_way_to_id, way_idxs);
  select_with_idx< Tag_MultiWay_Id_Local_Reader >(local_reader);
  Tag_Way_Id_Reader global_reader(global_way_to_id, global_way_idxs);
  select_with_idx< Tag_Way_Id_Reader >(global_reader);
  
  set< uint32 > ids_global;
  for (map< uint32, set< uint32 > >::const_iterator it(global_way_to_id.begin());
       it != global_way_to_id.end(); ++it)
  {
    for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      ids_global.insert(*it2);
  }
  sort(way_coords.begin(), way_coords.end(), Way_Less_By_Id());
  map< uint32, uint32 > ids_local;
  for (map< uint32, set< uint32 > >::const_iterator it(local_way_to_id.begin());
       it != local_way_to_id.end(); ++it)
  {
    const Way_& cur_wy(*(lower_bound
	(way_coords.begin(), way_coords.end(), Way_(0, it->first), Way_Less_By_Id())));
    uint32 ll_idx_(cur_wy.head.first & 0xffffff00);
    for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      ids_local[*it2] = ll_idx_;
  }
  map< uint32, pair< string, string > > kvs;
  
  select_way_ids_to_kvs(ids_local, ids_global, kvs);
  
  result.clear();
  result.resize(source.size());
  vector< vector< pair< string, string > > >::iterator rit(result.begin());
  for (set< Way >::const_iterator it(pos); it != endpos; ++it)
  {
    for (set< uint32 >::const_iterator it2(local_way_to_id[it->id].begin());
         it2 != local_way_to_id[it->id].end(); ++it2)
      rit->push_back(kvs[*it2]);
    for (set< uint32 >::const_iterator it2(global_way_to_id[it->id].begin());
         it2 != global_way_to_id[it->id].end(); ++it2)
      rit->push_back(kvs[*it2]);
    ++rit;
  }
  
  pos = endpos;
  return result;
}

set< Way_ >& kvs_multiWay_to_multiWay_query
    (vector< pair< string, string > >::const_iterator kvs_begin,
     vector< pair< string, string > >::const_iterator kvs_end,
     const set< Way_ >& source, set< Way_ >& result_set)
{
  vector< set< uint32 > > string_ids_global;
  vector< set< uint32 > > string_ids_local;
  set< uint32 > string_idxs_local;
  for (vector< pair< string, string > >::const_iterator it(kvs_begin);
       it != kvs_end; ++it)
  {
    string_ids_global.push_back(set< uint32 >());
    string_ids_local.push_back(set< uint32 >());
    select_way_kv_to_ids
	(it->first, it->second, string_ids_global.back(),
	 string_ids_local.back(), string_idxs_local);
  }
  
  set< uint32 > way_idxs;
  for (set< Way_ >::const_iterator it(source.begin()); it != source.end(); ++it)
    way_idxs.insert(it->head.first & 0xffffff00);
  set< uint32 > filtered_idxs_local;
  set_intersection
      (string_idxs_local.begin(), string_idxs_local.end(), way_idxs.begin(), way_idxs.end(),
       inserter(filtered_idxs_local, filtered_idxs_local.begin()));
  string_idxs_local.clear();
  
  map< uint32, set< uint32 > > local_way_to_id;
  for (set< Way_ >::const_iterator it(source.begin()); it != source.end(); ++it)
    local_way_to_id[it->head.second] = set< uint32 >();
  
  map< uint32, set< uint32 > > global_way_to_id;
  set< uint32 > global_way_idxs;
  for (set< Way_ >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    global_way_to_id[it->head.second] = set< uint32 >();
    global_way_idxs.insert(it->head.first);
  }
  
  Tag_MultiWay_Id_Local_Reader local_reader(local_way_to_id, filtered_idxs_local);
  select_with_idx< Tag_MultiWay_Id_Local_Reader >(local_reader);
  Tag_Way_Id_Reader global_reader(global_way_to_id, global_way_idxs);
  select_with_idx< Tag_Way_Id_Reader >(global_reader);
  
  for (set< Way_ >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    const set< uint32 >& local_ids(local_way_to_id[it->head.second]);
    const set< uint32 >& global_ids(global_way_to_id[it->head.second]);
    bool can_be_result(true);
    for (uint32 i(0); i < string_ids_local.size(); ++i)
    {
      bool local_match(false);
      for (set< uint32 >::const_iterator it2(local_ids.begin());
	   it2 != local_ids.end(); ++it2)
	local_match |= (string_ids_local[i].find(*it2) != string_ids_local[i].end());
      bool global_match(false);
      for (set< uint32 >::const_iterator it2(global_ids.begin());
	   it2 != global_ids.end(); ++it2)
	global_match |= (string_ids_global[i].find(*it2) != string_ids_global[i].end());
      can_be_result &= (local_match || global_match);
    }
    if (can_be_result)
      result_set.insert(*it);
  }
  
  return result_set;
}

//-----------------------------------------------------------------------------

set< Relation_::Id >& relation_kv_to_multiint_query(string key, string value, set< Relation_::Id >& result_set)
{
  set< Relation_::Id > string_ids_global;
  set< Relation_::Id > string_ids_local;
  set< Relation_::Id > string_idxs_local;
  select_relation_kv_to_ids(key, value, string_ids_global, string_ids_local, string_idxs_local);
  
  Tag_Id_Relation_Local_Reader local_reader(string_ids_local, string_idxs_local, result_set);
  select_with_idx< Tag_Id_Relation_Local_Reader >(local_reader);
  Tag_Id_Relation_Global_Reader global_reader(string_ids_global, result_set);
  select_with_idx< Tag_Id_Relation_Global_Reader >(global_reader);
  
  return result_set;
}

uint32 relation_kv_to_count_query(string key, string value)
{
  set< uint32 > string_ids;
  set< uint32 > string_idxs_local;
  select_relation_kv_to_ids(key, value, string_ids, string_ids, string_idxs_local);
  
  int source_fd = open64((DATADIR + db_subdir + RELATION_TAG_ID_STATS).c_str(),
                         O_RDONLY);
  if (source_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '
      <<DATADIR + db_subdir + RELATION_TAG_ID_STATS
      <<" relation_kv_to_count_query:1";
    runtime_error(temp.str(), cout);
    return 0;
  }
  
  uint32 result(0);
  for (set< uint32 >::const_iterator it(string_ids.begin()); it != string_ids.end(); ++it)
  {
    uint32 summand(0);
    lseek64(source_fd, (*it - 1)*sizeof(uint32), SEEK_SET);
    read(source_fd, &summand, sizeof(uint32));
    result += summand;
  }
  
  close(source_fd);
  
  return result;
}

vector< vector< pair< string, string > > >& multiRelation_to_kvs_query
    (const set< Relation_ >& source, set< Relation_ >::const_iterator& pos,
     vector< vector< pair< string, string > > >& result)
{
  set< Relation_ >::const_iterator endpos(pos);
  uint32 i(0);
  while ((endpos != source.end()) && (i < 64*1024))
  {
    ++endpos;
    ++i;
  }
  
  set< Relation_::Index > source_ids;
  for (set< Relation_ >::const_iterator it(pos); it != endpos; ++it)
    source_ids.insert(it->head);
  set< Relation_ > relation_coords_set;
  Indexed_Ordered_Id_To_Many_By_Id_Reader< Relation_Storage, set< Relation_::Index >, set< Relation_ > >
      relation_coord_reader(source_ids, relation_coords_set);
  select_by_id(relation_coord_reader);
  vector< Relation_ > relation_coords;
  relation_coords.reserve(relation_coords_set.size());
  for (set< Relation_ >::const_iterator it(relation_coords_set.begin());
       it != relation_coords_set.end(); ++it)
    relation_coords.push_back(*it);
  
  set< uint32 > relation_idxs;
  for (vector< Relation_ >::const_iterator it(relation_coords.begin()); it != relation_coords.end(); ++it)
    relation_idxs.insert(it->head & 0xffffff00);
  
  map< uint32, set< uint32 > > local_relation_to_id;
  for (set< Relation_ >::const_iterator it(pos); it != endpos; ++it)
    local_relation_to_id[it->head] = set< uint32 >();
  
  map< uint32, set< uint32 > > global_relation_to_id;
  set< uint32 > global_relation_idxs;
  for (vector< Relation_ >::const_iterator it(relation_coords.begin()); it != relation_coords.end(); ++it)
  {
    global_relation_to_id[it->head] = set< uint32 >();
    global_relation_idxs.insert(it->head);
  }
  
  Tag_MultiRelation_Id_Local_Reader local_reader(local_relation_to_id, relation_idxs);
  select_with_idx< Tag_MultiRelation_Id_Local_Reader >(local_reader);
  Tag_Relation_Id_Reader global_reader(global_relation_to_id, global_relation_idxs);
  select_with_idx< Tag_Relation_Id_Reader >(global_reader);
  
  set< uint32 > ids_global;
  for (map< uint32, set< uint32 > >::const_iterator it(global_relation_to_id.begin());
       it != global_relation_to_id.end(); ++it)
  {
    for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      ids_global.insert(*it2);
  }
  sort(relation_coords.begin(), relation_coords.end(), Relation_Less_By_Id());
  map< uint32, uint32 > ids_local;
  for (map< uint32, set< uint32 > >::const_iterator it(local_relation_to_id.begin());
       it != local_relation_to_id.end(); ++it)
  {
    const Relation_& cur_wy(*(lower_bound
	(relation_coords.begin(), relation_coords.end(), Relation_(it->first), Relation_Less_By_Id())));
    uint32 ll_idx_(cur_wy.head & 0xffffff00);
    for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      ids_local[*it2] = ll_idx_;
  }
  map< uint32, pair< string, string > > kvs;
  
  select_relation_ids_to_kvs(ids_local, ids_global, kvs);
  
  result.clear();
  result.resize(source.size());
  vector< vector< pair< string, string > > >::iterator rit(result.begin());
  for (set< Relation_ >::const_iterator it(pos); it != endpos; ++it)
  {
    for (set< uint32 >::const_iterator
         it2(local_relation_to_id[it->head].begin());
	 it2 != local_relation_to_id[it->head].end(); ++it2)
      rit->push_back(kvs[*it2]);
    for (set< uint32 >::const_iterator
         it2(global_relation_to_id[it->head].begin());
	 it2 != global_relation_to_id[it->head].end(); ++it2)
      rit->push_back(kvs[*it2]);
    ++rit;
  }
  
  pos = endpos;
  return result;
}

set< Relation_ >& kvs_multiRelation_to_multiRelation_query
    (vector< pair< string, string > >::const_iterator kvs_begin,
     vector< pair< string, string > >::const_iterator kvs_end,
     const set< Relation_ >& source, set< Relation_ >& result_set)
{
  vector< set< uint32 > > string_ids_global;
  vector< set< uint32 > > string_ids_local;
  set< uint32 > string_idxs_local;
  for (vector< pair< string, string > >::const_iterator it(kvs_begin);
       it != kvs_end; ++it)
  {
    string_ids_global.push_back(set< uint32 >());
    string_ids_local.push_back(set< uint32 >());
    select_relation_kv_to_ids
	(it->first, it->second, string_ids_global.back(),
	 string_ids_local.back(), string_idxs_local);
  }
  
  set< uint32 > relation_idxs;
  for (set< Relation_ >::const_iterator it(source.begin()); it != source.end(); ++it)
    relation_idxs.insert(it->head & 0xffffff00);
  set< uint32 > filtered_idxs_local;
  set_intersection
      (string_idxs_local.begin(), string_idxs_local.end(), relation_idxs.begin(), relation_idxs.end(),
       inserter(filtered_idxs_local, filtered_idxs_local.begin()));
  string_idxs_local.clear();
  
  map< uint32, set< uint32 > > local_relation_to_id;
  for (set< Relation_ >::const_iterator it(source.begin()); it != source.end(); ++it)
    local_relation_to_id[it->head] = set< uint32 >();
  
  map< uint32, set< uint32 > > global_relation_to_id;
  set< uint32 > global_relation_idxs;
  for (set< Relation_ >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    global_relation_to_id[it->head] = set< uint32 >();
    global_relation_idxs.insert(it->head);
  }
  
  Tag_MultiRelation_Id_Local_Reader local_reader(local_relation_to_id, filtered_idxs_local);
  select_with_idx< Tag_MultiRelation_Id_Local_Reader >(local_reader);
  Tag_Relation_Id_Reader global_reader(global_relation_to_id, global_relation_idxs);
  select_with_idx< Tag_Relation_Id_Reader >(global_reader);
  
  for (set< Relation_ >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    const set< uint32 >& local_ids(local_relation_to_id[it->head]);
    const set< uint32 >& global_ids(global_relation_to_id[it->head]);
    bool can_be_result(true);
    for (uint32 i(0); i < string_ids_local.size(); ++i)
    {
      bool local_match(false);
      for (set< uint32 >::const_iterator it2(local_ids.begin());
	   it2 != local_ids.end(); ++it2)
	local_match |= (string_ids_local[i].find(*it2) != string_ids_local[i].end());
      bool global_match(false);
      for (set< uint32 >::const_iterator it2(global_ids.begin());
	   it2 != global_ids.end(); ++it2)
	global_match |= (string_ids_global[i].find(*it2) != string_ids_global[i].end());
      can_be_result &= (local_match || global_match);
    }
    if (can_be_result)
      result_set.insert(*it);
  }
  
  return result_set;
}
