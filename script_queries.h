#ifndef SCRIPT_QUERIES
#define SCRIPT_QUERIES

#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "script_datatypes.h"
#include <mysql.h>

using namespace std;

MYSQL_RES* mysql_query_wrapper(MYSQL* mysql, string query);

int int_query(MYSQL* mysql, string query);
pair< int, int > intint_query(MYSQL* mysql, string query);

set< uint32 >& multiint_query(MYSQL* mysql, string query, set< uint32 >& result_set);

set< Area >& multiArea_query(MYSQL* mysql, string query, int lat, int lon, set< Area >& result_set);

set< uint32 >& multiint_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< uint32 >& source, set< uint32 >& result_set);
    
void multiint_to_null_query
    (MYSQL* mysql, string prefix, string suffix, const set< uint32 >& source);
    
set< Relation >& multiint_to_multiRelation_query
    (MYSQL* mysql,
     string prefix1, string suffix1, string prefix2, string suffix2, string prefix3, string suffix3,
     const set< uint32 >& source, set< Relation >& result_set);

set< int >& multiNode_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Node >& source, set< int >& result_set);

set< int >& multiWay_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Way >& source, set< int >& result_set);

set< int >& multiRelation_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Relation >& source, set< int >& result_set);

//-----------------------------------------------------------------------------

set< Node >& multiint_to_multiNode_query(const set< uint32 >& source, set< Node >& result_set);

// in_inside and in_border contain each a list of pairwise distinct intervals of ll_idxes.
// The nodes with ll_idx in in_inside will be returned in res_inside, the nodes with ll_idx
// in in_border will be returned in res_border
void multiRange_to_multiNode_query
    (const set< pair< int, int > >& in_inside, const set< pair< int, int > >& in_border,
     set< Node >& res_inside, set< Node >& res_border);
int multiRange_to_count_query
    (const set< pair< int, int > >& in_inside, const set< pair< int, int > >& in_border);

set< Way >& multiint_to_multiWay_query(const set< uint32 >& source, set< Way >& result_set);
set< Way_ >& multiint_to_multiWay_query(const set< uint32 >& source, set< Way_ >& result_set);
set< Way >& multiNode_to_multiWay_query(const set< Node >& source, set< Way >& result_set);

set< Relation_ >& multiint_to_multiRelation_query
    (const set< uint32 >& source, set< Relation_ >& result_set);
set< Relation >& multiint_to_multiRelation_query
    (const set< uint32 >& source, set< Relation >& result_set);

set< uint32 >& node_kv_to_multiint_query(string key, string value, set< uint32 >& result_set);
uint32 node_kv_to_count_query(string key, string value);
set< uint32 >& node_kv_multiint_to_multiint_query
    (string key, string value, const set< uint32 >& source, set< uint32 >& result_set);
set< Node >& kvs_multiNode_to_multiNode_query
    (vector< pair< string, string > >::const_iterator kvs_begin,
     vector< pair< string, string > >::const_iterator kvs_end,
     const set< Node >& source, set< Node >& result_set);
vector< vector< pair< string, string > > >& multiNode_to_kvs_query
    (const set< Node >& source, set< Node >::const_iterator& pos,
     vector< vector< pair< string, string > > >& result);

set< Way_::Id >& way_kv_to_multiint_query(string key, string value, set< Way_::Id >& result_set);
uint32 way_kv_to_count_query(string key, string value);
set< Way_ >& kvs_multiWay_to_multiWay_query
    (vector< pair< string, string > >::const_iterator kvs_begin,
     vector< pair< string, string > >::const_iterator kvs_end,
     const set< Way_ >& source, set< Way_ >& result_set);
vector< vector< pair< string, string > > >& multiWay_to_kvs_query
    (const set< Way >& source, set< Way >::const_iterator& pos,
     vector< vector< pair< string, string > > >& result);

set< Relation_::Id >& relation_kv_to_multiint_query
    (string key, string value, set< Relation_::Id >& result_set);
uint32 relation_kv_to_count_query(string key, string value);
set< Relation_ >& kvs_multiRelation_to_multiRelation_query
    (vector< pair< string, string > >::const_iterator kvs_begin,
     vector< pair< string, string > >::const_iterator kvs_end,
     const set< Relation_ >& source, set< Relation_ >& result_set);
vector< vector< pair< string, string > > >& multiRelation_to_kvs_query
    (const set< Relation >& source, set< Relation >::const_iterator& pos,
     vector< vector< pair< string, string > > >& result);

#endif
