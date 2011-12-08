#include "../../expat/map_ql_input.h"
#include "../core/datatypes.h"
#include "../dispatch/scripting_core.h"
#include "../statements/statement.h"
#include "../statements/statement_dump.h"
#include "map_ql_parser.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

pair< string, string > parse_setup(Tokenizer_Wrapper& token, Error_Output* error_output)
{
  ++token;

  if (*token == "]")
  {
    if (error_output)
      error_output->add_parse_error("Keyword expected", token.line_col().first);
    return make_pair< string, string >("", "");
  }
  pair< string, string > result = make_pair(*token, "");
  ++token;
  
  if (*token != ":")
  {
    if (error_output)
      error_output->add_parse_error("':' expected", token.line_col().first);
    return make_pair< string, string >("", "");
  }
  ++token;

  if (*token == "]")
  {
    if (error_output)
      error_output->add_parse_error("Value expected", token.line_col().first);
    return make_pair< string, string >("", "");
  }
  result.second = *token;
  ++token;

  while (*token != "]")
  {
    if (error_output)
      error_output->add_parse_error("']' expected", token.line_col().first);
    return make_pair< string, string >("", "");
    ++token;
  }
  ++token;
  
  return result;
}

template< class TStatement >
TStatement* create_union_statement(string into, uint line_nr)
{
  map< string, string > attr;
  attr["into"] = into;
  return TStatement::create_statement("union", line_nr, attr);
}

template< class TStatement >
TStatement* parse_statement(Tokenizer_Wrapper& token, Error_Output* error_output);

template< class TStatement >
TStatement* parse_union(Tokenizer_Wrapper& token, Error_Output* error_output)
{
  vector< TStatement* > substatements;
  pair< uint, uint > line_col = token.line_col();
  
  ++token;
  while (token.good() && *token != ")")
  {
    ++token;
    TStatement* substatement = parse_statement< TStatement >(token, error_output);
    if (substatement)
      substatements.push_back(substatement);
  }
  if (token.good())
    ++token;

  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    if (token.good() && *token == ".")
      ++token;
    else
    {
      if (error_output)
        error_output->add_parse_error("Variable expected", token.line_col().first);
    }
    if (token.good())
    {
      into = *token;
      ++token;
    }
  }
  
  TStatement* statement = create_union_statement< TStatement >(into, line_col.first);
  for (typename vector< TStatement* >::const_iterator it = substatements.begin();
      it != substatements.end(); ++it)
    statement->add_statement(*it, "");
  return statement;
}

template< class TStatement >
TStatement* create_foreach_statement(string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["into"] = into;
  return TStatement::create_statement("foreach", line_nr, attr);
}

template< class TStatement >
TStatement* parse_foreach(Tokenizer_Wrapper& token, Error_Output* error_output)
{
  string from = "_";
  ++token;
  if (token.good() && *token == ".")
  {
    ++token;
    if (token.good())
    {
      from = *token;
      ++token;
    }
  }
  
  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    if (token.good() && *token == ".")
      ++token;
    else
    {
      if (error_output)
	error_output->add_parse_error("Variable expected", token.line_col().first);
    }
    if (token.good())
    {
      into = *token;
      ++token;
    }
  }
  
  vector< TStatement* > substatements;
  pair< uint, uint > line_col = token.line_col();
  
  ++token;
  while (token.good() && *token != ")")
  {
    ++token;
    TStatement* substatement = parse_statement< TStatement >(token, error_output);
    if (substatement)
      substatements.push_back(substatement);
  }
  if (token.good())
    ++token;

  TStatement* statement = create_foreach_statement< TStatement >(from, into, line_col.first);
  for (typename vector< TStatement* >::const_iterator it = substatements.begin();
      it != substatements.end(); ++it)
    statement->add_statement(*it, "");
  return statement;
}

template< class TStatement >
TStatement* create_print_statement(string from, string mode, string order, string limit,
				  uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["mode"] = mode;
  attr["order"] = order;
  attr["limit"] = limit;
  return TStatement::create_statement("print", line_nr, attr);
}

