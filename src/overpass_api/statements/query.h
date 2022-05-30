/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__QUERY_H

#include <map>
#include <string>
#include <vector>
#include "../../expat/escape_json.h"
#include "../../expat/escape_xml.h"
#include "statement.h"


const int QUERY_NODE = 1;
const int QUERY_WAY = 2;
const int QUERY_CLOSED_WAY = 4;
const int QUERY_RELATION = 8;
const int QUERY_DERIVED = 16;
const int QUERY_AREA = 32;


typedef enum { nothing, /*ids_collected,*/ ranges_collected, data_collected } Answer_State;


class Regular_Expression;
class Bbox_Query_Statement;


template< typename Id >
struct Id_Constraint
{
  Id_Constraint() : invert(true) {}
  bool empty() const { return !invert && ids.empty(); }
  void restrict_to(const std::vector< Id >& ids);
  
  std::vector< Id > ids;
  bool invert;
};


/* === The Query Statement ===

The most important statement is the ''query'' statement. This is not a single statement but rather consists of one of the type specifiers ''node'', ''way'', ''relation'' (or shorthand ''rel''), ''derived'', ''area'', or ''nwr'' (shorthand for nodes, ways or relations) followed by one or more filters. The result set is the set of all elements that match the conditions of all the filters.

Example:

<source lang="cpp">
// one filter
  node[name="Foo"];
  way[name="Foo"];
  rel[name="Foo"];
  nwr[name="Foo"];
  derived[name="Foo"];
  area[name="Foo"];

// many filters
  node[name="Foo"][type="Bar"];
</source>

Here, ''node'', ''way'', ''rel'', ''nwr'', ''derived'', and ''area'' are the type specifier, ''[name="Foo"]'' resp. ''[type="Bar"]'' is the filter and the semicolon ends the statement.

The ''query'' statement has a result set that can be changed with the usual postfix notation.

<source lang="cpp">
  node[name="Foo"]->.a;
</source>

The individual filters may have in addition input sets that can be changed in the individual filters. Please see for this at the respective filter.
*/

class Query_Statement : public Output_Statement
{
  public:
    Query_Statement(int line_number_, const std::map< std::string, std::string >& input_attributes,
                    Parsed_Query& global_settings);
    virtual ~Query_Statement();
    virtual void add_statement(Statement* statement, std::string text);
    virtual std::string get_name() const { return "query"; }
    virtual void execute(Resource_Manager& rman);

    static Generic_Statement_Maker< Query_Statement > statement_maker;

    static bool area_query_exists() { return area_query_exists_; }

    static std::string to_string(int type)
    {
      if (type == QUERY_NODE)
        return "node";
      else if (type == QUERY_WAY)
        return "way";
      else if (type == QUERY_RELATION)
        return "relation";
      else if (type == QUERY_DERIVED)
        return "derived";
      else if (type == QUERY_AREA)
        return "area";
      else if (type == (QUERY_NODE | QUERY_WAY | QUERY_RELATION))
        return "nwr";
      else if (type == (QUERY_NODE | QUERY_WAY))
        return "nwr";
      else if (type == (QUERY_WAY | QUERY_RELATION))
        return "wr";
      else if (type == (QUERY_NODE | QUERY_RELATION))
        return "nr";

      return "area";
    }

    virtual std::string dump_xml(const std::string& indent) const
    {
      std::string result = indent + "<query" + dump_xml_result_name() + " type=\"" + to_string(type) + "\">\n";

      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += *it ? (*it)->dump_xml(indent + "  ") : "";

      return result + indent + "</query>\n";
    }

