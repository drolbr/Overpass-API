/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__USER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__USER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "../../expat/escape_json.h"
#include "../../expat/escape_xml.h"
#include "../data/utils.h"
#include "statement.h"


class User_Statement : public Output_Statement
{
  public:
    User_Statement(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
    virtual std::string get_name() const { return "user"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~User_Statement();

    struct Statement_Maker : public Generic_Statement_Maker< User_Statement >
    {
      Statement_Maker() : Generic_Statement_Maker< User_Statement >("user") {}
    };
    static Statement_Maker statement_maker;
    
    struct Criterion_Maker : public Statement::Criterion_Maker
    {
      virtual bool can_standalone(const std::string& type) { return true; }
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
      Criterion_Maker()
      {
        Statement::maker_by_ql_criterion()["uid"] = this;
        Statement::maker_by_ql_criterion()["user"] = this;
      }
    };
    static Criterion_Maker criterion_maker;

    virtual Query_Constraint* get_query_constraint();

    void calc_ranges
        (std::set< std::pair< Uint32_Index, Uint32_Index > >& node_req,
         std::set< std::pair< Uint31_Index, Uint31_Index > >& other_req,
         Transaction& transaction);
	
    // Reads the user id from the database.
    std::set< Uint32_Index > get_ids(Transaction& transaction);

    // Works only if get_id(Transaction&) has been called before.
    std::set< Uint32_Index > get_ids() const { return user_ids; }

    virtual std::string dump_xml(const std::string& indent) const
    {
      std::string result = indent + "<user" + std::string(" type=\"") + result_type + "\"";

      if (user_ids.size() == 1)
        result += " uid=\"" + to_string(user_ids.begin()->val()) + "\"";
      else
      {
        uint counter = 0;
        for (std::set< Uint32_Index >::const_iterator it = user_ids.begin(); it != user_ids.end(); ++it)
          result += " uid_" + to_string(++counter) + "=\"" + to_string(it->val()) + "\"";
      }

      if (user_names.size() == 1)
        result += " name=\"" + escape_xml(*user_names.begin()) + "\"";
      else
      {
        uint counter = 0;
        for (std::set< std::string >::const_iterator it = user_names.begin(); it != user_names.end(); ++it)
          result += " name_" + to_string(++counter) + "=\"" + escape_xml(*it) + "\"";
      }

      return result + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return result_type + dump_ql_in_query("") + dump_ql_result_name();
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }
    virtual std::string dump_ql_in_query(const std::string&) const
    {
      std::string result = user_ids.empty() ? "(user:" : "(uid:";

      if (!user_ids.empty())
      {
        std::set< Uint32_Index >::const_iterator it = user_ids.begin();
        result += to_string(it->val());
        for (++it; it != user_ids.end(); ++it)
          result += "," + to_string(it->val());
      }

      if (!user_names.empty())
      {
        std::set< std::string >::const_iterator it = user_names.begin();
        result += "\"" + escape_cstr(*it) + "\"";
        for (++it; it != user_names.end(); ++it)
          result += ",\"" + escape_cstr(*it) + "\"";
      }

      return result + ")";
    }

  private:
    std::string input;
    std::set< Uint32_Index > user_ids;
    std::set< std::string > user_names;
    std::string result_type;
    std::vector< Query_Constraint* > constraints;
    const Bbox_Double* bbox_limitation;
};

#endif
