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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "osm_script.h"
#include "statement.h"


std::map< std::string, Statement::Statement_Maker* >& Statement::maker_by_name()
{
  static std::map< std::string, Statement::Statement_Maker* > makers;
  return makers;
}


std::map< std::string, std::vector< Statement::Statement_Maker* > >& Statement::maker_by_token()
{
  static std::map< std::string, std::vector< Statement::Statement_Maker* > > makers;
  return makers;
}


std::map< std::string, std::vector< Statement::Statement_Maker* > >& Statement::maker_by_func_name()
{
  static std::map< std::string, std::vector< Statement::Statement_Maker* > > makers;
  return makers;
}


void Statement::eval_attributes_array(std::string element, std::map< std::string, std::string >& attributes,
				      const std::map< std::string, std::string >& input)
{
  for (std::map< std::string, std::string >::const_iterator it = input.begin(); it != input.end(); ++it)
  {
    std::map< std::string, std::string >::iterator ait(attributes.find(it->first));
    if (ait != attributes.end())
      ait->second = it->second;
    else
    {
      std::ostringstream temp;
      temp<<"Unknown attribute \""<<it->first<<"\" in element \""<<element<<"\".";
      add_static_error(temp.str());
    }
  }
}

void Statement::assure_no_text(std::string text, std::string name)
{
  for (unsigned int i(0); i < text.size(); ++i)
  {
    if (!isspace(text[i]))
    {
      std::ostringstream temp;
      temp<<"Element \""<<name<<"\" must not contain text.";
      add_static_error(temp.str());
      break;
    }
  }
}

void Statement::substatement_error(std::string parent, Statement* child)
{
  std::ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  add_static_error(temp.str());
}

void Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());
  substatement_error(get_name(), statement);
}

void Statement::add_final_text(std::string text)
{
  assure_no_text(text, this->get_name());
}

void Statement::display_full()
{
  //display_verbatim(get_source(startpos, endpos - startpos));
}

void Statement::display_starttag()
{
  //display_verbatim(get_source(startpos, tagendpos - startpos));
}


Statement::Factory::~Factory()
{
  for (std::vector< Statement* >::const_iterator it = created_statements.begin();
      it != created_statements.end(); ++it)
    delete *it;
}


Statement* Statement::Factory::create_statement
    (std::string element, int line_number, const std::map< std::string, std::string >& attributes)
{
  Statement* statement = 0;

  std::map< std::string, Statement::Statement_Maker* >::iterator maker_it =
      Statement::maker_by_name().find(element);

  if (maker_it != Statement::maker_by_name().end())
    statement = maker_it->second->create_statement(line_number, attributes, global_settings);

  if (statement)
    created_statements.push_back(statement);
  else
  {
    std::ostringstream temp;
    temp<<"Unknown tag \""<<element<<"\" in line "<<line_number<<'.';
    if (error_output_)
      error_output_->add_static_error(temp.str(), line_number);
  }

  return statement;
}


Statement* stmt_from_tree_node(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    const std::vector< Statement::Statement_Maker* >& makers, Statement::Factory& stmt_factory,
    Parsed_Query& global_settings, Error_Output* error_output)
{
  Statement* statement = 0;

  std::vector< Statement::Statement_Maker* >::const_iterator maker_it = makers.begin();

  while (!statement && maker_it != makers.end())
  {
    statement = (*maker_it)->create_statement(tree_it, tree_context, stmt_factory, global_settings, error_output);
    ++maker_it;
  }
  while (maker_it != makers.end())
  {
    Statement* bis = (*maker_it)->create_statement(tree_it, tree_context, stmt_factory, global_settings, error_output);
    if (bis)
    {
      if (error_output)
        error_output->add_static_error(std::string("Function name \"") + tree_it.lhs()->token
            + "\" has ambiguous resolutions to \"" + statement->get_name() + "\" and \"" + bis->get_name() + "\".",
            tree_it->line_col.first);
      delete bis;
      delete statement;
      statement = 0;
    }
    ++maker_it;
  }

  return statement;
}


