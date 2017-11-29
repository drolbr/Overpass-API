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
#include "set_prop.h"


Set_Prop_Statement::Statement_Maker Set_Prop_Statement::statement_maker;
Set_Prop_Statement::Evaluator_Maker Set_Prop_Statement::evaluator_maker;


Statement* Set_Prop_Statement::Evaluator_Maker::create_evaluator(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if ((tree_context == Statement::generic || tree_context == Statement::in_convert)
      && tree_it->token == "!" && tree_it->rhs)
  {
    std::map< std::string, std::string > attributes;
    attributes["k"] = decode_json(tree_it.rhs()->token, error_output);
    attributes["keytype"] = "tag";
    return new Set_Prop_Statement(tree_it->line_col.first, attributes, global_settings);
  }

  if (tree_context != Statement::generic && tree_context != Statement::in_convert)
    return 0;

  if (!tree_it->lhs)
  {
    if (error_output)
      error_output->add_parse_error("To set a property it must have a name to the left of the equal sign",
          tree_it->line_col.first);
    return 0;
  }

  if (!tree_it->rhs)
  {
    if (error_output)
      error_output->add_parse_error("To set a property it must have a value to the right of the equal sign",
          tree_it->line_col.first);
    return 0;
  }

  std::map< std::string, std::string > attributes;
  if (tree_it.lhs()->token == "::")
  {
    if (tree_it.lhs()->rhs)
    {
      if (tree_it.lhs().rhs()->token == "id")
        attributes["keytype"] = "id";
      else if (tree_it.lhs().rhs()->token == "geom")
        attributes["keytype"] = "geometry";
      else if (error_output)
        error_output->add_parse_error("The only allowed special property is \"id\"", tree_it->line_col.first);
    }
    else if (tree_it.lhs()->lhs)
    {
      attributes["keytype"] = "generic";
      attributes["from"] = tree_it.lhs().lhs()->token;
    }
    else if (tree_context == Statement::in_convert)
      attributes["keytype"] = "generic";
    else if (error_output)
      error_output->add_parse_error("The generic tag set property can only be called from convert",
          tree_it->line_col.first);
  }
  else
  {
    if (tree_it.lhs()->lhs || tree_it.lhs()->rhs)
    {
      if (error_output)
        error_output->add_parse_error(std::string("if special character \"") + tree_it.lhs()->token
            + "\" is present in property name then the property name must be in quotes",
            tree_it->line_col.first);
      return 0;
    }
    attributes["k"] = decode_json(tree_it.lhs()->token, error_output);
    attributes["keytype"] = "tag";
  }
  Statement* result = new Set_Prop_Statement(tree_it->line_col.first, attributes, global_settings);
  if (result)
  {
    Statement* rhs = stmt_factory.create_evaluator(tree_it.rhs(),
        tree_context == Statement::in_convert ? Statement::elem_eval_possible : Statement::evaluator_expected);
    if (rhs)
      result->add_statement(rhs, "");
    else if (error_output)
      error_output->add_parse_error("Property assignment needs a right-hand-side value", tree_it->line_col.first);
  }
  return result;
}


Set_Prop_Statement::Set_Prop_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), key(0), mode(Set_Prop_Task::single_key), tag_value(0)
{
  std::map< std::string, std::string > attributes;

  attributes["k"] = "";
  attributes["keytype"] = "tag";
  attributes["from"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  if (attributes["keytype"] == "tag")
  {
    if (attributes["k"] != "")
      key = new std::string(attributes["k"]);
    else
      add_static_error("For the statement \"set-prop\" in mode \"keytype\"=\"tag\", "
          "the attribute \"k\" must be nonempty.");
  }
  else if (attributes["keytype"] == "id")
    mode = Set_Prop_Task::set_id;
  else if (attributes["keytype"] == "geometry")
    mode = Set_Prop_Task::set_geometry;
  else if (attributes["keytype"] == "generic")
    mode = Set_Prop_Task::generic;
  else
    add_static_error("For the attribute \"keytype\" of the element \"set-prop\""
        " the only allowed values are \"tag\", \"id\", \"geometry\", or \"generic\".");
    
  if (!attributes["from"].empty())
  {
    if (attributes["keytype"] == "generic")
      input = attributes["from"];
    else
      add_static_error("The attribute \"from\" of the element \"set-prop\""
          " can only be set to a non-empty value if \"keytype\" is \"generic\".");
  }
}


void Set_Prop_Statement::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (tag_value_ && !tag_value)
    tag_value = tag_value_;
  else if (tag_value)
    add_static_error("set-prop must have exactly one evaluator substatement.");
  else
    substatement_error(get_name(), statement);
}


