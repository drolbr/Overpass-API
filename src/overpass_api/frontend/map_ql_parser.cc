/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../../expat/map_ql_input.h"
#include "../core/datatypes.h"
#include "../core/parsed_query.h"
#include "../dispatch/scripting_core.h"
#include "../statements/osm_script.h"
#include "../statements/statement.h"
#include "../statements/statement_dump.h"
#include "map_ql_parser.h"
#include "output_handler_parser.h"
#include "tokenizer_utils.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <queue>
#include <set>
#include <sstream>
#include <vector>


//-----------------------------------------------------------------------------

template< class TStatement >
TStatement* parse_statement(typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
			    Tokenizer_Wrapper& token, Error_Output* error_output, int depth);

string probe_into(Tokenizer_Wrapper& token, Error_Output* error_output)
{
  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    clear_until_after(token, error_output, ".");
    if (token.good())
      into = get_text_token(token, error_output, "Variable");
  }
  return into;
}

string probe_from(Tokenizer_Wrapper& token, Error_Output* error_output)
{
  string from = "_";
  if (token.good() && *token == ".")
  {
    ++token;
    if (token.good())
      from = get_text_token(token, error_output, "Variable");
  }
  return from;
}

template< class TStatement >
vector< TStatement* > collect_substatements(typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
					    Tokenizer_Wrapper& token, Error_Output* error_output, int depth)
{
  vector< TStatement* > substatements;
  clear_until_after(token, error_output, "(");
  while (token.good() && *token != ")")
  {
    TStatement* substatement = parse_statement< TStatement >
	(stmt_factory, parsed_query, token, error_output, ++depth);
    if (substatement)
      substatements.push_back(substatement);
    clear_until_after(token, error_output, ";", ")", false);
    if (*token == ";")
      ++token;
  }
  if (token.good())
    ++token;
  
  return substatements;
}

template< class TStatement >
vector< TStatement* > collect_substatements_and_probe
    (typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
     Tokenizer_Wrapper& token, Error_Output* error_output, bool& is_difference, int depth)
{
  is_difference = false;
  
  vector< TStatement* > substatements;
  clear_until_after(token, error_output, "(");
  if (token.good() && *token != ")")
  {
    TStatement* substatement = parse_statement< TStatement >
        (stmt_factory, parsed_query, token, error_output, ++depth);
    if (substatement)
      substatements.push_back(substatement);
    clear_until_after(token, error_output, ";", ")", "-", false);
    if (*token == ";")
      ++token;
    if (*token == "-")
    {
      is_difference = true;
      ++token;
    }
  }
  if (token.good() && *token != ")")
  {
    TStatement* substatement = parse_statement< TStatement >
	(stmt_factory, parsed_query, token, error_output, ++depth);
    if (substatement)
      substatements.push_back(substatement);
    clear_until_after(token, error_output, ";", ")", false);
    if (*token == ";")
      ++token;
    if (is_difference && *token != ")")
    {
      if (error_output)
        error_output->add_parse_error("difference takes always two operands", token.line_col().first);
      clear_until_after(token, error_output, ")", false);
    }
  }
  while (token.good() && *token != ")")
  {
    TStatement* substatement = parse_statement< TStatement >
	(stmt_factory, parsed_query, token, error_output, ++depth);
    if (substatement)
      substatements.push_back(substatement);
    clear_until_after(token, error_output, ";", ")", false);
    if (*token == ";")
      ++token;
  }
  if (token.good())
    ++token;
  
  return substatements;
}

//-----------------------------------------------------------------------------

template< class TStatement >
TStatement* create_union_statement(typename TStatement::Factory& stmt_factory,
				   string into, uint line_nr)
{
  map< string, string > attr;
  attr["into"] = into;
  return stmt_factory.create_statement("union", line_nr, attr);
}

template< class TStatement >
TStatement* create_difference_statement(typename TStatement::Factory& stmt_factory,
                                   string into, uint line_nr)
{
  map< string, string > attr;
  attr["into"] = into;
  return stmt_factory.create_statement("difference", line_nr, attr);
}

template< class TStatement >
TStatement* create_foreach_statement(typename TStatement::Factory& stmt_factory,
				     string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["into"] = into;
  return stmt_factory.create_statement("foreach", line_nr, attr);
}


template< class TStatement >
TStatement* create_make_statement(typename TStatement::Factory& stmt_factory,
    string from, string into, string type, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["into"] = into;
  attr["type"] = type;
  return stmt_factory.create_statement("make", line_nr, attr);
}