Statement* Statement::Factory::create_evaluator(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context)
{
  Statement* statement = 0;

  if (tree_it->token == "(")
  {
    if (tree_it->lhs)
    {
      std::map< std::string, std::vector< Statement::Statement_Maker* > >::iterator all_it =
          Statement::maker_by_func_name().find(tree_it.lhs()->token);
      if (all_it != Statement::maker_by_func_name().end())
        statement = stmt_from_tree_node(tree_it, tree_context,
            all_it->second, *this, global_settings, Statement::error_output);
      else
        error_output->add_parse_error(std::string("Function \"") + tree_it.lhs()->token + "\" not known",
            tree_it->line_col.first);
    }
    else if (tree_it->rhs)
      return create_evaluator(tree_it.rhs(), tree_context);
    else
    {
      Statement::error_output->add_static_error("Empty parentheses cannot be evaluated.", tree_it->line_col.first);
      return 0;
    }
  }
  else if (tree_it->token == ".")
  {
    if (tree_it->lhs && tree_it->rhs && tree_it.rhs()->token == "(" && tree_it.rhs()->lhs)
    {
      std::map< std::string, std::vector< Statement::Statement_Maker* > >::iterator all_it =
          Statement::maker_by_func_name().find(tree_it.rhs().lhs()->token);
      if (all_it != Statement::maker_by_func_name().end())
        statement = stmt_from_tree_node(tree_it, tree_context,
            all_it->second, *this, global_settings, Statement::error_output);
      else
        error_output->add_parse_error(std::string("Function \"") + tree_it.rhs().lhs()->token + "\" not known",
            tree_it->line_col.first);
    }
  }
  else if (tree_it->token == "")
    error_output->add_parse_error("Evaluator expected, but empty token found.", tree_it->line_col.first);
  else
  {
    std::map< std::string, std::vector< Statement::Statement_Maker* > >::iterator all_it =
        Statement::maker_by_token().find(tree_it->token);
    if (all_it != Statement::maker_by_token().end())
      statement = stmt_from_tree_node(tree_it, tree_context,
          all_it->second, *this, global_settings, Statement::error_output);

    if (!statement)
    {
      all_it = Statement::maker_by_token().find("");
      if (all_it != Statement::maker_by_token().end())
        statement = stmt_from_tree_node(tree_it, tree_context,
            all_it->second, *this, global_settings, Statement::error_output);

      if (statement)
        ;
      else if (tree_it->lhs)
      {
        if (tree_it->rhs)
          error_output->add_parse_error(
              std::string("\"") + tree_it->token + "\" cannot be used as binary operator", tree_it->line_col.first);
        else
          error_output->add_parse_error(
              std::string("\"") + tree_it->token + "\" needs a right hand side argument", tree_it->line_col.first);
      }
      else if (tree_it->rhs)
      {
        error_output->add_parse_error(
            std::string("\"") + tree_it->token + "\" cannot be used as unary operator", tree_it->line_col.first);
        return create_evaluator(tree_it.rhs(), tree_context);
      }
      else
        error_output->add_parse_error(std::string("Token \"") + tree_it->token
            + "\" has not been recognized as statement", tree_it->line_col.first);
    }
  }

  if (statement)
    created_statements.push_back(statement);

  return statement;
}


Statement* create_id_query_statement(Statement::Factory& stmt_factory,
    std::string type, const std::vector< std::string >& ref, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;

  attr["type"] = type;
  attr["into"] = into;

  for (uint i = 0; i < ref.size(); ++i)
  {
    std::stringstream id;
    if (i == 0)
      id << "ref";
    else
      id << "ref_" << i;
    attr[id.str()] = ref[i];
  }

  return stmt_factory.create_statement("id-query", line_nr, attr);
}


