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


#include "statement.h"
#include "../data/tag_store.h"

#include <map>
#include <string>
#include <vector>


struct Set_Usage
{
  Set_Usage(const std::string& set_name_, uint usage_) : set_name(set_name_), usage(usage_) {}
  
  std::string set_name;
  uint usage;
    
  const static uint SKELETON = 1;
  const static uint TAGS = 2;
  
  bool operator<(const Set_Usage& rhs) const { return this->set_name < rhs.set_name; }
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
  
  void prefetch(const Set_Usage& usage, const Set& set, Transaction& transaction);
  
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


struct Tag_Value : public Statement
{
  Tag_Value(int line_number) : Statement(line_number) {}

  virtual std::string eval(const std::string* key) = 0;
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) = 0;
      
  virtual void prefetch(const Set_With_Context& set) = 0;
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const = 0;
  
  virtual std::vector< std::string > used_tags() const = 0;
  
  virtual void clear() = 0;
};


class Set_Tag_Statement : public Statement
{
public:
  Set_Tag_Statement(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "set-tag"; }
  virtual string get_result_name() const { return ""; }
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Set_Tag_Statement() {}
    
  static Generic_Statement_Maker< Set_Tag_Statement > statement_maker;

  std::string eval(const std::string* key = 0);
  std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
  std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key = 0);
      
  void prefetch(const Set_With_Context& set);
  
  std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  
  std::vector< std::string > used_tags() const;
   
  const std::string* get_key() const { return input != "" ? 0 : &keys.front(); }
  bool has_value() const { return tag_value; }
  bool should_set_id() const { return set_id; }
  
  void clear();
    
private:
  std::string input;
  std::vector< std::string > keys;
  bool set_id;
  Tag_Value* tag_value;
};


class Tag_Value_Fixed : public Tag_Value
{
public:
  Tag_Value_Fixed(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-fixed"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Fixed() {}
  
  static Generic_Statement_Maker< Tag_Value_Fixed > statement_maker;

  virtual std::string eval(const std::string* key) { return value; }
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) { return value; }
      
  virtual void prefetch(const Set_With_Context& set) {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::pair< std::vector< Set_Usage >, uint >(); }
  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual void clear() {}
  
private:
  std::string value;
};


class Tag_Value_Id : public Tag_Value
{
public:
  Tag_Value_Id(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-id"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Id() {}
  
  static Generic_Statement_Maker< Tag_Value_Id > statement_maker;

  virtual std::string eval(const std::string* key) { return ""; }
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? to_string(elem->id.val()) : ""; }
      
  virtual void prefetch(const Set_With_Context& set) {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::SKELETON); }
  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual void clear() {}
};


class Tag_Value_Type : public Tag_Value
{
public:
  Tag_Value_Type(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-type"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Type() {}
  
  static Generic_Statement_Maker< Tag_Value_Type > statement_maker;

  virtual std::string eval(const std::string* key) { return ""; }
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "node"; }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "node"; }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "way"; }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "way"; }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "relation"; }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "relation"; }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return "area"; }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return elem ? elem->type_name : ""; }
      
  virtual void prefetch(const Set_With_Context& set) {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::SKELETON); }
  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual void clear() {}
};


std::string find_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key);


class Tag_Value_Value : public Tag_Value
{
public:
  Tag_Value_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Value() {}
  
  static Generic_Statement_Maker< Tag_Value_Value > statement_maker;

  virtual std::string eval(const std::string* key) { return ""; }
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, this->key); }
      
  virtual void prefetch(const Set_With_Context& set) {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::TAGS); }
  
  virtual std::vector< std::string > used_tags() const
  {
    std::vector< std::string > result;
    result.push_back(key);
    return result;
  }
  
  virtual void clear() {}
  
private:
  std::string key;
};


class Tag_Value_Generic : public Tag_Value
{
public:
  Tag_Value_Generic(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-generic"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Generic() {}
  
  static Generic_Statement_Maker< Tag_Value_Generic > statement_maker;

  virtual std::string eval(const std::string* key) { return ""; }
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return find_value(tags, *key); }
      
  virtual void prefetch(const Set_With_Context& set) {}
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const
  { return std::make_pair(std::vector< Set_Usage >(), Set_Usage::TAGS); }
  
  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }
  
  virtual void clear() {}
};


class Tag_Value_Count : public Tag_Value
{
public:
  enum Objects { nothing, nodes, ways, relations, deriveds, tags, members };
  
  Tag_Value_Count(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-count"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Count() {}
  
  static Generic_Statement_Maker< Tag_Value_Count > statement_maker;

  virtual std::string eval(const std::string* key);
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
      
  virtual void prefetch(const Set_With_Context& set);
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  
  virtual std::vector< std::string > used_tags() const;
  
  virtual void clear();
  
private:
  std::string input;
  Objects to_count;
  uint64 counter;
};


class Tag_Value_Pair_Operator : public Tag_Value
{
public:
  Tag_Value_Pair_Operator(int line_number_);
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}
  
  virtual string get_result_name() const { return ""; }
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const = 0;
  
  virtual std::string eval(const std::string* key);
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
      
