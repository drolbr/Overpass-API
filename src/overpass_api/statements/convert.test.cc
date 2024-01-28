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

#include "../data/utils.h"
#include "aggregators.h"
#include "binary_operators.h"
#include "id_query.h"
#include "convert.h"
#include "explicit_geometry.h"
#include "geometry_endomorphisms.h"
#include "item_geometry.h"
#include "make.h"
#include "per_member.h"
#include "print.h"
#include "set_prop.h"
#include "tag_value.h"
#include "testing_tools.h"
#include "union.h"


void prepare_value_test(Parsed_Query& global_settings, Resource_Manager& rman,
    std::string from, uint64 ref1, uint64 ref2, std::string derived_num, uint64 global_node_offset)
{
  Union_Statement union_(0, (from == "_" ? Attr() : Attr()("into", from)).kvs(), global_settings);

  Id_Query_Statement stmt1(0, Attr()("type", "node")("ref", to_string(ref1 + global_node_offset)).kvs(),
      global_settings);
  union_.add_statement(&stmt1, "");
  Id_Query_Statement stmt2(0, Attr()("type", "way")("ref", to_string(ref1)).kvs(), global_settings);
  union_.add_statement(&stmt2, "");
  Id_Query_Statement stmt3(0, Attr()("type", "relation")("ref", to_string(ref1)).kvs(), global_settings);
  union_.add_statement(&stmt3, "");

  Id_Query_Statement stmt4(0, Attr()("type", "node")("ref", to_string(ref2 + global_node_offset)).kvs(),
      global_settings);
  if (ref1 != ref2)
    union_.add_statement(&stmt4, "");

  Make_Statement stmt5(0, Attr()("type", "derivee").kvs(), global_settings);
  Set_Prop_Statement stmt50(0, Attr()("k", "number").kvs(), global_settings);
  stmt5.add_statement(&stmt50, "");
  Evaluator_Fixed stmt500(0, Attr()("v", derived_num).kvs(), global_settings);
  stmt50.add_statement(&stmt500, "");

  if (derived_num != "")
    union_.add_statement(&stmt5, "");

  union_.execute(rman);
}


void prepare_relation_test(Parsed_Query& global_settings, Resource_Manager& rman,
    std::string from, uint64 ref1, uint64 ref2, uint64 ref3, uint64 ref4)
{
  Union_Statement union_(0, (from == "_" ? Attr() : Attr()("into", from)).kvs(), global_settings);

  Id_Query_Statement stmt1(0, Attr()("type", "relation")("ref", to_string(ref1)).kvs(), global_settings);
  union_.add_statement(&stmt1, "");
  Id_Query_Statement stmt2(0, Attr()("type", "relation")("ref", to_string(ref2)).kvs(), global_settings);
  union_.add_statement(&stmt2, "");
  Id_Query_Statement stmt3(0, Attr()("type", "relation")("ref", to_string(ref3)).kvs(), global_settings);
  union_.add_statement(&stmt3, "");
  Id_Query_Statement stmt4(0, Attr()("type", "relation")("ref", to_string(ref4)).kvs(), global_settings);
  union_.add_statement(&stmt4, "");

  union_.execute(rman);
}