    virtual std::string dump_compact_ql(const std::string& indent) const { return dump_subquery_map_ql(indent, false); }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_subquery_map_ql(indent, true); }

    std::string dump_subquery_map_ql(const std::string& indent, bool pretty) const
    {
      std::string result = (pretty ? indent :  "") + to_string(type);

      uint proper_substatement_count = 0;
      for (std::vector< Statement* >::const_iterator it = substatements.begin();
          it != substatements.end(); ++it)
      {
        if ((*it)->get_name() == "item")
          result += (*it)->dump_ql_in_query("");
        else
          ++proper_substatement_count;
      }

      std::string prefix = (pretty && proper_substatement_count > 1 ? "\n  " + indent : "");

      for (std::vector< Statement* >::const_iterator it = substatements.begin();
          it != substatements.end(); ++it)
      {
        if ((*it)->get_name() != "item")
          result += prefix + (*it)->dump_ql_in_query("");
      }

      if (indent == "(bbox)" && type != QUERY_AREA)
        result += "(bbox)";

      return result + (pretty && proper_substatement_count > 1 && dump_ql_result_name() != "" ? "\n  " + indent : "")
          + dump_ql_result_name() + ";";
    }

  private:
    int type;
    std::vector< std::string > keys;
    std::vector< std::pair< std::string, std::string > > key_values;
    std::vector< std::pair< std::string, Regular_Expression* > > key_regexes;
    std::vector< std::pair< Regular_Expression*, Regular_Expression* > > regkey_regexes;
    std::vector< std::pair< std::string, std::string > > key_nvalues;
    std::vector< std::pair< std::string, Regular_Expression* > > key_nregexes;
    std::vector< std::pair< Regular_Expression*, Regular_Expression* > > regkey_nregexes;
    std::vector< Query_Constraint* > constraints;
    std::vector< Statement* > substatements;
    Bbox_Query_Statement* global_bbox_statement;

    static bool area_query_exists_;

    template< typename Skeleton, typename Id_Type >
    std::vector< std::pair< Id_Type, Uint31_Index > > collect_ids
        (const File_Properties& file_prop, const File_Properties& attic_file_prop,
         Resource_Manager& rman, uint64 timestamp, Query_Filter_Strategy& check_keys_late, bool& result_valid);

    template< class Id_Type >
    std::vector< Id_Type > collect_ids
        (const File_Properties& file_prop,
         Resource_Manager& rman, Query_Filter_Strategy check_keys_late);

    template< class Id_Type >
    void filter_non_ids
        (std::vector< std::pair< Id_Type, Uint31_Index > >& ids,
         const File_Properties& file_prop, const File_Properties& attic_file_prop,
         Resource_Manager& rman, uint64 timestamp);

    template< class Id_Type >
    std::vector< std::pair< Id_Type, Uint31_Index > > collect_non_ids
        (const File_Properties& file_prop, const File_Properties& attic_file_prop,
         Resource_Manager& rman, uint64 timestamp);

    template< class Id_Type >
    std::vector< Id_Type > collect_non_ids
        (const File_Properties& file_prop, Resource_Manager& rman);

    void get_elements_by_id_from_db
        (std::map< Uint31_Index, std::vector< Area_Skeleton > >& elements,
	 const std::vector< Area_Skeleton::Id_Type >& ids, bool invert_ids,
         Resource_Manager& rman, File_Properties& file_prop);

    template< class TIndex, class TObject >
    void filter_by_tags
        (std::map< TIndex, std::vector< TObject > >& items,
         std::map< TIndex, std::vector< Attic< TObject > > >* attic_items,
         uint64 timestamp,
         const File_Properties& file_prop, const File_Properties* attic_file_prop,
         Resource_Manager& rman, Transaction& transaction);

    template< class TIndex, class TObject >
    void filter_by_tags
        (std::map< TIndex, std::vector< TObject > >& items,
         const File_Properties& file_prop,
         Resource_Manager& rman, Transaction& transaction);

    void filter_by_tags(std::map< Uint31_Index, std::vector< Derived_Structure > >& items);

    template< typename Skeleton, typename Id_Type, typename Index >
    void progress_1(
        Id_Constraint< Id_Type >& ids, std::vector< Index >& range_req,
        uint64 timestamp, Answer_State& answer_state, Query_Filter_Strategy& check_keys_late,
        const File_Properties& file_prop, const File_Properties& attic_file_prop, Resource_Manager& rman);

    template< class Id_Type >
    void progress_1(
        Id_Constraint< Id_Type >& ids, Answer_State& answer_state, Query_Filter_Strategy check_keys_late,
        const File_Properties& file_prop, Resource_Manager& rman);

    template< class Id_Type >
    void collect_nodes(
        const Id_Constraint< Id_Type >& ids, Answer_State& answer_state, Set& into, Resource_Manager& rman);
    template< class Id_Type >
    void collect_elems(
        int type, const Id_Constraint< Id_Type >& ids, Answer_State& answer_state, Set& into, Resource_Manager& rman);

    void collect_elems(Answer_State& answer_state, Set& into, Resource_Manager& rman);
    void apply_all_filters(
        Resource_Manager& rman, uint64 timestamp, Query_Filter_Strategy check_keys_late, Set& into);
};


class Has_Kv_Statement : public Statement
{
  public:
    Has_Kv_Statement(int line_number_, const std::map< std::string, std::string >& input_attributes,
                     Parsed_Query& global_settings);
    virtual std::string get_name() const { return "has-kv"; }
    virtual std::string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman) {}
    virtual ~Has_Kv_Statement();

    static Generic_Statement_Maker< Has_Kv_Statement > statement_maker;

    std::string get_key() const { return key_regex ? "" : key; }
    Regular_Expression* get_key_regex() { return key_regex; }
    std::string get_value() const { return regex ? "" : value; }
    Regular_Expression* get_regex() { return regex; }
    bool get_straight() const { return straight; }

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<has-kv"
          + (key != "" ? (key_regex ? std::string(" regk=\"") : std::string(" k=\"")) + escape_xml(key) + "\"" : "")
          + (value != "" ? (regex ? std::string(" regv=\"") : std::string(" v=\"")) + escape_xml(value) + "\"" : "")
          + (straight ? "" : " modv=\"not\"")
          + (case_sensitive ? "" : " case=\"ignore\"")
          + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return std::string("[")
          + (key_regex ? "~\"" : "\"") + escape_cstr(key) + "\""
          + (value != "" ? std::string(straight ? "" : "!") + (regex ? "~\"" : "=\"") + escape_cstr(value) + "\"" : "")
          + (case_sensitive ? "" : ",i")
          + "]";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_compact_ql(indent); }

  private:
    std::string key, value;
    Regular_Expression* regex;
    Regular_Expression* key_regex;
    bool straight;
    bool case_sensitive;
};

#endif
