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


struct Value_Aggregator
{
  // The code of min and max relies on the relative order to gracefully degrade the type
  enum Type_Indicator { type_void = 0, type_int64 = 1, type_double = 2, type_string = 3 };

  virtual void update_value(const std::string& value) = 0;
  virtual std::string get_value() = 0;
  virtual ~Value_Aggregator() {}
};


/* == Aggregators ==

Aggregators need for execution both a set to operate on and an evaluator as argument.
That evaulator will loop over each element of the set, and the aggregator will combine its results.
*/

struct Evaluator_Aggregator : public Evaluator
{
  enum Object_Type { tag, generic, id, type };


  Evaluator_Aggregator(const std::string& func_name, int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}

  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context);

  virtual Value_Aggregator* get_aggregator() = 0;

  std::string input;
  Evaluator* rhs;
  const Set_With_Context* input_set;
};


bool try_parse_input_set(const Token_Node_Ptr& tree_it, Error_Output* error_output, const std::string& message,
    std::string& input_set, bool& explicit_input_set);


template< typename Evaluator_ >
struct Aggregator_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  virtual Statement* create_statement(
      const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    std::map< std::string, std::string > attributes;
    bool input_set = false;
    if (!try_parse_input_set(tree_it, error_output, Evaluator_::stmt_func_name() + "(...) needs an argument",
        attributes["from"], input_set))
      return 0;

    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    if (result)
    {
      Statement* rhs = stmt_factory.create_statement(
          input_set ? tree_it.rhs().rhs() : tree_it.rhs(), Statement::elem_eval_possible);
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
  Evaluator_Aggregator_Syntax(int line_number_, const std::map< std::string, std::string >& input_attributes,
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

  virtual std::string get_name() const { return Evaluator_::stmt_name(); }

  virtual std::string get_result_name() const { return ""; }
};


/* === Union and Set ===

The basic syntax is

  <Set>.u(<Evaluator>)

resp.

  <Set>.set(<Evaluator>)

If the set is the default set <em>_</em> then you can drop the set parameter:

  u(<Evaluator>)

resp.

  set(<Evaluator>)

These two evaluators execute their right hand side evulators on each element of the specified set.
<em>set</em> makes a semi-colon separated list of all distinct values that appear.
<em>u</em> returns the single found value if only one value is found.
If no value is found then <em>u</em> returns an empty string.
If multiple different values are found then <em>set</em> returns the text "< multiple values found >".
*/

class Evaluator_Union_Value : public Evaluator_Aggregator_Syntax< Evaluator_Union_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Union_Value > statement_maker;
  static std::string stmt_func_name() { return "u"; }
  static std::string stmt_name() { return "eval-union"; }

  Evaluator_Union_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Union_Value >(line_number_, input_attributes, global_settings) {}

  struct Aggregator : Value_Aggregator
  {
    virtual void update_value(const std::string& value);
    virtual std::string get_value() { return agg_value; }
    std::string agg_value;
  };
  virtual Value_Aggregator* get_aggregator() { return new Aggregator(); }
};


class Evaluator_Set_Value : public Evaluator_Aggregator_Syntax< Evaluator_Set_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Set_Value > statement_maker;
  static std::string stmt_func_name() { return "set"; }
  static std::string stmt_name() { return "eval-set"; }

  Evaluator_Set_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Set_Value >(line_number_, input_attributes, global_settings) {}

  struct Aggregator : Value_Aggregator
  {
    virtual void update_value(const std::string& value);
    virtual std::string get_value();
    std::set< std::string > values;
  };
  virtual Value_Aggregator* get_aggregator() { return new Aggregator(); }
};


/* === Min and Max ===

The basic syntax is

  <Set>.min(<Evaluator>)

resp.

  <Set>.max(<Evaluator>)

If the set is the default set <em>_</em> then you can drop the set parameter:

  min(<Evaluator>)

resp.

  max(<Evaluator>)

These two evaluators execute their right hand side evaluators on each element of the specified set.
If all return values are valid numbers then <em>min</em> returns the minimal amongst the numbers.
Likewise, if all return values are valid numbers then <em>max</em> returns the maximal amongst the numbers.
If not all return values are valid numbers then <em>min</em> returns the lexicographically first string.
Likewise, if not all return values are valid numbers then <em>max</em> returns the lexicographically last string.
If no value is found then each <em>min</em> and <em>max </em> return an empty string.
*/

