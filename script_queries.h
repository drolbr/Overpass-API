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

set< int >& multiint_query(MYSQL* mysql, string query, set< int >& result_set);

set< Node >& multiNode_query(MYSQL* mysql, string query, set< Node >& result_set);

set< Area >& multiArea_query(MYSQL* mysql, string query, int lat, int lon, set< Area >& result_set);

set< int >& multiint_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< int >& result_set);
    
void multiint_to_null_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source);
    
set< Node >& multiint_to_multiNode_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< Node >& result_set);

set< Way >& multiint_to_multiWay_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< Way >& result_set);
    
set< Relation >& multiint_to_multiRelation_query
    (MYSQL* mysql,
     string prefix1, string suffix1, string prefix2, string suffix2, string prefix3, string suffix3,
     const set< int >& source, set< Relation >& result_set);

set< int >& multiNode_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Node >& source, set< int >& result_set);

set< int >& multiWay_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Way >& source, set< int >& result_set);

set< int >& multiRelation_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Relation >& source, set< int >& result_set);

//-----------------------------------------------------------------------------

set< Node >& multiint_to_multiNode_query(const set< int >& source, set< Node >& result_set);

#endif
