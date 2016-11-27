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


class Evaluator_Fixed : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Fixed >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-fixed") { Statement::maker_by_token()[""].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-fixed v=\"" + escape_xml(value) + "\"/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return escape_cstr(value); }
  virtual std::string dump_pretty_ql(const std::string&) const { return escape_cstr(value); }

  Evaluator_Fixed(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-fixed"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Fixed() {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::pair< std::vector< Set_Usage >, uint >(); }  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Const_Eval_Task(value); }
  
private:
  std::string value;
};


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
  Evaluator_Id(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-id"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Id() {}
  
  static Generic_Statement_Maker< Evaluator_Id > statement_maker;

  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::SKELETON); }  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
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
  Evaluator_Type(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-type"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Type() {}
  
  static Generic_Statement_Maker< Evaluator_Type > statement_maker;

  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::SKELETON); }
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Type_Eval_Task(); }
};


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
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-value") { Statement::maker_by_token()["["].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-value k=\"" + escape_xml(key) + "\"/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return std::string("[") + escape_cstr(key) + "]"; }
  virtual std::string dump_pretty_ql(const std::string&) const { return std::string("[") + escape_cstr(key) + "]"; }

  Evaluator_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Value() {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::TAGS); }
  
  virtual std::vector< std::string > used_tags() const
  {
    std::vector< std::string > result;
    result.push_back(key);
    return result;
  }
  
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
  Evaluator_Is_Tag(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-is-tag"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Is_Tag() {}
  
  static Generic_Statement_Maker< Evaluator_Is_Tag > statement_maker;
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::TAGS); }
  
  virtual std::vector< std::string > used_tags() const
  {
    std::vector< std::string > result;
    result.push_back(key);
    return result;
  }
  
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
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker("eval-generic") { Statement::maker_by_token()["::"].push_back(this); }
  };
  static Statement_Maker statement_maker;
      
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-generic/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "::"; }
  virtual std::string dump_pretty_ql(const std::string&) const { return "::"; }

  Evaluator_Generic(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-generic"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Generic() {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::TAGS); }
  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context) { return new Generic_Eval_Task(); }
};


class Evaluator_Count : public Evaluator
{
public:
  enum Objects { nothing, nodes, ways, relations, deriveds, tags, members };
  
  Evaluator_Count(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-count"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Count() {}
  
  static Generic_Statement_Maker< Evaluator_Count > statement_maker;
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;  
  virtual std::vector< std::string > used_tags() const;
  
  virtual Eval_Task* get_task(const Prepare_Task_Context& context);
  
private:
  std::string input;
  Objects to_count;
};


struct Count_Eval_Task : public Eval_Task
{
  Count_Eval_Task(Evaluator_Count::Objects to_count_) : to_count(to_count_) {}
  
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
  Evaluator_Count::Objects to_count;
};


#endif
