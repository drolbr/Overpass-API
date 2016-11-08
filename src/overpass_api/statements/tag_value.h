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

#include <map>
#include <string>
#include <vector>


struct Tag_Value : public Statement
{
  Tag_Value(int line_number) : Statement(line_number) {}
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const = 0;
  
  virtual uint needs_tags(const std::string& set_name) const { return 0; }
  const static uint SKELETON = 1;
  const static uint TAGS = 2;
  
  virtual void tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) {}
  virtual void clear() {}
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
    
  const std::string* get_key() const { return input != "" ? 0 : &keys.front(); }
  const std::vector< std::string >* get_keys() const { return input != "" ? &keys : 0; }
  void set_keys(const std::vector< std::string >& keys_) { keys = keys_; }
  bool should_set_id() const { return set_id; }
  Tag_Value* get_tag_value() const { return tag_value; }
  const std::string& get_input_name() const { return input; }
  std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const
  { return tag_value ? tag_value->eval(sets, tag) : ""; }
    
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
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
  
private:
  std::string value;
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
  
  virtual uint needs_tags(const std::string& set_name) const;
  virtual void tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void clear() { counter = 0; }
  
  static Generic_Statement_Maker< Tag_Value_Count > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
  
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
  
  virtual uint needs_tags(const std::string& set_name) const
  {
    if (lhs && rhs)
      return lhs->needs_tags(set_name) | rhs->needs_tags(set_name);
    else if (lhs)
      return lhs->needs_tags(set_name);
    else if (rhs)
      return rhs->needs_tags(set_name);
    return 0;
  }
  virtual void tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
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
  
  virtual uint needs_tags(const std::string& set_name) const
  {
    if (rhs)
      return rhs->needs_tags(set_name);
    return 0;
  }
  virtual void tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
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
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Or : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Or(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-or"; }
  virtual ~Tag_Value_Or() {}
  
  static Generic_Statement_Maker< Tag_Value_Or > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Not : public Tag_Value_Prefix_Operator
{
public:
  Tag_Value_Not(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-not"; }
  virtual ~Tag_Value_Not() {}
  
  static Generic_Statement_Maker< Tag_Value_Not > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Equal : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Equal(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-equal"; }
  virtual ~Tag_Value_Equal() {}
  
  static Generic_Statement_Maker< Tag_Value_Equal > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Less : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Less(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-less"; }
  virtual ~Tag_Value_Less() {}
  
  static Generic_Statement_Maker< Tag_Value_Less > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Plus : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Plus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-plus"; }
  virtual ~Tag_Value_Plus() {}
  
  static Generic_Statement_Maker< Tag_Value_Plus > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Negate : public Tag_Value_Prefix_Operator
{
public:
  Tag_Value_Negate(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-negate"; }
  virtual ~Tag_Value_Negate() {}
  
  static Generic_Statement_Maker< Tag_Value_Negate > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Minus : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Minus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-minus"; }
  virtual ~Tag_Value_Minus() {}
  
  static Generic_Statement_Maker< Tag_Value_Minus > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Times : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Times(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-times"; }
  virtual ~Tag_Value_Times() {}
  
  static Generic_Statement_Maker< Tag_Value_Times > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


class Tag_Value_Divided : public Tag_Value_Pair_Operator
{
public:
  Tag_Value_Divided(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-divided-by"; }
  virtual ~Tag_Value_Divided() {}
  
  static Generic_Statement_Maker< Tag_Value_Divided > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
};


struct Tag_Value_Aggregator : public Tag_Value
{
  enum Object_Type { tag, generic, id, type };
  
  
  Tag_Value_Aggregator(const string& func_name, int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);  
  
  virtual uint needs_tags(const std::string& set_name) const { return set_name == input ? TAGS : 0; }
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
  virtual void update_value(const std::string& id, const std::string& type,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value) = 0;
  virtual void clear();
  
  virtual void tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);
  virtual void tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags);  
  
  std::string input;
  std::string key;
  Object_Type key_type;
  std::string value;
  bool value_set;
  mutable std::map< std::string, std::string > value_per_key;  
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
  
  virtual void update_value(const std::string& id, const std::string& type,
      const std::vector< std::pair< std::string, std::string > >* tags);  
  virtual std::string update_value(const std::string& agg_value, const std::string& new_value) { return ""; }
  virtual void clear();
  
  virtual std::string eval(const std::map< std::string, Set >& sets, const std::string* tag) const;
  
private:
  mutable std::vector< std::string > values;
  mutable std::map< std::string, std::vector< std::string > > values_per_key;
};


#endif
