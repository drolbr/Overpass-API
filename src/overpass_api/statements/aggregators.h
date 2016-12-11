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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AGGREGATORS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AGGREGATORS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


struct Evaluator_Aggregator : public Evaluator
{
  enum Object_Type { tag, generic, id, type };
  
  
  Evaluator_Aggregator(const string& func_name, int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);  
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }  
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context);
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value) = 0;
  
  std::string input;
  Evaluator* rhs;
  const Set_With_Context* input_set;
  bool value_set;
  std::string value;
};


bool try_parse_input_set(const Token_Node_Ptr& tree_it, Error_Output* error_output, const std::string& message,
    std::string& input_set, bool& explicit_input_set);


template< typename Evaluator_ >
struct Aggregator_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  Statement* create_statement(
      const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    map< string, string > attributes;
    bool input_set = false;
    if (!try_parse_input_set(tree_it, error_output, Evaluator_::stmt_func_name() + "(...) needs an argument",
        attributes["from"], input_set))
      return 0;
  
    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    if (result)
    {
      Statement* rhs = stmt_factory.create_statement(
          input_set ? tree_it.rhs().rhs() : tree_it.rhs(), Statement::evaluator_expected);
      if (rhs)
        result->add_statement(rhs, "");
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
    }
    return result;
  }

  Aggregator_Statement_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name())
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


template< typename Evaluator_ >
struct Evaluator_Aggregator_Syntax : public Evaluator_Aggregator
{
  Evaluator_Aggregator_Syntax(int line_number_, const map< string, string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator(Evaluator_::stmt_name(), line_number_, input_attributes, global_settings) {}
  
  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + " from=\"" + input + "\">\n"
        + (rhs ? rhs->dump_xml(indent + "  ") : "")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }
  
  virtual std::string dump_compact_ql(const std::string&) const
  {
    return (input != "_" ? input + "." : "")
        + Evaluator_::stmt_func_name() + "("
        + (rhs ? rhs->dump_compact_ql("") : "")
        + ")";
  }
  
  virtual string get_name() const { return Evaluator_::stmt_name(); }
  
  virtual string get_result_name() const { return ""; }
};


class Evaluator_Union_Value : public Evaluator_Aggregator_Syntax< Evaluator_Union_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Union_Value > statement_maker;
  static std::string stmt_func_name() { return "u"; }
  static std::string stmt_name() { return "eval-union"; }
      
  Evaluator_Union_Value(int line_number_, const map< string, string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Union_Value >(line_number_, input_attributes, global_settings) {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Min_Value : public Evaluator_Aggregator_Syntax< Evaluator_Min_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Min_Value > statement_maker;
  static std::string stmt_func_name() { return "min"; }
  static std::string stmt_name() { return "eval-min"; }

  Evaluator_Min_Value(int line_number_, const map< string, string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Min_Value >(line_number_, input_attributes, global_settings) {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Max_Value : public Evaluator_Aggregator_Syntax< Evaluator_Max_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Max_Value > statement_maker;
  static std::string stmt_func_name() { return "max"; }
  static std::string stmt_name() { return "eval-umax"; }

  Evaluator_Max_Value(int line_number_, const map< string, string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Max_Value >(line_number_, input_attributes, global_settings) {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Sum_Value : public Evaluator_Aggregator_Syntax< Evaluator_Sum_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Sum_Value > statement_maker;
  static std::string stmt_func_name() { return "sum"; }
  static std::string stmt_name() { return "eval-sum"; }

  Evaluator_Sum_Value(int line_number_, const map< string, string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Sum_Value >(line_number_, input_attributes, global_settings) {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Set_Value : public Evaluator_Aggregator_Syntax< Evaluator_Set_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Set_Value > statement_maker;
  static std::string stmt_func_name() { return "set"; }
  static std::string stmt_name() { return "eval-set"; }

  Evaluator_Set_Value(int line_number_, const map< string, string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Set_Value >(line_number_, input_attributes, global_settings) {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
  
  std::set< std::string > values;
};


#endif
