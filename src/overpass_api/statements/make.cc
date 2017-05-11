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
#include "make.h"
#include "set_prop.h"


Generic_Statement_Maker< Make_Statement > Make_Statement::statement_maker("make");

Make_Statement::Make_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), id_evaluator(0), multi_evaluator(0)
{
  std::map< std::string, std::string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  
  if (attributes["type"] == "")
    add_static_error("The attribute type must be set to a nonempty string.");

  type = attributes["type"];
}


Make_Statement::~Make_Statement()
{
}


void Make_Statement::add_statement(Statement* statement, std::string text)
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
    else if (set_prop->should_set_id())
    {
      if (!id_evaluator)
        id_evaluator = set_prop;
      else
        add_static_error("A make statement can have at most one set-prop statement of subtype setting the id.");
    }
    else if (!multi_evaluator)
      multi_evaluator = set_prop;
    else
      add_static_error("A make statement can have at most one any-key set-prop statement.");
    evaluators.push_back(set_prop);
  }
  else
    substatement_error(get_name(), statement);
}


void Make_Statement::execute(Resource_Manager& rman)
{
  Requested_Context requested_context;
  for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    requested_context.add((*it)->request_context());
  
  Prepare_Task_Context context(requested_context, rman);
  
  Owning_Array< Set_Prop_Task* > tasks;
  for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    tasks.push_back((*it)->get_task(context));
  
  Derived_Structure result(type, 0ull);
  bool id_fixed = false;
  
  for (uint i = 0; i < tasks.size(); ++i)
  {
    if (tasks[i])
      tasks[i]->process(result, id_fixed);
  }
  
  if (!id_fixed)
    result.id = rman.get_global_settings().dispense_derived_id();
      
  Set into;  
  into.deriveds[Uint31_Index(0u)].push_back(result);
  
  transfer_output(rman, into);
  rman.health_check(*this);
}
