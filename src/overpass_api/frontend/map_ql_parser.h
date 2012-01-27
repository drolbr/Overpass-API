#ifndef MAP_QL_PARSER
#define MAP_QL_PARSER

#include "../../expat/map_ql_input.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

using namespace std;

void parse_and_validate_map_ql
    (Statement::Factory& stmt_factory, const string& xml_raw, Error_Output* error_output);

void parse_and_dump_xml_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

void parse_and_dump_compact_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

void parse_and_dump_pretty_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

#endif
