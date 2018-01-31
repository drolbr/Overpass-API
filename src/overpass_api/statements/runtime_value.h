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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__RUNTIME_VALUE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__RUNTIME_VALUE_H


#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>


/* === Set Key-Value Evaluator ===

Sets can have not only members.
The may get assigned properties by other statements.

An example is the property "val":
This property contains inside a "for"-loop the respective value of the current loop.

The syntax is

  <Set>.<Property>

As opposed to most other situations, it is mandatory to explicitly state the set.

*/

class Evaluator_Set_Key : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Set_Key >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Set_Key >("eval-set-key") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_token()["."].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

  Evaluator_Set_Key(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings);// : Evaluator(line_number_) {}

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<eval-set-key from=\"" + input + "\" key=\"" + escape_xml(key) + "\"/>\n";
  }

  virtual std::string dump_compact_ql(const std::string&) const
  {
    return input + "." + key;
  }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual std::string get_name() const { return "eval-set-key"; }
  virtual std::string get_result_name() const { return ""; }

  virtual void execute(Resource_Manager& rman) {}

  virtual Requested_Context request_context() const
  { 
    return Requested_Context().add_usage(input, Set_Usage::SET_KEY_VALUES);
  }

  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string*);
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context) { return 0; }

private:
  std::string input;
  std::string key;
};


#endif
