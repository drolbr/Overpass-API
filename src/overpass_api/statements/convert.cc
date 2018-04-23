/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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
#include "set_prop.h"


Generic_Statement_Maker< Convert_Statement > Convert_Statement::statement_maker("convert");


Convert_Statement::Convert_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), geom_evaluator(0), id_evaluator(0), multi_evaluator(0)
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
  Set_Prop_Statement* set_prop = dynamic_cast< Set_Prop_Statement* >(statement);
  if (set_prop)
  {
    if (set_prop->get_key())
    {
      for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      {
        if ((*it)->get_key() && *(*it)->get_key() == *set_prop->get_key())
          add_static_error(std::string("A key cannot be added twice to an element: \"") + *set_prop->get_key() + '\"');
      }
    }
    else if (set_prop->get_mode() == Set_Prop_Task::set_id)
    {
      if (!id_evaluator)
        id_evaluator = set_prop;
      else
        add_static_error("A convert statement can have at most one set-prop statement of subtype setting the id.");
    }
    else if (set_prop->get_mode() == Set_Prop_Task::set_geometry)
    {
      if (!geom_evaluator)
        geom_evaluator = set_prop;
      else
        add_static_error("A make statement can have at most one set-prop statement of subtype setting the geometry.");
    }
    else if (!multi_evaluator)
      multi_evaluator = set_prop;
    else
      add_static_error("A convert statement can have at most one any-key set-prop statement.");
    evaluators.push_back(set_prop);
  }
  else
    substatement_error(get_name(), statement);
}


template< typename Index, typename Maybe_Attic >
void generate_elems(const std::string& set_name,
    const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Owning_Array< Set_Prop_Task* >& tasks, const std::vector< std::string >& declared_keys,
    Set& into, Resource_Manager& rman, const std::string& type)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      Derived_Structure result(type, 0ull);
      bool id_fixed = false;

      for (uint i = 0; i < tasks.size(); ++i)
      {
        if (tasks[i])
          tasks[i]->process(context_from.get_context(it_idx->first, *it_elem), declared_keys, result, id_fixed);
      }

      if (!id_fixed)
        result.id = rman.get_global_settings().dispense_derived_id();

      into.deriveds[Uint31_Index(0u)].push_back(result);
    }
  }
}


void Convert_Statement::execute(Resource_Manager& rman)
{
  std::vector< std::string > declared_keys;
  for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
  {
    if ((*it)->get_key())
      declared_keys.push_back(*(*it)->get_key());
  }
  std::sort(declared_keys.begin(), declared_keys.end());
  declared_keys.erase(std::unique(declared_keys.begin(), declared_keys.end()), declared_keys.end());

  Requested_Context requested_context;
  for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    requested_context.add((*it)->request_context());
  requested_context.bind(input);

  Prepare_Task_Context context(requested_context, *this, rman);

  Owning_Array< Set_Prop_Task* > tasks;
  for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    tasks.push_back((*it)->get_task(context, declared_keys));

  Set into;
  Set_With_Context* context_from = context.get_set(input);

  if (context_from && context_from->base)
  {
    generate_elems< Uint32_Index, Node_Skeleton >(
        context_from->name, context_from->base->nodes, *context_from, tasks,
        declared_keys, into, rman, type);
    if (!context_from->base->attic_nodes.empty())
      generate_elems< Uint32_Index, Attic< Node_Skeleton > >(
          context_from->name, context_from->base->attic_nodes, *context_from, tasks,
          declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Way_Skeleton >(
        context_from->name, context_from->base->ways, *context_from, tasks,
        declared_keys, into, rman, type);
    if (!context_from->base->attic_ways.empty())
      generate_elems< Uint31_Index, Attic< Way_Skeleton > >(
          context_from->name, context_from->base->attic_ways, *context_from, tasks,
          declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Relation_Skeleton >(
        context_from->name, context_from->base->relations, *context_from, tasks,
        declared_keys, into, rman, type);
    if (!context_from->base->attic_relations.empty())
      generate_elems< Uint31_Index, Attic< Relation_Skeleton > >(
          context_from->name, context_from->base->attic_relations, *context_from, tasks,
          declared_keys, into, rman, type);
    if (!context_from->base->areas.empty())
      generate_elems< Uint31_Index, Area_Skeleton >(
          context_from->name, context_from->base->areas, *context_from, tasks,
          declared_keys, into, rman, type);
    generate_elems< Uint31_Index, Derived_Structure >(
        context_from->name, context_from->base->deriveds, *context_from, tasks,
        declared_keys, into, rman, type);
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}