Statement* create_id_query_statement(Statement::Factory& stmt_factory,
    Token_Node_Ptr tree_it, Error_Output* error_output, uint line_nr,
    const std::string& type, const std::string& into)
{
  std::vector< std::string > ref;
  
  while (tree_it->token == "," && tree_it->rhs && tree_it->lhs)
  {
    ref.push_back(tree_it.rhs()->token);
    tree_it = tree_it.lhs();
  }
  
  if (tree_it->token == ":" && tree_it->rhs)
    ref.push_back(tree_it.rhs()->token);
  else
    ref.push_back(tree_it->token);
  
  std::reverse(ref.begin(), ref.end());

  return create_id_query_statement(stmt_factory, type, ref, into, line_nr);
}


Statement* create_user_statement
    (Statement::Factory& stmt_factory,
     std::string type, const std::vector< std::string >& name, const std::vector< std::string >& uid, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  std::vector< std::string >::const_iterator it;
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


Statement* create_user_statement(Statement::Factory& stmt_factory,
    Token_Node_Ptr tree_it, Error_Output* error_output, uint line_nr,
    const std::string& type, const std::string& into)
{
  std::vector< std::string > users;
  
  while (tree_it->token == "," && tree_it->rhs && tree_it->lhs)
  {
    users.push_back(tree_it.rhs()->token);
    tree_it = tree_it.lhs();
  }
  
  if (tree_it->token == ":" && tree_it->rhs)
    users.push_back(tree_it.rhs()->token);
  
  std::reverse(users.begin(), users.end());

  if (tree_it->lhs && tree_it.lhs()->token == "user")
  {
    for (std::vector< std::string >::iterator it = users.begin(); it != users.end(); ++it)
      *it = decode_json(*it, error_output);
    return create_user_statement(stmt_factory, type, users, std::vector< std::string >(), into, line_nr);
  }
  return create_user_statement(stmt_factory, type, std::vector< std::string >(), users, into, line_nr);
}


Statement* create_recurse_statement(Statement::Factory& stmt_factory,
				     std::string type, std::string from, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["type"] = type;
  return stmt_factory.create_statement("recurse", line_nr, attr);
}


Statement* create_recurse_statement(Statement::Factory& stmt_factory,
                                     std::string type, std::string from, std::string role, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["type"] = type;
  attr["role"] = role;
  attr["role-restricted"] = "yes";
  return stmt_factory.create_statement("recurse", line_nr, attr);
}


Statement* create_recurse_statement(Statement::Factory& stmt_factory,
    Token_Node_Ptr tree_it, Error_Output* error_output, uint line_nr, const std::string& result_type,
    const std::string& into)
{
  std::string from = "_";
  std::string role;
  bool role_found = false;
  
  if (tree_it->token == ":" && tree_it->rhs)
  {
    role_found = true;
    role = decode_json(tree_it.rhs()->token, error_output);
    tree_it = tree_it.lhs();
  }

  if (tree_it->token == "." && tree_it->rhs)
  {
    from = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
  }
  
  std::string type = tree_it->token;

  if (type == ">")
    return create_recurse_statement(stmt_factory, "down", from, "_", line_nr);
  else if (type == ">>")
    return create_recurse_statement(stmt_factory, "down-rel", from, "_", line_nr);
  else if (type == "<")
    return create_recurse_statement(stmt_factory, "up", from, "_", line_nr);
  else if (type == "<<")
    return create_recurse_statement(stmt_factory, "up-rel", from, "_", line_nr);
  else if (type == "r")
  {
    if (result_type == "node")
      type = "relation-node";
    else if (result_type == "way")
      type = "relation-way";
    else if (result_type == "relation")
      type = "relation-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'r' produces nodes, ways, or relations.", line_nr);
  }
  else if (type == "w")
  {
    if (result_type == "node")
      type = "way-node";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'w' produces nodes.", line_nr);
  }
  else if (type == "br")
  {
    if (result_type == "relation")
      type = "relation-backwards";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'br' produces relations.", line_nr);
  }
  else if (type == "bw")
  {
    if (result_type == "relation")
      type = "way-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'bw' produces relations.", line_nr);
  }
  else if (type == "bn")
  {
    if (result_type == "way")
      type = "node-way";
    else if (result_type == "relation")
      type = "node-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'bn' produces ways or relations.", line_nr);
  }
  else
    return 0;

  if (role_found)
    return create_recurse_statement(stmt_factory, type, from, role, into, line_nr);
  return create_recurse_statement(stmt_factory, type, from, into, line_nr);
}


