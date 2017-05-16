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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__SET_TAG_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__SET_TAG_H


#include "../../expat/escape_json.h"
#include "../../expat/escape_xml.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


struct Set_Prop_Task
{
  enum Mode { single_key, set_id, generic };

  Set_Prop_Task(Eval_Task* rhs_, const std::string& key_, Mode mode_) : rhs(rhs_), key(key_), mode(mode_) {}
  ~Set_Prop_Task() { delete rhs; }

  void process(Derived_Structure& result, bool& id_set) const;

  void process(const Element_With_Context< Node_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Attic< Node_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Way_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Attic< Way_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Relation_Skeleton>& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Attic< Relation_Skeleton > >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Area_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;
  void process(const Element_With_Context< Derived_Skeleton >& data,
    const std::vector< std::string >& declared_keys, Derived_Structure& result, bool& id_set) const;

private:
  Eval_Task* rhs;
  std::string key;
  Mode mode;
};


class Set_Prop_Statement : public Statement
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Set_Prop_Statement >
  {
    virtual Statement* create_statement(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Statement_Maker() : Generic_Statement_Maker< Set_Prop_Statement >("set-prop")
    {
      Statement::maker_by_token()["="].push_back(this);
      Statement::maker_by_token()["!"].push_back(this);
    }
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const;
  virtual std::string dump_compact_ql(const std::string&) const;
  virtual std::string dump_pretty_ql(const std::string&) const { return dump_compact_ql(""); }

  Set_Prop_Statement(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "set-prop"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Set_Prop_Statement() {}

  virtual Requested_Context request_context() const;

  Set_Prop_Task* get_task(const Prepare_Task_Context& context);

  const std::string* get_key() const { return input != "" ? 0 : &keys.front(); }
  bool has_value() const { return tag_value; }
  bool should_set_id() const { return set_id; }

private:
  std::string input;
  std::vector< std::string > keys;
  bool set_id;
  Evaluator* tag_value;
};


#endif
