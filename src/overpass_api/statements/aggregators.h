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

  virtual void clear() {}
  
  std::string input;
  Evaluator* rhs;
  const Set_With_Context* input_set;
  bool value_set;
  std::string value;
};


class Evaluator_Union_Value : public Evaluator_Aggregator
{
public:
  Evaluator_Union_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-union-value"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Union_Value() {}
  
  static Generic_Statement_Maker< Evaluator_Union_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Min_Value : public Evaluator_Aggregator
{
public:
  Evaluator_Min_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-min-value"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Min_Value() {}
  
  static Generic_Statement_Maker< Evaluator_Min_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Max_Value : public Evaluator_Aggregator
{
public:
  Evaluator_Max_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-max-value"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Max_Value() {}
  
  static Generic_Statement_Maker< Evaluator_Max_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Sum_Value : public Evaluator_Aggregator
{
public:
  Evaluator_Sum_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-sum-value"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Sum_Value() {}
  
  static Generic_Statement_Maker< Evaluator_Sum_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Evaluator_Set_Value : public Evaluator_Aggregator
{
public:
  Evaluator_Set_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-set-value"; }
  virtual string get_result_name() const { return ""; }
  virtual ~Evaluator_Set_Value() {}
  
  static Generic_Statement_Maker< Evaluator_Set_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
  
  virtual void clear();
  
  std::vector< std::string > values;
};


#endif
