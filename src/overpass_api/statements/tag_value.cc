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


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "tag_value.h"


Evaluator_Fixed::Statement_Maker Evaluator_Fixed::statement_maker;


Statement* Evaluator_Fixed::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_it->lhs || tree_it->rhs)
    return 0;

  int64 value_l = 0;
  double value_d = 0;
  if (tree_it->token.empty() ||
      (tree_it->token[0] != '\'' && tree_it->token[0] != '\"'
      && !try_int64(tree_it->token, value_l) && !try_double(tree_it->token, value_d)))
  {
    if (error_output)
      error_output->add_parse_error(std::string("Put quotation marks around \"") + tree_it->token
          + "\" if it should be a constant and not a tag evaluation.", tree_it->line_col.first);
    return 0;
  }

  std::map< std::string, std::string > attributes;
  attributes["v"] = decode_json(tree_it->token, error_output);
  return new Evaluator_Fixed(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Fixed::Evaluator_Fixed
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["v"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  value = attributes["v"];
}


std::string Evaluator_Fixed::dump_compact_ql(const std::string&) const
{
  int64 value_l = 0;
  if (try_int64(value, value_l))
    return to_string(value_l);

  double value_d = 0;
  if (try_double(value, value_d))
    return to_string(value_d);

  return std::string("\"") + escape_cstr(value) + "\"";
}


//-----------------------------------------------------------------------------


Evaluator_Id::Statement_Maker Evaluator_Id::statement_maker;


Statement* Evaluator_Id::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_context != Statement::elem_eval_possible)
  {
    if (error_output)
      error_output->add_parse_error("id() must be called in a context where it can evaluate an element",
          tree_it->line_col.first);
    return 0;
  }

  if (tree_it->token != "(")
  {
    if (error_output)
      error_output->add_parse_error("id() cannot have an input set", tree_it->line_col.first);
    return 0;
  }
  if (tree_it->rhs)
  {
    if (error_output)
      error_output->add_parse_error("id() cannot have an argument", tree_it->line_col.first);
    return 0;
  }
  std::map< std::string, std::string > attributes;
  return new Evaluator_Id(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Id::Evaluator_Id
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Evaluator_Type::Statement_Maker Evaluator_Type::statement_maker;


Statement* Evaluator_Type::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_context != Statement::elem_eval_possible)
  {
    if (error_output)
      error_output->add_parse_error("type() must be called in a context where it can evaluate an element",
          tree_it->line_col.first);
    return 0;
  }

  if (tree_it->token != "(")
  {
    if (error_output)
      error_output->add_parse_error("type() cannot have an input set", tree_it->line_col.first);
    return 0;
  }
  if (tree_it->rhs)
  {
    if (error_output)
      error_output->add_parse_error("type() cannot have an argument", tree_it->line_col.first);
    return 0;
  }
  std::map< std::string, std::string > attributes;
  return new Evaluator_Type(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Type::Evaluator_Type
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


std::string find_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key)
{
  if (!tags)
    return "";

  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      return it->second;
  }

  return "";
}


Evaluator_Value::Statement_Maker Evaluator_Value::statement_maker;


Statement* Evaluator_Value::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_context != Statement::elem_eval_possible)
  {
    if (error_output)
      error_output->add_parse_error("Operator \"[\" must be called in a context where it can evaluate an element",
          tree_it->line_col.first);
    return 0;
  }

  if (!tree_it->lhs || tree_it.lhs()->lhs || tree_it.lhs()->rhs || tree_it.lhs()->token != "t")
  {
    if (error_output)
      error_output->add_parse_error("Tag evaulation needs function name \"t\" before the left bracket",
          tree_it->line_col.first);
    return 0;
  }
  if (!tree_it->rhs)
  {
    if (error_output)
      error_output->add_parse_error("Operator \"[\" needs a tag key as argument", tree_it->line_col.first);
    return 0;
  }
  if (tree_it.rhs()->lhs || tree_it.rhs()->rhs)
  {
    if (error_output)
      error_output->add_parse_error("Operator \"[\" needs a single string (the tag key) as argument", tree_it->line_col.first);
    return 0;
  }
  std::map< std::string, std::string > attributes;
  attributes["k"] = decode_json(tree_it.rhs()->token, error_output);
  return new Evaluator_Value(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Value::Evaluator_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["k"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  key = attributes["k"];
}


//-----------------------------------------------------------------------------


std::string exists_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key)
{
  if (!tags)
    return "0";

  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      return "1";
  }

  return "0";
}


Evaluator_Is_Tag::Statement_Maker Evaluator_Is_Tag::statement_maker;