class Evaluator_Min_Value : public Evaluator_Aggregator_Syntax< Evaluator_Min_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Min_Value > statement_maker;
  static std::string stmt_func_name() { return "min"; }
  static std::string stmt_name() { return "eval-min"; }

  Evaluator_Min_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Min_Value >(line_number_, input_attributes, global_settings) {}

  struct Aggregator : Value_Aggregator
  {
    Aggregator() : relevant_type(type_void), result_l(std::numeric_limits< int64 >::max()),
        result_d(std::numeric_limits< double >::max()) {}
    virtual void update_value(const std::string& value);
    virtual std::string get_value();
    Type_Indicator relevant_type;
    int64 result_l;
    double result_d;
    std::string result_s;
  };
  virtual Value_Aggregator* get_aggregator() { return new Aggregator(); }
};


class Evaluator_Max_Value : public Evaluator_Aggregator_Syntax< Evaluator_Max_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Max_Value > statement_maker;
  static std::string stmt_func_name() { return "max"; }
  static std::string stmt_name() { return "eval-umax"; }

  Evaluator_Max_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Max_Value >(line_number_, input_attributes, global_settings) {}

  struct Aggregator : Value_Aggregator
  {
    Aggregator() : relevant_type(type_void), result_l(std::numeric_limits< int64 >::min()),
        result_d(-std::numeric_limits< double >::max()) {}
    virtual void update_value(const std::string& value);
    virtual std::string get_value();
    Type_Indicator relevant_type;
    int64 result_l;
    double result_d;
    std::string result_s;
  };
  virtual Value_Aggregator* get_aggregator() { return new Aggregator(); }
};


/* === Sum ===

The basic syntax is

  <Set>.sum(<Evaluator>)

If the set is the default set <em>_</em> then you can drop the set parameter:

  sum(<Evaluator>)

This evaluator executes its right hand side evaluators on each element of the specified set.
If all return values are valid numbers then <em>sum</em> returns their sum.
If not all return values are valid numbers then <em>sum</em> returns "NaN".
*/

class Evaluator_Sum_Value : public Evaluator_Aggregator_Syntax< Evaluator_Sum_Value >
{
public:
  static Aggregator_Statement_Maker< Evaluator_Sum_Value > statement_maker;
  static std::string stmt_func_name() { return "sum"; }
  static std::string stmt_name() { return "eval-sum"; }

  Evaluator_Sum_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
      : Evaluator_Aggregator_Syntax< Evaluator_Sum_Value >(line_number_, input_attributes, global_settings) {}

  struct Aggregator : Value_Aggregator
  {
    Aggregator() : relevant_type(type_int64), result_l(0), result_d(0) {}
    virtual void update_value(const std::string& value);
    virtual std::string get_value();
    Type_Indicator relevant_type;
    int64 result_l;
    double result_d;
  };
  virtual Value_Aggregator* get_aggregator() { return new Aggregator(); }
};


/* == Statistical Count ==

This variant of the <em>count</em> operator counts elements of a given type in a set.

The syntax variants

  count(nodes)
  count(ways)
  count(relations)
  count(deriveds)

counts elements in the default set <em>_</em>, and the syntax variant

  <Set>.count(nodes)
  <Set>.count(ways)
  <Set>.count(relations)
  <Set>.count(deriveds)

counts elements in the set &lt;Set&gt;.
*/

class Evaluator_Set_Count : public Evaluator
{
public:
  enum Objects { nothing, nodes, ways, relations, deriveds };
  static std::string to_string(Objects objects);

  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Set_Count >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Set_Count >("eval-set-count")
    { Statement::maker_by_func_name()["count"].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-set-count from=\"" + input + "\" type=\"" + to_string(to_count) + "\"/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return (input != "_" ? input + "." : "") + "count(" + to_string(to_count) + ")"; }

  Evaluator_Set_Count(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-set-count"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Set_Count() {}

  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context);

private:
  std::string input;
  Objects to_count;
};


#endif