void just_copy_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, std::string derived_num, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, derived_num, global_node_offset);

  Convert_Statement stmt(0, Attr()("from", from)("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("keytype", "generic").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Generic stmt10(0, Attr().kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void into_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string into, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 7, 14, "1000", global_node_offset);

  Convert_Statement stmt(0, Attr()("type", type)("into", into).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("keytype", "generic").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Generic stmt10(0, Attr().kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");

  stmt.execute(rman);
  Print_Statement(0, Attr()("from", into).kvs(), global_settings).execute(rman);
}


void tag_manipulation_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, std::string derived_num, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, derived_num, global_node_offset);

  Convert_Statement stmt(0, Attr()("from", from)("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("keytype", "generic").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Generic stmt10(0, Attr().kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");

  Set_Prop_Statement stmt2(0, Attr()("k", "extra_key").kvs(), global_settings);
  stmt.add_statement(&stmt2, "");
  Evaluator_Fixed stmt20(0, Attr()("v", "extra_value").kvs(), global_settings);
  stmt2.add_statement(&stmt20, "");

  Set_Prop_Statement stmt3(0, Attr()("k", "node_key").kvs(), global_settings);
  stmt.add_statement(&stmt3, "");
  Set_Prop_Statement stmt4(0, Attr()("k", "number").kvs(), global_settings);
  stmt.add_statement(&stmt4, "");

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void count_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);

  {
    std::map< std::string, std::string > attributes;
    if (from != "_")
      attributes["into"] = from;
    Union_Statement union_(0, attributes, global_settings);

    attributes.clear();
    attributes["type"] = "node";
    attributes["ref"] = to_string(ref + global_node_offset);
    Id_Query_Statement stmt1(0, attributes, global_settings);
    union_.add_statement(&stmt1, "");

    attributes.clear();
    attributes["type"] = "way";
    attributes["ref"] = to_string(ref);
    Id_Query_Statement stmt2(0, attributes, global_settings);
    union_.add_statement(&stmt2, "");

    attributes.clear();
    attributes["type"] = "relation";
    attributes["ref"] = to_string(ref);
    Id_Query_Statement stmt3(0, attributes, global_settings);
    union_.add_statement(&stmt3, "");

    union_.execute(rman);
  }

  Convert_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("k", "nodes").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Set_Count stmt10(0, (from == "_" ? Attr() : Attr()("from", from))("type", "nodes").kvs(),
      global_settings);
  stmt1.add_statement(&stmt10, "");

  Set_Prop_Statement stmt2(0, Attr()("k", "ways").kvs(), global_settings);
  stmt.add_statement(&stmt2, "");
  Evaluator_Set_Count stmt20(0, (from == "_" ? Attr() : Attr()("from", from))("type", "ways").kvs(),
      global_settings);
  stmt2.add_statement(&stmt20, "");

  Set_Prop_Statement stmt3(0, Attr()("k", "relations").kvs(), global_settings);
  stmt.add_statement(&stmt3, "");
  Evaluator_Set_Count stmt30(0, (from == "_" ? Attr() : Attr()("from", from))("type", "relations").kvs(),
      global_settings);
  stmt3.add_statement(&stmt30, "");

  Set_Prop_Statement stmt4(0, Attr()("k", "tags").kvs(), global_settings);
  stmt.add_statement(&stmt4, "");
  Evaluator_Properties_Count stmt40(0, (from == "_" ? Attr() : Attr()("from", from))("type", "tags").kvs(),
      global_settings);
  stmt4.add_statement(&stmt40, "");

  Set_Prop_Statement stmt5(0, Attr()("k", "members").kvs(), global_settings);
  stmt.add_statement(&stmt5, "");
  Evaluator_Properties_Count stmt50(0, (from == "_" ? Attr() : Attr()("from", from))("type", "members").kvs(),
      global_settings);
  stmt5.add_statement(&stmt50, "");

  Set_Prop_Statement stmt6(0, Attr()("k", "distinct_members").kvs(), global_settings);
  stmt.add_statement(&stmt6, "");
  Evaluator_Properties_Count stmt60(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "distinct_members").kvs(), global_settings);
  stmt6.add_statement(&stmt60, "");

  Set_Prop_Statement stmt7(0, Attr()("k", "by_role").kvs(), global_settings);
  stmt.add_statement(&stmt7, "");
  Evaluator_Properties_Count stmt70(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "by_role")("role", "one").kvs(), global_settings);
  stmt7.add_statement(&stmt70, "");

  Set_Prop_Statement stmt8(0, Attr()("k", "distinct_by_role").kvs(), global_settings);
  stmt.add_statement(&stmt8, "");
  Evaluator_Properties_Count stmt80(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "distinct_by_role")("role", "one").kvs(), global_settings);
  stmt8.add_statement(&stmt80, "");

  Set_Prop_Statement stmt_10(0, Attr()("k", "members_with_type").kvs(), global_settings);
  stmt.add_statement(&stmt_10, "");
  Evaluator_Properties_Count stmt_100(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "members")("members_type", "nodes").kvs(), global_settings);
  stmt_10.add_statement(&stmt_100, "");

  Set_Prop_Statement stmt_11(0, Attr()("k", "distinct_members_with_type").kvs(), global_settings);
  stmt.add_statement(&stmt_11, "");
  Evaluator_Properties_Count stmt_110(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "distinct_members")("members_type", "nodes").kvs(), global_settings);
  stmt_11.add_statement(&stmt_110, "");

  Set_Prop_Statement stmt_12(0, Attr()("k", "by_role_with_type").kvs(), global_settings);
  stmt.add_statement(&stmt_12, "");
  Evaluator_Properties_Count stmt_120(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "by_role")("role", "one")("members_type", "nodes").kvs(), global_settings);
  stmt_12.add_statement(&stmt_120, "");

  Set_Prop_Statement stmt_13(0, Attr()("k", "distinct_by_role_with_type").kvs(), global_settings);
  stmt.add_statement(&stmt_13, "");
  Evaluator_Properties_Count stmt_130(0, (from == "_" ? Attr() : Attr()("from", from))
      ("type", "distinct_by_role")("role", "one")("members_type", "nodes").kvs(), global_settings);
  stmt_13.add_statement(&stmt_130, "");

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void is_tag_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 7, 14, "1000", global_node_offset);

  Convert_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("k", "node_key").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Is_Tag stmt10(0, Attr()("k", "node_key").kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");
  Set_Prop_Statement stmt2(0, Attr()("k", "way_key").kvs(), global_settings);
  stmt.add_statement(&stmt2, "");
  Evaluator_Is_Tag stmt20(0, Attr()("k", "way_key").kvs(), global_settings);
  stmt2.add_statement(&stmt20, "");
  Set_Prop_Statement stmt3(0, Attr()("k", "relation_key").kvs(), global_settings);
  stmt.add_statement(&stmt3, "");
  Evaluator_Is_Tag stmt30(0, Attr()("k", "relation_key").kvs(), global_settings);
  stmt3.add_statement(&stmt30, "");
  Set_Prop_Statement stmt4(0, Attr()("k", "number").kvs(), global_settings);
  stmt.add_statement(&stmt4, "");
  Evaluator_Is_Tag stmt40(0, Attr()("k", "number").kvs(), global_settings);
  stmt4.add_statement(&stmt40, "");

  stmt.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void lat_lon_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 7, 14, "1000", global_node_offset);

  Convert_Statement stmt1(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt11(0, Attr()("k", "lat").kvs(), global_settings);
  stmt1.add_statement(&stmt11, "");
  Evaluator_Latitude stmt110(0, Attr().kvs(), global_settings);
  stmt11.add_statement(&stmt110, "");
  Set_Prop_Statement stmt12(0, Attr()("k", "lon").kvs(), global_settings);
  stmt1.add_statement(&stmt12, "");
  Evaluator_Longitude stmt120(0, Attr().kvs(), global_settings);
  stmt12.add_statement(&stmt120, "");

  stmt1.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);

  Make_Statement stmt2(0, Attr()("type", "derivee").kvs(), global_settings);
  Set_Prop_Statement stmt20(0, Attr()("keytype", "geometry").kvs(), global_settings);
  stmt2.add_statement(&stmt20, "");
  Evaluator_Point stmt200(0, Attr().kvs(), global_settings);
  stmt20.add_statement(&stmt200, "");
  Evaluator_Fixed stmt2001(0, Attr()("v", "-15.0").kvs(), global_settings);
  stmt200.add_statement(&stmt2001, "");
  Evaluator_Fixed stmt2002(0, Attr()("v", "5.55").kvs(), global_settings);
  stmt200.add_statement(&stmt2002, "");
  stmt2.execute(rman);

  Convert_Statement stmt3(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt31(0, Attr()("k", "lat").kvs(), global_settings);
  stmt3.add_statement(&stmt31, "");
  Evaluator_Latitude stmt310(0, Attr().kvs(), global_settings);
  stmt31.add_statement(&stmt310, "");
  Set_Prop_Statement stmt32(0, Attr()("k", "lon").kvs(), global_settings);
  stmt3.add_statement(&stmt32, "");
  Evaluator_Longitude stmt320(0, Attr().kvs(), global_settings);
  stmt32.add_statement(&stmt320, "");

  stmt3.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void per_member_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 10, 10, "1000", global_node_offset);

  Convert_Statement stmt1(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt11(0, Attr()("k", "ref").kvs(), global_settings);
  stmt1.add_statement(&stmt11, "");
  Evaluator_Per_Member stmt110(0, Attr().kvs(), global_settings);
  stmt11.add_statement(&stmt110, "");
  Evaluator_Ref stmt1100(0, Attr().kvs(), global_settings);
  stmt110.add_statement(&stmt1100, "");

  Set_Prop_Statement stmt12(0, Attr()("k", "refpos").kvs(), global_settings);
  stmt1.add_statement(&stmt12, "");
  Evaluator_Per_Member stmt120(0, Attr().kvs(), global_settings);
  stmt12.add_statement(&stmt120, "");
  Evaluator_Plus stmt1200(0, Attr().kvs(), global_settings);
  stmt120.add_statement(&stmt1200, "");
  Evaluator_Plus stmt12001(0, Attr().kvs(), global_settings);
  stmt1200.add_statement(&stmt12001, "");
  Evaluator_Pos stmt120011(0, Attr().kvs(), global_settings);
  stmt12001.add_statement(&stmt120011, "");
  Evaluator_Fixed stmt120012(0, Attr()("v", ":").kvs(), global_settings);
  stmt12001.add_statement(&stmt120012, "");
  Evaluator_Ref stmt12002(0, Attr().kvs(), global_settings);
  stmt1200.add_statement(&stmt12002, "");

  Set_Prop_Statement stmt13(0, Attr()("k", "full").kvs(), global_settings);
  stmt1.add_statement(&stmt13, "");
  Evaluator_Per_Member stmt130(0, Attr().kvs(), global_settings);
  stmt13.add_statement(&stmt130, "");
  Evaluator_Plus stmt1300(0, Attr().kvs(), global_settings);
  stmt130.add_statement(&stmt1300, "");
  Evaluator_Plus stmt13001(0, Attr().kvs(), global_settings);
  stmt1300.add_statement(&stmt13001, "");
  Evaluator_Plus stmt130011(0, Attr().kvs(), global_settings);
  stmt13001.add_statement(&stmt130011, "");
  Evaluator_Plus stmt1300111(0, Attr().kvs(), global_settings);
  stmt130011.add_statement(&stmt1300111, "");
  Evaluator_Membertype stmt13001111(0, Attr().kvs(), global_settings);
  stmt1300111.add_statement(&stmt13001111, "");
  Evaluator_Fixed stmt13001112(0, Attr()("v", ",").kvs(), global_settings);
  stmt1300111.add_statement(&stmt13001112, "");
  Evaluator_Ref stmt1300112(0, Attr().kvs(), global_settings);
  stmt130011.add_statement(&stmt1300112, "");
  Evaluator_Fixed stmt130012(0, Attr()("v", ",").kvs(), global_settings);
  stmt13001.add_statement(&stmt130012, "");
  Evaluator_Role stmt13002(0, Attr().kvs(), global_settings);
  stmt1300.add_statement(&stmt13002, "");

  Set_Prop_Statement stmt14(0, Attr()("k", "vertexrefpos").kvs(), global_settings);
  stmt1.add_statement(&stmt14, "");
  Evaluator_Per_Vertex stmt140(0, Attr().kvs(), global_settings);
  stmt14.add_statement(&stmt140, "");
  Evaluator_Plus stmt1400(0, Attr().kvs(), global_settings);
  stmt140.add_statement(&stmt1400, "");
  Evaluator_Plus stmt14001(0, Attr().kvs(), global_settings);
  stmt1400.add_statement(&stmt14001, "");
  Evaluator_Pos stmt140011(0, Attr().kvs(), global_settings);
  stmt14001.add_statement(&stmt140011, "");
  Evaluator_Fixed stmt140012(0, Attr()("v", ":").kvs(), global_settings);
  stmt14001.add_statement(&stmt140012, "");
  Evaluator_Ref stmt14002(0, Attr().kvs(), global_settings);
  stmt1400.add_statement(&stmt14002, "");

  stmt1.execute(rman);
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


void geom_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 8, 14, "1000", global_node_offset);

  Convert_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("keytype", "geometry").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Geometry stmt10(0, Attr().kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");

  stmt.execute(rman);
  Print_Statement(0, Attr()("geometry", "full").kvs(), global_settings).execute(rman);
}


