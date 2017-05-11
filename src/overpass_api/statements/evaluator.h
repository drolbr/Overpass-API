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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__EVALUATOR_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__EVALUATOR_H


#include "statement.h"
#include "../data/collect_members.h"
#include "../data/tag_store.h"
#include "../data/utils.h"

#include <map>
#include <string>
#include <vector>


struct Set_Usage
{
  Set_Usage(const std::string& set_name_, uint usage_) : set_name(set_name_), usage(usage_) {}

  std::string set_name;
  uint usage;

  static const uint SKELETON;
  static const uint TAGS;

  bool operator<(const Set_Usage& rhs) const { return this->set_name < rhs.set_name; }
};


struct Requested_Context
{
  Requested_Context() : object_usage(0), role_names_requested(false) {}
  Requested_Context& add_usage(const std::string& set_name, uint usage);
  Requested_Context& add_usage(uint usage);
  Requested_Context& add_role_names();
  void add(const Requested_Context& rhs);
  void bind(const std::string& set_name);
  
  std::vector< Set_Usage > set_usage;
  uint object_usage;
  bool role_names_requested;
};


struct Set_With_Context
{
private:
  Set_With_Context(const Set_With_Context&);
  Set_With_Context& operator=(const Set_With_Context&);

public:
  Set_With_Context() : base(0),
      tag_store_nodes(0), tag_store_attic_nodes(0),
      tag_store_ways(0), tag_store_attic_ways(0),
      tag_store_relations(0), tag_store_attic_relations(0),
      tag_store_areas(0), tag_store_deriveds(0) {}

  ~Set_With_Context()
  {
    delete tag_store_nodes;
    delete tag_store_attic_nodes;
    delete tag_store_ways;
    delete tag_store_attic_ways;
    delete tag_store_relations;
    delete tag_store_attic_relations;
    delete tag_store_areas;
    delete tag_store_deriveds;
  }

  void prefetch(uint usage, const Set& set, Transaction& transaction);

  std::string name;
  const Set* base;
  Tag_Store< Uint32_Index, Node_Skeleton >* tag_store_nodes;
  Tag_Store< Uint32_Index, Node_Skeleton >* tag_store_attic_nodes;
  Tag_Store< Uint31_Index, Way_Skeleton >* tag_store_ways;
  Tag_Store< Uint31_Index, Way_Skeleton >* tag_store_attic_ways;
  Tag_Store< Uint31_Index, Relation_Skeleton >* tag_store_relations;
  Tag_Store< Uint31_Index, Relation_Skeleton >* tag_store_attic_relations;
  Tag_Store< Uint31_Index, Area_Skeleton >* tag_store_areas;
  Tag_Store< Uint31_Index, Derived_Structure >* tag_store_deriveds;
};


struct Prepare_Task_Context
{
  Prepare_Task_Context(const Requested_Context& requested, Resource_Manager& rman);
  
  const Set_With_Context* get_set(const std::string& set_name) const;  
  uint32 get_role_id(const std::string& role) const;

private:
  Array< Set_With_Context > contexts;
  const std::map< uint32, std::string >* relation_member_roles_;
};


/* == Evaluators ==

Evaluators are building blocks that yield on execution a value.
The use of which of the evaluators makes sense depends on the context.

Evaluators help to filter for elements within a query statement.
They allow to get statistical information about query results.
And they allow to remove or add tags from elements.

Currently only tag evaluators are supported.
Geometry evaluators are planned but not implemented in this version.

The following types of evaluators exist and are explained further down:
* Const evaluators deliver always the same value independent of context.
* Element dependent evaluators deliver information about an individual object.
They only make sense in the context of a single element.
* Statistical evaluators deliver information about a set as a whole.
* Aggregators let an element dependent evaluator loop over all elements of a set and combine its results.
* Operators and endomorphisms combine the result of one or two evaluator executions into a new result.

*/


struct Eval_Task
{
  virtual ~Eval_Task() {}

  virtual std::string eval(const std::string* key) const = 0;

  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return eval(key); }
};


struct Const_Eval_Task : public Eval_Task
{
  Const_Eval_Task(const std::string& value_) : value(value_) {}

  virtual std::string eval(const std::string* key) const { return value; }

private:
  std::string value;
};


struct Evaluator : public Statement
{
  Evaluator(int line_number) : Statement(line_number) {}

  virtual Requested_Context request_context() const = 0;

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) = 0;

  virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_compact_ql(indent); }
  virtual int get_operator_priority() const { return std::numeric_limits< int >::max(); }
};


template< typename Evaluator_ >
struct Operator_Stmt_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  virtual Statement* create_statement(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    if (tree_context != Statement::evaluator_expected && tree_context != Statement::elem_eval_possible)
      return 0;
    if (!Evaluator_::applicable_by_subtree_structure(tree_it))
      return 0;

    std::map< std::string, std::string > attributes;
    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    Evaluator_::add_substatements(result, Evaluator_::stmt_operator(), tree_it, tree_context,
        stmt_factory, error_output);
    return result;
  }

  Operator_Stmt_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name())
  {
    Statement::maker_by_token()[Evaluator_::stmt_operator()].push_back(this);
  }
};


#endif
