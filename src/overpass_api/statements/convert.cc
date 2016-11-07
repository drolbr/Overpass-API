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


template< typename Index, typename Object >
void generate_elems(Transaction& transaction, const std::string& set_name,
    const std::map< Index, std::vector< Object > >& items,
    std::vector< Set_Tag_Statement* >& evaluators, Set_Tag_Statement* multi_evaluator,
    const std::vector< std::string >& declared_keys, Set& into, Resource_Manager& rman, const std::string& type)
{
  Tag_Store< Index, Object > tag_store(transaction);  
  tag_store.prefetch_all(items);
  
  for (typename std::map< Index, std::vector< Object > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const std::vector< std::pair< std::string, std::string > >* tags =
          tag_store.get(it_idx->first, *it_elem);
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if ((*it_evals)->get_tag_value() && (*it_evals)->get_tag_value()->needs_tags(set_name))
          (*it_evals)->get_tag_value()->tag_notice(set_name, *it_elem, tags);
      }
      
      if (!tags)
        continue;
      if (multi_evaluator)
      {
        std::vector< std::string > found_keys;
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = tags->begin();
            it_keys != tags->end(); ++it_keys)
          found_keys.push_back(it_keys->first);
        std::sort(found_keys.begin(), found_keys.end());
        found_keys.erase(std::unique(found_keys.begin(), found_keys.end()), found_keys.end());
        found_keys.erase(std::set_difference(found_keys.begin(), found_keys.end(),
            declared_keys.begin(), declared_keys.end(), found_keys.begin()), found_keys.end());
        multi_evaluator->set_keys(found_keys);
      }
      
      int64 id = 0;
      bool id_fixed = false;
  
      std::vector< std::pair< std::string, std::string > > result_tags;
      for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      {
        if (!(*it)->get_tag_value())
          continue;
    
        if ((*it)->should_set_id())
          id_fixed = try_int64((*it)->eval(rman.sets(), 0), id);
        else if ((*it)->get_key())
          result_tags.push_back(std::make_pair(*(*it)->get_key(), (*it)->eval(rman.sets(), 0)));
        else
        {
          const std::vector< std::string >& keys = *(*it)->get_keys();
          for (std::vector< std::string >::const_iterator it_keys = keys.begin(); it_keys != keys.end(); ++it_keys)
            result_tags.push_back(std::make_pair(*it_keys, (*it)->eval(rman.sets(), &*it_keys)));
        }
        (*it)->get_tag_value()->clear();
      }

      into.deriveds[Uint31_Index(0u)].push_back(Derived_Structure(
          type, id_fixed ? id : rman.get_global_settings().dispense_derived_id(), result_tags));
    }
  }
}


template< typename Index, typename Object >
void generate_elems(Transaction& transaction, const std::string& set_name,
    const std::map< Index, std::vector< Attic< Object > > >& items,
    std::vector< Set_Tag_Statement* >& evaluators, Set_Tag_Statement* multi_evaluator,
    const std::vector< std::string >& declared_keys, Set& into, Resource_Manager& rman, const std::string& type)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);
  
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const std::vector< std::pair< std::string, std::string > >* tags =
          tag_store.get(it_idx->first, *it_elem);
      if (!tags)
        continue;
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if ((*it_evals)->get_tag_value() && (*it_evals)->get_tag_value()->needs_tags(set_name))
          (*it_evals)->get_tag_value()->tag_notice(set_name, *it_elem, tags);
      }
      
      if (multi_evaluator)
      {
        std::vector< std::string > found_keys;
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = tags->begin();
            it_keys != tags->end(); ++it_keys)
          found_keys.push_back(it_keys->first);
        std::sort(found_keys.begin(), found_keys.end());
        found_keys.erase(std::unique(found_keys.begin(), found_keys.end()), found_keys.end());
        found_keys.erase(std::set_difference(found_keys.begin(), found_keys.end(),
            declared_keys.begin(), declared_keys.end(), found_keys.begin()), found_keys.end());
        multi_evaluator->set_keys(found_keys);
      }
      
      int64 id = 0;
      bool id_fixed = false;
  
      std::vector< std::pair< std::string, std::string > > result_tags;
      for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      {
        if (!(*it)->get_tag_value())
          continue;
    
        if ((*it)->should_set_id())
          id_fixed = try_int64((*it)->eval(rman.sets(), 0), id);
        else if ((*it)->get_key())
          result_tags.push_back(std::make_pair(*(*it)->get_key(), (*it)->eval(rman.sets(), 0)));
        else
        {
          const std::vector< std::string >& keys = *(*it)->get_keys();
          for (std::vector< std::string >::const_iterator it_keys = keys.begin(); it_keys != keys.end(); ++it_keys)
            result_tags.push_back(std::make_pair(*it_keys, (*it)->eval(rman.sets(), &*it_keys)));
        }
        (*it)->get_tag_value()->clear();
      }

      into.deriveds[Uint31_Index(0u)].push_back(Derived_Structure(
          type, id_fixed ? id : rman.get_global_settings().dispense_derived_id(), result_tags));
    }
  } 
}


void Convert_Statement::execute(Resource_Manager& rman)
{
  std::vector< std::string > declared_keys;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
  {
    if ((*it)->get_key())
      declared_keys.push_back(*(*it)->get_key());
  }
  std::sort(declared_keys.begin(), declared_keys.end());
  declared_keys.erase(std::unique(declared_keys.begin(), declared_keys.end()), declared_keys.end());

  Set into;
  
  std::map< std::string, Set >::const_iterator mit(rman.sets().find(input));
  
  if (mit != rman.sets().end())
  {
    generate_elems< Uint32_Index, Node_Skeleton >(
        *rman.get_transaction(), input, mit->second.nodes, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
    if (rman.get_desired_timestamp() != NOW)
      generate_elems< Uint32_Index, Node_Skeleton >(
          *rman.get_transaction(), input, mit->second.attic_nodes, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Way_Skeleton >(
        *rman.get_transaction(), input, mit->second.ways, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
    if (rman.get_desired_timestamp() != NOW)
      generate_elems< Uint31_Index, Way_Skeleton >(
          *rman.get_transaction(), input, mit->second.attic_ways, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Relation_Skeleton >(
        *rman.get_transaction(), input, mit->second.relations, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
    if (rman.get_desired_timestamp() != NOW)
      generate_elems< Uint31_Index, Relation_Skeleton >(
          *rman.get_transaction(), input, mit->second.attic_relations, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    if (!mit->second.areas.empty())
      generate_elems< Uint31_Index, Area_Skeleton >(
          *rman.get_transaction(), input, mit->second.areas, evaluators,
          multi_evaluator, declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Derived_Structure >(
        *rman.get_transaction(), input, mit->second.deriveds, evaluators,
        multi_evaluator, declared_keys, into, rman, type);
  }
    
  transfer_output(rman, into);
  rman.health_check(*this);
}