void trace_test_1(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 2, 3, "1000", global_node_offset);

  Convert_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("keytype", "geometry").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Trace stmt10(0, Attr().kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");
  Evaluator_Geometry stmt100(0, Attr().kvs(), global_settings);
  stmt10.add_statement(&stmt100, "");

  stmt.execute(rman);
  Print_Statement(0, Attr()("geometry", "full").kvs(), global_settings).execute(rman);
}


void trace_test_2(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_relation_test(global_settings, rman, "_", 2, 6, 9, 10);

  Convert_Statement stmt(0, Attr()("type", type).kvs(), global_settings);

  Set_Prop_Statement stmt1(0, Attr()("keytype", "geometry").kvs(), global_settings);
  stmt.add_statement(&stmt1, "");
  Evaluator_Trace stmt10(0, Attr().kvs(), global_settings);
  stmt1.add_statement(&stmt10, "");
  Evaluator_Geometry stmt100(0, Attr().kvs(), global_settings);
  stmt10.add_statement(&stmt100, "");

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
    Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
    Parsed_Query global_settings;
    global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    if ((test_to_execute == "") || (test_to_execute == "1"))
      just_copy_test(global_settings, transaction, "just-copy", "_", 7, 14, "1000", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "2"))
      just_copy_test(global_settings, transaction, "just-copy", "_", 0, 0, "", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "3"))
      just_copy_test(global_settings, transaction, "just-copy", "some_set", 7, 14, "1000", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "4"))
      into_test(global_settings, transaction, "into", "_", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "5"))
      into_test(global_settings, transaction, "into", "some_set", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "6"))
      tag_manipulation_test(global_settings, transaction, "rewrite", "some_set", 7, 14, "1000", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "7"))
      count_test(global_settings, transaction, "count-from-default", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "8"))
      is_tag_test(global_settings, transaction, "is-tag", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "9"))
      count_test(global_settings, transaction, "count-from-default", "_", 2, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "10"))
      count_test(global_settings, transaction, "count-from-default", "_", 10, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "11"))
      geom_test(global_settings, transaction, "geometry", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "12"))
      trace_test_1(global_settings, transaction, "trace", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "13"))
      trace_test_2(global_settings, transaction, "trace", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "14"))
      lat_lon_test(global_settings, transaction, "lat-lon", global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "15"))
      per_member_test(global_settings, transaction, "per-member", global_node_offset);

    std::cout<<"</osm>\n";
  }
  catch (File_Error e)
  {
    std::cerr<<"File error: "<<e.error_number<<' '<<e.origin<<' '<<e.filename<<'\n';
    return 1;
  }
  return 0;
}
