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
#include "statement.h"
#include "tag_value.h"

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
  Evaluator* lhs;
  Evaluator* rhs;
};


class Evaluator_And : public Evaluator_Pair_Operator
{
public:
  Evaluator_And(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-and"; }
  virtual ~Evaluator_And() {}
  
  static Generic_Statement_Maker< Evaluator_And > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Or : public Evaluator_Pair_Operator
{
public:
  Evaluator_Or(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-or"; }
  virtual ~Evaluator_Or() {}
  
  static Generic_Statement_Maker< Evaluator_Or > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Equal : public Evaluator_Pair_Operator
{
public:
  Evaluator_Equal(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-equal"; }
  virtual ~Evaluator_Equal() {}
  
  static Generic_Statement_Maker< Evaluator_Equal > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Less : public Evaluator_Pair_Operator
{
public:
  Evaluator_Less(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-less"; }
  virtual ~Evaluator_Less() {}
  
  static Generic_Statement_Maker< Evaluator_Less > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Plus : public Evaluator_Pair_Operator
{
public:
  Evaluator_Plus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-plus"; }
  virtual ~Evaluator_Plus() {}
  
  static Generic_Statement_Maker< Evaluator_Plus > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Minus : public Evaluator_Pair_Operator
{
public:
  Evaluator_Minus(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-minus"; }
  virtual ~Evaluator_Minus() {}
  
  static Generic_Statement_Maker< Evaluator_Minus > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Times : public Evaluator_Pair_Operator
{
public:
  Evaluator_Times(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-times"; }
  virtual ~Evaluator_Times() {}
  
  static Generic_Statement_Maker< Evaluator_Times > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


class Evaluator_Divided : public Evaluator_Pair_Operator
{
public:
  Evaluator_Divided(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "eval-divided-by"; }
  virtual ~Evaluator_Divided() {}
  
  static Generic_Statement_Maker< Evaluator_Divided > statement_maker;
  
  virtual std::string process(const std::string& lhs_result, const std::string& rhs_result) const;
};


#endif
