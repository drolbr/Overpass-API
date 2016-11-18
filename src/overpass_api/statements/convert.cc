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
#include "convert.h"
#include "tag_value.h"


Generic_Statement_Maker< Convert_Statement > Convert_Statement::statement_maker("convert");


Convert_Statement::Convert_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), id_evaluator(0), multi_evaluator(0)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  
  input = attributes["from"];
  
  if (attributes["type"] == "")
    add_static_error("The attribute type must be set to a nonempty string.");
  type = attributes["type"];
}


Convert_Statement::~Convert_Statement()
{
}


void Convert_Statement::add_statement(Statement* statement, std::string text)
{
  Set_Tag_Statement* set_tag = dynamic_cast< Set_Tag_Statement* >(statement);
  if (set_tag)
  {
    if (set_tag->get_key())
    {
      for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      {
        if ((*it)->get_key() && *(*it)->get_key() == *set_tag->get_key())
          add_static_error(std::string("A key cannot be added twice to an element: \"") + *set_tag->get_key() + '\"');
      }
    }
    else if (set_tag->should_set_id())
    {
      if (!id_evaluator)
        id_evaluator = set_tag;
      else
        add_static_error("A convert statement can have at most one set-tag statement of subtype setting the id.");
    }
    else if (!multi_evaluator)
      multi_evaluator = set_tag;
    else
      add_static_error("A convert statement can have at most one any-key set-tag statement.");
    evaluators.push_back(set_tag);
  }
  else
    substatement_error(get_name(), statement);
}


template< typename Index, typename Maybe_Attic, typename Object >
void generate_elems(const std::string& set_name,
    const std::map< Index, std::vector< Maybe_Attic > >& items, Tag_Store< Index, Object >* tag_store,
    std::vector< Set_Tag_Statement* >& evaluators, Set_Tag_Statement* multi_evaluator,
    const std::vector< std::string >& declared_keys, Set& into, Resource_Manager& rman, const std::string& type)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const std::vector< std::pair< std::string, std::string > >* tags =
          tag_store ? tag_store->get(it_idx->first, *it_elem) : 0;
      std::vector< std::pair< std::string, std::string > > result_tags;
      
      int64 id = 0;
      bool id_fixed = false;
  
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if (!(*it_evals)->has_value())
          continue;
        else if ((*it_evals)->get_key())
          result_tags.push_back(std::make_pair(*(*it_evals)->get_key(), (*it_evals)->eval(&*it_elem, tags)));
        else if ((*it_evals)->should_set_id())
          id_fixed = try_int64((*it_evals)->eval(&*it_elem, tags), id);
      }
      
      if (multi_evaluator && tags)
      {
        std::vector< std::string > found_keys;
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = tags->begin();
            it_keys != tags->end(); ++it_keys)
          found_keys.push_back(it_keys->first);
        std::sort(found_keys.begin(), found_keys.end());
        found_keys.erase(std::unique(found_keys.begin(), found_keys.end()), found_keys.end());
        found_keys.erase(std::set_difference(found_keys.begin(), found_keys.end(),
            declared_keys.begin(), declared_keys.end(), found_keys.begin()), found_keys.end());
        
        for (std::vector< std::string >::const_iterator it_keys = found_keys.begin();
            it_keys != found_keys.end(); ++it_keys)
          result_tags.push_back(std::make_pair(*it_keys, multi_evaluator->eval(&*it_elem, tags, &*it_keys)));
      }

      into.deriveds[Uint31_Index(0u)].push_back(Derived_Structure(
          type, id_fixed ? id : rman.get_global_settings().dispense_derived_id(), result_tags));
    }
  }
}


void Convert_Statement::execute(Resource_Manager& rman)
{
  std::pair< std::vector< Set_Usage >, uint > set_usage;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    set_usage = union_usage(set_usage, (*it)->used_sets());
  
  std::vector< Set_Usage >::size_type input_pos = 0;
  while (input_pos < set_usage.first.size() && set_usage.first[input_pos].set_name != input)
    ++input_pos;
  
  if (input_pos == set_usage.first.size())
    set_usage.first.push_back(Set_Usage(input, set_usage.second));
  else
    set_usage.first[input_pos].usage |= set_usage.second;
  
  Array< Set_With_Context > set_contexts(set_usage.first.size());
  for (std::vector< Set_Usage >::iterator it = set_usage.first.begin(); it != set_usage.first.end(); ++it)
  {
    Set_With_Context& context = set_contexts.ptr[std::distance(set_usage.first.begin(), it)];
    context.name = it->set_name;
    
    std::map< std::string, Set >::const_iterator mit(rman.sets().find(context.name));
    if (mit != rman.sets().end())
      context.prefetch(*it, mit->second, *rman.get_transaction());
    
    for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      (*it)->prefetch(context);
  }

  std::vector< std::string > declared_keys;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
  {
    if ((*it)->get_key())
      declared_keys.push_back(*(*it)->get_key());
  }
  std::sort(declared_keys.begin(), declared_keys.end());
  declared_keys.erase(std::unique(declared_keys.begin(), declared_keys.end()), declared_keys.end());

  Set into;

  if (set_contexts.ptr[input_pos].base)
  {
    Set_With_Context& context = set_contexts.ptr[input_pos];
    
    generate_elems< Uint32_Index, Node_Skeleton >(
        context.name, context.base->nodes, context.tag_store_nodes, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
    if (rman.get_desired_timestamp() != NOW)
      generate_elems< Uint32_Index, Attic< Node_Skeleton > >(
          context.name, context.base->attic_nodes, context.tag_store_attic_nodes, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Way_Skeleton >(
        context.name, context.base->ways, context.tag_store_ways, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
    if (rman.get_desired_timestamp() != NOW)
      generate_elems< Uint31_Index, Attic< Way_Skeleton > >(
          context.name, context.base->attic_ways, context.tag_store_attic_ways, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Relation_Skeleton >(
        context.name, context.base->relations, context.tag_store_relations, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
    if (rman.get_desired_timestamp() != NOW)
      generate_elems< Uint31_Index, Attic< Relation_Skeleton > >(
          context.name, context.base->attic_relations, context.tag_store_attic_relations, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    if (!context.base->areas.empty())
      generate_elems< Uint31_Index, Area_Skeleton >(
          context.name, context.base->areas, context.tag_store_areas, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Derived_Structure >(
        context.name, context.base->deriveds, context.tag_store_deriveds, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
  }
      
  for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
      it_evals != evaluators.end(); ++it_evals)
    (*it_evals)->clear();
    
  transfer_output(rman, into);
  rman.health_check(*this);
}
