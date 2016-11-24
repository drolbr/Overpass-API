/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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
      into = get_identifier_token(token, error_output, "Variable");
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
      from = get_identifier_token(token, error_output, "Variable");
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
        error_output->add_parse_error("difference always requires two operands", token.line_col().first);
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
TStatement* create_convert_statement(typename TStatement::Factory& stmt_factory,
    string into, string type, uint line_nr)
{
  map< string, string > attr;
  attr["into"] = into;
  attr["type"] = type;
  return stmt_factory.create_statement("convert", line_nr, attr);
}


template< class TStatement >
TStatement* create_make_statement(typename TStatement::Factory& stmt_factory,
    string strategy, string from, string into, string type, uint line_nr)
{
  map< string, string > attr;
  if (from != "")
    attr["from"] = from;
  attr["into"] = into;
  attr["type"] = type;
  return stmt_factory.create_statement(strategy, line_nr, attr);
}

template< class TStatement >
TStatement* create_filter_statement(typename TStatement::Factory& stmt_factory, uint line_nr)
{
  map< string, string > attr;
  return stmt_factory.create_statement("filter", line_nr, attr);
}


enum Object_Type { tag, generic, id, object_type };

template< class TStatement >
TStatement* create_set_prop_statement(typename TStatement::Factory& stmt_factory,
    string key, Object_Type key_type, uint line_nr)
{
  map< string, string > attr;
  if (key_type == id)
    attr["keytype"] = "id";
  else if (key_type == generic)    
    attr["keytype"] = "generic";
  else
    attr["k"] = key;
  return stmt_factory.create_statement("set-prop", line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_fixed(typename TStatement::Factory& stmt_factory,
    string value, uint line_nr)
{
  map< string, string > attr;
  attr["v"] = value;
  return stmt_factory.create_statement("eval-fixed", line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_value(typename TStatement::Factory& stmt_factory,
    string key, uint line_nr)
{
  map< string, string > attr;
  attr["k"] = key;
  return stmt_factory.create_statement("eval-value", line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_count(typename TStatement::Factory& stmt_factory,
    string type, string from, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["type"] = type;
  return stmt_factory.create_statement("eval-count", line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_union_value(typename TStatement::Factory& stmt_factory,
    string type, string from, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  return stmt_factory.create_statement(type, line_nr, attr);
}


template< class TStatement >
TStatement* create_tag_value_x(const std::string& type, typename TStatement::Factory& stmt_factory, uint line_nr)
{
  map< string, string > attr;
  return stmt_factory.create_statement(type, line_nr, attr);
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
  else
    attr["regk"] = key;
  
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
     string type, vector< string > name, vector< string > uid, string into, uint line_nr)
{
  map< string, string > attr;
  std::vector< string >::iterator it;
  int i;

  attr["into"] = into;

  if (uid.empty())
    attr["uid"] = "";

  if (name.empty())
    attr["name"] = "";

  for(it = name.begin(), i = 0; it != name.end(); ++it, ++i)
  {
    std::stringstream id;
    if (i == 0)
      id << "name";
    else
      id << "name_" << i;
    attr[id.str()] = *it;
  }

  for(it = uid.begin(), i = 0; it != uid.end(); ++it, ++i)
  {
    std::stringstream id;
    if (i == 0)
      id << "uid";
    else
      id << "uid_" << i;
    attr[id.str()] = *it;
  }

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
  result.push_back(get_identifier_token(token, error_output, "Keyword"));
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
TStatement* stmt_from_value_tree(typename TStatement::Factory& stmt_factory, Error_Output* error_output,
    const Token_Node_Ptr& tree_it);


template< class TStatement >
TStatement* stmt_from_blank_function_expr(typename TStatement::Factory& stmt_factory, Error_Output* error_output,
    const std::string& type, const std::string& from, const Token_Node_Ptr& tree_it)
{
  TStatement* stmt = 0;
  if (type == "count")
    return create_tag_value_count< TStatement >(stmt_factory, tree_it->token, from, tree_it->line_col.first);
  else if (type == "u")
    stmt = create_tag_value_union_value< TStatement >(
        stmt_factory, "eval-union-value", from, tree_it->line_col.first);
  else if (type == "min")
    stmt = create_tag_value_union_value< TStatement >(
        stmt_factory, "eval-min-value", from, tree_it->line_col.first);
  else if (type == "max")
    stmt = create_tag_value_union_value< TStatement >(
        stmt_factory, "eval-max-value", from, tree_it->line_col.first);
  else if (type == "sum")
    stmt = create_tag_value_union_value< TStatement >(
        stmt_factory, "eval-sum-value", from, tree_it->line_col.first);
  else if (type == "set")
    stmt = create_tag_value_union_value< TStatement >(
        stmt_factory, "eval-set-value", from, tree_it->line_col.first);
    
  if (stmt)
  {
    TStatement* rhs = stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it);
    if (stmt && rhs)
      stmt->add_statement(rhs, "");
  }
  else
    error_output->add_parse_error(std::string("Function \"") + type + "\" not known", tree_it->line_col.first);
  return stmt;
}


template< class TStatement >
TStatement* stmt_from_value_tree(typename TStatement::Factory& stmt_factory, Error_Output* error_output,
    const Token_Node_Ptr& tree_it)
{
  if (tree_it->lhs != 0)
  {
    if (tree_it->rhs != 0)
    {
      if (tree_it->token == "(")
        return stmt_from_blank_function_expr< TStatement >(stmt_factory, error_output,
            tree_it.lhs()->token, "_", tree_it.rhs());
      else if (tree_it->token == ".")
      {
        if (tree_it.rhs()->token == "(")
          return stmt_from_blank_function_expr< TStatement >(stmt_factory, error_output,
              tree_it.lhs()->token, tree_it.rhs().lhs()->token, tree_it.rhs().rhs());
      }
      
      TStatement* stmt = 0;
      if (tree_it->token == "&&")
        stmt = create_tag_value_x< TStatement >("eval-and", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "||")
        stmt = create_tag_value_x< TStatement >("eval-or", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "==")
        stmt = create_tag_value_x< TStatement >("eval-equal", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "<")
        stmt = create_tag_value_x< TStatement >("eval-less", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "+")
        stmt = create_tag_value_x< TStatement >("eval-plus", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "-")
        stmt = create_tag_value_x< TStatement >("eval-minus", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "*")
        stmt = create_tag_value_x< TStatement >("eval-times", stmt_factory, tree_it->line_col.first);
      if (tree_it->token == "/")
        stmt = create_tag_value_x< TStatement >("eval-divided", stmt_factory, tree_it->line_col.first);
    
      if (stmt)
      {
        TStatement* lhs = stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.lhs());
        if (lhs)
          stmt->add_statement(lhs, "");
        
        TStatement* rhs = stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.rhs());
        if (rhs)
          stmt->add_statement(rhs, "");
      }
      else
        error_output->add_parse_error(
          std::string("\"") + tree_it->token + "\" cannot be used as binary operator",
          tree_it->line_col.first);
      return stmt;
    }
    
    if (tree_it->token == "(")
    {
      if (tree_it.lhs()->token == "id")
        return create_tag_value_x< TStatement >("eval-id", stmt_factory, tree_it->line_col.first);
      else if (tree_it.lhs()->token == "type")
        return create_tag_value_x< TStatement >("eval-type", stmt_factory, tree_it->line_col.first);
    }
    
    error_output->add_parse_error(
        std::string("\"") + tree_it->token + "\" needs a right hand side argument",
        tree_it->line_col.first);
    return stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.lhs());
  }
  
  if (tree_it->rhs != 0)
  {
    if (tree_it->token == "(")
      return stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.rhs());
    
    if (tree_it->token == "[")
      return create_tag_value_value< TStatement >(stmt_factory, decode_json(tree_it.rhs()->token, error_output),
          tree_it->line_col.first);
    
    if (tree_it->token == "-")
    {
      TStatement* stmt = create_tag_value_x< TStatement >("eval-minus", stmt_factory, tree_it->line_col.first);
      TStatement* lhs = create_tag_value_fixed< TStatement >(stmt_factory, "0", tree_it->line_col.first);
      if (lhs)
        stmt->add_statement(lhs, "");
      TStatement* rhs = stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.rhs());
      if (rhs)
        stmt->add_statement(rhs, "");
      return stmt;
    }
    else if (tree_it->token == "!")
    {
      TStatement* stmt = create_tag_value_x< TStatement >("eval-not", stmt_factory, tree_it->line_col.first);
      TStatement* lhs = create_tag_value_fixed< TStatement >(stmt_factory, "0", tree_it->line_col.first);
      if (lhs)
        stmt->add_statement(lhs, "");
      TStatement* rhs = stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.rhs());
      if (rhs)
        stmt->add_statement(rhs, "");
      return stmt;
    }
    
    error_output->add_parse_error(
        std::string("\"") + tree_it->token + "\" cannot be used as unary operator", tree_it->line_col.first);
    return stmt_from_value_tree< TStatement >(stmt_factory, error_output, tree_it.rhs());
  }
  
  if (tree_it->token == "::")
    return create_tag_value_x< TStatement >("eval-generic", stmt_factory, tree_it->line_col.first);
  
  return create_tag_value_fixed< TStatement >(stmt_factory,
      decode_json(tree_it->token, error_output), tree_it->line_col.first);
}


template< class TStatement >
TStatement* parse_value_tree(typename TStatement::Factory& stmt_factory, Tokenizer_Wrapper& token,
    Error_Output* error_output, bool parenthesis_expected)
{
  Token_Tree tree(token, error_output, parenthesis_expected);
  if (!tree.tree.empty())
  {
    TStatement* stmt = stmt_from_value_tree< TStatement >(stmt_factory, error_output,
                                                          Token_Node_Ptr(tree, tree.tree[0].rhs));
    if (stmt)
      return stmt;
  }
  
  return 0;
}


template< class TStatement >
TStatement* parse_make(typename TStatement::Factory& stmt_factory, const std::string& from,
                       Tokenizer_Wrapper& token, Error_Output* error_output, const std::string& strategy)
{
  TStatement* statement = 0;
  std::vector< TStatement* > evaluators;
  std::string type = "";
  if (*token == strategy)
  {
    ++token;
    if (*token != ";")
      type = get_identifier_token(token, error_output, "Element class name");
    
    while (token.good() && *token != ";" && *token != "->")
    {
      if (*token == ",")
        ++token;
      
      if (token.good() && *token == "!")
      {
        ++token;
        
        std::string key = get_text_token(token, error_output, "Tag key");
        evaluators.push_back(create_set_prop_statement< TStatement >(
            stmt_factory, key, tag, token.line_col().first));
        
        clear_until_after(token, error_output, ",", ";", "->", false);
        continue;
      }
      
      std::string key;
      Object_Type key_type = tag;
      if (token.good() && *token == "::")
      {
        ++token;
        
        key = "_";
        key_type = generic;
        if (token.good())
        {
          if (*token == ".")
          {
            ++token;
            key = get_identifier_token(token, error_output, "Input set");
          }
          else if (*token == "id")
          {
            ++token;
            key_type = id;
          }
        }
      }
      else
        key = get_text_token(token, error_output, "Tag key");
      
      clear_until_after(token, error_output, "=");
      TStatement* stmt = parse_value_tree< TStatement >(stmt_factory, token, error_output, false);
      if (stmt)
      {
        TStatement* key_stmt = create_set_prop_statement< TStatement >(
            stmt_factory, key, key_type, token.line_col().first);
        key_stmt->add_statement(stmt, "");
        evaluators.push_back(key_stmt);
      }
    }
    string into = probe_into(token, error_output);
    
    statement = create_make_statement< TStatement >(stmt_factory, strategy, from, into, type, token.line_col().first);
    {
      for (typename std::vector< TStatement* >::const_iterator it = evaluators.begin();
          it != evaluators.end(); ++it)
        statement->add_statement(*it, "");
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
  else if (clause.statement == "has-kv_keyregex_icase")
    return create_has_kv_statement< TStatement >
        (stmt_factory, clause.attributes[0], clause.attributes[1], haskv_icase, haskv_regex,
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
        (stmt_factory, type, clause.attributes, vector<string>(), into, clause.line_col.first);
  else if (clause.statement == "uid")
    return create_user_statement< TStatement >
        (stmt_factory, type, vector<string>(), clause.attributes, into, clause.line_col.first);
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
  vector< TStatement* > value_stmts;
  while (token.good() && (*token == "[" || *token == "(" || *token == "."))
  {
    if (*token == "[")
    {
      ++token;
      
      bool key_regex = (*token == "~");
      if (key_regex)
	++token;
      
      if (*token == "!")    // [!key] as shortcut for [key !~ ".*"]
      {
        ++token;
        string key = get_text_token(token, error_output, "Key");
        clear_until_after(token, error_output, "]");
        Statement_Text clause("has-kv_regex", token.line_col());
        clause.attributes.push_back(key);
        clause.attributes.push_back(".*");
        clause.attributes.push_back("!");
        clauses.push_back(clause);
        continue;
      }

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
	    clause.statement = "has-kv_keyregex_icase";
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
	clause.attributes.push_back(get_text_token(token, error_output, "list of coordinates"));
	clear_until_after(token, error_output, ")");
	clauses.push_back(clause);
      }
      else if (*token == "user")
      {
	Statement_Text clause("user", token.line_col());
	++token;
	clear_until_after(token, error_output, ":", false);
        if (*token == ":")
	{
	  do
	  {
	    ++token;
	    clause.attributes.push_back(get_text_token(token, error_output, "User name"));
	    clear_until_after(token, error_output, ",", ")", false);
	  } while (token.good() && *token == ",");

          clear_until_after(token, error_output, ")");
        }
	clauses.push_back(clause);
      }
      else if (*token == "uid")
      {
	Statement_Text clause("uid", token.line_col());
	++token;
	clear_until_after(token, error_output, ":", false);
        if (*token == ":")
        {
          do
          {
            ++token;
            clause.attributes.push_back(get_text_token(token, error_output, "Positive integer"));
	    clear_until_after(token, error_output, ",", ")", false);
          } while (token.good() && *token == ",");

          clear_until_after(token, error_output, ")");
        }
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
	clause.attributes.push_back(get_identifier_token(token, error_output, "Recurse type"));
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
      else if (*token == "if")
      {
        Statement_Text clause("if", token.line_col());
        ++token;
	if (*token == ":")
	{
	  ++token;
          value_stmts.push_back(parse_value_tree< TStatement >(stmt_factory, token, error_output, true));
	}
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
    {
      if (type == "")
        statement = create_item_statement< TStatement >(stmt_factory, from, query_line_col.first);
      else
      {
        statement = create_query_statement< TStatement >
           (stmt_factory, type, into, query_line_col.first);
        TStatement* substatement = create_item_statement< TStatement >(stmt_factory, from, query_line_col.first);
        statement->add_statement(substatement, "");
      }
    }
  }
  else if (clauses.size() == 1 && from == "" && value_stmts.empty())
  {
    if (clauses.front().statement == "has-kv"
       || clauses.front().statement == "has-kv_regex"
       || clauses.front().statement == "has-kv_keyregex"
       || clauses.front().statement == "has-kv_icase"
       || clauses.front().statement == "has-kv_keyregex_icase"
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
    for (typename vector< TStatement* >::const_iterator it = value_stmts.begin(); it != value_stmts.end(); ++it)
    {
      TStatement* substatement = create_filter_statement< TStatement >(stmt_factory, query_line_col.first);
      if (substatement)
      {
        if (*it)
          substatement->add_statement(*it, "");
	statement->add_statement(substatement, "");
      }
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
      error_output->add_parse_error("Nesting of statements is limited to 1023 levels", token.line_col().first);
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
  if (token.good() && *token == "convert")
    return parse_make< TStatement >(stmt_factory, from, token, error_output, "convert");
  if (token.good() && *token == "make")
    return parse_make< TStatement >(stmt_factory, from, token, error_output, "make");
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
