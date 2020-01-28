/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PER_MEMBER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PER_MEMBER_H


#include "../../expat/escape_json.h"
#include "../../expat/escape_xml.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


struct Per_Member_Aggregator : public Evaluator
{
  Per_Member_Aggregator(int line_number_) : Evaluator(line_number_), rhs(0) {}
  virtual void add_statement(Statement* statement, std::string text);

  Evaluator* rhs;
};


template< typename Evaluator_ >
struct Per_Member_Aggregator_Syntax : public Per_Member_Aggregator
{
  Per_Member_Aggregator_Syntax(int line_number_) : Per_Member_Aggregator(line_number_) {}

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + ">\n"
        + (rhs ? rhs->dump_xml(indent + "  ") : "")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }

  virtual std::string dump_compact_ql(const std::string&) const
  {
    return Evaluator_::stmt_func_name() + "("
        + (rhs ? rhs->dump_compact_ql("") : "")
        + ")";
  }

  virtual std::string get_name() const { return Evaluator_::stmt_name(); }
  virtual std::string get_result_name() const { return ""; }
};


template< typename Evaluator_ >
struct Per_Member_Aggregator_Maker : Statement::Evaluator_Maker
{
  virtual Statement* create_evaluator(
      const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
        || !tree_it.assert_has_arguments(error_output, true)
        || !assert_element_in_context(error_output, tree_it, tree_context))
      return 0;

    Statement* result = new Evaluator_(
        tree_it->line_col.first, std::map< std::string, std::string >(), global_settings);
    if (result)
    {
      Statement* rhs = stmt_factory.create_evaluator(
          tree_it.rhs(), Statement::member_eval_possible,
          Statement::Single_Return_Type_Checker(Statement::string));
      if (rhs)
        result->add_statement(rhs, "");
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
    }
    return result;
  }

  Per_Member_Aggregator_Maker()
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


/* === Per Member Aggregators ===

Aggregators per member help to process information that does apply to only a single member of a way or relation.
Examples of this are the position within the way or relation, geometric properties like the angles, or roles.
A per member aggregator needs an element to operate on and executes its argument multiple times,
passing the possible positions one by one in addition to the element to its argument.

==== Per Member ====

For each element, the aggregator executes its argument once per member of the element.
The return value is a semicolon separated list of the return values.
No deduplication or sotring takes place.
Note that the aggregator returns an empty value for nodes and deriveds
and does not execute its argument in that case.

The syntax is

  per_member(<Evaluator>)
*/

struct Per_Member_Eval_Task : public Eval_Task
{
  Per_Member_Eval_Task(Eval_Task* rhs) : rhs_task(rhs) {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const;

private:
  Owner< Eval_Task > rhs_task;
};


class Evaluator_Per_Member : public Per_Member_Aggregator_Syntax< Evaluator_Per_Member >
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Per_Member >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Per_Member >("eval-per-member") {}
  };
  static Statement_Maker statement_maker;
  static Per_Member_Aggregator_Maker< Evaluator_Per_Member > evaluator_maker;

  static std::string stmt_func_name() { return "per_member"; }
  static std::string stmt_name() { return "eval-per-member"; }

  Evaluator_Per_Member(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Per_Member() {}

  virtual Requested_Context request_context() const
  { return (rhs ? rhs->request_context() : Requested_Context()).add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key);
};


/* ==== Per Vertex ====

For each element, the aggregator executes its argument once per (inner) vertex of the element.
Otherwise it behaves similar to the per_member operator.
For closed ways, this means that it is executed once for all vertices but the first member.
For open ways, it is executed once for all vertices but the first and last member.
For relations its behaviour is currently undefined.

The syntax is

  per_vertex(<Evaluator>)
*/

