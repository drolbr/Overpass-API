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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__TAG_VALUE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__TAG_VALUE_H


#include "../../expat/escape_json.h"
#include "../../expat/escape_xml.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


/* == Fixed Value evaluator ==

This operator always returns a fixed value.
It does not take any argument.

Its syntax is

  <Value>
*/

class Evaluator_Fixed : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Fixed >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Fixed >("eval-fixed") { Statement::maker_by_token()[""].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-fixed v=\"" + escape_xml(value) + "\"/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const;

  Evaluator_Fixed(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-fixed"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Fixed() {}

  virtual Requested_Context request_context() const { return Requested_Context(); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Const_Eval_Task(value); }

private:
  std::string value;
};


/* == Element Dependend Operators ==

Element dependend operators depend on one or no parameter.
Their syntax varies,
but most have the apppearance of a function name plus parentheses.

They can only be called in a context where elements are processed.
This applies to convert, to filter, or to arguments of aggregate functions.
They cannot be called directly from make.


=== Id and Type of the Element ===

The operator <em>id</em> returns the id of the element.
The operator <em>type</em> returns the type of the element.
Both operators take no parameters.

The syntax is

  id()

resp.

  type()
*/

struct Id_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? to_string(elem->id.val()) : ""; }
};


class Evaluator_Id : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Id >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Id >("eval-id") { Statement::maker_by_func_name()["id"].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-id/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "id()"; }

  Evaluator_Id(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-id"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Id() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Id_Eval_Task(); }
};


struct Type_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "node"; }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "node"; }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "way"; }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "way"; }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "relation"; }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "relation"; }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return "area"; }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return elem ? elem->type_name : ""; }
};


class Evaluator_Type : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Type >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Type >("eval-type") { Statement::maker_by_func_name()["type"].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const { return indent + "<eval-type/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "type()"; }

  Evaluator_Type(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-type"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Type() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Type_Eval_Task(); }
};


/* === Tag Value and Generic Value Operators ===

The tag operator returns the value of the tag of the given key.
The <em>is_tag</em> operator returns "1" if the given element has a tag with this key and "0" otherwise.
They only can be called in the context of an element.

Their syntax is

  t[<Key name>]

resp.

  is_tag(<Key name >)

The <Key name> must be in quotation marks if it contains special characters.

The generic tag operator returns the value of the tag of the key it is called for.
It only can be called in the context of an element.
In addition, it must be part of the value of a generic property to have its key specified.

Its syntax is

  ::
*/

std::string find_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key);


struct Value_Eval_Task : public Eval_Task
{
  Value_Eval_Task(const std::string& key_) : key(key_) {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return find_value(tags, this->key); }

private:
  std::string key;
};


class Evaluator_Value : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Value >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Value >("eval-value") { Statement::maker_by_token()["["].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-value k=\"" + escape_xml(key) + "\"/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return std::string("[\"") + escape_cstr(key) + "\"]"; }

  Evaluator_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-value"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Value() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::TAGS); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Value_Eval_Task(key); }

private:
  std::string key;
};


std::string exists_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key);


struct Is_Tag_Eval_Task : public Eval_Task
{
  Is_Tag_Eval_Task(const std::string& key_) : key(key_) {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return exists_value(tags, this->key); }

private:
  std::string key;
};


class Evaluator_Is_Tag : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Is_Tag >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Is_Tag >("eval-is-tag")
    { Statement::maker_by_func_name()["is_tag"].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-is-tag k=\"" + escape_xml(key) + "\"/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("is_tag(\"") + escape_cstr(key) + "\")"; }

  Evaluator_Is_Tag(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-is-tag"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Is_Tag() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::TAGS); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Is_Tag_Eval_Task(key); }

private:
  std::string key;
};


struct Generic_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
      { return key ? find_value(tags, *key) : ""; }
};


class Evaluator_Generic : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Generic >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Generic >("eval-generic") { Statement::maker_by_token()["::"].push_back(this); }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-generic/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "::"; }

  Evaluator_Generic(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-generic"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Generic() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::TAGS); }

  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Generic_Eval_Task(); }
};


/* === Element Properties Count ===

This variant of the <em>count</em> operator counts tags or members
of the given element.
Opposed to the statistical variant of the count operator,
they cannot take an input set as extra argument.

The syntax for tags is

  count_tags()

The syntax to count the number of member entries is

  count_members()

The syntax to count the number of distinct members is

  count_distinct_members()

The syntax to count the number of member entries with a specific role is

  count_by_role()

The syntax to count the number of distinct members with a specific role is

  count_distinct_by_role()
*/

class Evaluator_Properties_Count : public Evaluator
{
public:
  enum Objects { nothing, tags, members, distinct_members, by_role, distinct_by_role };
  static std::string to_string(Objects objects);
  enum Members_Type { all, nodes, ways, relations };
  static std::string to_string(Members_Type types);

  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Properties_Count >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Properties_Count >("eval-prop-count")
    {
      Statement::maker_by_func_name()["count_tags"].push_back(this);
      Statement::maker_by_func_name()["count_members"].push_back(this);
      Statement::maker_by_func_name()["count_distinct_members"].push_back(this);
      Statement::maker_by_func_name()["count_by_role"].push_back(this);
      Statement::maker_by_func_name()["count_distinct_by_role"].push_back(this);
    }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<eval-prop-count type=\"" + to_string(to_count) + "\""
        + (to_count == by_role || to_count == distinct_by_role ?
            std::string(" role=\"") + escape_xml(role) + "\"" : std::string("")) 
        + (type_to_count != all ?
            std::string(" members_type=\"") + to_string(type_to_count) + "\"" : std::string("")) + "/>\n";
  }
  virtual std::string dump_compact_ql(const std::string&) const
  {
    return std::string("count_") + to_string(to_count) + "("
        + (to_count == by_role || to_count == distinct_by_role ?
            std::string("\"") + escape_cstr(role) + "\""
            + (type_to_count != all ? "," : "") : std::string("")) 
        + (type_to_count != all ? to_string(type_to_count) : std::string("")) + ")";
  }

  Evaluator_Properties_Count(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-prop-count"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Properties_Count() {}

  virtual Requested_Context request_context() const;

  virtual Eval_Task* get_task(const Prepare_Task_Context& context);

private:
  Objects to_count;
  Members_Type type_to_count;
  std::string role;
};


struct Prop_Count_Eval_Task : public Eval_Task
{
  Prop_Count_Eval_Task(
      Evaluator_Properties_Count::Objects to_count_, Evaluator_Properties_Count::Members_Type type_to_count_,
      uint32 role_id_ = std::numeric_limits< uint32 >::max())
      : to_count(to_count_), type_to_count(type_to_count_), role_id(role_id_) {}

  virtual std::string eval(const std::string* key) const { return "0"; }

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
  Evaluator_Properties_Count::Objects to_count;
  Evaluator_Properties_Count::Members_Type type_to_count;
  uint32 role_id;
};


#endif
