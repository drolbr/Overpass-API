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


#include "../data/utils.h"
#include "aggregators.h"
#include "binary_operators.h"
#include "explicit_geometry.h"
#include "id_query.h"
#include "make.h"
#include "print.h"
#include "set_list_operators.h"
#include "set_prop.h"
#include "string_endomorphisms.h"
#include "tag_value.h"
#include "ternary_operator.h"
#include "testing_tools.h"
#include "unary_operators.h"
#include "union.h"


Statement* add_prop_stmt(const std::string& value, Statement* parent, Statement_Container& stmt_cont)
{
  return stmt_cont.create_stmt< Set_Prop_Statement >(Attr()("k", value).kvs(), parent);
}


Statement* add_fixed_stmt(const std::string& value, Statement* parent, Statement_Container& stmt_cont)
{
  return stmt_cont.create_stmt< Evaluator_Fixed >(Attr()("v", value).kvs(), parent);
}


void attribute_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string into, std::string type)
{
  Resource_Manager rman(transaction, &global_settings);

  Make_Statement stmt(0, Attr()("into", into)("type", type).kvs(), global_settings);
  stmt.execute(rman);

  if (into == "_")
    Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
  else
    Print_Statement(0, Attr()("from", "target").kvs(), global_settings).execute(rman);
}


void plain_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key1, std::string value1, std::string key2 = "", std::string value2 = "")
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt(key1, &stmt, stmt_cont);
  add_fixed_stmt(value1, subs, stmt_cont);

  if (key2 != "")
  {
    subs = add_prop_stmt(key2, &stmt, stmt_cont);
    add_fixed_stmt(value2, subs, stmt_cont);
  }

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void count_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  {
    Union_Statement union_(0, (from == "" ? Attr() : Attr()("into", from)).kvs(), global_settings);

    Id_Query_Statement stmt1(0, Attr()("type", "node")("ref", to_string(ref + global_node_offset)).kvs(),
                             global_settings);
    union_.add_statement(&stmt1, "");

    Id_Query_Statement stmt2(0, Attr()("type", "way")("ref", to_string(ref)).kvs(), global_settings);
    union_.add_statement(&stmt2, "");

    Id_Query_Statement stmt3(0, Attr()("type", "relation")("ref", to_string(ref)).kvs(), global_settings);
    union_.add_statement(&stmt3, "");

    Make_Statement stmt4(0, Attr()("type", "foo").kvs(), global_settings);
    union_.add_statement(&stmt4, "");

    union_.execute(rman);
  }

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("nodes", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Count(0,
      (from == "" ? Attr() : Attr()("from", from))("type", "nodes").kvs(),
      global_settings), subs);

  subs = add_prop_stmt("ways", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Count(0,
      (from == "" ? Attr() : Attr()("from", from))("type", "ways").kvs(),
      global_settings), subs);

  subs = add_prop_stmt("relations", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Count(0,
      (from == "" ? Attr() : Attr()("from", from))("type", "relations").kvs(),
      global_settings), subs);

  subs = add_prop_stmt("deriveds", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Count(0,
      (from == "" ? Attr() : Attr()("from", from))("type", "deriveds").kvs(),
      global_settings), subs);

  subs = add_prop_stmt("tags", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Sum_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(),
      global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Properties_Count(0, Attr()("type", "tags").kvs(), global_settings),
                            subs);

  subs = add_prop_stmt("members", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Sum_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(),
      global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Properties_Count(0, Attr()("type", "members").kvs(), global_settings),
                            subs);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