struct Per_Vertex_Eval_Task : public Eval_Task
{
  Per_Vertex_Eval_Task(Eval_Task* rhs) : rhs_task(rhs) {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const;

private:
  Owner< Eval_Task > rhs_task;
};


class Evaluator_Per_Vertex : public Per_Member_Aggregator_Syntax< Evaluator_Per_Vertex >
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Per_Vertex >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Per_Vertex >("eval-per-vertex") {}
  };
  static Statement_Maker statement_maker;
  static Per_Member_Aggregator_Maker< Evaluator_Per_Vertex > evaluator_maker;

  static std::string stmt_func_name() { return "per_vertex"; }
  static std::string stmt_name() { return "eval-per-vertex"; }

  Evaluator_Per_Vertex(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Per_Vertex() {}

  virtual Requested_Context request_context() const
  { return (rhs ? rhs->request_context() : Requested_Context()).add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key);
};


/* === Member Dependend Functions ===

Member dependend functions can only be used when an element plus a position within the element is in context.
They then deliver specific information for that member.

==== Position of the Member ====

The position function returns for a member its one-based position within the element.

The syntax is

  pos()
*/

struct Pos_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(uint pos, const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return to_string(pos+1); }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return to_string(pos+1); }
  virtual std::string eval(uint pos, const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return to_string(pos+1); }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return to_string(pos+1); }
};


class Evaluator_Pos : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Pos >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Pos >("eval-pos") {}
  };
  static Statement_Maker statement_maker;
  static Member_Function_Maker< Evaluator_Pos > evaluator_maker;

  static std::string stmt_func_name() { return "pos"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-pos/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "pos()"; }

  Evaluator_Pos(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-pos"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Pos() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key) { return new Pos_Eval_Task(); }
};


/* ==== Reference to the Member ====

The functions mtype and reference return for a member the type resp. id of the referenced object.

The syntax is

  mtype()

resp.

  ref()
*/

struct Membertype_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(uint pos, const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return "node"; }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return "node"; }
  virtual std::string eval(uint pos, const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return member_type_name(data.object->members[pos].type); }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return member_type_name(data.object->members[pos].type); }
};


class Evaluator_Membertype : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Membertype >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Membertype >("eval-membertype") {}
  };
  static Statement_Maker statement_maker;
  static Member_Function_Maker< Evaluator_Membertype > evaluator_maker;

  static std::string stmt_func_name() { return "mtype"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-membertype/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "mtype()"; }

  Evaluator_Membertype(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-membertype"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Membertype() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key) { return new Membertype_Eval_Task(); }
};


struct Ref_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(uint pos, const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return to_string(data.object->nds[pos].val()); }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return to_string(data.object->nds[pos].val()); }
  virtual std::string eval(uint pos, const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return to_string(data.object->members[pos].ref.val()); }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return to_string(data.object->members[pos].ref.val()); }
};


class Evaluator_Ref : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Ref >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Ref >("eval-ref") {}
  };
  static Statement_Maker statement_maker;
  static Member_Function_Maker< Evaluator_Ref > evaluator_maker;

  static std::string stmt_func_name() { return "ref"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-ref/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "ref()"; }

  Evaluator_Ref(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-ref"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Ref() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key) { return new Ref_Eval_Task(); }
};


/* ==== Role of the Member ====

The role function returns the role of the member.

The syntax is

  role()
*/

struct Role_Eval_Task : public Eval_Task
{
  Role_Eval_Task(const std::map< uint32, std::string >* roles_) : roles(roles_) {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(uint pos, const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return ""; }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return ""; }
  virtual std::string eval(uint pos, const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return roles ? roles->find(data.object->members[pos].role)->second : std::string(); }
  virtual std::string eval(uint pos, const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return roles ? roles->find(data.object->members[pos].role)->second : std::string(); }

private:
  const std::map< uint32, std::string >* roles;
};


class Evaluator_Role : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Role >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Role >("eval-role") {}
  };
  static Statement_Maker statement_maker;
  static Member_Function_Maker< Evaluator_Role > evaluator_maker;

  static std::string stmt_func_name() { return "role"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-role/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "role()"; }

  Evaluator_Role(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-role"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Role() {}

  virtual Requested_Context request_context() const
  { return Requested_Context().add_usage(Set_Usage::SKELETON).add_role_names(); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; }
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Role_Eval_Task(context.get_roles()); }
};


#endif