Statement* create_area_statement(Statement::Factory& stmt_factory,
				   std::string ref, std::string from, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  attr["ref"] = ref;
  return stmt_factory.create_statement("area-query", line_nr, attr);
}


Statement* create_area_statement(Statement::Factory& stmt_factory,
    Token_Node_Ptr tree_it, Error_Output* error_output, uint line_nr, const std::string& into)
{
  std::string from = "_";
  std::string ref;
  
  if (tree_it->token == ":" && tree_it->rhs)
  {
    ref = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
  }
  
  if (tree_it->token == "." && tree_it->rhs)
    from = tree_it.rhs()->token;

  return create_area_statement(stmt_factory, ref, from, into, line_nr);
}


Statement* create_around_statement(Statement::Factory& stmt_factory,
				    std::string radius, std::string lat, std::string lon,
                                    std::string from, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["from"] = from;
  attr["into"] = into;
  attr["radius"] = radius;
  attr["lat"] = lat;
  attr["lon"] = lon;
  return stmt_factory.create_statement("around", line_nr, attr);
}


Statement* create_around_statement(Statement::Factory& stmt_factory,
    Token_Node_Ptr tree_it, Error_Output* error_output, uint line_nr, const std::string& into)
{
  std::string lat;
  std::string lon;
  
  if (tree_it->token == "," && tree_it->rhs && tree_it->lhs)
  {
    lon = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
    
    if (tree_it->token != "," || !tree_it->rhs || !tree_it->lhs)
    {
      if (error_output)
        error_output->add_parse_error("around requires one or three arguments", line_nr);
      return 0;
    }
    
    lat = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
  }
  
  if (tree_it->token == ":" && tree_it->rhs)
  {
    std::string radius = decode_json(tree_it.rhs()->token, error_output);
    
    tree_it = tree_it.lhs();
    std::string from = "_";
    if (tree_it->token == "." && tree_it->rhs)
      from = tree_it.rhs()->token;
    
    return create_around_statement(stmt_factory, radius, lat, lon, from, into, line_nr);
  }
  else if (error_output)
    error_output->add_parse_error("around requires the radius as first argument", line_nr);

  return 0;
}


Statement* create_pivot_statement(Statement::Factory& stmt_factory,
                                   std::string from, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["from"] = (from == "" ? "_" : from);
  attr["into"] = into;
  return stmt_factory.create_statement("pivot", line_nr, attr);
}


Statement* create_pivot_statement(Statement::Factory& stmt_factory,
    const Token_Node_Ptr& tree_it, Error_Output* error_output, uint line_nr, const std::string& into)
{
  std::string from = "_";
  if (tree_it->rhs && tree_it->token == ".")
      from = tree_it.rhs()->token;
  
  return create_pivot_statement(stmt_factory, from, into, line_nr);
}


Statement* create_polygon_statement(Statement::Factory& stmt_factory,
				   std::string bounds, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["bounds"] = bounds;
  attr["into"] = into;
  return stmt_factory.create_statement("polygon-query", line_nr, attr);
}


Statement* create_polygon_statement(Statement::Factory& stmt_factory,
    const Token_Node_Ptr& tree_it, Error_Output* error_output, uint line_nr, const std::string& into)
{
  if (tree_it->token == ":" && tree_it->rhs)
  {
    std::string bounds = decode_json(tree_it.rhs()->token, error_output);
    return create_polygon_statement(stmt_factory, bounds, into, line_nr);
  }

  return 0;
}


