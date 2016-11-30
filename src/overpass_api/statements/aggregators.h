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


class Evaluator_Union_Value : public Evaluator_Aggregator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Union_Value >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-union")
    { Statement::maker_by_func_name()["u"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-union from=\"" + input + "\">\n"
      + (rhs ? rhs->dump_xml(indent + "  ") : "") + indent + "</eval-union>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("u") + (input != "_" ? std::string(".") + input : "") + "(\""
    + (rhs ? rhs->dump_compact_ql("") : "") + "\")"; }

  Evaluator_Union_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-union"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Union_Value() {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Min_Value : public Evaluator_Aggregator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Min_Value >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-min")
    { Statement::maker_by_func_name()["min"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-min from=\"" + input + "\">\n"
      + (rhs ? rhs->dump_xml(indent + "  ") : "") + indent + "</eval-min>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("min") + (input != "_" ? std::string(".") + input : "") + "(\""
    + (rhs ? rhs->dump_compact_ql("") : "") + "\")"; }

  Evaluator_Min_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-min"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Min_Value() {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Max_Value : public Evaluator_Aggregator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Max_Value >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-max")
    { Statement::maker_by_func_name()["max"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-max from=\"" + input + "\">\n"
      + (rhs ? rhs->dump_xml(indent + "  ") : "") + indent + "</eval-max>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("max") + (input != "_" ? std::string(".") + input : "") + "(\""
    + (rhs ? rhs->dump_compact_ql("") : "") + "\")"; }

  Evaluator_Max_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-max"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Max_Value() {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Sum_Value : public Evaluator_Aggregator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Sum_Value >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-sum")
    { Statement::maker_by_func_name()["sum"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-sum from=\"" + input + "\">\n"
      + (rhs ? rhs->dump_xml(indent + "  ") : "") + indent + "</eval-sum>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("sum") + (input != "_" ? std::string(".") + input : "") + "(\""
    + (rhs ? rhs->dump_compact_ql("") : "") + "\")"; }

  Evaluator_Sum_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-sum"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Sum_Value() {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Set_Value : public Evaluator_Aggregator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Set_Value >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-set")
    { Statement::maker_by_func_name()["set"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-set from=\"" + input + "\">\n"
      + (rhs ? rhs->dump_xml(indent + "  ") : "") + indent + "</eval-set>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("set") + (input != "_" ? std::string(".") + input : "") + "(\""
    + (rhs ? rhs->dump_compact_ql("") : "") + "\")"; }

  Evaluator_Set_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-set"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Set_Value() {}
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
  
  std::set< std::string > values;
};


#endif
