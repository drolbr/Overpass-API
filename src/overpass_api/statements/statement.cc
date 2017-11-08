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


std::map< std::string, Statement::Criterion_Maker* >& Statement::maker_by_ql_criterion()
{
  static std::map< std::string, Statement::Criterion_Maker* > makers;
  return makers;
}


std::map< std::string, std::vector< Statement::Evaluator_Maker* > >& Statement::maker_by_token()
{
  static std::map< std::string, std::vector< Statement::Evaluator_Maker* > > makers;
  return makers;
}


std::map< std::string, std::vector< Statement::Evaluator_Maker* > >& Statement::maker_by_func_name()
{
  static std::map< std::string, std::vector< Statement::Evaluator_Maker* > > makers;
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
    const std::vector< Statement::Evaluator_Maker* >& makers, Statement::Factory& stmt_factory,
    Parsed_Query& global_settings, Error_Output* error_output)
{
  Statement* statement = 0;

  std::vector< Statement::Evaluator_Maker* >::const_iterator maker_it = makers.begin();

  while (!statement && maker_it != makers.end())
  {
    statement = (*maker_it)->create_evaluator(tree_it, tree_context, stmt_factory, global_settings, error_output);
    ++maker_it;
  }
  while (maker_it != makers.end())
  {
    Statement* bis = (*maker_it)->create_evaluator(tree_it, tree_context, stmt_factory, global_settings, error_output);
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
      std::map< std::string, std::vector< Statement::Evaluator_Maker* > >::iterator all_it =
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
      std::map< std::string, std::vector< Statement::Evaluator_Maker* > >::iterator all_it =
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
    std::map< std::string, std::vector< Statement::Evaluator_Maker* > >::iterator all_it =
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
  std::string criterion_s = criterion->token;
  
  if (!criterion_s.empty()
      && (isdigit(criterion_s[0])
          || (criterion_s[0] == '-' && criterion_s.size() > 1 && isdigit(criterion_s[1]))))
    criterion_s = (tree_it->token == "," ? "bbox" : "id");
  
  std::map< std::string, Criterion_Maker* >::iterator it = Statement::maker_by_ql_criterion().find(criterion_s);
  if (it != Statement::maker_by_ql_criterion().end() && it->second)
  {
    can_standalone = it->second->can_standalone(type);
    return it->second->create_criterion(tree_it, type, into, *this, global_settings, error_output);
  }

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


void Output_Statement::transfer_output(Resource_Manager& rman, Diff_Set& into) const
{
  rman.swap_diff_set(output, into);
  into.clear();
}