Statement* create_bbox_statement(Statement::Factory& stmt_factory,
  std::string south, std::string north, std::string west, std::string east, std::string into, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["s"] = south;
  attr["n"] = north;
  attr["w"] = west;
  attr["e"] = east;
  attr["into"] = into;
  return stmt_factory.create_statement("bbox-query", line_nr, attr);
}


Statement* create_bbox_statement(Statement::Factory& stmt_factory,
    Token_Node_Ptr tree_it, Error_Output* error_output, uint line_nr, const std::string& into)
{
  if (tree_it->token != "," || !tree_it->rhs || !tree_it->lhs)
  {
    if (error_output)
      error_output->add_parse_error("bbox requires four arguments", line_nr);
    return 0;
  }
  
  std::string east = tree_it.rhs()->token;
  tree_it = tree_it.lhs();
  
  if (tree_it->token != "," || !tree_it->rhs || !tree_it->lhs)
  {
    if (error_output)
      error_output->add_parse_error("bbox requires four arguments", line_nr);
    return 0;
  }
  
  std::string north = tree_it.rhs()->token;
  tree_it = tree_it.lhs();
  
  if (tree_it->token != "," || !tree_it->rhs || !tree_it->lhs)
  {
    if (error_output)
      error_output->add_parse_error("bbox requires four arguments", line_nr);
    return 0;
  }
  
  std::string west = tree_it.rhs()->token;
  std::string south = tree_it.lhs()->token;
  
  return create_bbox_statement(stmt_factory, south, north, west, east, into, line_nr);
}


Statement* create_changed_statement(Statement::Factory& stmt_factory,
				   std::string since, std::string until, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["since"] = since;
  attr["until"] = until;
  return stmt_factory.create_statement("changed", line_nr, attr);
}


Statement* create_changed_statement(Statement::Factory& stmt_factory,
    const Token_Node_Ptr& tree_it, Error_Output* error_output, uint line_nr)
{
  std::string since;
  std::string until;
  
  if (tree_it->token == ":" && tree_it->rhs)
  {
    since = decode_json(tree_it.rhs()->token, error_output);
    until = since;
  }
  else if (tree_it->token == "," && tree_it->lhs && tree_it.lhs()->token == ":" && tree_it.lhs()->rhs
      && tree_it->rhs)
  {
    since = decode_json(tree_it.lhs().rhs()->token, error_output);
    until = decode_json(tree_it.rhs()->token, error_output);
  }
  else if (tree_it->token == "changed")
  {
    since = "auto";
    until = "auto";
  }
  else if (tree_it->token == ":")
  {
    if (error_output)
      error_output->add_parse_error("Date required after \"changed\" with colon",
          tree_it->line_col.first);
  }
  else
    if (error_output)
      error_output->add_parse_error("Unexpected token \"" + tree_it->token + "\" after \"changed\"",
          tree_it->line_col.first);

  return create_changed_statement(stmt_factory, since, until, line_nr);
}


Statement* create_filter_statement(Statement::Factory& stmt_factory, uint line_nr)
{
  std::map< std::string, std::string > attr;
  return stmt_factory.create_statement("filter", line_nr, attr);
}


Statement* create_filter_statement(Statement::Factory& stmt_factory,
    const Token_Node_Ptr& tree_it, Error_Output* error_output, uint line_nr)
{
  Statement* filter = 0;
  
  if (tree_it->token == ":" && tree_it->rhs)
  {
    Statement* criterion = stmt_factory.create_evaluator(tree_it.rhs(), Statement::elem_eval_possible);
    if (criterion)
    {
      filter = create_filter_statement(stmt_factory, line_nr);
      if (filter)
        filter->add_statement(criterion, "");
    }
  }

  return filter;
}


Statement* create_newer_statement(Statement::Factory& stmt_factory,
				   std::string than, uint line_nr)
{
  std::map< std::string, std::string > attr;
  attr["than"] = than;
  return stmt_factory.create_statement("newer", line_nr, attr);
}