Statement* Evaluator_Is_Tag::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_context != Statement::elem_eval_possible)
  {
    if (error_output)
      error_output->add_parse_error("is_tag(...) must be called in a context where it can evaluate an element",
          tree_it->line_col.first);
    return 0;
  }

  if (tree_it->token != "(")
  {
    if (error_output)
      error_output->add_parse_error("is_tag(...) cannot have an input set", tree_it->line_col.first);
    return 0;
  }
  if (!tree_it->rhs)
  {
    if (error_output)
      error_output->add_parse_error("is_tag(key) needs a string as argument", tree_it->line_col.first);
    return 0;
  }
  if (tree_it.rhs()->lhs || tree_it.rhs()->rhs)
  {
    if (error_output)
      error_output->add_parse_error("is_tag(key) needs a simple string as argument", tree_it->line_col.first);
    return 0;
  }
  std::map< std::string, std::string > attributes;
  attributes["k"] = decode_json(tree_it.rhs()->token, error_output);
  return new Evaluator_Is_Tag(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Is_Tag::Evaluator_Is_Tag
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["k"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  key = attributes["k"];
}


//-----------------------------------------------------------------------------


Evaluator_Generic::Statement_Maker Evaluator_Generic::statement_maker;


Statement* Evaluator_Generic::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_it->lhs || tree_it->rhs)
    return 0;
  std::map< std::string, std::string > attributes;
  return new Evaluator_Generic(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Generic::Evaluator_Generic
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Evaluator_Properties_Count::Statement_Maker Evaluator_Properties_Count::statement_maker;


Statement* Evaluator_Properties_Count::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  std::map< std::string, std::string > attributes;

  if (tree_it->token != "(")
  {
    if (error_output && tree_it->rhs && tree_it.rhs()->lhs)
      error_output->add_parse_error(tree_it.rhs().lhs()->token + "(...) cannot have an input set",
          tree_it->line_col.first);
    return 0;
  }

  if (!tree_it->lhs)
    return 0;
  if (tree_it->rhs)
  {
    if (error_output)
      error_output->add_parse_error(tree_it.rhs().lhs()->token +  "() cannot have an argument", tree_it->line_col.first);
    return 0;
  }

  attributes["type"] = tree_it.lhs()->token.size() > 6 ? tree_it.lhs()->token.substr(6) : "";

  return new Evaluator_Properties_Count(tree_it->line_col.first, attributes, global_settings);
}


std::string Evaluator_Properties_Count::to_string(Evaluator_Properties_Count::Objects objects)
{
  if (objects == tags)
    return "tags";
  if (objects == members)
    return "members";
  if (objects == distinct_members)
    return "distinct_members";
  if (objects == by_role)
    return "by_role";
  if (objects == distinct_by_role)
    return "distinct_by_role";

  return "nothing";
}


Evaluator_Properties_Count::Evaluator_Properties_Count
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["type"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  if (attributes["type"] == "tags")
    to_count = tags;
  else if (attributes["type"] == "members")
    to_count = members;
  else if (attributes["type"] == "distinct_members")
    to_count = distinct_members;
  else
  {
    add_static_error("For the attribute \"type\" of the element \"eval-prop-count\""
        " the only allowed values are \"tags\", \"members\", \"distinct_members\","
        " \"by_role\", or \"distinct_by_role\" strings.");
  }
}


Requested_Context Evaluator_Properties_Count::request_context() const
{
  if (to_count == Evaluator_Properties_Count::tags)
    return Requested_Context().add_usage(Set_Usage::TAGS);
  else if (to_count == Evaluator_Properties_Count::members || to_count == Evaluator_Properties_Count::distinct_members
      || to_count == Evaluator_Properties_Count::by_role || to_count == Evaluator_Properties_Count::distinct_by_role)
    return Requested_Context().add_usage(Set_Usage::SKELETON);

  return Requested_Context();
}


Eval_Task* Evaluator_Properties_Count::get_task(const Prepare_Task_Context& context)
{
  return new Prop_Count_Eval_Task(to_count);
}


std::string Prop_Count_Eval_Task::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members && elem)
    return to_string(elem->nds.size());
  else if (to_count == Evaluator_Properties_Count::distinct_members && elem)
  {
    std::vector< Node::Id_Type > distinct = elem->nds;
    std::sort(distinct.begin(), distinct.end());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members && elem)
    return to_string(elem->nds.size());
  else if (to_count == Evaluator_Properties_Count::distinct_members && elem)
  {
    std::vector< Node::Id_Type > distinct = elem->nds;
    std::sort(distinct.begin(), distinct.end());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


struct Relation_Member_Comparer
{
  bool operator()(const Relation_Entry& lhs, const Relation_Entry& rhs) const
  {
    if (lhs.type != rhs.type)
      return lhs.type < rhs.type;
    
    return lhs.ref < rhs.ref;
  }
};


std::string Prop_Count_Eval_Task::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members && elem)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Properties_Count::distinct_members && elem)
  {
    std::vector< Relation_Entry > distinct = elem->members;
    std::sort(distinct.begin(), distinct.end(), Relation_Member_Comparer());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::by_role && elem)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Properties_Count::distinct_by_role && elem)
  {
    std::vector< Relation_Entry > distinct = elem->members;
    std::sort(distinct.begin(), distinct.end(), Relation_Member_Comparer());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members && elem)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Properties_Count::distinct_members && elem)
  {
    std::vector< Relation_Entry > distinct = elem->members;
    std::sort(distinct.begin(), distinct.end(), Relation_Member_Comparer());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::by_role && elem)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Properties_Count::distinct_by_role && elem)
  {
    std::vector< Relation_Entry > distinct = elem->members;
    std::sort(distinct.begin(), distinct.end(), Relation_Member_Comparer());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && tags)
    return to_string(tags->size());
  return "0";
}
