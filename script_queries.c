#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "file_types.h"
#include "node_strings_file_io.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "user_interface.h"
#include "vigilance_control.h"

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

set< int >& multiint_query(MYSQL* mysql, string query, set< int >& result_set)
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

set< Node >& multiNode_query(MYSQL* mysql, string query, set< Node >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]))
  {
    result_set.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
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
	
  map< int, bool > area_cands;
  set< int > area_definitives;
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]) && (row[3]) && (row[4]))
  {
    int id(atoi(row[0]));
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
  for (set< int >::const_iterator it(area_definitives.begin());
       it != area_definitives.end(); ++it)
    result_set.insert(Area(*it));
  for (map< int, bool >::const_iterator it(area_cands.begin());
       it != area_cands.end(); ++it)
  {
    if (it->second)
      result_set.insert(Area(it->first));
  }
  return result_set;
}

set< int >& multiint_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< int >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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

set< Node >& multiint_to_multiNode_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< Node >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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
    while ((row) && (row[0]) && (row[1]) && (row[2]))
    {
      result_set.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< Way >& multiint_to_multiWay_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< Way >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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
      Way way(atoi(row[0]));
      way.members.reserve(10);
      while ((row) && (row[0]) && (way.id == atoi(row[0])))
      {
	if ((row[1]) && (row[2]))
	{
	  unsigned int count((unsigned int)atol(row[1]));
	  if (way.members.capacity() < count)
	    way.members.reserve(count+10);
	  if (way.members.size() < count)
	    way.members.resize(count);
	  way.members[count-1] = atoi(row[2]);
	}
	row = mysql_fetch_row(result);
      }
      result_set.insert(way);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< Relation >& multiint_to_multiRelation_query
    (MYSQL* mysql, 
     string prefix1, string suffix1, string prefix2, string suffix2, string prefix3, string suffix3,
     const set< int >& source, set< Relation >& result_set)
{
  map< int, set< pair< int, int > > > node_members;
  map< int, set< pair< int, int > > > way_members;
  map< int, set< pair< int, int > > > relation_members;
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
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
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); ++it)
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
    (MYSQL* mysql, string prefix, string suffix, const set< Relation >& source, set< int >& result_set)
{
  for (set< Relation >::const_iterator it(source.begin()); it != source.end(); )
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

//-----------------------------------------------------------------------------

typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

set< Node >& multiint_to_multiNode_query(const set< int >& source, set< Node >& result_set)
{
  Node_Id_Node_By_Id_Reader reader(source, result_set);
  select_by_id< Node_Id_Node_By_Id_Reader >(reader);
  
  return result_set;
}

void multiRange_to_multiNode_query
    (const set< pair< int, int > >& in_inside, const set< pair< int, int > >& in_border,
     set< Node >& res_inside, set< Node >& res_border)
{
/*  vector< pair< int32, uint32 > > block_index;

  int nodes_idx_fd = open64(NODE_IDX, O_RDONLY);
  if (nodes_idx_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<NODE_IDX<<" multiRange_to_multiNode_query:1";
    runtime_error(temp.str(), cout);
    return;
  }
  
  int32* buf = (int32*) malloc(sizeof(int32)*2);
  while (read(nodes_idx_fd, buf, sizeof(int)*2))
    block_index.push_back(make_pair< int32, uint32 >(buf[0], buf[1]));
  close(nodes_idx_fd);
  
  int nodes_dat_fd = open64(NODE_DATA, O_RDONLY);
  if (nodes_dat_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<NODE_IDX<<" multiRange_to_multiNode_query:2";
    runtime_error(temp.str(), cout);
    free(buf);
    return;
  }
  
  int32* buf_count = (int32*) malloc(sizeof(int) + BLOCKSIZE*sizeof(Node));
  Node* nd_buf = (Node*) &buf_count[1];
  set< pair< int, int > >::const_iterator it_inside(in_inside.begin());
  set< pair< int, int > >::const_iterator it_border(in_border.begin());
  for (unsigned int i(1); i < block_index.size(); ++i)
  {
    bool block_inside((it_inside != in_inside.end()) &&
	(it_inside->first <= block_index[i].first));
    bool block_border((it_border != in_border.end()) &&
	(it_border->first <= block_index[i].first));
    if (block_inside || block_border)
    {
      lseek64(nodes_dat_fd,
	      (int64)(block_index[i-1].second)*(sizeof(int) + BLOCKSIZE*sizeof(Node)), SEEK_SET);
      read(nodes_dat_fd, buf_count, sizeof(int) + BLOCKSIZE*sizeof(Node));
    }
    if (block_inside)
    {
      for (int32 j(0); j < buf_count[0]; ++j)
      {
	int32 nd_idx(ll_idx(nd_buf[j].lat, nd_buf[j].lon));
	if (nd_idx >= it_inside->first)
	{
	  if (nd_idx <= it_inside->second)
	    res_inside.insert(nd_buf[j]);
	  else
	  {
	    while ((it_inside != in_inside.end()) && (it_inside->second < nd_idx))
	      ++it_inside;
	    if (it_inside == in_inside.end())
	      break;
	    if (nd_idx <= it_inside->second)
	      res_inside.insert(nd_buf[j]);
	  }
	}
      }
    }
    if (block_border)
    {
      for (int32 j(0); j < buf_count[0]; ++j)
      {
	int32 nd_idx(ll_idx(nd_buf[j].lat, nd_buf[j].lon));
	if (nd_idx >= it_border->first)
	{
	  if (nd_idx <= it_border->second)
	    res_border.insert(nd_buf[j]);
	  else
	  {
	    while ((it_border != in_border.end()) && (it_border->second < nd_idx))
	      ++it_border;
	    if (it_border == in_border.end())
	      break;
	    if (nd_idx <= it_border->second)
	      res_border.insert(nd_buf[j]);
	  }
	}
      }
    }
    while ((it_inside != in_inside.end()) && (it_inside->second < block_index[i].first))
      ++it_inside;
    while ((it_border != in_border.end()) && (it_border->second < block_index[i].first))
      ++it_border;
  }
  
  close(nodes_dat_fd);
  
  free(buf);
  free(buf_count);*/
  
  Node_Id_Node_Range_Reader reader(in_inside, in_border, res_inside, res_border);
  select_with_idx< Node_Id_Node_Range_Reader >(reader);
/*  cout<<'['<<res_border.size()<<"]\n";*/
/*  cout<<'['<<(res_border.find(Node(36478259, 0, 0)) != res_border.end())<<"]\n";*/
}

int multiRange_to_count_query
    (const set< pair< int, int > >& in_inside, const set< pair< int, int > >& in_border)
{
  Node_Id_Node_Range_Count reader(in_inside, in_border);
  count_with_idx< Node_Id_Node_Range_Count >(reader);
  
  return reader.get_result();
}

//-----------------------------------------------------------------------------

set< Way >& multiint_to_multiWay_query(const set< int >& source, set< Way >& result_set)
{
  int ways_dat_fd = open64(WAY_IDXA, O_RDONLY);
  if (ways_dat_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<WAY_IDXA<<" multiint_to_multiWay_query:1";
    runtime_error(temp.str(), cout);
    return result_set;
  }
  
  set< int > blocks;
  int16 idx_buf(0);
  for (set< int >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    lseek64(ways_dat_fd, ((*it)-1)*sizeof(int16), SEEK_SET);
    read(ways_dat_fd, &idx_buf, sizeof(int16));
    blocks.insert(idx_buf);
  }
  
  close(ways_dat_fd);
  
  uint32* way_buf = (uint32*) malloc(sizeof(uint32) + WAY_BLOCKSIZE*sizeof(uint32));
  if (!way_buf)
  {
    runtime_error("Bad alloc in way query", cout);
    return result_set;
  }

  ways_dat_fd = open64(WAY_DATA, O_RDONLY);
  if (ways_dat_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<WAY_DATA<<" multiint_to_multiWay_query:2";
    runtime_error(temp.str(), cout);
    return result_set;
  }
  
  for (set< int >::const_iterator it(blocks.begin()); it != blocks.end(); ++it)
  {
    lseek64(ways_dat_fd, (int64)(*it)*(sizeof(uint32) + WAY_BLOCKSIZE*sizeof(uint32)), SEEK_SET);
    read(ways_dat_fd, way_buf, sizeof(uint32) + WAY_BLOCKSIZE*sizeof(uint32));
    for (uint32 i(1); i < way_buf[0]; i += way_buf[i+2]+3)
    {
      if (source.find(way_buf[i]) != source.end())
      {
	Way way(way_buf[i]);
	for (uint32 j(0); j < way_buf[i+2]; ++j)
	  way.members.push_back(way_buf[j+i+3]);
	result_set.insert(way);
      }
    }
  }

  close(ways_dat_fd);
  
  free(way_buf);
  
  return result_set;
}

set< Way >& multiNode_to_multiWay_query(const set< Node >& source, set< Way >& result_set)
{
  int ways_dat_fd = open64(WAY_IDXSPAT, O_RDONLY);
  if (ways_dat_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<WAY_IDXSPAT<<" multiNode_to_multiWay_query:1";
    runtime_error(temp.str(), cout);
    return result_set;
  }
  uint32 spat_idx_buf_size(lseek64(ways_dat_fd, 0, SEEK_END)/sizeof(pair< int32, uint16 >));
  pair< int32, uint16 >* spat_idx_buf = (pair< int32, uint16 >*)
      malloc(spat_idx_buf_size * sizeof(pair< int32, uint16 >));
  if (!spat_idx_buf)
  {
    runtime_error("Bad alloc in way query", cout);
    return result_set;
  }
  lseek64(ways_dat_fd, 0, SEEK_SET);
  read(ways_dat_fd, spat_idx_buf, spat_idx_buf_size * sizeof(pair< int32, uint16 >));
  close(ways_dat_fd);
  
  set< int > ll_idxs;
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); ++it)
    ll_idxs.insert(ll_idx(it->lat, it->lon) & WAY_IDX_BITMASK);
  
  set< int > blocks;
  for (uint32 i(0); i < spat_idx_buf_size; ++i)
  {
    if (ll_idxs.find((spat_idx_buf[i].first)) != ll_idxs.end())
      blocks.insert(spat_idx_buf[i].second);
  }
  
  free(spat_idx_buf);
  
  uint32* way_buf = (uint32*) malloc(sizeof(uint32) + WAY_BLOCKSIZE*sizeof(uint32));
  if (!way_buf)
  {
    runtime_error("Bad alloc in way query", cout);
    return result_set;
  }

  ways_dat_fd = open64(WAY_DATA, O_RDONLY);
  if (ways_dat_fd < 0)
  {
    free(way_buf);
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<WAY_DATA<<" multiNode_to_multiWay_query:2";
    runtime_error(temp.str(), cout);
    return result_set;
  }
  
  for (set< int >::const_iterator it(blocks.begin()); it != blocks.end(); ++it)
  {
    lseek64(ways_dat_fd, (int64)(*it)*(sizeof(uint32) + WAY_BLOCKSIZE*sizeof(uint32)), SEEK_SET);
    read(ways_dat_fd, way_buf, sizeof(uint32) + WAY_BLOCKSIZE*sizeof(uint32));
    for (uint32 i(1); i < way_buf[0]; i += way_buf[i+2]+3)
    {
      for (uint32 j(0); j < way_buf[i+2]; ++j)
      {
	if (source.find(Node(way_buf[j+i+3], 200*10*1000*1000, 0)) != source.end())
	{
	  Way way(way_buf[i]);
	  for (j = 0; j < way_buf[i+2]; ++j)
	    way.members.push_back(way_buf[j+i+3]);
	  result_set.insert(way);
	}
      }
    }
  }

  close(ways_dat_fd);
  
  free(way_buf);
  
  return result_set;
}

