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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__BINARY_OPERATORS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__BINARY_OPERATORS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Evaluator_Pair_Operator : public Evaluator
{
public:
  Evaluator_Pair_Operator(int line_number_);
  virtual ~Evaluator_Pair_Operator() {}

  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}  
  virtual string get_result_name() const { return ""; }
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  virtual std::vector< std::string > used_tags() const;
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context);
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const = 0;

  static bool applicable_by_subtree_structure(const Token_Node_Ptr& tree_it) { return tree_it->lhs && tree_it->rhs; }
  static void add_substatements(Statement* result, const std::string& operator_name, const Token_Node_Ptr& tree_it,
      Statement::Factory& stmt_factory, Error_Output* error_output);
  
protected:
  Evaluator* lhs;
  Evaluator* rhs;
};


struct Binary_Eval_Task : public Eval_Task
{
  Binary_Eval_Task(Eval_Task* lhs_, Eval_Task* rhs_, Evaluator_Pair_Operator* evaluator_)
      : lhs(lhs_), rhs(rhs_), evaluator(evaluator_) {}
  ~Binary_Eval_Task()
  {
    delete lhs;
    delete rhs;
  }
  
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
  Eval_Task* lhs;
  Eval_Task* rhs;
  Evaluator_Pair_Operator* evaluator;
};


template< typename Evaluator_ >
struct Evaluator_Pair_Operator_Syntax : public Evaluator_Pair_Operator
{
  Evaluator_Pair_Operator_Syntax(int line_number_, const map< string, string >& input_attributes)
    : Evaluator_Pair_Operator(line_number_)
  {
    std::map< std::string, std::string > attributes;  
    eval_attributes_array(Evaluator_::stmt_name(), attributes, input_attributes);    
  }
  
  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + ">\n"
        + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }
  
  virtual std::string dump_compact_ql(const std::string&) const
  {
    return (lhs->get_operator_priority() < get_operator_priority() ? std::string("(") + lhs->dump_compact_ql("") + ")"
        : lhs->dump_compact_ql("")) + Evaluator_::stmt_operator()
        + (rhs->get_operator_priority() < get_operator_priority() ? std::string("(") + rhs->dump_compact_ql("") + ")"
        : rhs->dump_compact_ql(""));
  }
  
  virtual string get_name() const { return Evaluator_::stmt_name(); }
  virtual int get_operator_priority() const { return operator_priority(Evaluator_::stmt_operator(), false); }  
};


struct Evaluator_And : public Evaluator_Pair_Operator_Syntax< Evaluator_And >
{
  static Operator_Stmt_Maker< Evaluator_And > statement_maker;
  static std::string stmt_operator() { return "&&"; }
  static std::string stmt_name() { return "eval-and"; }

  Evaluator_And(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_And >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Or : public Evaluator_Pair_Operator_Syntax< Evaluator_Or >
{
  static Operator_Stmt_Maker< Evaluator_Or > statement_maker;
  static std::string stmt_operator() { return "||"; }
  static std::string stmt_name() { return "eval-or"; }
      
  Evaluator_Or(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Or >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Equal : public Evaluator_Pair_Operator_Syntax< Evaluator_Equal >
{
  static Operator_Stmt_Maker< Evaluator_Equal > statement_maker;
  static std::string stmt_operator() { return "=="; }
  static std::string stmt_name() { return "eval-equal"; }
      
  Evaluator_Equal(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Equal >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Less : public Evaluator_Pair_Operator_Syntax< Evaluator_Less >
{
  static Operator_Stmt_Maker< Evaluator_Less > statement_maker;
  static std::string stmt_operator() { return "<"; }
  static std::string stmt_name() { return "eval-less"; }
      
  Evaluator_Less(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Less >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Less_Equal : public Evaluator_Pair_Operator_Syntax< Evaluator_Less_Equal >
{
  static Operator_Stmt_Maker< Evaluator_Less_Equal > statement_maker;
  static std::string stmt_operator() { return "<="; }
  static std::string stmt_name() { return "eval-less-equal"; }
      
  Evaluator_Less_Equal(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Less_Equal >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Greater : public Evaluator_Pair_Operator_Syntax< Evaluator_Greater >
{
  static Operator_Stmt_Maker< Evaluator_Greater > statement_maker;
  static std::string stmt_operator() { return ">"; }
  static std::string stmt_name() { return "eval-greater"; }
      
  Evaluator_Greater(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Greater >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Greater_Equal : public Evaluator_Pair_Operator_Syntax< Evaluator_Greater_Equal >
{
  static Operator_Stmt_Maker< Evaluator_Greater_Equal > statement_maker;
  static std::string stmt_operator() { return ">="; }
  static std::string stmt_name() { return "eval-greater-equal"; }
      
  Evaluator_Greater_Equal(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Greater_Equal >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Plus : public Evaluator_Pair_Operator_Syntax< Evaluator_Plus >
{
  static Operator_Stmt_Maker< Evaluator_Plus > statement_maker;
  static std::string stmt_operator() { return "+"; }
  static std::string stmt_name() { return "eval-plus"; }
      
  Evaluator_Plus(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Plus >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Minus : public Evaluator_Pair_Operator_Syntax< Evaluator_Minus >
{
  static Operator_Stmt_Maker< Evaluator_Minus > statement_maker;
  static std::string stmt_operator() { return "-"; }
  static std::string stmt_name() { return "eval-minus"; }
  
  Evaluator_Minus(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Minus >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Times : public Evaluator_Pair_Operator_Syntax< Evaluator_Times >
{
  static Operator_Stmt_Maker< Evaluator_Times > statement_maker;
  static std::string stmt_operator() { return "*"; }
  static std::string stmt_name() { return "eval-times"; }
      
  Evaluator_Times(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Times >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Evaluator_Divided : public Evaluator_Pair_Operator_Syntax< Evaluator_Divided >
{
  static Operator_Stmt_Maker< Evaluator_Divided > statement_maker;
  static std::string stmt_operator() { return "/"; }
  static std::string stmt_name() { return "eval-divided-by"; }
      
  Evaluator_Divided(int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Pair_Operator_Syntax< Evaluator_Divided >(line_number_, input_attributes) {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


#endif
