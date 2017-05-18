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
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, false)
      || !assert_element_in_context(error_output, tree_it, tree_context))
    return 0;
  
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
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, false)
      || !assert_element_in_context(error_output, tree_it, tree_context))
    return 0;
  
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
  if (!assert_element_in_context(error_output, tree_it, tree_context))
    return 0;

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
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, true)
      || !assert_element_in_context(error_output, tree_it, tree_context))
    return 0;
  
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


Evaluator_Version::Statement_Maker Evaluator_Version::statement_maker;


Statement* Evaluator_Version::Statement_Maker::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, false)
      || !assert_element_in_context(error_output, tree_it, tree_context))
    return 0;
  
  return new Evaluator_Version(tree_it->line_col.first, std::map< std::string, std::string >(), global_settings);
}


Evaluator_Version::Evaluator_Version
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  eval_attributes_array(get_name(), attributes, input_attributes);
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
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !assert_element_in_context(error_output, tree_it, tree_context))
    return 0;
  
  std::map< std::string, std::string > attributes;
  
  const std::string* func_name = tree_it.function_name();
  attributes["type"] = func_name->size() > 6 ? func_name->substr(6) : "";

  if (*func_name == "count_by_role" || *func_name == "count_distinct_by_role")
  {
    if (tree_it->rhs)
    {
      if (tree_it.rhs()->token == "," && tree_it.rhs()->lhs && tree_it.rhs()->rhs)
      {
        if (!tree_it.rhs().lhs()->lhs && !tree_it.rhs().lhs()->rhs)
          attributes["role"] = decode_json(tree_it.rhs().lhs()->token, error_output);
        else
        {
          if (error_output)
            error_output->add_parse_error(tree_it.lhs()->token 
                + "() needs a single literal expression as argument for the wanted role", tree_it->line_col.first);
          return 0;
        }
        
        if (tree_it.rhs().rhs()->lhs || tree_it.rhs().rhs()->rhs
            || (tree_it.rhs().rhs()->token != "nodes" && tree_it.rhs().rhs()->token != "ways"
              && tree_it.rhs().rhs()->token != "relations" && tree_it.rhs().rhs()->token != "rels"))
        {
          if (error_output)
            error_output->add_parse_error(tree_it.lhs()->token
                + "(...) can only have one of the words nodes, ways, or relations as second argument",
                tree_it->line_col.first);
          return 0;
        }
        else
          attributes["members_type"] = tree_it.rhs().rhs()->token;
      }
      else if (!tree_it.rhs()->lhs && !tree_it.rhs()->rhs)
        attributes["role"] = decode_json(tree_it.rhs()->token, error_output);
      else
      {
        if (error_output)
          error_output->add_parse_error(tree_it.lhs()->token 
              + "() needs a single literal expression as argument for the wanted role", tree_it->line_col.first);
        return 0;
      }
    }
    else
    {
      if (error_output)
        error_output->add_parse_error(tree_it.lhs()->token 
            + "() must have the name of the wanted role as argument", tree_it->line_col.first);
      return 0;
    }
  }
  else if (*func_name == "count_members" || *func_name == "count_distinct_members")
  {
    if (tree_it->rhs)
    {
      if (tree_it.rhs()->lhs || tree_it.rhs()->rhs
          || (tree_it.rhs()->token != "nodes" && tree_it.rhs()->token != "ways"
            && tree_it.rhs()->token != "relations" && tree_it.rhs()->token != "rels"))
      {
        if (error_output)
          error_output->add_parse_error(tree_it.lhs()->token
              + "(...) can only have one of the words nodes, ways, or relations as an argument",
              tree_it->line_col.first);
        return 0;
      }
      else
        attributes["members_type"] = tree_it.rhs()->token;
    }
  }
  else
  {
    if (!tree_it.assert_has_arguments(error_output, false))
      return 0;
  }

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


std::string Evaluator_Properties_Count::to_string(Evaluator_Properties_Count::Members_Type type_to_count)
{
  if (type_to_count == nodes)
    return "nodes";
  if (type_to_count == ways)
    return "ways";
  if (type_to_count == relations)
    return "relations";

  return "all";
}


Evaluator_Properties_Count::Evaluator_Properties_Count
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_), to_count(nothing), type_to_count(all)
{
  std::map< std::string, std::string > attributes;

  attributes["type"] = "";
  attributes["role"] = "";
  attributes["members_type"] = "all";

  eval_attributes_array(get_name(), attributes, input_attributes);

  if (attributes["type"] == "tags")
    to_count = tags;
  else if (attributes["type"] == "members")
    to_count = members;
  else if (attributes["type"] == "distinct_members")
    to_count = distinct_members;
  else if (attributes["type"] == "by_role")
    to_count = by_role;
  else if (attributes["type"] == "distinct_by_role")
    to_count = distinct_by_role;
  else
  {
    add_static_error("For the attribute \"type\" of the element \"eval-prop-count\""
        " the only allowed values are \"tags\", \"members\", \"distinct_members\","
        " \"by_role\", or \"distinct_by_role\" strings.");
  }
  
  if (attributes["members_type"] == "all")
    type_to_count = all;
  else if (attributes["members_type"] == "nodes")
    type_to_count = nodes;
  else if (attributes["members_type"] == "ways")
    type_to_count = ways;
  else if (attributes["members_type"] == "relations" || attributes["members_type"] == "rels")
    type_to_count = relations;
  else
  {
    add_static_error("For the attribute \"members_type\" of the element \"eval-prop-count\""
        " the only allowed values are \"all\", \"nodes\", \"ways\", or \"relations\" strings.");
  }
  
  role = attributes["role"];
}


