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
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Fixed >("eval-fixed") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_token()[""].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

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

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task(value); }

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

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.object ? to_string(data.object->id.val()) : ""; }
};


class Evaluator_Id : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Id >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Id >("eval-id") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Id > evaluator_maker;

  static std::string stmt_func_name() { return "id"; }
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

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key) { return new Id_Eval_Task(); }
};


struct Type_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return "node"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return "node"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return "way"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return "way"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return "relation"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return "relation"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return "area"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.object ? data.object->type_name : ""; }
};


class Evaluator_Type : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Type >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Type >("eval-type") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Type > evaluator_maker;

  static std::string stmt_func_name() { return "type"; }
  virtual std::string dump_xml(const std::string& indent) const { return indent + "<eval-type/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "type()"; }

  Evaluator_Type(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-type"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Type() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Type_Eval_Task(); }
};


/* === Closedness ===

The operator <em>is_closed</em> returns whether the element is a closed way.
The operator is undefined for any other type of element.
For ways, it returns "1" if the first member of the way is equal to the last member of the way
and "0" otherwise.
The operators takes no parameters.

The syntax is

  is_closed()
*/

struct Is_Closed_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return !data.object->nds.empty() && data.object->nds.front() == data.object->nds.back() ? "1" : "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return !data.object->nds.empty() && data.object->nds.front() == data.object->nds.back() ? "1" : "0"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
};


class Evaluator_Is_Closed : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Is_Closed >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Is_Closed >("eval-is-closed") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Is_Closed > evaluator_maker;

  static std::string stmt_func_name() { return "is_closed"; }
  virtual std::string dump_xml(const std::string& indent) const { return indent + "<eval-is-closed/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "is_closed()"; }

  Evaluator_Is_Closed(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-is-closed"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Is_Closed() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Is_Closed_Eval_Task(); }
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
  Value_Eval_Task(Eval_Task* rhs_) : rhs(rhs_) {}

  virtual std::string eval(const std::string* key) const;

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const;

private:
  Eval_Task* rhs;
};


class Evaluator_Value : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Value >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Value >("eval-value") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_token()["["].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<eval-value>\n"
        + (rhs ? rhs->dump_xml(indent + "  ") : "")
        + indent + "</eval-value>\n";
  }
  virtual std::string dump_compact_ql(const std::string&) const
  { return std::string("t[\"") + (rhs ? rhs->dump_compact_ql("") : "") + "\"]"; }

  Evaluator_Value(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual void add_statement(Statement* statement, std::string text);
  virtual std::string get_name() const { return "eval-value"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Value() {}

  virtual Requested_Context request_context() const;

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* target_key);

private:
  Evaluator* rhs;
};


std::string exists_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key);


struct Is_Tag_Eval_Task : public Eval_Task
{
  Is_Tag_Eval_Task(const std::string& key_) : key(key_) {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return exists_value(data.tags, this->key); }

private:
  std::string key;
};


class Evaluator_Is_Tag : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Is_Tag >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Is_Tag >("eval-is-tag") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_func_name()["is_tag"].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

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

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* target_key)
  { return new Is_Tag_Eval_Task(key); }

private:
  std::string key;
};


struct Generic_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return key ? find_value(data.tags, *key) : ""; }
};


class Evaluator_Generic : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Generic >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Generic >("eval-generic") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_token()["::"].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

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

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Generic_Eval_Task(); }
};


/* === All Keys Evaluator ===

The all keys evaluator returns a container of the keys of the given element.

Its syntax is

  keys()
*/


std::vector< std::string > all_keys(const std::vector< std::pair< std::string, std::string > >* tags);


struct All_Keys_Eval_Task : public Eval_Container_Task
{
  virtual std::vector< std::string > eval(const std::string* key) const { return std::vector< std::string >(); }

  virtual std::vector< std::string > eval(
      const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return all_keys(data.tags); }
  virtual std::vector< std::string > eval(
      const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return all_keys(data.tags); }
};


class Evaluator_All_Keys : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_All_Keys >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_All_Keys >("eval-all-keys") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_All_Keys > evaluator_maker;

  static std::string stmt_func_name() { return "keys"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-all-keys/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "keys()"; }

  Evaluator_All_Keys(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-all-keys"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_All_Keys() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::TAGS); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::container; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* target_key) { return 0; }
  virtual Eval_Container_Task* get_container_task(Prepare_Task_Context& context, const std::string* key)
  { return new All_Keys_Eval_Task(); }

private:
  std::string key;
};


/* === Geometry Related Operators ===

==== Geometry ====

The <em>geometry</em> operator returns the geometry of a single object
as a geometry that can be put into the other geometry converting operators.

Its syntax is:

  geom()
*/

struct Geometry_Geometry_Task : Eval_Geometry_Task
{
  Geometry_Geometry_Task() {}

