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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H


#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Set_Tag_Statement;


class Make_Statement : public Output_Statement
{
public:
  Make_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "make"; }
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman);
  virtual ~Make_Statement();
  static Generic_Statement_Maker< Make_Statement > statement_maker;
    
  std::string get_source_name() const { return input; }

private:
  std::string input;
  std::string type;
  std::vector< Set_Tag_Statement* > evaluators;
};


struct Tag_Value : public Statement
{
  Tag_Value(int line_number) : Statement(line_number) {}
  
  virtual std::string eval(const std::map< std::string, Set >& sets) const = 0;
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
    
  std::string get_key() const { return key; }
  std::string eval(const std::map< std::string, Set >& sets) const { return tag_value ? tag_value->eval(sets) : ""; }
    
private:
  std::string key;
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
  
  virtual std::string eval(const std::map< std::string, Set >& sets) const;
  
private:
  std::string value;
};


class Tag_Value_Count : public Tag_Value
{
public:
  enum Objects { nothing, nodes, ways, relations, deriveds };
  
  Tag_Value_Count(int line_number_, const map< string, string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual string get_name() const { return "value-count"; }
  virtual string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Tag_Value_Count() {}
  
  static Generic_Statement_Maker< Tag_Value_Count > statement_maker;
  
  virtual std::string eval(const std::map< std::string, Set >& sets) const;
  
private:
  std::string input;
  Objects to_count;
};


#endif
