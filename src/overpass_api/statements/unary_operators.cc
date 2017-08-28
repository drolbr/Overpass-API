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
#include "unary_operators.h"


Evaluator_Prefix_Operator::Evaluator_Prefix_Operator(int line_number_)
    : Evaluator_Unary_Function(line_number_) {}


void Evaluator_Prefix_Operator::add_substatements(Statement* result, const std::string& operator_name,
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Error_Output* error_output)
{
  if (result)
  {
    Statement* rhs = stmt_factory.create_statement(tree_it.rhs(), tree_context);
    if (rhs)
      result->add_statement(rhs, "");
    else if (error_output)
      error_output->add_parse_error(std::string("Operator \"") + operator_name
          + "\" needs a right-hand-side argument", tree_it->line_col.first);
  }  
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Not > Evaluator_Not::statement_maker;


std::string Evaluator_Not::process(const std::string& rhs_s) const
{
  double rhs_d = 0;  
  if (try_double(rhs_s, rhs_d))
    return !rhs_d ? "1" : "0";
  
  return rhs_s == "" ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Negate > Evaluator_Negate::statement_maker;


std::string Evaluator_Negate::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;  
  if (try_int64(rhs_s, rhs_l))
    return to_string(-rhs_l);
  
  double rhs_d = 0;  
  if (try_double(rhs_s, rhs_d))
    return to_string(-rhs_d);
  
  return "NaN";
}
