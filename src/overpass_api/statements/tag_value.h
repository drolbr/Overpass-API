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
  Evaluator_Fixed(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-fixed"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Fixed() {}
  
  static Generic_Statement_Maker< Evaluator_Fixed > statement_maker;

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


class Evaluator_Value : public Evaluator
{
public:
  Evaluator_Value(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-value"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Value() {}
  
  static Generic_Statement_Maker< Evaluator_Value > statement_maker;

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


class Evaluator_Generic : public Evaluator
{
public:
  Evaluator_Generic(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-generic"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Generic() {}
  
  static Generic_Statement_Maker< Evaluator_Generic > statement_maker;

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


#endif
