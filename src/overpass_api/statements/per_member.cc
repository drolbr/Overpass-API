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
#include "per_member.h"


void Per_Member_Aggregator::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one evaluator substatements.");
}

//-----------------------------------------------------------------------------

Evaluator_Per_Member::Statement_Maker Evaluator_Per_Member::statement_maker;
Per_Member_Aggregator_Maker< Evaluator_Per_Member > Evaluator_Per_Member::evaluator_maker;


Evaluator_Per_Member::Evaluator_Per_Member
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Per_Member_Aggregator_Syntax< Evaluator_Per_Member >(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


Eval_Task* Evaluator_Per_Member::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  if (!rhs)
    return 0;

  Eval_Task* rhs_task = rhs->get_string_task(context, key);
  if (!rhs_task)
    return 0;

  return new Per_Member_Eval_Task(rhs_task);
}


std::string Per_Member_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  std::string result;
  if (!data.object->nds.empty())
  {
    result = rhs_task->eval(0, data, key);
    for (uint i = 1; i < data.object->nds.size(); ++i)
      result += ";" + rhs_task->eval(i, data, key);
  }
  return result;
}


std::string Per_Member_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  std::string result;
  if (!data.object->nds.empty())
  {
    result = rhs_task->eval(0, data, key);
    for (uint i = 1; i < data.object->nds.size(); ++i)
      result += ";" + rhs_task->eval(i, data, key);
  }
  return result;
}


std::string Per_Member_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
{
  std::string result;
  if (!data.object->members.empty())
  {
    result = rhs_task->eval(0, data, key);
    for (uint i = 1; i < data.object->members.size(); ++i)
      result += ";" + rhs_task->eval(i, data, key);
  }
  return result;
}


std::string Per_Member_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
{
  std::string result;
  if (!data.object->members.empty())
  {
    result = rhs_task->eval(0, data, key);
    for (uint i = 1; i < data.object->members.size(); ++i)
      result += ";" + rhs_task->eval(i, data, key);
  }
  return result;
}

//-----------------------------------------------------------------------------

Evaluator_Pos::Statement_Maker Evaluator_Pos::statement_maker;
Member_Function_Maker< Evaluator_Pos > Evaluator_Pos::evaluator_maker;


Evaluator_Pos::Evaluator_Pos
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}