  virtual void prefetch(const Set_With_Context& set);
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;
  
  virtual std::vector< std::string > used_tags() const;
  
  virtual void clear();
  
protected:
  Tag_Value* lhs;
  Tag_Value* rhs;
};


class Tag_Value_Prefix_Operator : public Tag_Value
{
public:
  Tag_Value_Prefix_Operator(int line_number_);
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman) {}
  
  virtual string get_result_name() const { return ""; }
  
  virtual std::string process(const std::string& rhs_result) const = 0;
  
  virtual std::string eval(const std::string* key);
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key);
      
  virtual void prefetch(const Set_With_Context& set);
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;

  virtual std::vector< std::string > used_tags() const;
  
  virtual void clear();
  
protected:
  Tag_Value* rhs;
};


class Tag_Value_And : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_And(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-and"; }
  virtual ~Tag_Value_And() {}
  
  static Generic_Statement_Maker< Tag_Value_And > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Or : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Or(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-or"; }
  virtual ~Tag_Value_Or() {}
  
  static Generic_Statement_Maker< Tag_Value_Or > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Not : public Tag_Value_Prefix_Operator
{
public:
  Tag_Value_Not(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-not"; }
  virtual ~Tag_Value_Not() {}
  
  static Generic_Statement_Maker< Tag_Value_Not > statement_maker;
  
  virtual std::string process(const std::string& rhs_result) const;
};


class Tag_Value_Equal : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Equal(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-equal"; }
  virtual ~Tag_Value_Equal() {}
  
  static Generic_Statement_Maker< Tag_Value_Equal > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Less : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Less(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-less"; }
  virtual ~Tag_Value_Less() {}
  
  static Generic_Statement_Maker< Tag_Value_Less > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Plus : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Plus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-plus"; }
  virtual ~Tag_Value_Plus() {}
  
  static Generic_Statement_Maker< Tag_Value_Plus > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Negate : public Tag_Value_Prefix_Operator
{
public:
  Tag_Value_Negate(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-negate"; }
  virtual ~Tag_Value_Negate() {}
  
  static Generic_Statement_Maker< Tag_Value_Negate > statement_maker;
  
  virtual std::string process(const std::string& rhs_result) const;
};


class Tag_Value_Minus : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Minus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-minus"; }
  virtual ~Tag_Value_Minus() {}
  
  static Generic_Statement_Maker< Tag_Value_Minus > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Times : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Times(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-times"; }
  virtual ~Tag_Value_Times() {}
  
  static Generic_Statement_Maker< Tag_Value_Times > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Tag_Value_Divided : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Divided(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-divided-by"; }
  virtual ~Tag_Value_Divided() {}
  
  static Generic_Statement_Maker< Tag_Value_Divided > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


struct Tag_Value_Aggregator : public Tag_Value
{
  enum Object_Type { tag, generic, id, type };
  
  
  Tag_Value_Aggregator(const string& func_name, int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);  
  virtual void add_statement(Statement* statement, string text);
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value) = 0;
  
  std::string eval_input(const std::string* key);
  
  virtual std::string eval(const std::string* key) { return eval_input(key); }
  virtual std::string eval(const Node_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Attic< Node_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Way_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Attic< Way_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Relation_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Attic< Relation_Skeleton >* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Area_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
  virtual std::string eval(const Derived_Skeleton* elem,
      const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
  { return eval_input(key); }
      
  virtual void prefetch(const Set_With_Context& set);
  
  virtual std::pair< std::vector< Set_Usage >, uint > used_sets() const;

  virtual std::vector< std::string > used_tags() const { return std::vector< std::string >(); }  
  virtual void clear() {}
  
  std::string input;
  Tag_Value* rhs;
  const Set_With_Context* input_set;
  bool value_set;
  std::string value;
};


class Tag_Value_Union_Value : public Tag_Value_Aggregator
{
public:
  Tag_Value_Union_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-union-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Union_Value() {}
  
  static Generic_Statement_Maker< Tag_Value_Union_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Tag_Value_Min_Value : public Tag_Value_Aggregator
{
public:
  Tag_Value_Min_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-min-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Min_Value() {}
  
  static Generic_Statement_Maker< Tag_Value_Min_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Tag_Value_Max_Value : public Tag_Value_Aggregator
{
public:
  Tag_Value_Max_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-max-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Max_Value() {}
  
  static Generic_Statement_Maker< Tag_Value_Max_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Tag_Value_Sum_Value : public Tag_Value_Aggregator
{
public:
  Tag_Value_Sum_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-sum-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Sum_Value() {}
  
  static Generic_Statement_Maker< Tag_Value_Sum_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
};


class Tag_Value_Set_Value : public Tag_Value_Aggregator
{
public:
  Tag_Value_Set_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-set-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Set_Value() {}
  
  static Generic_Statement_Maker< Tag_Value_Set_Value > statement_maker;
  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value);
  
  virtual void clear();
  
  std::vector< std::string > values;
};


std::pair< std::vector< Set_Usage >, uint > union_usage(const std::pair< std::vector< Set_Usage >, uint >& lhs,
    const std::pair< std::vector< Set_Usage >, uint >& rhs);


#endif