std::string Set_Prop_Statement::dump_xml(const std::string& indent) const
{
  if (!tag_value)
    return indent + "<set-prop keytype=\"tag\" k=\"" + (key ? *key : "") + "\"/>\n";

  std::string attributes;
  if (mode == Set_Prop_Task::set_id)
    attributes = " keytype=\"id\"";
  if (mode == Set_Prop_Task::set_geometry)
    attributes = " keytype=\"geometry\"";
  else if (!key)
    attributes = " keytype=\"generic\"";
  else
    attributes = std::string(" keytype=\"tag\" k=\"") + (key ? *key : "") + "\"";
  
  if (!input.empty())
    attributes += " from=\"" + input + "\"";
  
  return indent + "<set-prop" + attributes + ">\n"
      + tag_value->dump_xml(indent + "  ")
      + indent + "</set-prop>\n";
}


std::string Set_Prop_Statement::dump_compact_ql(const std::string&) const
{
  std::string result;
  if (mode == Set_Prop_Task::set_id)
    result = "::id=";
  if (mode == Set_Prop_Task::set_geometry)
    result = "::geom=";
  else if (!key)
    result = input + "::=";
  else if (!tag_value)
    return std::string("!") + escape_cstr(key ? *key : "");
  else
    result = escape_cstr(key ? *key : "") + "=";
  
  if (tag_value)
    return result + tag_value->dump_compact_ql("");
  
  return result;
}



Requested_Context Set_Prop_Statement::request_context() const
{
  if (tag_value)
  {
    Requested_Context result = tag_value->request_context();
    if (input.empty())
      result.add_usage(Set_Usage::TAGS);
    else
      result.add_usage(input, Set_Usage::TAGS);

    return result;
  }

  Requested_Context result;
  result.add_usage(Set_Usage::TAGS);
  return result;
}


template< typename Index, typename Maybe_Attic >
void eval_elems(std::set< std::string >& existing_keys, Set_With_Context& input_set,
    const std::map< Index, std::vector< Maybe_Attic > >& elems,
    const std::vector< std::string >& otherwise_set_keys)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator idx_it = elems.begin();
      idx_it != elems.end(); ++idx_it)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator elem_it = idx_it->second.begin();
        elem_it != idx_it->second.end(); ++elem_it)
    {
      Element_With_Context< Maybe_Attic > data = input_set.get_context(idx_it->first, *elem_it);
      if (data.tags)
      {
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = data.tags->begin();
            it_keys != data.tags->end(); ++it_keys)
        {
          if (!std::binary_search(otherwise_set_keys.begin(), otherwise_set_keys.end(), it_keys->first))
            existing_keys.insert(it_keys->first);
        }
      }
    }
  }
}


void eval_elems(std::set< std::string >& existing_keys, Set_With_Context& input_set,
    const std::map< Uint31_Index, std::vector< Derived_Structure > >& elems,
    const std::vector< std::string >& otherwise_set_keys)
{
  for (std::map< Uint31_Index, std::vector< Derived_Structure > >::const_iterator idx_it = elems.begin();
      idx_it != elems.end(); ++idx_it)
  {
    for (std::vector< Derived_Structure >::const_iterator elem_it = idx_it->second.begin();
        elem_it != idx_it->second.end(); ++elem_it)
    {
      Element_With_Context< Derived_Skeleton > data = input_set.get_context(idx_it->first, *elem_it);
      if (data.tags)
      {
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = data.tags->begin();
            it_keys != data.tags->end(); ++it_keys)
        {
          if (!std::binary_search(otherwise_set_keys.begin(), otherwise_set_keys.end(), it_keys->first))
            existing_keys.insert(it_keys->first);
        }
      }
    }
  }
}