template< class TStatement >
TStatement* parse_output(const string& from, Tokenizer_Wrapper& token, Error_Output* error_output)
{
  TStatement* statement = 0;
  ++token;
  while (token.good() && *token != "}")
  {
    if (*token == "xml")
    {
      ++token;
      string mode = "body";
      string order = "id";
      string limit = "";
      while (token.good() && *token != ";")
      {
	if (*token == "ids")
	  mode = "ids_only";
	else if (*token == "skel")
	  mode = "skeleton";
	else if (*token == "body")
	  mode = "body";
	else if (*token == "meta")
	  mode = "meta";
	else if (*token == "quirks")
	  mode = "quirks";
	else if (*token == "qt")
	  order = "quadtile";
	else if (*token == "asc")
	  order = "id";
	else if (isdigit((*token)[0]))
	  limit = *token;
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
	    (from == "" ? "_" : from, mode, order, limit, token.line_col().first);
      }
      else
      {
	if (error_output)
	  error_output->add_parse_error("Garbage after first output statement found.",
					token.line_col().first);
      }
    }
    else if (*token == "make-area")
    {
    }
    else
    {
      if (error_output)
	error_output->add_parse_error
	    (string("Unknown output type \"") + *token + "\"", token.line_col().first);
    }
    ++token;
  }
  
  if (token.good())
    ++token;
  
  return statement;
}

template< class TStatement >
TStatement* create_query_statement(string type, string into, uint line_nr)
{
  map< string, string > attr;
  attr["type"] = type;
  attr["into"] = into;
  return TStatement::create_statement("query", line_nr, attr);
}

template< class TStatement >
TStatement* create_has_kv_statement(string key, string value, uint line_nr)
{
  map< string, string > attr;
  attr["k"] = key;
  attr["v"] = value;
  return TStatement::create_statement("has-kv", line_nr, attr);
}

