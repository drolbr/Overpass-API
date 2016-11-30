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
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}  
  virtual string get_result_name() const { return ""; }
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  virtual std::vector< std::string > used_tags() const;
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context);
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const = 0;
  
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


class Evaluator_And : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_And >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-and") { Statement::maker_by_token()["&&"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-and>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-and>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "&&" + rhs->dump_compact_ql(""); }
      
  Evaluator_And(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-and"; }
  virtual ~Evaluator_And() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Or : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Or >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-or") { Statement::maker_by_token()["||"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-or>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-or>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "||" + rhs->dump_compact_ql(""); }
      
  Evaluator_Or(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-or"; }
  virtual ~Evaluator_Or() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Equal : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Equal >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-equal") { Statement::maker_by_token()["=="].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-equal>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-equal>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "==" + rhs->dump_compact_ql(""); }
      
  Evaluator_Equal(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-equal"; }
  virtual ~Evaluator_Equal() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Less : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Less >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-less") { Statement::maker_by_token()["<"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-less>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-less>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "<" + rhs->dump_compact_ql(""); }
      
  Evaluator_Less(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-less"; }
  virtual ~Evaluator_Less() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Plus : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Plus >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-plus") { Statement::maker_by_token()["+"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-plus>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-plus>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "+" + rhs->dump_compact_ql(""); }
      
  Evaluator_Plus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-plus"; }
  virtual ~Evaluator_Plus() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Minus : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Minus >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-minus") { Statement::maker_by_token()["-"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-minus>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-minus>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "-" + rhs->dump_compact_ql(""); }
  
  Evaluator_Minus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-minus"; }
  virtual ~Evaluator_Minus() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Times : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Times >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-times") { Statement::maker_by_token()["*"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-times>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-times>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "*" + rhs->dump_compact_ql(""); }
      
  Evaluator_Times(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-times"; }
  virtual ~Evaluator_Times() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Divided : public Evaluator_Pair_Operator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Divided >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-divided-by") { Statement::maker_by_token()["/"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-divided-by>\n" + lhs->dump_xml(indent + "  ") + rhs->dump_xml(indent + "  ")
      + indent + "</eval-divided-by>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return lhs->dump_compact_ql("") + "/" + rhs->dump_compact_ql(""); }
      
  Evaluator_Divided(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-divided-by"; }
  virtual ~Evaluator_Divided() {}
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


#endif