Set_Prop_Task* Set_Prop_Statement::get_task(
    Prepare_Task_Context& context, const std::vector< std::string >& otherwise_set_keys)
{
  if (input.empty())
  {
    if (mode == Set_Prop_Task::set_geometry)
      return new Set_Prop_Geometry_Task(tag_value ? tag_value->get_geometry_task(context) : 0);
    
    return new Set_Prop_Plain_Task(tag_value ? tag_value->get_string_task(context, key) : 0,
        key ? *key : "", mode);
  }
  
  Set_Prop_Generic_Task* result = new Set_Prop_Generic_Task();
  
  Set_With_Context* input_set = context.get_set(input);
  if (input_set && input_set->base)
  {
    std::set< std::string > existing_keys;
    
    eval_elems(existing_keys, *input_set, input_set->base->nodes, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->attic_nodes, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->ways, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->attic_ways, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->relations, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->attic_relations, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->areas, otherwise_set_keys);
    eval_elems(existing_keys, *input_set, input_set->base->deriveds, otherwise_set_keys);
    
    for (std::set< std::string >::const_iterator it = existing_keys.begin(); it != existing_keys.end(); ++it)
      result->add_key(*it, tag_value ? tag_value->get_string_task(context, &*it) : 0);
  }
  
  return result;
}


void Set_Prop_Plain_Task::process(Derived_Structure& result, bool& id_set) const
{
  if (!rhs)
    return;

  if (mode == single_key)
    result.tags.push_back(std::make_pair(key, rhs->eval(0)));
  else if (mode == set_id)
  {
    if (!id_set)
    {
      int64 id = 0;
      id_set |= try_int64(rhs->eval(0), id);
      if (id_set)
        result.id = Uint64(id);
    }
  }
}


template< typename Object >
void process(const std::string& key, Set_Prop_Task::Mode mode, Eval_Task* rhs,
    const Element_With_Context< Object >& data, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set)
{
  if (!rhs)
    return;

  if (mode == Set_Prop_Task::single_key)
    result.tags.push_back(std::make_pair(key, rhs->eval(data, &key)));
  else if (mode == Set_Prop_Task::generic)
  {
    if (data.tags)
    {
      std::vector< std::string > found_keys;
      for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = data.tags->begin();
          it_keys != data.tags->end(); ++it_keys)
        found_keys.push_back(it_keys->first);
      std::sort(found_keys.begin(), found_keys.end());
      found_keys.erase(std::unique(found_keys.begin(), found_keys.end()), found_keys.end());
      found_keys.erase(std::set_difference(found_keys.begin(), found_keys.end(),
          declared_keys.begin(), declared_keys.end(), found_keys.begin()), found_keys.end());

      for (std::vector< std::string >::const_iterator it_keys = found_keys.begin();
          it_keys != found_keys.end(); ++it_keys)
        result.tags.push_back(std::make_pair(*it_keys, rhs->eval(data, &*it_keys)));
    }
  }
  else if (mode == Set_Prop_Task::set_id)
  {
    if (!id_set)
    {
      int64 id = 0;
      id_set |= try_int64(rhs->eval(data, 0), id);
      if (id_set)
        result.id = Uint64(id);
    }
  }
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Node_Skeleton>& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Attic< Node_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Way_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Attic< Way_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Relation_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Attic< Relation_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Area_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Plain_Task::process(const Element_With_Context< Derived_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, data, declared_keys, result, id_set);
}


void Set_Prop_Generic_Task::add_key(const std::string& key, Eval_Task* task)
{
  keys.push_back(key);
  rhs.push_back(task);
}


void Set_Prop_Generic_Task::process(Derived_Structure& result, bool& id_set) const
{
  for (unsigned int i = 0; i < keys.size(); ++i)
    result.tags.push_back(std::make_pair(keys[i], rhs[i]->eval(&keys[i])));
}


template< typename Object >
void process_generic(const Owning_Array< Eval_Task* >& rhs, const std::vector< std::string >& keys,
    const Element_With_Context< Object >& data, Derived_Structure& result)
{
  for (unsigned int i = 0; i < keys.size(); ++i)
    result.tags.push_back(std::make_pair(keys[i], rhs[i]->eval(data, &keys[i])));
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Node_Skeleton>& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Attic< Node_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Way_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Attic< Way_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Relation_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Attic< Relation_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Area_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Generic_Task::process(const Element_With_Context< Derived_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  ::process_generic(rhs, keys, data, result);
}


void Set_Prop_Geometry_Task::process(Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval());
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Node_Skeleton>& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Attic< Node_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Way_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Attic< Way_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Relation_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Attic< Relation_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Area_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}


void Set_Prop_Geometry_Task::process(const Element_With_Context< Derived_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const
{
  if (rhs)
    result.acquire_geometry(rhs->eval(data));
}