template< class TStatement >
TStatement* create_id_query_statement(string type, string ref, string into, uint line_nr)
{
  map< string, string > attr;
  attr["type"] = type;
  attr["ref"] = ref;
  attr["into"] = into;
  return TStatement::create_statement("id-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_item_statement(string from, uint line_nr)
{
  map< string, string > attr;
  attr["set"] = from;
  return TStatement::create_statement("item", line_nr, attr);
}

template< class TStatement >
TStatement* create_bbox_statement(string south, string north, string west, string east,
				 string into, uint line_nr)
{
  map< string, string > attr;
  attr["s"] = south;
  attr["n"] = north;
  attr["w"] = west;
  attr["e"] = east;
  attr["into"] = into;
  return TStatement::create_statement("bbox-query", line_nr, attr);
}

template< class TStatement >
TStatement* create_around_statement(string radius, string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["into"] = into;
  attr["radius"] = radius;
  return TStatement::create_statement("around", line_nr, attr);
}

template< class TStatement >
TStatement* create_recurse_statement(string type, string from, string into, uint line_nr)
{
  map< string, string > attr;
  attr["from"] = from;
  attr["into"] = into;
  attr["type"] = type;
  return TStatement::create_statement("recurse", line_nr, attr);
}

template< class TStatement >
TStatement* create_user_statement
    (string type, string name, string uid, string into, uint line_nr)
{
  map< string, string > attr;
  attr["into"] = into;
  attr["uid"] = uid;
  attr["name"] = name;
  attr["type"] = type;
  return TStatement::create_statement("user", line_nr, attr);
}

template< class TStatement >
TStatement* create_newer_statement(string than, uint line_nr)
{
  map< string, string > attr;
  attr["than"] = than;
  return TStatement::create_statement("newer", line_nr, attr);
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
	error_output->add_parse_error("A recursion from type 'bn' produces relations.",
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
	error_output->add_parse_error("A recursion from type 'bn' produces relations.",
				      line_col.first);
    }
    else if (type == "relation")
      recurse_type = "relation-backwards";
  }
  
  return recurse_type;
}

template< class TStatement >
TStatement* assemble_around_statement
    (const vector< string >& tokens, Error_Output* error_output,
     string into, const pair< uint, uint >& line_col)
{
  if (tokens.size() < 4)
  {
    if (error_output)
      error_output->add_parse_error("around needs a floating point number as radius.",
      line_col.first);
    
    return 0;
  }
  else
  {
    string around_from = "_";
    uint radius_pos = 3;
    if (tokens[2] == ".")
    {
      around_from = tokens[3];
      if (tokens.size() < 6 || tokens[4] != ":")
      {
	if (error_output)
	  error_output->add_parse_error("':' expected.", line_col.first);
      }
      else
	radius_pos = 5;
    }
    else if (tokens[2] != ":")
    {
      if (error_output)
	error_output->add_parse_error("':' or '.' expected.", line_col.first);
    }
    return create_around_statement< TStatement >
        (tokens[radius_pos],
         around_from, into, line_col.first);
  }
}

template< class TStatement >
TStatement* parse_query(const string& type, const string& from, Tokenizer_Wrapper& token,
		 Error_Output* error_output)
{
  vector< vector< string > > substatement_str;
  pair< uint, uint > line_col = token.line_col();
  while (token.good() && (*token == "[" || *token == "("))
  {
    vector< string > cur_substmt_str;
    
    while (token.good() && *token != "]" && *token != ")")
    {
      cur_substmt_str.push_back(*token);
      ++token;
    }
    
    if (*token == "]" && cur_substmt_str.front() == "(")
    {
      if (error_output)
	error_output->add_parse_error("'(' closed by ']'", token.line_col().first);
    }
    else if (*token == ")" && cur_substmt_str.front() == "[")
    {
      if (error_output)
	error_output->add_parse_error("'[' closed by ')'", token.line_col().first);
    }
    else
      substatement_str.push_back(cur_substmt_str);
    
    if (token.good())
      ++token;
  }
  
  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    if (token.good() && *token == ".")
      ++token;
    else
    {
      if (error_output)
	error_output->add_parse_error("Variable expected", token.line_col().first);
    }
    if (token.good())
    {
      into = *token;
      ++token;
    }
  }
  
  TStatement* statement = 0;
  
  if (substatement_str.size() == 0)
  {
    if (from == "")
    {
      if (error_output)
        error_output->add_parse_error("An empty query is not allowed", token.line_col().first);
    }
    else
      statement = create_item_statement< TStatement >(from, line_col.first);
  }
  else if (substatement_str.size() == 1 && from == "")
  {
    if (substatement_str[0][0] == "[")
    {
      TStatement* substatement = 0;
      
      if (substatement_str[0].size() == 2)
      {
	substatement = create_has_kv_statement< TStatement >
	    (substatement_str[0][1], "", line_col.first);
      }
      else
      {
	if (substatement_str[0][2] != "=")
	{
	  if (error_output)
	    error_output->add_parse_error("'=' or ']' expected.", line_col.first);
	}
	substatement = create_has_kv_statement< TStatement >
	    (substatement_str[0][1], substatement_str[0][3], line_col.first);
      }
      
      statement = create_query_statement< TStatement >(type, into, line_col.first);
      if (substatement)
	statement->add_statement(substatement, "");
    }
    else if (substatement_str[0][1] == "around")
    {
      statement = assemble_around_statement< TStatement >
          (substatement_str[0], error_output, into, line_col);
    }
    else if (substatement_str[0][1] == "user")
    {
      if (substatement_str[0].size() != 4 || substatement_str[0][2] != ":")
      {
	if (error_output)
	  error_output->add_parse_error("A user clause has the form '(user:_string_)'",
					line_col.first);
      }
      else
        statement = create_user_statement< TStatement >
            (type == "rel" ? "relation" : type, substatement_str[0][3], "", into, line_col.first);
    }
    else if (substatement_str[0][1] == "uid")
    {
      if (substatement_str[0].size() != 4 || substatement_str[0][2] != ":")
      {
	if (error_output)
	  error_output->add_parse_error("A uid clause has the form '(uid:_int_)'",
					line_col.first);
      }
      else
	statement = create_user_statement< TStatement >
            (type == "rel" ? "relation" : type, "", substatement_str[0][3], into, line_col.first);
    }
    else if (substatement_str[0][1] == "newer")
    {
      if (substatement_str[0].size() != 4 || substatement_str[0][2] != ":")
      {
	if (error_output)
	  error_output->add_parse_error
	      ("A newer clause has the form '(newer:\"YYYY-MM-DDThh:mm:ssZ\")'", line_col.first);
      }
      else
      {
	string than = substatement_str[0][3].substr(1, substatement_str[0][3].size()-2);
	statement = create_newer_statement< TStatement >(than, line_col.first);
      }
    }
    else if (substatement_str[0][1] == "r" || substatement_str[0][1] == "w"
      || substatement_str[0][1] == "bn" || substatement_str[0][1] == "bw"
      || substatement_str[0][1] == "br")
    {
      string recurse_from = "_";
      if (substatement_str[0].size() == 4 && substatement_str[0][2] == ".")
	recurse_from = substatement_str[0][3];
      else if (substatement_str[0].size() != 2)
      {
	if (error_output)
	  error_output->add_parse_error("Unknown tokens behind recurse", line_col.first);
      }
      
      string recurse_type = determine_recurse_type
          (substatement_str[0][1], type, error_output, line_col);
      statement = create_recurse_statement< TStatement >
          (recurse_type, recurse_from, into, line_col.first);
    }
    else if (substatement_str[0].size() == 2 && isdigit(substatement_str[0][1][0]))
    {
      statement = create_id_query_statement< TStatement >
          (type, substatement_str[0][1], into, line_col.first);      
    }
    else if (substatement_str[0].size() == 8 && isdigit(substatement_str[0][1][0])
      && substatement_str[0][2] == "," && isdigit(substatement_str[0][3][0])
      && substatement_str[0][4] == "," && isdigit(substatement_str[0][5][0])
      && substatement_str[0][6] == "," && isdigit(substatement_str[0][7][0]))
    {
      statement = create_bbox_statement< TStatement >
          (substatement_str[0][1],
	   substatement_str[0][5],
	   substatement_str[0][3],
	   substatement_str[0][7],
	   into, line_col.first);
    }
    else
    {
      if (error_output)
	error_output->add_parse_error("Unknown query clause", line_col.first);
    }
  }
  else if (substatement_str.size() == 1)
    statement = create_item_statement< TStatement >(from, line_col.first);
  else
  {
    statement = create_query_statement< TStatement >(type, into, line_col.first);
    if (!statement)
      return 0;
    
    if (from != "")
    {
      TStatement* substatement = create_item_statement< TStatement >(from, line_col.first);
      if (substatement)
	statement->add_statement(substatement, "");
    }
    
    for (vector< vector< string > >::const_iterator it = substatement_str.begin();
        it != substatement_str.end(); ++it)
    {
      TStatement* substatement = 0;
      if ((*it)[0] == "[")
      {
	if ((*it).size() == 2)
	{
	  substatement = create_has_kv_statement< TStatement >
	      ((*it)[1], "", line_col.first);
	}
	else
	{
	  if ((*it)[2] != "=")
	  {
	    if (error_output)
	      error_output->add_parse_error("'=' or ']' expected.", line_col.first);
	  }
	  substatement = create_has_kv_statement< TStatement >
	      ((*it)[1], (*it)[3], line_col.first);
	}
      }
      else if ((*it)[1] == "around")
      {
	substatement = assemble_around_statement< TStatement >
	    (substatement_str[0], error_output, into, line_col);
      }
      else if ((*it)[1] == "user")
      {
	if ((*it).size() != 4 || (*it)[2] != ":")
	{
	  if (error_output)
	    error_output->add_parse_error("A user clause has the form '(user:_string_)'",
	    line_col.first);
	}
	else
	  statement = create_user_statement< TStatement >
	      (type == "rel" ? "relation" : type, (*it)[3], "", into, line_col.first);
      }
      else if ((*it)[1] == "uid")
      {
	if ((*it).size() != 4 || (*it)[2] != ":")
	{
	  if (error_output)
	    error_output->add_parse_error("A uid clause has the form '(uid:_int_)'",
	    line_col.first);
	}
	else
	  statement = create_user_statement< TStatement >
	      (type == "rel" ? "relation" : type, "", (*it)[3], into, line_col.first);
      }
      else if ((*it)[1] == "newer")
      {
	if ((*it).size() != 4 || (*it)[2] != ":")
	{
	  if (error_output)
	    error_output->add_parse_error
	        ("A newer clause has the form '(newer:YYYY-MM-DDThh:mm:ssZ)'", line_col.first);
	}
	else
	{
	  string than = (*it)[3].substr(1, (*it)[3].size()-2);
	  statement = create_newer_statement< TStatement >(than, line_col.first);
	}
      }
      else if ((*it)[1] == "r" || (*it)[1] == "w"
	  || (*it)[1] == "bn" || (*it)[1] == "bw"
	  || (*it)[1] == "br")
      {
	string recurse_from = "_";
	if ((*it).size() == 4 && (*it)[2] == ".")
	  recurse_from = (*it)[3];
	else if ((*it).size() != 2)
	{
	  if (error_output)
	    error_output->add_parse_error("Unknown tokens behind recurse", line_col.first);
	}
	
	string recurse_type = determine_recurse_type
	    ((*it)[1], type, error_output, line_col);
	substatement = create_recurse_statement< TStatement >
	    (recurse_type, recurse_from, into, line_col.first);
      }
      else if ((*it).size() == 2 && isdigit((*it)[1][0]))
      {
        substatement = create_id_query_statement< TStatement >
            (type, (*it)[1], into, line_col.first);      
      }
      else if ((*it).size() == 8 && isdigit((*it)[1][0])
	  && (*it)[2] == "," && isdigit((*it)[3][0])
	  && (*it)[4] == "," && isdigit((*it)[5][0])
	  && (*it)[6] == "," && isdigit((*it)[7][0]))
      {
	substatement = create_bbox_statement< TStatement >
	    ((*it)[1], (*it)[5], (*it)[3], (*it)[7], "_", line_col.first);
      }
      else
      {
	if (error_output)
	  error_output->add_parse_error("Unknown query clause", line_col.first);
      }
      
      if (substatement)
	statement->add_statement(substatement, "");
    }
  }
  
  return statement;
}

