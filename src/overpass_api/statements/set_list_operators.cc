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
#include "set_list_operators.h"


std::string trim(const std::string& input)
{
  if (input.empty())
    return input;
  
  std::string::size_type from = 0;
  while (from < input.size() && isspace(input[from]))
    ++from;
  
  if (from == input.size())
    return " ";
  
  std::string::size_type to = input.size()-1;
  while (isspace(input[to]))
    --to;
  
  return input.substr(from, to+1);
}


std::vector< std::string > members(const std::string& lrs)
{
  std::vector< std::string > result;
  
  if (lrs.empty())
    return result;
  
  std::string::size_type from = 0;
  while (from < lrs.size())
  {
    std::string::size_type to = lrs.find(';', from);
    if (to == std::string::npos)
      break;
    
    result.push_back(trim(lrs.substr(from, to - from)));
    
    from = to + 1;
  }
  result.push_back(trim(lrs.substr(from)));
  
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}


//-----------------------------------------------------------------------------


Binary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_In > Evaluator_Lrs_In::statement_maker;


std::string Evaluator_Lrs_In::process(const std::string& first_s, const std::string& second_s) const
{
  std::string first = trim(first_s);
  
  std::string::size_type from = 0;
  while (from < second_s.size())
  {
    std::string::size_type to = second_s.find(';', from);
    if (to == std::string::npos)
      break;
    
    if (first == trim(second_s.substr(from, to - from)))
      return "1";
    
    from = to + 1;
  }
  
  if (first == trim(second_s.substr(from)))
    return "1";
  
  return "0";
}


//-----------------------------------------------------------------------------


Binary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Isect > Evaluator_Lrs_Isect::statement_maker;


std::string Evaluator_Lrs_Isect::process(const std::string& first_s, const std::string& second_s) const
{
  std::vector< std::string > first;
  members(first_s).swap(first);
  std::vector< std::string > second;
  members(second_s).swap(second);
  std::vector< std::string > result(std::min(first.size(), second.size()));
  
  result.erase(std::set_intersection(first.begin(), first.end(), second.begin(), second.end(), result.begin()),
      result.end());
  
  std::string result_s;
  if (!result.empty())
    result_s = result[0];
  for (unsigned int i = 1; i < result.size(); ++i)
    result_s += ";" + result[i];
  
  return result_s;
}


//-----------------------------------------------------------------------------


Binary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Union > Evaluator_Lrs_Union::statement_maker;


std::string Evaluator_Lrs_Union::process(const std::string& first_s, const std::string& second_s) const
{
  std::vector< std::string > first;
  members(first_s).swap(first);
  std::vector< std::string > second;
  members(second_s).swap(second);
  std::vector< std::string > result(first.size() + second.size());
  
  result.erase(std::set_union(first.begin(), first.end(), second.begin(), second.end(), result.begin()),
      result.end());
  
  std::string result_s;
  if (!result.empty())
    result_s = result[0];
  for (unsigned int i = 1; i < result.size(); ++i)
    result_s += ";" + result[i];
  
  return result_s;
}


//-----------------------------------------------------------------------------


enum Type_Indicator { type_void = 0, type_int64 = 1, type_double = 2, type_string = 3 };


Unary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Max > Evaluator_Lrs_Max::statement_maker;


void update_maximum(const std::string& elem, Type_Indicator& relevant_type,
    int64& result_l, double& result_d, std::string& result_s)
{
  if (relevant_type <= type_int64)
  {
    int64 rhs_l = 0;
    if (try_int64(elem, rhs_l))
      result_l = std::max(result_l, rhs_l);
    else
      relevant_type = type_double;
  }

  if (relevant_type <= type_double)
  {
    double rhs_d = 0;
    if (try_double(elem, rhs_d))
      result_d = std::max(result_d, rhs_d);
    else
      relevant_type = type_string;
  }

  if (!elem.empty())
    result_s = (result_s.empty() ? elem : std::max(result_s, elem));
}


std::string Evaluator_Lrs_Max::process(const std::string& rhs_s) const
{
  if (rhs_s.empty())
    return "";
  
  Type_Indicator relevant_type = type_int64;
  int64 result_l = std::numeric_limits< int64 >::min();
  double result_d = -std::numeric_limits< double >::max();
  std::string result_s;
  
  std::string::size_type from = 0;
  while (from < rhs_s.size())
  {
    std::string::size_type to = rhs_s.find(';', from);
    if (to == std::string::npos)
      break;
    
    update_maximum(trim(rhs_s.substr(from, to - from)), relevant_type, result_l, result_d, result_s);
    
    from = to + 1;
  }
  
  update_maximum(trim(rhs_s.substr(from)), relevant_type, result_l, result_d, result_s);
  
  if (relevant_type == type_int64)
    return to_string(result_l);
  else if (relevant_type == type_double)
    return to_string(result_d);

  return result_s;
}


//-----------------------------------------------------------------------------


Unary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Min > Evaluator_Lrs_Min::statement_maker;


void update_minimum(const std::string& elem, Type_Indicator& relevant_type,
    int64& result_l, double& result_d, std::string& result_s)
{
  if (relevant_type <= type_int64)
  {
    int64 rhs_l = 0;
    if (try_int64(elem, rhs_l))
      result_l = std::min(result_l, rhs_l);
    else
      relevant_type = type_double;
  }

  if (relevant_type <= type_double)
  {
    double rhs_d = 0;
    if (try_double(elem, rhs_d))
      result_d = std::min(result_d, rhs_d);
    else
      relevant_type = type_string;
  }

  if (!elem.empty())
    result_s = (result_s.empty() ? elem : std::min(result_s, elem));
}


std::string Evaluator_Lrs_Min::process(const std::string& rhs_s) const
{
  if (rhs_s.empty())
    return "";
  
  Type_Indicator relevant_type = type_int64;
  int64 result_l = std::numeric_limits< int64 >::max();
  double result_d = std::numeric_limits< double >::max();
  std::string result_s;
  
  std::string::size_type from = 0;
  while (from < rhs_s.size())
  {
    std::string::size_type to = rhs_s.find(';', from);
    if (to == std::string::npos)
      break;
    
    update_minimum(trim(rhs_s.substr(from, to - from)), relevant_type, result_l, result_d, result_s);
    
    from = to + 1;
  }
  
  update_minimum(trim(rhs_s.substr(from)), relevant_type, result_l, result_d, result_s);
  
  if (relevant_type == type_int64)
    return to_string(result_l);
  else if (relevant_type == type_double)
    return to_string(result_d);

  return result_s;
}