template< class TStatement >
TStatement* create_set_tag_statement(typename TStatement::Factory& stmt_factory,
    string key, uint line_nr)
{
  map< string, string > attr;
  attr["k"] = key;
  return stmt_factory.create_statement("set-tag", line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_fixed(typename TStatement::Factory& stmt_factory,
    string value, uint line_nr)
{
  map< string, string > attr;
  attr["v"] = value;
  return stmt_factory.create_statement("value-fixed", line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_count(typename TStatement::Factory& stmt_factory,
    string type, uint line_nr)
{
  map< string, string > attr;
  attr["type"] = type;
  return stmt_factory.create_statement("value-count", line_nr, attr);
}


template< class TStatement >
TStatement* create_print_statement(typename TStatement::Factory& stmt_factory,
                                   string from, string mode, string order, string limit, string geometry,
                                   string south, string north, string west, string east,
                                  uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["mode"] = mode;
  attr["order"] = order;
  attr["limit"] = limit;
  attr["geometry"] = geometry;
  attr["s"] = south;
  attr["n"] = north;
  attr["w"] = west;
  attr["e"] = east;
  return stmt_factory.create_statement("print", line_nr, attr);
}

template< class TStatement >
TStatement* create_query_statement(typename TStatement::Factory& stmt_factory,
				   string type, string into, uint line_nr)
{
  map< string, string > attr;
  attr["type"] = type;
  attr["into"] = into;
  return stmt_factory.create_statement("query", line_nr, attr);
}

typedef enum { haskv_plain, haskv_regex, haskv_icase } haskv_type;

template< class TStatement >
TStatement* create_has_kv_statement(typename TStatement::Factory& stmt_factory,
				    string key, string value, haskv_type regex, haskv_type key_regex, bool straight,
				    uint line_nr)
{
  map< string, string > attr;
  if (key_regex == haskv_plain)
    attr["k"] = key;
  else if (key_regex == haskv_regex)
    attr["regk"] = key;
  else
  {
    attr["regk"] = key;
    attr["key-case"] = "ignore";
  }
  if (regex == haskv_plain)
    attr["v"] = value;
  else if (regex == haskv_regex)
    attr["regv"] = value;
  else
  {
    attr["regv"] = value;
    attr["case"] = "ignore";
  }
  attr["modv"] = (straight ? "" : "not");
  return stmt_factory.create_statement("has-kv", line_nr, attr);
}

template< class TStatement >
TStatement* create_id_query_statement(typename TStatement::Factory& stmt_factory,
				      string type, string ref, string into, uint line_nr)
{
  map< string, string > attr;
  attr["type"] = type;
  attr["ref"] = ref;
  attr["into"] = into;
  return stmt_factory.create_statement("id-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_item_statement(typename TStatement::Factory& stmt_factory,
				  string from, uint line_nr)
{
  map< string, string > attr;
  attr["set"] = from;
  return stmt_factory.create_statement("item", line_nr, attr);
}

template< class TStatement >
TStatement* create_bbox_statement(typename TStatement::Factory& stmt_factory,
				  string south, string north, string west, string east,
				 string into, uint line_nr)
{
  map< string, string > attr;
  attr["s"] = south;
  attr["n"] = north;
  attr["w"] = west;
  attr["e"] = east;
  attr["into"] = into;
  return stmt_factory.create_statement("bbox-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_around_statement(typename TStatement::Factory& stmt_factory,
				    string radius, string lat, string lon,
                                    string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["into"] = into;
  attr["radius"] = radius;
  attr["lat"] = lat;
  attr["lon"] = lon;
  return stmt_factory.create_statement("around", line_nr, attr);
}

template< class TStatement >
TStatement* create_recurse_statement(typename TStatement::Factory& stmt_factory,
				     string type, string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["type"] = type;
  return stmt_factory.create_statement("recurse", line_nr, attr);
}

template< class TStatement >
TStatement* create_recurse_statement(typename TStatement::Factory& stmt_factory,
                                     string type, string from, string role, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["type"] = type;
  attr["role"] = role;
  attr["role-restricted"] = "yes";
  return stmt_factory.create_statement("recurse", line_nr, attr);
}

template< class TStatement >
TStatement* create_polygon_statement(typename TStatement::Factory& stmt_factory,
				   string bounds, string into, uint line_nr)
{
  map< string, string > attr;
  attr["bounds"] = bounds;
  attr["into"] = into;
  return stmt_factory.create_statement("polygon-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_user_statement
    (typename TStatement::Factory& stmt_factory,
     string type, string name, string uid, string into, uint line_nr)
{
  map< string, string > attr;
  attr["into"] = into;
  attr["uid"] = uid;
  attr["name"] = name;
  attr["type"] = type;
  return stmt_factory.create_statement("user", line_nr, attr);
}

template< class TStatement >
TStatement* create_newer_statement(typename TStatement::Factory& stmt_factory,
				   string than, uint line_nr)
{
  map< string, string > attr;
  attr["than"] = than;
  return stmt_factory.create_statement("newer", line_nr, attr);
}

template< class TStatement >
TStatement* create_area_statement(typename TStatement::Factory& stmt_factory,
				   string ref, string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["ref"] = ref;
  return stmt_factory.create_statement("area-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_coord_query_statement(typename TStatement::Factory& stmt_factory,
				     string lat, string lon, string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["lat"] = lat;
  attr["lon"] = lon;
  return stmt_factory.create_statement("coord-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_map_to_area_statement(typename TStatement::Factory& stmt_factory,
				     string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  return stmt_factory.create_statement("map-to-area", line_nr, attr);
}

template< class TStatement >
TStatement* create_pivot_statement(typename TStatement::Factory& stmt_factory,
                                   string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  return stmt_factory.create_statement("pivot", line_nr, attr);
}

template< class TStatement >
TStatement* create_changed_statement(typename TStatement::Factory& stmt_factory,
				   string since, string until, string into, uint line_nr)
{
  map< string, string > attr;
  attr["since"] = since;
  attr["until"] = until;
  attr["into"] = into;
  return stmt_factory.create_statement("changed", line_nr, attr);
}

//-----------------------------------------------------------------------------

std::vector< std::string > parse_setup(Tokenizer_Wrapper& token,
				       Error_Output* error_output, Parsed_Query& parsed_query)
{
  ++token;
  std::vector< std::string > result;
  result.push_back(get_text_token(token, error_output, "Keyword"));  
  clear_until_after(token, error_output, ":", "]");
  result.push_back(get_text_token(token, error_output, "Value"));
  if (result.front() == "out")
  {
    Output_Handler_Parser* format_parser =
	Output_Handler_Parser::get_format_parser(result.back());
	
    if (!format_parser)
    {
      if (error_output)
	error_output->add_parse_error("Unknown output format: " + result.back(), token.line_col().first);
    }
    else
      parsed_query.set_output_handler(format_parser, &token, error_output);
    
    clear_until_after(token, error_output, "]", true);      
  }
  else if (result.front() == "diff" || result.front() == "adiff")
  {
    clear_until_after(token, error_output, ",", "]", false);
    if (*token == ",")
    {
      ++token;
      result.push_back(get_text_token(token, error_output, "Value"));
      clear_until_after(token, error_output, "]", true);      
    }
    else
      ++token;
  }
  else if (result.front() == "bbox")
  {
    clear_until_after(token, error_output, ",", "]", false);
    if (*token == ",")
    {
      ++token;
      result.back() += "," + get_text_token(token, error_output, "Value");
      clear_until_after(token, error_output, ",", "]", false);
    }
    if (*token == ",")
    {
      ++token;
      result.back() += "," + get_text_token(token, error_output, "Value");
      clear_until_after(token, error_output, ",", "]", false);
    }
    if (*token == ",")
    {
      ++token;
      result.back() += "," + get_text_token(token, error_output, "Value");
      clear_until_after(token, error_output, "]");
    }
  }
  else
    clear_until_after(token, error_output, "]");
  return result;
}

template< class TStatement >
TStatement* parse_union(typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
			Tokenizer_Wrapper& token, Error_Output* error_output, int depth)
{
  pair< uint, uint > line_col = token.line_col();
  
  bool is_difference = false;
  vector< TStatement* > substatements =
      collect_substatements_and_probe< TStatement >(stmt_factory, parsed_query, token, error_output,
						    is_difference, depth);
  string into = probe_into(token, error_output);
  
  if (is_difference)
  {
    TStatement* statement = create_difference_statement< TStatement >(stmt_factory, into, line_col.first);
    for (typename vector< TStatement* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      statement->add_statement(*it, "");
    return statement;
  }
  else
  {
    TStatement* statement = create_union_statement< TStatement >(stmt_factory, into, line_col.first);
    for (typename vector< TStatement* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      statement->add_statement(*it, "");
    return statement;
  }
}

template< class TStatement >
TStatement* parse_foreach(typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
			  Tokenizer_Wrapper& token, Error_Output* error_output, int depth)
{
  pair< uint, uint > line_col = token.line_col();
  ++token;
  
  string from = probe_from(token, error_output);
  string into = probe_into(token, error_output);
  vector< TStatement* > substatements =
      collect_substatements< TStatement >(stmt_factory, parsed_query, token, error_output, depth);

  TStatement* statement = create_foreach_statement< TStatement >
      (stmt_factory, from, into, line_col.first);
  for (typename vector< TStatement* >::const_iterator it = substatements.begin();
      it != substatements.end(); ++it)
    statement->add_statement(*it, "");
  return statement;
}

template< class TStatement >
TStatement* parse_output(typename TStatement::Factory& stmt_factory,
			 const string& from, Tokenizer_Wrapper& token, Error_Output* error_output)
{
  TStatement* statement = 0;
  if (*token == "out")
  {
    ++token;
    string mode = "body";
    string order = "id";
    string limit = "";
    string geometry = "skeleton";
    string south = "";
    string north = "";
    string west = "";
    string east = "";
    while (token.good() && *token != ";")
    {
      if (*token == "ids")
	mode = "ids_only";
      else if (*token == "tags")
        mode = "tags";
      else if (*token == "skel")
	mode = "skeleton";
      else if (*token == "body")
	mode = "body";
      else if (*token == "meta")
	mode = "meta";
      else if (*token == "quirks")
	mode = "quirks";
      else if (*token == "count")
    mode = "count";
      else if (*token == "qt")
	order = "quadtile";
      else if (*token == "asc")
	order = "id";
      else if (*token == "geom")
        geometry = "full";
      else if (*token == "bb")
        geometry = "bounds";
      else if (*token == "center")
        geometry = "center";
      else if (isdigit((*token)[0]))
	limit = *token;
      else if (*token == "(")
      {
        ++token;
        south = get_text_token(token, error_output, "Number");
        clear_until_after(token, error_output, ",");
        west = get_text_token(token, error_output, "Number");
        clear_until_after(token, error_output, ",");
        north = get_text_token(token, error_output, "Number");
        clear_until_after(token, error_output, ",");
        east = get_text_token(token, error_output, "Number");
        clear_until_after(token, error_output, ")", false);
      }
      else
      {
	if (error_output)
	  error_output->add_parse_error
	      (string("Invalid parameter for print: \"") + *token + "\"", token.line_col().first);
      }
      ++token;
    }
    
    if (statement == 0)
    {
      statement = create_print_statement< TStatement >
          (stmt_factory, from == "" ? "_" : from, mode, order, limit, geometry,
           south, north, west, east,
           token.line_col().first);
    }
    else
    {
      if (error_output)
	error_output->add_parse_error("Garbage after output statement found.",
				      token.line_col().first);
    }
  }
  
  return statement;
}

template< class TStatement >
TStatement* parse_make(typename TStatement::Factory& stmt_factory,
                       const string& from, Tokenizer_Wrapper& token, Error_Output* error_output)
{
  TStatement* statement = 0;
  std::vector< std::pair< std::string, TStatement* > > evaluators;
  std::string type = "";
  if (*token == "make")
  {
    ++token;
    if (*token != ";")
    {
      type = *token;
      ++token;
    }
    while (token.good() && *token != ";" && *token != "->")
    {
      if (*token == ",")
        ++token;
      std::string key = get_text_token(token, error_output, "Tag key");
      clear_until_after(token, error_output, "=");
      std::string value = get_text_token(token, error_output, "Tag value");
      if (value == "count" && token.good() && *token == "(")
      {
        ++token;
        std::string type = get_text_token(token, error_output, "Count type");
        evaluators.push_back(std::make_pair(key, create_tag_value_count< TStatement >(
            stmt_factory, type, token.line_col().first)));
        clear_until_after(token, error_output, ")", true);
      }
      else
        evaluators.push_back(std::make_pair(key, create_tag_value_fixed< TStatement >(
            stmt_factory, value, token.line_col().first)));
      clear_until_after(token, error_output, ",", ";", "->", false);
    }
    string into = probe_into(token, error_output);
    
    if (statement == 0)
    {
      statement = create_make_statement< TStatement >
          (stmt_factory, from == "" ? "_" : from, into, type, token.line_col().first);
      for (typename std::vector< std::pair< std::string, TStatement* > >::const_iterator it = evaluators.begin();
          it != evaluators.end(); ++it)
      {
        TStatement* stmt_key = create_set_tag_statement< TStatement >(
            stmt_factory, it->first, token.line_col().first);
        statement->add_statement(stmt_key, "");
        stmt_key->add_statement(it->second, "");
      }
    }
    else
    {
      if (error_output)
        error_output->add_parse_error("Garbage after make statement found.",
                                      token.line_col().first);
    }
  }
  
  return statement;
}

string determine_recurse_type(string flag, string type, Error_Output* error_output,
			      const pair< uint, uint >& line_col)
{
  string recurse_type;
  if (flag == "r")
  {
    if (type == "node")
      recurse_type = "relation-node";
    else if (type == "way")
      recurse_type = "relation-way";
    else if (type == "relation")
      recurse_type = "relation-relation";
  }
  else if (flag == "w")
  {
    if (type == "node")
      recurse_type = "way-node";
    else
    {
      if (error_output)
	error_output->add_parse_error("A recursion from type 'w' produces nodes.",
				      line_col.first);
    }
  }
  else if (flag == "bn")
  {
    if (type == "node")
    {
      if (error_output)
	error_output->add_parse_error("A recursion from type 'bn' produces ways or relations.",
				      line_col.first);
    }
    else if (type == "way")
      recurse_type = "node-way";
    else if (type == "relation")
      recurse_type = "node-relation";
  }
  else if (flag == "bw")
  {
    if (type == "node" || type == "way")
    {
      if (error_output)
	error_output->add_parse_error("A recursion from type 'bw' produces relations.",
				      line_col.first);
    }
    else if (type == "relation")
      recurse_type = "way-relation";
  }
  else if (flag == "br")
  {
    if (type == "node" || type == "way")
    {
      if (error_output)
	error_output->add_parse_error("A recursion from type 'br' produces relations.",
				      line_col.first);
    }
    else if (type == "relation")
      recurse_type = "relation-backwards";
  }
  else if (flag == ">")
    recurse_type = "down";
  else if (flag == ">>")
    recurse_type = "down-rel";
  else if (flag == "<")
    recurse_type = "up";
  else if (flag == "<<")
    recurse_type = "up-rel";
  
  return recurse_type;
}

struct Statement_Text
{
  Statement_Text(string statement_ = "",
		 pair< uint, uint > line_col_ = make_pair(0, 0))
    : statement(statement_), line_col(line_col_) {}
  
  string statement;
  pair< uint, uint > line_col;
  vector< string > attributes;
};

template< class TStatement >
TStatement* create_query_substatement
    (typename TStatement::Factory& stmt_factory,
     Tokenizer_Wrapper& token, Error_Output* error_output,
     const Statement_Text& clause, string type, string from, string into)
{
  if (clause.statement == "has-kv")
    return create_has_kv_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.attributes[1], haskv_plain, haskv_plain,
	 (clause.attributes[2] == ""), clause.line_col.first);
  else if (clause.statement == "has-kv_regex")
    return create_has_kv_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.attributes[1], haskv_regex, haskv_plain,
	 (clause.attributes[2] == ""), clause.line_col.first);
  else if (clause.statement == "has-kv_icase")
    return create_has_kv_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.attributes[1], haskv_icase, haskv_plain,
	 (clause.attributes[2] == ""), clause.line_col.first);
  else if (clause.statement == "has-kv_keyregex")
    return create_has_kv_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.attributes[1], haskv_regex, haskv_regex,
	 (clause.attributes[2] == ""), clause.line_col.first);
  else if (clause.statement == "around")
    return create_around_statement< TStatement >
        (stmt_factory, clause.attributes[1], clause.attributes[2], clause.attributes[3],
         clause.attributes[0], into, clause.line_col.first);
  else if (clause.statement == "polygon")
    return create_polygon_statement< TStatement >
        (stmt_factory, clause.attributes[0], into, clause.line_col.first);
  else if (clause.statement == "user")
    return create_user_statement< TStatement >
        (stmt_factory, type, clause.attributes[0], "", into, clause.line_col.first);
  else if (clause.statement == "uid")
    return create_user_statement< TStatement >
        (stmt_factory, type, "", clause.attributes[0], into, clause.line_col.first);
  else if (clause.statement == "newer")
    return create_newer_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.line_col.first);
  else if (clause.statement == "recurse")
  {
    if (clause.attributes.size() == 2)
      return create_recurse_statement< TStatement >
          (stmt_factory,
	   determine_recurse_type(clause.attributes[0], type, error_output, clause.line_col),
	   clause.attributes[1], into, clause.line_col.first);
    else
      return create_recurse_statement< TStatement >
          (stmt_factory,
           determine_recurse_type(clause.attributes[0], type, error_output, clause.line_col),
           clause.attributes[1], clause.attributes[2], into, clause.line_col.first);
  }
  else if (clause.statement == "id-query")
    return create_id_query_statement< TStatement >
        (stmt_factory, type, clause.attributes[0], into, clause.line_col.first);
  else if (clause.statement == "bbox-query")
    return create_bbox_statement< TStatement >
        (stmt_factory,
	 clause.attributes[0], clause.attributes[2], clause.attributes[1], clause.attributes[3],
	 into, clause.line_col.first);
  else if (clause.statement == "area")
    return create_area_statement< TStatement >
        (stmt_factory, clause.attributes.size() <= 1 ? "" : clause.attributes[1],
	 clause.attributes[0], into, clause.line_col.first);
  else if (clause.statement == "pivot")
    return create_pivot_statement< TStatement >
        (stmt_factory, clause.attributes[0], into, clause.line_col.first);
  else if (clause.statement == "item")
    return create_item_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.line_col.first);
  else if (clause.statement == "changed")
    return create_changed_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.attributes[1], into, clause.line_col.first);
  return 0;
}

template< class TStatement >
TStatement* parse_full_recurse(typename TStatement::Factory& stmt_factory,
    Tokenizer_Wrapper& token, const string& from, Error_Output* error_output)
{
  string type = *token;
  uint line_col = token.line_col().first;
  ++token;
  string into = probe_into(token, error_output);
  
  if (type == ">")
    return create_recurse_statement< TStatement >(stmt_factory, "down", from, into, line_col);
  else if (type == ">>")
    return create_recurse_statement< TStatement >(stmt_factory, "down-rel", from, into, line_col);
  else if (type == "<")
    return create_recurse_statement< TStatement >(stmt_factory, "up", from, into, line_col);
  else if (type == "<<")
    return create_recurse_statement< TStatement >(stmt_factory, "up-rel", from, into, line_col);
  else
    return 0;
}

template< class TStatement >
TStatement* parse_coord_query(typename TStatement::Factory& stmt_factory,
    Tokenizer_Wrapper& token, const string& from, Error_Output* error_output)
{
  string type = *token;
  uint line_col = token.line_col().first;
  ++token;
  
  string lat, lon;
  if (*token == "(")
  {
    ++token;
    lat = get_text_token(token, error_output, "Number");
    clear_until_after(token, error_output, ",", ")", false);
    if (*token == ",")
    {
      ++token;
      lon = get_text_token(token, error_output, "Number");
      clear_until_after(token, error_output, ")", false);
    }
    ++token;
  }
  string into = probe_into(token, error_output);
  
  return create_coord_query_statement< TStatement >(stmt_factory, lat, lon, from, into, line_col);
}

template< class TStatement >
TStatement* parse_map_to_area(typename TStatement::Factory& stmt_factory,
    Tokenizer_Wrapper& token, const string& from, Error_Output* error_output)
{
  string type = *token;
  uint line_col = token.line_col().first;
  ++token;
  
  string into = probe_into(token, error_output);
  
  return create_map_to_area_statement< TStatement >(stmt_factory, from, into, line_col);
}


template< class TStatement >
TStatement* parse_query(typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
			const string& type, const string& from, Tokenizer_Wrapper& token,
		 Error_Output* error_output)
{
  pair< uint, uint > query_line_col = token.line_col();
  
  vector< Statement_Text > clauses;
  while (token.good() && (*token == "[" || *token == "(" || *token == "."))
  {
    if (*token == "[")
    {
      ++token;
      
      bool key_regex = (*token == "~");
      if (key_regex)
	++token;
      
      string key = get_text_token(token, error_output, "Key");
      clear_until_after(token, error_output, "!", "~", "=", "!=", "]", false);
      
      bool straight = true;
      if (*token == "!")
      {
	straight = false;
	++token;
	clear_until_after(token, error_output, "~", "=", "]", false);
      }
      
      if (*token == "]")
      {
	if (key_regex && error_output)
	  error_output->add_parse_error(
	      "A regular expression for a key can only be combined with a regular expression as value criterion",
	      token.line_col().first);
	
	Statement_Text clause("has-kv", token.line_col());
	clause.attributes.push_back(key);
	clause.attributes.push_back("");
	clause.attributes.push_back(straight ? "" : "!");
	++token;
	clauses.push_back(clause);
      }
      else if (*token == "=" || *token == "!=")
      {
	if (key_regex && error_output)
	  error_output->add_parse_error(
	      "A regular expression for a key can only be combined with a regular expression as value criterion",
	      token.line_col().first);
	
	straight = (*token == "=");
	++token;
	if (token.good() && *token == "]")
	  ++token;
	else
	{
	  Statement_Text clause("has-kv", token.line_col());
	  clause.attributes.push_back(key);
	  clause.attributes.push_back(get_text_token(token, error_output, "Value"));
	  if (clause.attributes.back() != "")
	  {
	    clause.attributes.push_back(straight ? "" : "!");
	    clauses.push_back(clause);
	  }
	  clear_until_after(token, error_output, "]");
	}
      }
      else //if (*token == "~")
      {
	if (key_regex)
	{
	  ++token;
	  Statement_Text clause("has-kv_keyregex", token.line_col());
	  clause.attributes.push_back(key);
	  clause.attributes.push_back(get_text_token(token, error_output, "Value"));
	  clause.attributes.push_back(straight ? "" : "!");
	  clear_until_after(token, error_output, ",", "]", false);
	  if (*token == ",")
	  {
	    clause.statement = "has-kv_icase";
	    ++token;
	    clear_until_after(token, error_output, "i");
	    clear_until_after(token, error_output, "]", false);
	  }
	  ++token;
	  clauses.push_back(clause);
	}
	else
	{
	  ++token;
	  Statement_Text clause("has-kv_regex", token.line_col());
	  clause.attributes.push_back(key);
	  clause.attributes.push_back(get_text_token(token, error_output, "Value"));
	  clause.attributes.push_back(straight ? "" : "!");
	  clear_until_after(token, error_output, ",", "]", false);
	  if (*token == ",")
	  {
	    clause.statement = "has-kv_icase";
	    ++token;
	    clear_until_after(token, error_output, "i");
	    clear_until_after(token, error_output, "]", false);
	  }
	  ++token;
	  clauses.push_back(clause);
	}
      }
    }
    else if (*token == "(")
    {
      ++token;
      if (!token.good())
      {
	if (error_output)
	  error_output->add_parse_error("':' or '.' expected.", token.line_col().first);
	break;
      }
      
      if (*token == "around")
      {
	Statement_Text clause("around", token.line_col());
	++token;
	clause.attributes.push_back(probe_from(token, error_output));
	clear_until_after(token, error_output, ":");
	clause.attributes.push_back(get_text_token(token, error_output, "Floating point number"));
	clear_until_after(token, error_output, ",", ")", false);
        if (*token == ",")
        {
          ++token;
          clause.attributes.push_back(get_text_token(token, error_output, "Floating point number"));
          clear_until_after(token, error_output, ",");
          clause.attributes.push_back(get_text_token(token, error_output, "Floating point number"));
        }
        else
        {
          clause.attributes.push_back("");
          clause.attributes.push_back("");
        }
	clauses.push_back(clause);
        clear_until_after(token, error_output, ")");
      }
      else if (*token == "poly")
      {
	Statement_Text clause("polygon", token.line_col());
	++token;
	clear_until_after(token, error_output, ":");
	clause.attributes.push_back(get_text_token(token, error_output, "List of Coordinates"));
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "user")
      {
	Statement_Text clause("user", token.line_col());
	++token;
	clear_until_after(token, error_output, ":");
	clause.attributes.push_back(get_text_token(token, error_output, "User name"));
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "uid")
      {
	Statement_Text clause("uid", token.line_col());
	++token;
	clear_until_after(token, error_output, ":");
	clause.attributes.push_back(get_text_token(token, error_output, "Positive integer"));
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "newer")
      {
	Statement_Text clause("newer", token.line_col());
	++token;
	clear_until_after(token, error_output, ":");
	clause.attributes.push_back(get_text_token(token, error_output, "\"YYYY-MM-DDThh:mm:ssZ\""));
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "changed")
      {
	Statement_Text clause("changed", token.line_col());
	++token;
	clear_until_after(token, error_output, ":", ")", false);
	if (*token == ":")
	{
	  ++token;
	  clause.attributes.push_back(get_text_token(token, error_output, "\"YYYY-MM-DDThh:mm:ssZ\""));
	  clear_until_after(token, error_output, ",", ")", false);
          if (*token == ",")
          {
            ++token;
	    clause.attributes.push_back(get_text_token(token, error_output, "\"YYYY-MM-DDThh:mm:ssZ\""));
	  }
	  else
	    clause.attributes.push_back(clause.attributes.back());
	}
	else
	{
	  clause.attributes.push_back("auto");
	  clause.attributes.push_back("auto");
	}
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "r" || *token == "w"
	    || *token == "bn" || *token == "bw" || *token == "br")
      {
	Statement_Text clause("recurse", token.line_col());
	clause.attributes.push_back(get_text_token(token, error_output, "Recurse type"));
	clause.attributes.push_back(probe_from(token, error_output));
        clear_until_after(token, error_output, ":", ")", false);
        if (*token == ":")
        {
          ++token;
          clause.attributes.push_back(get_text_token(token, error_output, "Role"));
        }
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "area")
      {
	Statement_Text clause("area", token.line_col());
	++token;
	clause.attributes.push_back(probe_from(token, error_output));
	if (*token == ":")
	{
	  ++token;
	  clause.attributes.push_back(get_text_token(token, error_output, "Positive integer"));
	}
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "pivot")
      {
        Statement_Text clause("pivot", token.line_col());
        ++token;
        clause.attributes.push_back(probe_from(token, error_output));
        clear_until_after(token, error_output, ")");
        clauses.push_back(clause);
      }
      else if (*token == ">" || *token == ">>" || *token == "<" || *token == "<<")
      {
	Statement_Text clause("recurse", token.line_col());
	clause.attributes.push_back(*token);
	++token;
	clause.attributes.push_back(probe_from(token, error_output));
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (isdigit((*token)[0]) || 
	       ((*token)[0] == '-' && (*token).size() > 1 && isdigit((*token)[1])))
      {
	string first_number = get_text_token(token, error_output, "Number");
	clear_until_after(token, error_output, ",", ")", false);
	if (*token == ")")
	{
	  Statement_Text clause("id-query", token.line_col());
	  clause.attributes.push_back(first_number);
	  clear_until_after(token, error_output, ")");
	  clauses.push_back(clause);
	}
	else
	{
	  Statement_Text clause("bbox-query", token.line_col());
	  clause.attributes.push_back(first_number);
	  clear_until_after(token, error_output, ",");
	  clause.attributes.push_back(get_text_token(token, error_output, "Number"));
	  clear_until_after(token, error_output, ",");
	  clause.attributes.push_back(get_text_token(token, error_output, "Number"));
	  clear_until_after(token, error_output, ",");
	  clause.attributes.push_back(get_text_token(token, error_output, "Number"));
	  clear_until_after(token, error_output, ")");
	  clauses.push_back(clause);
	}
      }
      else
      {
	if (error_output)
	  error_output->add_parse_error("Unknown query clause", token.line_col().first);
	clear_until_after(token, error_output, ")");
      }
    }
    else
    {
      Statement_Text clause("item", token.line_col());
      clause.attributes.push_back(probe_from(token, error_output));
      clauses.push_back(clause);
    }
  }
  
  string into = probe_into(token, error_output);
  
  TStatement* statement = 0;
  if (clauses.size() == 0)
  {
    if (from == "")
    {
      if (parsed_query.get_global_bbox_limitation().valid())
      {
        statement = create_query_statement< TStatement >
            (stmt_factory, type, into, query_line_col.first);
      }
      else if (error_output)
	error_output->add_parse_error("An empty query is not allowed", token.line_col().first);
    }
    else
      statement = create_item_statement< TStatement >(stmt_factory, from, query_line_col.first);
  }
  else if (clauses.size() == 1 && from == "")
  {
    if (clauses.front().statement == "has-kv"
       || clauses.front().statement == "has-kv_regex"
       || clauses.front().statement == "has-kv_keyregex"
       || clauses.front().statement == "has-kv_icase"
       || (clauses.front().statement == "area" && type != "node")
       || (clauses.front().statement == "around" && type != "node")
       || (clauses.front().statement == "pivot" && type != "node")
       || (clauses.front().statement == "polygon" && type != "node")
       || (clauses.front().statement == "bbox-query" && type != "node")
       || clauses.front().statement == "changed"
       || (clauses.front().statement == "recurse" &&
           (clauses.front().attributes[0] == "<" || clauses.front().attributes[0] == "<<"
	   || clauses.front().attributes[0] == ">" || clauses.front().attributes[0] == ">>")))
    {
      statement = create_query_statement< TStatement >
          (stmt_factory, type, into, query_line_col.first);
      TStatement* substatement = create_query_substatement< TStatement >
          (stmt_factory, token, error_output, clauses.front(), type, from, into);
      if (substatement)
	statement->add_statement(substatement, "");
    }
    else
    {
      statement = create_query_substatement< TStatement >
          (stmt_factory, token, error_output, clauses.front(), type, from, into);
    }
  }
  else
  {
    statement = create_query_statement< TStatement >(stmt_factory, type, into, query_line_col.first);
    if (!statement)
      return 0;
    
    if (from != "")
    {
      TStatement* substatement = create_item_statement< TStatement >
          (stmt_factory, from, query_line_col.first);
      if (substatement)
	statement->add_statement(substatement, "");
    }
    
    for (vector< Statement_Text >::const_iterator it = clauses.begin();
        it != clauses.end(); ++it)
    {
      TStatement* substatement = create_query_substatement< TStatement >
          (stmt_factory, token, error_output, *it, type, from, "_");
      if (substatement)
	statement->add_statement(substatement, "");
    }
  }
  
  return statement;
}

template< class TStatement >
TStatement* parse_statement(typename TStatement::Factory& stmt_factory, Parsed_Query& parsed_query,
			    Tokenizer_Wrapper& token, Error_Output* error_output, int depth)
{
  if (!token.good())
    return 0;
  
  if (depth >= 1024)
  {
    if (error_output)
      error_output->add_parse_error("Nesting of statements limited to 1023 levels", token.line_col().first);
    return 0;
  }
  
  if (*token == "(")
    return parse_union< TStatement >(stmt_factory, parsed_query, token, error_output, depth);
  else if (*token == "foreach")
    return parse_foreach< TStatement >(stmt_factory, parsed_query, token, error_output, depth);

  string from = "";
  if (token.good() && *token == ".")
  {
    ++token;
    if (token.good())
    {
      from = *token;
      ++token;
    }
  }

  if (token.good() && *token == "out")
    return parse_output< TStatement >(stmt_factory, from, token, error_output);
  if (token.good() && *token == "make")
    return parse_make< TStatement >(stmt_factory, from, token, error_output);
  if (token.good() && (*token == "<" || *token == "<<" || *token == ">" || *token == ">>"))
    return parse_full_recurse< TStatement >(stmt_factory, token, from, error_output);
  if (token.good() && *token == "is_in")
    return parse_coord_query< TStatement >(stmt_factory, token, from, error_output);
  if (token.good() && *token == "map_to_area")
    return parse_map_to_area< TStatement >(stmt_factory, token, from, error_output);
  
  string type = "";
  if (*token != "out" && from == "")
  {
    type = *token;
    if (type == "rel")
      type = "relation";
    else if (type != "node" && type != "way" && type != "relation" && type != "area")
    {
      if (error_output)
	error_output->add_parse_error("Unknown type \"" + type + "\"", token.line_col().first);
    }
    ++token;
  }
  if (token.good() && *token == ".")
  {
    ++token;
    if (token.good())
    {
      from = *token;
      ++token;
    }
  }
  
  return parse_query< TStatement >(stmt_factory, parsed_query, type, from, token, error_output);
}


template< class TStatement >
void generic_parse_and_validate_map_ql
    (typename TStatement::Factory& stmt_factory,
     const string& xml_raw, Error_Output* error_output, vector< TStatement* >& stmt_seq, Parsed_Query& parsed_query)
{
  istringstream in(xml_raw);
  Tokenizer_Wrapper token(in);

  map< string, string > attr;
  while (token.good() && *token == "[")
  {
    std::vector< string > kv = parse_setup(token, error_output, parsed_query);
    if (kv.size() < 2)
      continue;
    if (kv[0] == "maxsize")
      kv[0] = "element-limit";
    else if (kv[0] == "out")
      kv[0] = "output";
    else if (kv[0] == "diff" || kv[0] == "adiff")
    {
      if (kv[0] == "adiff")
	attr["augmented"] = "deletions";
      if (kv.size() >= 3)
        attr["date"] = kv[2];
      kv[0] = "from";
    }
    attr[kv[0]] = kv[1];
  }
  
  TStatement* base_statement = stmt_factory.create_statement
      ("osm-script", token.line_col().first, attr);
    
  if (!attr.empty())
    clear_until_after(token, error_output, ";");
  
  while (token.good())
  {
    TStatement* statement = parse_statement< TStatement >(stmt_factory, parsed_query, token, error_output, 0);
    if (statement)
    {
      base_statement->add_statement(statement, "");
      clear_until_after(token, error_output, ";");
    }
  }
  
  stmt_seq.push_back(base_statement);
}

void parse_and_validate_map_ql
    (Statement::Factory& stmt_factory, const string& xml_raw, Error_Output* error_output, Parsed_Query& parsed_query)
{
  generic_parse_and_validate_map_ql< Statement >
      (stmt_factory, xml_raw, error_output, *get_statement_stack(), parsed_query);
}

void parse_and_dump_xml_from_map_ql
    (const string& xml_raw, Error_Output* error_output, Parsed_Query& parsed_query)
{
  Statement_Dump::Factory stmt_factory;
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(stmt_factory, xml_raw, error_output, stmt_seq, parsed_query);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_xml();
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}

void parse_and_dump_compact_from_map_ql
    (const string& xml_raw, Error_Output* error_output, Parsed_Query& parsed_query)
{
  Statement_Dump::Factory stmt_factory;
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(stmt_factory, xml_raw, error_output, stmt_seq, parsed_query);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_compact_map_ql()<<'\n';
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}

void parse_and_dump_bbox_from_map_ql
    (const string& xml_raw, Error_Output* error_output, Parsed_Query& parsed_query)
{
  Statement_Dump::Factory stmt_factory;
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(stmt_factory, xml_raw, error_output, stmt_seq, parsed_query);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_bbox_map_ql()<<'\n';
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}

void parse_and_dump_pretty_from_map_ql
    (const string& xml_raw, Error_Output* error_output, Parsed_Query& parsed_query)
{
  Statement_Dump::Factory stmt_factory;
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(stmt_factory, xml_raw, error_output, stmt_seq, parsed_query);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_pretty_map_ql();
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}
