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

Evaluator_Per_Vertex::Statement_Maker Evaluator_Per_Vertex::statement_maker;
Per_Member_Aggregator_Maker< Evaluator_Per_Vertex > Evaluator_Per_Vertex::evaluator_maker;


Evaluator_Per_Vertex::Evaluator_Per_Vertex
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Per_Member_Aggregator_Syntax< Evaluator_Per_Vertex >(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


Eval_Task* Evaluator_Per_Vertex::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  if (!rhs)
    return 0;

  Eval_Task* rhs_task = rhs->get_string_task(context, key);
  if (!rhs_task)
    return 0;

  return new Per_Vertex_Eval_Task(rhs_task);
}


std::string Per_Vertex_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  std::string result;
  if (data.object->nds.size() > 2)
  {
    result = rhs_task->eval(1, data, key);
    for (uint i = 2; i < data.object->nds.size() - 1; ++i)
      result += ";" + rhs_task->eval(i, data, key);
    if (data.object->nds.front() == data.object->nds.back())
      result += ";" + rhs_task->eval(data.object->nds.size() - 1, data, key);
  }
  return result;
}


std::string Per_Vertex_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  std::string result;
  if (data.object->nds.size() > 2)
  {
    result = rhs_task->eval(1, data, key);
    for (uint i = 2; i < data.object->nds.size() - 1; ++i)
      result += ";" + rhs_task->eval(i, data, key);
    if (data.object->nds.front() == data.object->nds.back())
      result += ";" + rhs_task->eval(data.object->nds.size() - 1, data, key);
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

//-----------------------------------------------------------------------------

Evaluator_Ref::Statement_Maker Evaluator_Ref::statement_maker;
Member_Function_Maker< Evaluator_Ref > Evaluator_Ref::evaluator_maker;


Evaluator_Ref::Evaluator_Ref
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}

//-----------------------------------------------------------------------------

Evaluator_Membertype::Statement_Maker Evaluator_Membertype::statement_maker;
Member_Function_Maker< Evaluator_Membertype > Evaluator_Membertype::evaluator_maker;


Evaluator_Membertype::Evaluator_Membertype
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}

//-----------------------------------------------------------------------------

Evaluator_Role::Statement_Maker Evaluator_Role::statement_maker;
Member_Function_Maker< Evaluator_Role > Evaluator_Role::evaluator_maker;


Evaluator_Role::Evaluator_Role
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}

//-----------------------------------------------------------------------------

Evaluator_Angle::Statement_Maker Evaluator_Angle::statement_maker;
Member_Function_Maker< Evaluator_Angle > Evaluator_Angle::evaluator_maker;


Evaluator_Angle::Evaluator_Angle
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Angle_Eval_Task::eval(uint pos, const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  if (!data.geometry)
    return "";
  keep_cartesians_up_to_date(data.object->id, data.geometry);

  if (pos+1 < data.object->nds.size() || data.object->nds.front() == data.object->nds.back())
    return prettyprinted_angle(pos);
  return "";
}


std::string Angle_Eval_Task::eval(uint pos, const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  if (!data.geometry)
    return "";
  keep_cartesians_up_to_date(data.object->id, data.geometry);

  if (pos+1 < data.object->nds.size() || data.object->nds.front() == data.object->nds.back())
    return prettyprinted_angle(pos);
  return "";
}


std::string Angle_Eval_Task::prettyprinted_angle(uint pos) const
{
  if (pos == 0)
    return "";
  const Cartesian& prev = cached[pos-1];
  const Cartesian& mid = cached[pos];
  const Cartesian& next = (pos+1 < cached.size() ? cached[pos+1] : cached[1]);

  Cartesian in(mid.y*prev.z - mid.z*prev.y, mid.z*prev.x - mid.x*prev.z, mid.x*prev.y - mid.y*prev.x);
  double lg_in = sqrt(in.x*in.x + in.y*in.y + in.z*in.z);
  Cartesian out(next.y*mid.z - next.z*mid.y, next.z*mid.x - next.x*mid.z, next.x*mid.y - next.y*mid.x);
  double lg_out = sqrt(out.x*out.x + out.y*out.y + out.z*out.z);
  if (lg_in < 1e-10 || lg_out < 1e-10)
    return "NaN";
  double prod = (in.x*out.x + in.y*out.y + in.z*out.z)/lg_in/lg_out;
  return fabs(prod) > 1 ? "0.000" : fixed_to_string(acos(prod)/acos(0)*90., 3);
}


void Angle_Eval_Task::keep_cartesians_up_to_date(Way_Skeleton::Id_Type ref, const Opaque_Geometry* current) const
{
  if (!(ref == cache_way_ref) || current != cache_geom_ref)
  {
    if (current && current->has_line_geometry())
    {
      cached.resize(current->way_size());
      for (uint i = 0; i < current->way_size(); ++i)
        cached[i] = current->way_pos_is_valid(i)
            ? Cartesian(current->way_pos_lat(i), current->way_pos_lon(i)) : Cartesian();
    }
    else
      cached.clear();
    cache_way_ref = ref;
    cache_geom_ref = current;
  }
}