  virtual Opaque_Geometry* eval() const { return 0; }

  virtual Opaque_Geometry* eval(const Element_With_Context< Node_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Node_Skeleton > >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Way_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Way_Skeleton > >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Relation_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Relation_Skeleton > >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Area_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Derived_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
};


class Evaluator_Geometry : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Geometry >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Geometry >("eval-geometry") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Geometry > evaluator_maker;

  static std::string stmt_func_name() { return "geom"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-geometry/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "geom(\"\")"; }

  Evaluator_Geometry(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-geometry"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Geometry() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::GEOMETRY); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::geometry; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task("<Opaque_Geometry>"); }
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context)
  { return new Geometry_Geometry_Task(); }
  virtual bool returns_geometry() const { return true; }
};


/* ==== Length ====

Its syntax is:

  length()
*/

struct Length_Eval_Task : public Eval_Task
{
  Length_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return "0"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return "0"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return "0"; }
};


class Evaluator_Length : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Length >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Length >("eval-length") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Length > evaluator_maker;

  static std::string stmt_func_name() { return "length"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-length/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "length()"; }

  Evaluator_Length(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-length"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Length() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::GEOMETRY); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Length_Eval_Task(); }
};


/* === Meta Data Operators ===

The <em>version</em> operator returns the version number of the given element.
Its syntax is:

  version()

The <em>timestamp</em> operator returns the timestamp of the given element.
Its syntax is:

  timestamp()

The <em>changeset</em> operator returns the changeset id of the changeset
in which the given element has been last edited.
Its syntax is:

  changeset()

The <em>uid</em> operator returns the id of the user
that has last touched the given element.
Its syntax is:

  uid()

The <em>uid</em> operator returns the name of the user
that has last touched the given element.
Its syntax is:

  user()
*/

struct Version_Eval_Task : public Eval_Task
{
  Version_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->version) : ""; }
};


class Evaluator_Version : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Version >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Version >("eval-version") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Version > evaluator_maker;

  static std::string stmt_func_name() { return "version"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-version/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "version(\"\")"; }

  Evaluator_Version(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-version"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Version() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::META); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Version_Eval_Task(); }
};


struct Timestamp_Eval_Task : public Eval_Task
{
  Timestamp_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.meta ? Timestamp(data.meta->timestamp).str() : ""; }
};


class Evaluator_Timestamp : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Timestamp >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Timestamp >("eval-timestamp") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Timestamp > evaluator_maker;

  static std::string stmt_func_name() { return "timestamp"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-timestamp/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "timestamp(\"\")"; }

  Evaluator_Timestamp(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-timestamp"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Timestamp() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::META); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Timestamp_Eval_Task(); }
};


struct Changeset_Eval_Task : public Eval_Task
{
  Changeset_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->changeset) : ""; }
};


class Evaluator_Changeset : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Changeset >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Changeset >("eval-changeset") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Changeset > evaluator_maker;

  static std::string stmt_func_name() { return "changeset"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-changeset/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "changeset(\"\")"; }

  Evaluator_Changeset(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-changeset"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Changeset() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::META); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Changeset_Eval_Task(); }
};


struct Uid_Eval_Task : public Eval_Task
{
  Uid_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.meta ? to_string(data.meta->user_id) : ""; }
};


class Evaluator_Uid : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Uid >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Uid >("eval-uid") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Uid > evaluator_maker;

  static std::string stmt_func_name() { return "uid"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-uid/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "uid(\"\")"; }

  Evaluator_Uid(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-uid"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Uid() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::META); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Uid_Eval_Task(); }
};


struct User_Eval_Task : public Eval_Task
{
  User_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.user_name ? *data.user_name : ""; }
};


class Evaluator_User : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_User >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_User >("eval-user") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_User > evaluator_maker;

  static std::string stmt_func_name() { return "user"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-user/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "user(\"\")"; }

  Evaluator_User(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-user"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_User() {}

  virtual Requested_Context request_context() const
  { return Requested_Context().add_usage(Set_Usage::META).add_user_names(); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new User_Eval_Task(); }
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
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Properties_Count >("eval-prop-count") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker()
    {
      Statement::maker_by_func_name()["count_tags"].push_back(this);
      Statement::maker_by_func_name()["count_members"].push_back(this);
      Statement::maker_by_func_name()["count_distinct_members"].push_back(this);
      Statement::maker_by_func_name()["count_by_role"].push_back(this);
      Statement::maker_by_func_name()["count_distinct_by_role"].push_back(this);
    }
  };
  static Evaluator_Maker evaluator_maker;

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

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key);

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

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const;

private:
  Evaluator_Properties_Count::Objects to_count;
  Evaluator_Properties_Count::Members_Type type_to_count;
  uint32 role_id;
};


#endif