template< typename Evaluator_Pair >
void pair_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key, std::string value1, std::string value2)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt(key, &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Pair(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(value1, subs, stmt_cont);
  add_fixed_stmt(value2, subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void triple_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key, std::string condition, std::string value1, std::string value2)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt(key, &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Ternary_Evaluator(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(condition, subs, stmt_cont);
  add_fixed_stmt(value1, subs, stmt_cont);
  add_fixed_stmt(value2, subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


template< typename Evaluator_Prefix >
void prefix_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key, std::string value)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt(key, &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Prefix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(value, subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void prepare_value_test(Parsed_Query& global_settings, Resource_Manager& rman,
    std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Statement_Container stmt_cont(global_settings);
  Union_Statement union_(0, (from == "" ? Attr() : Attr()("into", from)).kvs(), global_settings);

  stmt_cont.add_stmt(new Id_Query_Statement(0,
      Attr()("type", "node")("ref", to_string(ref1 + global_node_offset)).kvs(), global_settings), &union_);
  stmt_cont.add_stmt(new Id_Query_Statement(0,
      Attr()("type", "way")("ref", to_string(ref1)).kvs(), global_settings), &union_);
  stmt_cont.add_stmt(new Id_Query_Statement(0,
      Attr()("type", "relation")("ref", to_string(ref1)).kvs(), global_settings), &union_);
  if (ref1 != ref2)
    stmt_cont.add_stmt(new Id_Query_Statement(0,
        Attr()("type", "node")("ref", to_string(ref2 + global_node_offset)).kvs(), global_settings), &union_);

  union_.execute(rman);
}


void union_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref, ref, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("node_key", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Union_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "node_key").kvs(), global_settings), subs);

  subs = add_prop_stmt("way_key", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Union_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "way_key").kvs(), global_settings), subs);

  subs = add_prop_stmt("relation_key", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Union_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "relation_key").kvs(), global_settings), subs);

  subs = add_prop_stmt("unused_key", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Union_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "unused_key").kvs(), global_settings), subs);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void min_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("node_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Min_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "node_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("way_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Min_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "way_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("relation_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Min_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "relation_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("unused_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Min_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "unused_key_7").kvs(), global_settings), subs);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void max_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("node_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "node_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("way_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "way_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("relation_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "relation_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("unused_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "unused_key_7").kvs(), global_settings), subs);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void set_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("node_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "node_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("way_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "way_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("relation_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "relation_key_7").kvs(), global_settings), subs);

  subs = add_prop_stmt("unused_key_7", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Value(0, Attr()("k", "unused_key_7").kvs(), global_settings), subs);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void generic_key_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2,
    bool set_value_const, bool exclude_a_key,
    uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = stmt_cont.create_stmt< Set_Prop_Statement >(
      Attr()("keytype", "generic")("from", from).kvs(), &stmt);
  
  if (set_value_const)
    add_fixed_stmt("...", subs, stmt_cont);
  else
  {
    subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
        (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
    subs = stmt_cont.add_stmt(new Evaluator_Generic(0, Attr().kvs(), global_settings), subs);
  }
  
  if (exclude_a_key)
    subs = stmt_cont.create_stmt< Set_Prop_Statement >(
      Attr()("keytype", "tag")("k", "way_key").kvs(), &stmt);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void value_id_type_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref, ref+1, global_node_offset);
  
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("id", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      from == "_" ? Attr().kvs() : Attr()("from", from).kvs(),
      global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), subs);

  subs = add_prop_stmt("type", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      from == "_" ? Attr().kvs() : Attr()("from", from).kvs(),
      global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Type(0, Attr().kvs(), global_settings), subs);

  subs = add_prop_stmt("is_closed", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Set_Value(0,
      from == "_" ? Attr().kvs() : Attr()("from", from).kvs(),
      global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Closed(0, Attr().kvs(), global_settings), subs);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void number_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("nan", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Number(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("-.", subs, stmt_cont);

  subs = add_prop_stmt("three", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Number(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("3.", subs, stmt_cont);

  subs = add_prop_stmt("one_trillion", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Number(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e12", subs, stmt_cont);

  subs = add_prop_stmt("minus_fourty-two", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Number(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("-42", subs, stmt_cont);

  subs = add_prop_stmt("is_nan", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Num(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("-.", subs, stmt_cont);

  subs = add_prop_stmt("is_three", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Num(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("3.", subs, stmt_cont);

  subs = add_prop_stmt("is_one_trillion", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Num(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e12", subs, stmt_cont);

  subs = add_prop_stmt("is_minus_fourty-two", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Num(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("-42", subs, stmt_cont);

  subs = add_prop_stmt("empty_isnt_num", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Num(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("", subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void date_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("year_only", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("2006", subs, stmt_cont);

  subs = add_prop_stmt("year_month_day", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("2012-09-13", subs, stmt_cont);

  subs = add_prop_stmt("full_iso", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("2013-01-02T12:30:45Z", subs, stmt_cont);

  subs = add_prop_stmt("nonsense", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("christmas_day", subs, stmt_cont);

  subs = add_prop_stmt("is_year", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("2006", subs, stmt_cont);

  subs = add_prop_stmt("is_year_month_day", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("2012-09-13", subs, stmt_cont);

  subs = add_prop_stmt("is_full_iso", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("2013-01-02T12:30:45Z", subs, stmt_cont);

  subs = add_prop_stmt("is_nonsense", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("christmas_day", subs, stmt_cont);

  subs = add_prop_stmt("empty_isnt_date", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Is_Date(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("", subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void suffix_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("empty", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("", subs, stmt_cont);

  subs = add_prop_stmt("pure", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e100", subs, stmt_cont);

  subs = add_prop_stmt("unit", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("5.5m", subs, stmt_cont);

  subs = add_prop_stmt("whitespace", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("200 ", subs, stmt_cont);

  subs = add_prop_stmt("whitespace_and_unit", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("40 m/s", subs, stmt_cont);

  subs = add_prop_stmt("second_number", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("4 2", subs, stmt_cont);

  subs = add_prop_stmt("comma_sep_number", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("3,14", subs, stmt_cont);

  subs = add_prop_stmt("possible_exp", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e", subs, stmt_cont);

  subs = add_prop_stmt("misc", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Suffix(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("9 3/4", subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void lrs_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);
  
  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = add_prop_stmt("lrs_in_1_positive", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a", subs, stmt_cont);
  add_fixed_stmt("a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_1_negative", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("z", subs, stmt_cont);
  add_fixed_stmt("a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_2_positive_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a", subs, stmt_cont);
  add_fixed_stmt("a;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_2_positive_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("b", subs, stmt_cont);
  add_fixed_stmt("a;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_2_negative", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("z", subs, stmt_cont);
  add_fixed_stmt("a;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_3_positive_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a  ", subs, stmt_cont);
  add_fixed_stmt("a;b;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_3_positive_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("b", subs, stmt_cont);
  add_fixed_stmt("a;b;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_3_positive_3", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("  c", subs, stmt_cont);
  add_fixed_stmt("a;b;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_3_negative", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("z", subs, stmt_cont);
  add_fixed_stmt("a;b;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_space_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("", subs, stmt_cont);
  add_fixed_stmt("a;  ;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_space_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("  ", subs, stmt_cont);
  add_fixed_stmt("a;;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_space_3", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("  ", subs, stmt_cont);
  add_fixed_stmt("\t", subs, stmt_cont);

  subs = add_prop_stmt("lrs_in_space_4", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_In(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("  ", subs, stmt_cont);
  add_fixed_stmt("", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_11", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(" foo", subs, stmt_cont);
  add_fixed_stmt("foo ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_12", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("\t", subs, stmt_cont);
  add_fixed_stmt("\n", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_21", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("b;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_22", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a ; b", subs, stmt_cont);
  add_fixed_stmt("b ; a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_23", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(" a;b ", subs, stmt_cont);
  add_fixed_stmt(" b;a ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_24", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;a", subs, stmt_cont);
  add_fixed_stmt("a;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_31", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("b;c;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_32", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(" a; b;c ", subs, stmt_cont);
  add_fixed_stmt(" c;a ;b ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_self_33", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;b", subs, stmt_cont);
  add_fixed_stmt("b;a;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_zero_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("c;d", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_zero_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_one_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("d;a;e", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_one_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("d;b;e", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_two_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("c;d;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_isect_two_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Isect(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;;b", subs, stmt_cont);
  add_fixed_stmt("a; ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_11", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(" foo", subs, stmt_cont);
  add_fixed_stmt("foo ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_12", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("\t", subs, stmt_cont);
  add_fixed_stmt("\n", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_21", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("b;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_22", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a ; b", subs, stmt_cont);
  add_fixed_stmt("b ; a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_23", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(" a;b ", subs, stmt_cont);
  add_fixed_stmt(" b;a ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_24", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;a", subs, stmt_cont);
  add_fixed_stmt("a;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_31", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("b;c;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_32", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(" a; b;c ", subs, stmt_cont);
  add_fixed_stmt(" c;a ;b ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_self_33", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;b", subs, stmt_cont);
  add_fixed_stmt("b;a;a", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_disjoint_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("c;d", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_zero_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_one_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("d;a;e", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_one_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("d;b;e", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_two_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);
  add_fixed_stmt("c;d;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_union_two_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Union(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);
  add_fixed_stmt("a; ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_one_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("  foo", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_one_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("foo  ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_one_3", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e3", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_one_4", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("100000000000000001", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_two_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_two_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("9;10", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_two_3", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e-2;1e-1", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_two_4", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("9 bis;10 bis", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_three_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_max_three_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Max(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt(".;9;10", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_one_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("  foo", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_one_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("foo  ", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_one_3", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e3", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_one_4", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("100000000000000001", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_two_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_two_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("9;10", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_two_3", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("1e-2;1e-1", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_two_4", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("9 bis;10 bis", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_three_1", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;b;c", subs, stmt_cont);

  subs = add_prop_stmt("lrs_min_three_2", &stmt, stmt_cont);
  subs = stmt_cont.add_stmt(new Evaluator_Lrs_Min(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("a;9;10", subs, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void key_id_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);
  if (ref > 0)
    prepare_value_test(global_settings, rman, from, ref, ref+1, global_node_offset);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = stmt_cont.add_stmt(
      new Set_Prop_Statement(0, Attr()("keytype", "id").kvs(), global_settings), &stmt);

  if (ref > 0)
  {
    subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0,
        (from == "" ? Attr() : Attr()("from", from)).kvs(), global_settings), subs);
    stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), subs);
  }
  else
    add_fixed_stmt("42", subs, stmt_cont);

  stmt.execute(rman);

  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void make_point_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 ref, const std::string& lat, const std::string& lon, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", ref, ref, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = stmt_cont.create_stmt< Set_Prop_Statement >(Attr()("keytype", "geometry").kvs(), &stmt);
  subs = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
  if (lon.empty())
  {
    add_fixed_stmt(lat, subs, stmt_cont);
    subs = stmt_cont.add_stmt(new Evaluator_Times(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt(".0000001", subs, stmt_cont);
    subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0, Attr()("from", "_").kvs(), global_settings), subs);
    stmt_cont.add_stmt(new Evaluator_Length(0, Attr().kvs(), global_settings), subs);
  }
  else
  {
    add_fixed_stmt(lat, subs, stmt_cont);
    add_fixed_stmt(lon, subs, stmt_cont);
  }

  stmt.execute(rman);
  Print_Statement(0, Attr()("geometry", "full").kvs(), global_settings).execute(rman);
}


void make_linestring_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 ref, uint num_points, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", ref, ref, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = stmt_cont.create_stmt< Set_Prop_Statement >(Attr()("keytype", "geometry").kvs(), &stmt);
  subs = stmt_cont.add_stmt(new Evaluator_Linestring(0, Attr().kvs(), global_settings), subs);
  
  if (num_points > 0)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.1", pt, stmt_cont);
    add_fixed_stmt("7.1", pt, stmt_cont);
  }
  if (num_points > 1)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.2", pt, stmt_cont);
    add_fixed_stmt("7.2", pt, stmt_cont);
  }
  if (num_points > 2)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.3", pt, stmt_cont);
    add_fixed_stmt("7.3", pt, stmt_cont);
  }
  if (num_points > 3)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.4", pt, stmt_cont);
    subs = stmt_cont.add_stmt(new Evaluator_Times(0, Attr().kvs(), global_settings), pt);
    add_fixed_stmt(".0000001", subs, stmt_cont);
    subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0, Attr()("from", "_").kvs(), global_settings), subs);
    stmt_cont.add_stmt(new Evaluator_Length(0, Attr().kvs(), global_settings), subs);
  }

  stmt.execute(rman);
  Print_Statement(0, Attr()("geometry", "full").kvs(), global_settings).execute(rman);
}


void make_polygon_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 ref, uint num_points, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", ref, ref, global_node_offset);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = stmt_cont.create_stmt< Set_Prop_Statement >(Attr()("keytype", "geometry").kvs(), &stmt);
  Statement* poly = stmt_cont.add_stmt(new Evaluator_Polygon(0, Attr().kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Linestring(0, Attr().kvs(), global_settings), poly);
  
  if (num_points > 0)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt(num_points <= 4 ? "51.1" : "51.101", pt, stmt_cont);
    add_fixed_stmt("7.1", pt, stmt_cont);
  }
  if (num_points > 1)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.4", pt, stmt_cont);
    add_fixed_stmt("7.2", pt, stmt_cont);
  }
  if (num_points > 2)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.3", pt, stmt_cont);
    add_fixed_stmt("7.3", pt, stmt_cont);
  }
  if (num_points > 3)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("41.4", pt, stmt_cont);
    subs = stmt_cont.add_stmt(new Evaluator_Times(0, Attr().kvs(), global_settings), pt);
    add_fixed_stmt(".0000001", subs, stmt_cont);
    subs = stmt_cont.add_stmt(new Evaluator_Max_Value(0, Attr()("from", "_").kvs(), global_settings), subs);
    stmt_cont.add_stmt(new Evaluator_Length(0, Attr().kvs(), global_settings), subs);
  }
  if (num_points > 4)
  {
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.101", pt, stmt_cont);
    add_fixed_stmt("7.1", pt, stmt_cont);
  }
  
  if (num_points > 5)
  {
    subs = stmt_cont.add_stmt(new Evaluator_Linestring(0, Attr().kvs(), global_settings), poly);
    
    Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.31", pt, stmt_cont);
    add_fixed_stmt("7.2", pt, stmt_cont);
    pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.29", pt, stmt_cont);
    add_fixed_stmt("7.19", pt, stmt_cont);
    pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
    add_fixed_stmt("51.29", pt, stmt_cont);
    add_fixed_stmt("7.21", pt, stmt_cont);
  }

  stmt.execute(rman);
  Print_Statement(0, Attr()("geometry", "full").kvs(), global_settings).execute(rman);
}


void make_polygon_date_line_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  Statement_Container stmt_cont(global_settings);

  Make_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Statement* subs = stmt_cont.create_stmt< Set_Prop_Statement >(Attr()("keytype", "geometry").kvs(), &stmt);
  Statement* poly = stmt_cont.add_stmt(new Evaluator_Polygon(0, Attr().kvs(), global_settings), subs);
  subs = stmt_cont.add_stmt(new Evaluator_Linestring(0, Attr().kvs(), global_settings), poly);
  
  Statement* pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("45", pt, stmt_cont);
  add_fixed_stmt("179.99", pt, stmt_cont);
  
  pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("44.99", pt, stmt_cont);
  add_fixed_stmt("-179.99", pt, stmt_cont);
  
  pt = stmt_cont.add_stmt(new Evaluator_Point(0, Attr().kvs(), global_settings), subs);
  add_fixed_stmt("44.98", pt, stmt_cont);
  add_fixed_stmt("179.99", pt, stmt_cont);

  stmt.execute(rman);
  Print_Statement(0, Attr()("geometry", "full").kvs(), global_settings).execute(rman);
}


int main(int argc, char* args[])
{
  if (argc < 5)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir node_id_offset\n";
    return 0;
  }
  std::string test_to_execute = args[1];
  //uint pattern_size = atoi(args[2]);
  uint64 global_node_offset = atoll(args[4]);

  try
  {
    Nonsynced_Transaction transaction(false, false, args[3], "");
    Parsed_Query global_settings;
    global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    if ((test_to_execute == "") || (test_to_execute == "1"))
      attribute_test(global_settings, transaction, "_", "one");
    if ((test_to_execute == "") || (test_to_execute == "2"))
      attribute_test(global_settings, transaction, "_", "two");
    if ((test_to_execute == "") || (test_to_execute == "3"))
      attribute_test(global_settings, transaction, "target", "into_target");
    if ((test_to_execute == "") || (test_to_execute == "4"))
      plain_value_test(global_settings, transaction, "with-tags", "single", "value");
    if ((test_to_execute == "") || (test_to_execute == "5"))
      plain_value_test(global_settings, transaction, "with-tags", "not", "in", "alphabetic", "order");
    if ((test_to_execute == "") || (test_to_execute == "6"))
      count_test(global_settings, transaction, "count-from-default", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "7"))
      count_test(global_settings, transaction, "count-from-default", "_", 0, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "8"))
      count_test(global_settings, transaction, "count-from-foo", "foo", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "9"))
      pair_test< Evaluator_And >(global_settings, transaction, "test-and", "and", "1", "0");
    if ((test_to_execute == "") || (test_to_execute == "10"))
      pair_test< Evaluator_And >(global_settings, transaction, "test-and", "and", "0", "1");
    if ((test_to_execute == "") || (test_to_execute == "11"))
      pair_test< Evaluator_And >(global_settings, transaction, "test-and", "and", "false", "false");
    if ((test_to_execute == "") || (test_to_execute == "12"))
      pair_test< Evaluator_And >(global_settings, transaction, "test-and", "and", "true", "");
    if ((test_to_execute == "") || (test_to_execute == "13"))
      pair_test< Evaluator_Or >(global_settings, transaction, "test-or", "or", "1", "0");
    if ((test_to_execute == "") || (test_to_execute == "14"))
      pair_test< Evaluator_Or >(global_settings, transaction, "test-or", "or", "0", "1");
    if ((test_to_execute == "") || (test_to_execute == "15"))
      pair_test< Evaluator_Or >(global_settings, transaction, "test-or", "or", "true", "true");
    if ((test_to_execute == "") || (test_to_execute == "16"))
      pair_test< Evaluator_Or >(global_settings, transaction, "test-or", "or", "", "");
    if ((test_to_execute == "") || (test_to_execute == "17"))
      prefix_test< Evaluator_Not >(global_settings, transaction, "test-not", "not", "0");
    if ((test_to_execute == "") || (test_to_execute == "18"))
      prefix_test< Evaluator_Not >(global_settings, transaction, "test-not", "not", "1");
    if ((test_to_execute == "") || (test_to_execute == "19"))
      prefix_test< Evaluator_Not >(global_settings, transaction, "test-not", "not", "false");
    if ((test_to_execute == "") || (test_to_execute == "20"))
      prefix_test< Evaluator_Not >(global_settings, transaction, "test-not", "not", "");
    if ((test_to_execute == "") || (test_to_execute == "21"))
      pair_test< Evaluator_Equal >(global_settings, transaction, "test-equal", "equal", "9.5", "9.50");
    if ((test_to_execute == "") || (test_to_execute == "22"))
      pair_test< Evaluator_Equal >(global_settings, transaction, "test-equal", "equal", "99", "099");
    if ((test_to_execute == "") || (test_to_execute == "23"))
      pair_test< Evaluator_Equal >(global_settings, transaction, "test-equal", "equal", "nine", "nine");
    if ((test_to_execute == "") || (test_to_execute == "24"))
      pair_test< Evaluator_Equal >(global_settings, transaction, "test-equal", "equal", "nine", "nine ");
    if ((test_to_execute == "") || (test_to_execute == "25"))
      pair_test< Evaluator_Equal >(global_settings, transaction, "test-equal", "equal", "99", "99 ");
    if ((test_to_execute == "") || (test_to_execute == "26"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "9.5", "10");
    if ((test_to_execute == "") || (test_to_execute == "27"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "9", "10.1");
    if ((test_to_execute == "") || (test_to_execute == "28"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "9", "10");
    if ((test_to_execute == "") || (test_to_execute == "29"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "10", "9");
    if ((test_to_execute == "") || (test_to_execute == "30"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "a", "b");
    if ((test_to_execute == "") || (test_to_execute == "31"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "b", "a");
    if ((test_to_execute == "") || (test_to_execute == "32"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", "1", "a");
    if ((test_to_execute == "") || (test_to_execute == "33"))
      pair_test< Evaluator_Less >(global_settings, transaction, "test-less", "less", " ", "1");
    if ((test_to_execute == "") || (test_to_execute == "34"))
      pair_test< Evaluator_Less_Equal >(global_settings, transaction, "test-less-equal", "less-equal", "a", "a.0");
    if ((test_to_execute == "") || (test_to_execute == "35"))
      pair_test< Evaluator_Less_Equal >(global_settings, transaction, "test-less-equal", "less-equal", "a.0", "a");
    if ((test_to_execute == "") || (test_to_execute == "36"))
      pair_test< Evaluator_Less_Equal >(global_settings, transaction, "test-less-equal", "less-equal", "a", "a");
    if ((test_to_execute == "") || (test_to_execute == "37"))
      pair_test< Evaluator_Less_Equal >(global_settings, transaction, "test-less-equal", "less-equal", "9.0", "9");
    if ((test_to_execute == "") || (test_to_execute == "38"))
      pair_test< Evaluator_Less_Equal >(global_settings, transaction, "test-less-equal", "less-equal", "9.1", "9");
    if ((test_to_execute == "") || (test_to_execute == "39"))
      pair_test< Evaluator_Less_Equal >(global_settings, transaction, "test-less-equal", "less-equal", "9", "10");
    if ((test_to_execute == "") || (test_to_execute == "40"))
      pair_test< Evaluator_Greater >(global_settings, transaction, "test-greater", "greater", "a", "a.0");
    if ((test_to_execute == "") || (test_to_execute == "41"))
      pair_test< Evaluator_Greater >(global_settings, transaction, "test-greater", "greater", "a.0", "a");
    if ((test_to_execute == "") || (test_to_execute == "42"))
      pair_test< Evaluator_Greater >(global_settings, transaction, "test-greater", "greater", "a", "a");
    if ((test_to_execute == "") || (test_to_execute == "43"))
      pair_test< Evaluator_Greater >(global_settings, transaction, "test-greater", "greater", "9", "10");
    if ((test_to_execute == "") || (test_to_execute == "44"))
      pair_test< Evaluator_Greater >(global_settings, transaction, "test-greater", "greater", "9.0", "9");
    if ((test_to_execute == "") || (test_to_execute == "45"))
      pair_test< Evaluator_Greater >(global_settings, transaction, "test-greater", "greater", "10", "9");
    if ((test_to_execute == "") || (test_to_execute == "46"))
      pair_test< Evaluator_Greater_Equal >(global_settings, transaction, "test-gr-equal", "greater-equal", "a", "a.0");
    if ((test_to_execute == "") || (test_to_execute == "47"))
      pair_test< Evaluator_Greater_Equal >(global_settings, transaction, "test-gr-equal", "greater-equal", "a.0", "a");
    if ((test_to_execute == "") || (test_to_execute == "48"))
      pair_test< Evaluator_Greater_Equal >(global_settings, transaction, "test-gr-equal", "greater-equal", "a", "a");
    if ((test_to_execute == "") || (test_to_execute == "49"))
      pair_test< Evaluator_Greater_Equal >(global_settings, transaction, "test-gr-equal", "greater-equal", "9", "10");
    if ((test_to_execute == "") || (test_to_execute == "50"))
      pair_test< Evaluator_Greater_Equal >(global_settings, transaction, "test-gr-equal", "greater-equal", "9.0", "9");
    if ((test_to_execute == "") || (test_to_execute == "51"))
      pair_test< Evaluator_Greater_Equal >(global_settings, transaction, "test-gr-equal", "greater-equal", "10", "9");
    if ((test_to_execute == "") || (test_to_execute == "52"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", "5.5", "3.5");
    if ((test_to_execute == "") || (test_to_execute == "53"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", "1", "0 ");
    if ((test_to_execute == "") || (test_to_execute == "54"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", " 1", "10");
    if ((test_to_execute == "") || (test_to_execute == "55"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", " 1", "2_");
    if ((test_to_execute == "") || (test_to_execute == "56"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", "100000000000000000", "1");
    if ((test_to_execute == "") || (test_to_execute == "57"))
      pair_test< Evaluator_Times >(global_settings, transaction, "test-times", "product", "2", "6.5");
    if ((test_to_execute == "") || (test_to_execute == "58"))
      pair_test< Evaluator_Times >(global_settings, transaction, "test-times", "product", "_2", "7");
    if ((test_to_execute == "") || (test_to_execute == "59"))
      pair_test< Evaluator_Minus >(global_settings, transaction, "test-minus", "difference", "2", "5");
    if ((test_to_execute == "") || (test_to_execute == "60"))
      pair_test< Evaluator_Minus >(global_settings, transaction, "test-minus", "difference", "_2", "5");
    if ((test_to_execute == "") || (test_to_execute == "61"))
      pair_test< Evaluator_Minus >(global_settings, transaction, "test-minus", "difference", "100000000000000001", "100000000000000000");
    if ((test_to_execute == "") || (test_to_execute == "62"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "3.14");
    if ((test_to_execute == "") || (test_to_execute == "63"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "-3.");
    if ((test_to_execute == "") || (test_to_execute == "64"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "100000000000000000");
    if ((test_to_execute == "") || (test_to_execute == "65"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "one");
    if ((test_to_execute == "") || (test_to_execute == "66"))
      pair_test< Evaluator_Divided >(global_settings, transaction, "test-divided", "quotient", "8", "9");
    if ((test_to_execute == "") || (test_to_execute == "67"))
      pair_test< Evaluator_Divided >(global_settings, transaction, "test-divided", "quotient", "_8", "9");
    if ((test_to_execute == "") || (test_to_execute == "68"))
      union_value_test(global_settings, transaction, "union-value", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "69"))
      union_value_test(global_settings, transaction, "union-value", "foo", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "70"))
      min_value_test(global_settings, transaction, "min-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "71"))
      min_value_test(global_settings, transaction, "min-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "72"))
      max_value_test(global_settings, transaction, "max-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "73"))
      max_value_test(global_settings, transaction, "max-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "74"))
      set_value_test(global_settings, transaction, "value-set", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "75"))
      set_value_test(global_settings, transaction, "value-set", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "76"))
      value_id_type_test(global_settings, transaction, "id-and-type", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "77"))
      key_id_test(global_settings, transaction, "key-id", "_", 0, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "78"))
      key_id_test(global_settings, transaction, "key-id", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "79"))
      number_test(global_settings, transaction, "test-number", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "80"))
      date_test(global_settings, transaction, "test-date", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "81"))
      suffix_test(global_settings, transaction, "test-suffix", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "82"))
      lrs_test(global_settings, transaction, "test-lrs", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "83"))
      triple_test(global_settings, transaction, "test-ternary", "ternary", "1", "A", "B");
    if ((test_to_execute == "") || (test_to_execute == "84"))
      triple_test(global_settings, transaction, "test-ternary", "ternary", "false", "A", "B");
    if ((test_to_execute == "") || (test_to_execute == "85"))
      triple_test(global_settings, transaction, "test-ternary", "ternary", "0", "A", "B");
    if ((test_to_execute == "") || (test_to_execute == "86"))
      triple_test(global_settings, transaction, "test-ternary", "ternary", "", "A", "B");
    if ((test_to_execute == "") || (test_to_execute == "87"))
      generic_key_test(global_settings, transaction, "generic-key", "_", 7, 14, false, false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "88"))
      generic_key_test(global_settings, transaction, "generic-key", "foo", 7, 14, false, false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "89"))
      generic_key_test(global_settings, transaction, "generic-key", "_", 7, 14, false, true, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "90"))
      generic_key_test(global_settings, transaction, "generic-key", "_", 7, 14, true, false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "91"))
      make_point_test(global_settings, transaction, "make-point", 7, "51.25", "7.15", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "92"))
      make_point_test(global_settings, transaction, "make-point-invalid-north", 7, "91.25", "7.15",
          global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "93"))
      make_point_test(global_settings, transaction, "make-point-invalid-east", 7,  "51.25", "187.15",
          global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "94"))
      make_point_test(global_settings, transaction, "make-point-dependencies", 34, "51.25", "", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "95"))
      make_linestring_test(global_settings, transaction, "make-linestring", 7, 0, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "96"))
      make_linestring_test(global_settings, transaction, "make-linestring", 7, 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "97"))
      make_linestring_test(global_settings, transaction, "make-linestring", 7, 2, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "98"))
      make_linestring_test(global_settings, transaction, "make-linestring", 7, 3, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "99"))
      make_linestring_test(global_settings, transaction, "make-linestring", 34, 4, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "100"))
      make_polygon_test(global_settings, transaction, "make-polygon", 7, 0, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "101"))
      make_polygon_test(global_settings, transaction, "make-polygon", 7, 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "102"))
      make_polygon_test(global_settings, transaction, "make-polygon", 7, 2, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "103"))
      make_polygon_test(global_settings, transaction, "make-polygon", 7, 3, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "104"))
      make_polygon_test(global_settings, transaction, "make-polygon", 34, 4, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "105"))
      make_polygon_test(global_settings, transaction, "make-polygon", 34, 5, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "106"))
      make_polygon_test(global_settings, transaction, "make-polygon", 34, 6, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "107"))
      make_polygon_date_line_test(global_settings, transaction, "make-polygon", global_node_offset);

    std::cout<<"</osm>\n";
  }
  catch (File_Error e)
  {
    std::cerr<<"File error: "<<e.error_number<<' '<<e.origin<<' '<<e.filename<<'\n';
    return 1;
  }
  return 0;
}