//-----------------------------------------------------------------------------

set< int >& kv_to_multiint_query(string key, string value, set< int >& result_set)
{
  set< uint32 > string_ids_global;
  set< uint32 > string_ids_local;
  set< uint32 > string_idxs_local;
  select_kv_to_ids(key, value, string_ids_global, string_ids_local, string_idxs_local);
  
  Tag_Id_Node_Local_Reader local_reader(string_ids_local, string_idxs_local, result_set);
  select_with_idx< Tag_Id_Node_Local_Reader >(local_reader);
  Tag_Id_Node_Global_Reader global_reader(string_ids_global, result_set);
  select_with_idx< Tag_Id_Node_Global_Reader >(global_reader);
  
  return result_set;
}

uint32 kv_to_count_query(string key, string value)
{
  set< uint32 > string_ids;
  set< uint32 > string_idxs_local;
  select_kv_to_ids(key, value, string_ids, string_ids, string_idxs_local);
  
  int source_fd = open64(NODE_TAG_ID_STATS.c_str(), O_RDONLY);
  if (source_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno<<' '<<NODE_TAG_ID_STATS<<" kv_to_count_query:1";
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

set< int >& kv_multiint_to_multiint_query
    (string key, string value, const set< int >& source, set< int >& result_set)
{
  set< uint32 > string_ids_global;
  set< uint32 > string_ids_local;
  set< uint32 > string_idxs_local;
  select_kv_to_ids(key, value, string_ids_global, string_ids_local, string_idxs_local);
  
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
    select_kv_to_ids
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
  
  select_ids_to_kvs(ids_local, ids_global, kvs);
  
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