Requested_Context Evaluator_Properties_Count::request_context() const
{
  if (to_count == Evaluator_Properties_Count::tags)
    return Requested_Context().add_usage(Set_Usage::TAGS);
  else if (to_count == Evaluator_Properties_Count::members || to_count == Evaluator_Properties_Count::distinct_members)
    return Requested_Context().add_usage(Set_Usage::SKELETON);
  else if (to_count == Evaluator_Properties_Count::by_role || to_count == Evaluator_Properties_Count::distinct_by_role)
    return Requested_Context().add_usage(Set_Usage::SKELETON).add_role_names();

  return Requested_Context();
}


Eval_Task* Evaluator_Properties_Count::get_task(const Prepare_Task_Context& context)
{
  if (to_count == Evaluator_Properties_Count::by_role || to_count == Evaluator_Properties_Count::distinct_by_role)
    return new Prop_Count_Eval_Task(to_count, type_to_count, context.get_role_id(role));
  
  return new Prop_Count_Eval_Task(to_count, type_to_count);
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Attic< Node_Skeleton > >& data,
    const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data,
    const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members
      && type_to_count == Evaluator_Properties_Count::all && data.object)
    return to_string(data.object->nds.size());
  else if (to_count == Evaluator_Properties_Count::distinct_members
      && type_to_count == Evaluator_Properties_Count::all && data.object)
  {
    std::vector< Node::Id_Type > distinct = data.object->nds;
    std::sort(distinct.begin(), distinct.end());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data,
    const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members
      && type_to_count == Evaluator_Properties_Count::all && data.object)
    return to_string(data.object->nds.size());
  else if (to_count == Evaluator_Properties_Count::distinct_members
      && type_to_count == Evaluator_Properties_Count::all && data.object)
  {
    std::vector< Node::Id_Type > distinct = data.object->nds;
    std::sort(distinct.begin(), distinct.end());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


bool matches_criterion(const Relation_Entry& member, Evaluator_Properties_Count::Objects to_count,
    Evaluator_Properties_Count::Members_Type type_to_count, uint32 role_id)
{
  if (type_to_count != Evaluator_Properties_Count::all)
  {
    if (type_to_count == Evaluator_Properties_Count::nodes && member.type != Relation_Entry::NODE)
      return false;
    if (type_to_count == Evaluator_Properties_Count::ways && member.type != Relation_Entry::WAY)
      return false;
    if (type_to_count == Evaluator_Properties_Count::relations && member.type != Relation_Entry::RELATION)
      return false;
  }
  
  if (member.role != role_id &&
      (to_count == Evaluator_Properties_Count::by_role || to_count == Evaluator_Properties_Count::distinct_by_role))
    return false;
  
  return true;
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


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data,
    const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members || to_count == Evaluator_Properties_Count::by_role)
  {
    if (!data.object)
      return "0";
    
    if (to_count == Evaluator_Properties_Count::members && type_to_count == Evaluator_Properties_Count::all)
      return to_string(data.object->members.size());
    
    uint counter = 0;
    for (std::vector< Relation_Entry >::const_iterator it = data.object->members.begin(); it != data.object->members.end(); ++it)
    {
      if (matches_criterion(*it, to_count, type_to_count, role_id))
        ++counter;
    }
    return to_string(counter);
  }
  else if (to_count == Evaluator_Properties_Count::distinct_members
      || to_count == Evaluator_Properties_Count::distinct_by_role)
  {
    if (!data.object)
      return "0";
    
    std::vector< Relation_Entry > distinct = data.object->members;
    
    if (to_count != Evaluator_Properties_Count::distinct_members || type_to_count != Evaluator_Properties_Count::all)
    {
      std::vector< Relation_Entry >::size_type from = 0;
      std::vector< Relation_Entry >::size_type to = 0;
      for (; from < distinct.size(); ++from)
      {
        if (matches_criterion(distinct[from], to_count, type_to_count, role_id))
          distinct[to++] = distinct[from];
      }
      distinct.resize(to);
    }
    
    std::sort(distinct.begin(), distinct.end(), Relation_Member_Comparer());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data,
    const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::members || to_count == Evaluator_Properties_Count::by_role)
  {
    if (!data.object)
      return "0";
    
    if (to_count == Evaluator_Properties_Count::members && type_to_count == Evaluator_Properties_Count::all)
      return to_string(data.object->members.size());
    
    uint counter = 0;
    for (std::vector< Relation_Entry >::const_iterator it = data.object->members.begin(); it != data.object->members.end(); ++it)
    {
      if (matches_criterion(*it, to_count, type_to_count, role_id))
        ++counter;
    }
    return to_string(counter);
  }
  else if (to_count == Evaluator_Properties_Count::distinct_members
      || to_count == Evaluator_Properties_Count::distinct_by_role)
  {
    if (!data.object)
      return "0";
    
    std::vector< Relation_Entry > distinct = data.object->members;
    
    if (to_count != Evaluator_Properties_Count::distinct_members || type_to_count != Evaluator_Properties_Count::all)
    {
      std::vector< Relation_Entry >::size_type from = 0;
      std::vector< Relation_Entry >::size_type to = 0;
      for (; from < distinct.size(); ++from)
      {
        if (matches_criterion(distinct[from], to_count, type_to_count, role_id))
          distinct[to++] = distinct[from];
      }
      distinct.resize(to);
    }
    
    std::sort(distinct.begin(), distinct.end(), Relation_Member_Comparer());
    return to_string(std::distance(distinct.begin(), std::unique(distinct.begin(), distinct.end())));
  }
  else if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}


std::string Prop_Count_Eval_Task::eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
{
  if (to_count == Evaluator_Properties_Count::tags && data.tags)
    return to_string(data.tags->size());
  return "0";
}