template< class TStatement >
TStatement* parse_statement(Tokenizer_Wrapper& token, Error_Output* error_output)
{
  if (!token.good())
    return 0;
  
  if (*token == "union")
    return parse_union< TStatement >(token, error_output);
  else if (*token == "foreach")
    return parse_foreach< TStatement >(token, error_output);

  string type = "";
  if (*token != "{" && *token != ".")
  {
    type = *token;
    if (type == "rel")
      type = "relation";
    else if (type != "node" && type != "way" && type != "relation" && type != "all")
    {
      if (error_output)
	error_output->add_parse_error("Unknown type \"" + type + "\"", token.line_col().first);
    }
    ++token;
  }
    
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

  if (token.good() && type == "" && *token == "{")
    return parse_output< TStatement >(from, token, error_output);
  else
    return parse_query< TStatement >(type, from, token, error_output);
}

template< class TStatement >
void generic_parse_and_validate_map_ql
    (const string& xml_raw, Error_Output* error_output, vector< TStatement* >& stmt_seq)
{
  istringstream in(xml_raw);
  Tokenizer_Wrapper token(in);

  map< string, string > attr;
  while (token.good() && *token == "[")
  {
    pair< string, string > kv = parse_setup(token, error_output);
    if (kv.first == "maxsize")
      kv.first = "element-limit";
    attr.insert(kv);
  }
  
  TStatement* base_statement = TStatement::create_statement
      ("osm-script", token.line_col().first, attr);
  
  while (token.good())
  {
    TStatement* statement = parse_statement< TStatement >(token, error_output);
    if (statement)
      base_statement->add_statement(statement, "");
  }
  
  stmt_seq.push_back(base_statement);
}

void parse_and_validate_map_ql
    (const string& xml_raw, Error_Output* error_output)
{
  generic_parse_and_validate_map_ql< Statement >(xml_raw, error_output, *get_statement_stack());
}

void parse_and_dump_xml_from_map_ql
    (const string& xml_raw, Error_Output* error_output)
{
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(xml_raw, error_output, stmt_seq);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_xml();
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}

void parse_and_dump_compact_from_map_ql
    (const string& xml_raw, Error_Output* error_output)
{
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(xml_raw, error_output, stmt_seq);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_compact_map_ql();
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}

void parse_and_dump_pretty_from_map_ql
    (const string& xml_raw, Error_Output* error_output)
{
  vector< Statement_Dump* > stmt_seq;
  generic_parse_and_validate_map_ql< Statement_Dump >(xml_raw, error_output, stmt_seq);
  for (vector< Statement_Dump* >::const_iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    cout<<(*it)->dump_compact_map_ql();
  for (vector< Statement_Dump* >::iterator it = stmt_seq.begin();
      it != stmt_seq.end(); ++it)
    delete *it;
}
