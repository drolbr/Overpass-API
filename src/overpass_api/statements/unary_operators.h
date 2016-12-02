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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__UNARY_OPERATORS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__UNARY_OPERATORS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Evaluator_Prefix_Operator : public Evaluator
{
public:
  Evaluator_Prefix_Operator(int line_number_);
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}
  
  virtual string get_result_name() const { return ""; }
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  virtual std::vector< std::string > used_tags() const;
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context);
  
  virtual std::string process(const std::string& rhs_result) const = 0;

  static bool applicable_by_subtree_structure(const Token_Node_Ptr& tree_it) { return !tree_it->lhs && tree_it->rhs; }
  static void add_substatements(Statement* result, const std::string& operator_name, const Token_Node_Ptr& tree_it,
      Statement::Factory& stmt_factory, Error_Output* error_output);
    
protected:
  Evaluator* rhs;
};


struct Unary_Eval_Task : public Eval_Task
{
  Unary_Eval_Task(Eval_Task* rhs_, Evaluator_Prefix_Operator* evaluator_) : rhs(rhs_), evaluator(evaluator_) {}
  ~Unary_Eval_Task() { delete rhs; }
  
  virtual std::string eval(const std::string* key) const;
  
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const;

private:
  Eval_Task* rhs;
  Evaluator_Prefix_Operator* evaluator;
};


template< typename Evaluator_ >
struct Evaluator_Prefix_Operator_Syntax : public Evaluator_Prefix_Operator
{
  Evaluator_Prefix_Operator_Syntax(int line_number_, const map< string, string >& input_attributes)
    : Evaluator_Prefix_Operator(line_number_)
  {
    std::map< std::string, std::string > attributes;  
    eval_attributes_array(Evaluator_::stmt_name(), attributes, input_attributes);    
  }
  
  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + ">\n"
        + rhs->dump_xml(indent + "  ")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }
  
  virtual std::string dump_compact_ql(const std::string&) const
  {
    if (rhs->get_operator_priority() < get_operator_priority())
      return Evaluator_::stmt_operator() + "(" + rhs->dump_compact_ql("") + ")";
    return Evaluator_::stmt_operator() + rhs->dump_compact_ql("");
  }
  
  virtual string get_name() const { return Evaluator_::stmt_name(); }
  virtual int get_operator_priority() const { return operator_priority(Evaluator_::stmt_operator(), true); }  
};


class Evaluator_Not : public Evaluator_Prefix_Operator_Syntax< Evaluator_Not >
{
public:
  static Operator_Stmt_Maker< Evaluator_Not > statement_maker;
  static std::string stmt_operator() { return "!"; }
  static std::string stmt_name() { return "eval-not"; }

  Evaluator_Not(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Prefix_Operator_Syntax< Evaluator_Not >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& rhs_result) const;
};


class Evaluator_Negate : public Evaluator_Prefix_Operator_Syntax< Evaluator_Negate >
{
public:
  static Operator_Stmt_Maker< Evaluator_Negate > statement_maker;
  static std::string stmt_operator() { return "-"; }
  static std::string stmt_name() { return "eval-negate"; }
      
  Evaluator_Negate(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Prefix_Operator_Syntax< Evaluator_Negate >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& rhs_result) const;
};


template< typename Evaluator_ >
struct String_Endom_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  virtual Statement* create_statement(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    if (tree_it->token != "(")
    {
      if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) cannot have an input set",
            tree_it->line_col.first);
      return 0;
    }
    if (!tree_it->rhs)
    {
      if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
      return 0;
    }
    map< string, string > attributes;
    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    if (result)
    {
      Statement* rhs = stmt_factory.create_statement(tree_it.rhs(), Statement::evaluator_expected);
      if (rhs)
        result->add_statement(rhs, "");
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
    }
    return result;
  }
  
  String_Endom_Statement_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name())
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


template< typename Evaluator_ >
struct Evaluator_String_Endom_Syntax : public Evaluator_Prefix_Operator
{
  Evaluator_String_Endom_Syntax(int line_number_, const map< string, string >& input_attributes)
    : Evaluator_Prefix_Operator(line_number_)
  {
    std::map< std::string, std::string > attributes;  
    eval_attributes_array(Evaluator_::stmt_name(), attributes, input_attributes);    
  }
  
  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + ">\n"
        + rhs->dump_xml(indent + "  ")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }
  
  virtual std::string dump_compact_ql(const std::string&) const
  {
    return Evaluator_::stmt_func_name() + "(" + (rhs ? rhs->dump_compact_ql("") : "") + ")";
  }
  
  virtual string get_name() const { return Evaluator_::stmt_name(); }
};


class Evaluator_Number : public Evaluator_String_Endom_Syntax< Evaluator_Number >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Number > statement_maker;
  static std::string stmt_func_name() { return "number"; }
  static std::string stmt_name() { return "eval-number"; }

  Evaluator_Number(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Number >(line_number_, input_attributes) {}
  virtual string get_name() const { return "eval-number"; }
  virtual ~Evaluator_Number() {}
  
  virtual std::string process(const std::string& rhs_result) const;
};


class Evaluator_Is_Num : public Evaluator_String_Endom_Syntax< Evaluator_Is_Num >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Is_Num > statement_maker;
  static std::string stmt_func_name() { return "is_num"; }
  static std::string stmt_name() { return "eval-is-num"; }

  Evaluator_Is_Num(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Is_Num >(line_number_, input_attributes) {}
  virtual string get_name() const { return "eval-is-num"; }
  virtual ~Evaluator_Is_Num() {}
  
  virtual std::string process(const std::string& rhs_result) const;
};


#endif