Statement* create_newer_statement(Statement::Factory& stmt_factory,
    const Token_Node_Ptr& tree_it, Error_Output* error_output, uint line_nr)
{
  if (tree_it->token == ":" && tree_it->rhs)
  {
    std::string date = decode_json(tree_it.rhs()->token, error_output);
    return create_newer_statement(stmt_factory, date, line_nr);
  }

  return 0;
}


Token_Node_Ptr find_leftmost_token(Token_Node_Ptr tree_it)
{
  while (tree_it->lhs)
    tree_it = tree_it.lhs();
  
  return tree_it;
}


Statement* Statement::Factory::create_criterion(const Token_Node_Ptr& tree_it,
    const std::string& type, bool& can_standalone, const std::string& into)
{
  Token_Node_Ptr criterion = find_leftmost_token(tree_it);
  uint line_nr = criterion->line_col.first;
  
  bool criterion_is_number = (!criterion->token.empty()
      && (isdigit(criterion->token[0])
          || (criterion->token[0] == '-' && criterion->token.size() > 1 && isdigit(criterion->token[1]))));
  
  can_standalone = true;
  if (criterion->token == "id" || (criterion_is_number && tree_it->token != ","))
    return create_id_query_statement(*this, tree_it, error_output, line_nr, type, into);
  else if (criterion->token == "uid" || criterion->token == "user")
    return create_user_statement(*this, tree_it, error_output, line_nr, type, into);
  else if (criterion->token == "r" || criterion->token == "w"
      || criterion->token == "bn" || criterion->token == "bw" || criterion->token == "br")
    return create_recurse_statement(*this, tree_it, error_output, line_nr, type, into);
  
  can_standalone = (type == "node");
  if (criterion->token == "area")
    return create_area_statement(*this, tree_it, error_output, line_nr, into);
  else if (criterion->token == "around")
    return create_around_statement(*this, tree_it, error_output, line_nr, into);
  else if (criterion->token == "pivot")
    return create_pivot_statement(*this, tree_it, error_output, line_nr, into);
  else if (criterion->token == "poly")
    return create_polygon_statement(*this, tree_it, error_output, line_nr, into);
  else if (criterion_is_number && tree_it->token == ",")
    return create_bbox_statement(*this, tree_it, error_output, line_nr, into);
  
  can_standalone = false;
  if (criterion->token == "changed")
    return create_changed_statement(*this, tree_it, error_output, line_nr);
  else if (criterion->token == "if")
    return create_filter_statement(*this, tree_it, error_output, line_nr);
  else if (criterion->token == "newer")
    return create_newer_statement(*this, tree_it, error_output, line_nr);
  else if (criterion->token == ">" || criterion->token == ">>"
      || criterion->token == "<" || criterion->token == "<<")
    return create_recurse_statement(*this, tree_it, error_output, line_nr, type, "_");

  if (error_output)
    error_output->add_parse_error("Unknown query clause", line_nr);
  return 0;
}


Error_Output* Statement::error_output = 0;


void Statement::add_static_error(std::string error)
{
  if (error_output)
    error_output->add_static_error(error, line_number);
}


void Statement::add_static_remark(std::string remark)
{
  if (error_output)
    error_output->add_static_remark(remark, line_number);
}


void Statement::runtime_error(std::string error) const
{
  if (error_output)
    error_output->runtime_error(error);
}


void Statement::runtime_remark(std::string error) const
{
  if (error_output)
    error_output->runtime_remark(error);
}


std::map< std::string, std::string > convert_c_pairs(const char** attr)
{
  std::map< std::string, std::string > result;
  for (int i = 0; attr[i]; i+=2)
    result[attr[i]] = attr[i+1];
  return result;
}


void Output_Statement::transfer_output(Resource_Manager& rman, Set& into) const
{
  rman.swap_set(output, into);
  into.clear();
}
