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
#include "id_query.h"
#include "convert.h"
#include "make.h"
#include "print.h"
#include "set_prop.h"
#include "tag_value.h"
#include "union.h"


void prepare_value_test(Parsed_Query& global_settings, Resource_Manager& rman,
    std::string from, uint64 ref1, uint64 ref2, std::string derived_num, uint64 global_node_offset)
{
  std::map< std::string, std::string > attributes;
  if (from != "_")
    attributes["into"] = from;
  Union_Statement union_(0, attributes, global_settings);

  attributes.clear();
  attributes["type"] = "node";
  attributes["ref"] = to_string(ref1 + global_node_offset);
  Id_Query_Statement stmt1(0, attributes, global_settings);
  union_.add_statement(&stmt1, "");

  attributes.clear();
  attributes["type"] = "way";
  attributes["ref"] = to_string(ref1);
  Id_Query_Statement stmt2(0, attributes, global_settings);
  union_.add_statement(&stmt2, "");

  attributes.clear();
  attributes["type"] = "relation";
  attributes["ref"] = to_string(ref1);
  Id_Query_Statement stmt3(0, attributes, global_settings);
  union_.add_statement(&stmt3, "");

  attributes.clear();
  attributes["type"] = "node";
  attributes["ref"] = to_string(ref2 + global_node_offset);
  Id_Query_Statement stmt4(0, attributes, global_settings);
  if (ref1 != ref2)
    union_.add_statement(&stmt4, "");

  attributes.clear();
  attributes["type"] = "derivee";
  Make_Statement stmt5(0, attributes, global_settings);

  attributes.clear();
  attributes["k"] = "number";
  Set_Prop_Statement stmt50(0, attributes, global_settings);
  stmt5.add_statement(&stmt50, "");
  attributes.clear();
  attributes["v"] = derived_num;
  Evaluator_Fixed stmt500(0, attributes, global_settings);
  stmt50.add_statement(&stmt500, "");

  if (derived_num != "")
    union_.add_statement(&stmt5, "");

  union_.execute(rman);
}


void just_copy_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, std::string derived_num, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, derived_num, global_node_offset);

  std::map< std::string, std::string > attributes;
  attributes["from"] = from;
  attributes["type"] = type;
  Convert_Statement stmt(0, attributes, global_settings);

  attributes.clear();
  attributes["keytype"] = "generic";
  Set_Prop_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  Evaluator_Generic stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");

  stmt.execute(rman);

  {
    attributes.clear();
    Print_Statement stmt(0, attributes, global_settings);
    stmt.execute(rman);
  }
}


void into_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string into, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 7, 14, "1000", global_node_offset);

  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  attributes["into"] = into;
  Convert_Statement stmt(0, attributes, global_settings);

  attributes.clear();
  attributes["keytype"] = "generic";
  Set_Prop_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  Evaluator_Generic stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");

  stmt.execute(rman);

  {
    attributes.clear();
    attributes["from"] = into;
    Print_Statement stmt(0, attributes, global_settings);
    stmt.execute(rman);
  }
}


void tag_manipulation_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, std::string derived_num, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, derived_num, global_node_offset);

  std::map< std::string, std::string > attributes;
  attributes["from"] = from;
  attributes["type"] = type;
  Convert_Statement stmt(0, attributes, global_settings);

  attributes.clear();
  attributes["keytype"] = "generic";
  Set_Prop_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  Evaluator_Generic stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");

  attributes.clear();
  attributes["k"] = "extra_key";
  Set_Prop_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["v"] = "extra_value";
  Evaluator_Fixed stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");

  attributes.clear();
  attributes["k"] = "node_key";
  Set_Prop_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");

  attributes.clear();
  attributes["k"] = "number";
  Set_Prop_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");

  stmt.execute(rman);

  {
    attributes.clear();
    Print_Statement stmt(0, attributes, global_settings);
    stmt.execute(rman);
  }
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

  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Convert_Statement stmt(0, attributes, global_settings);

  attributes.clear();
  attributes["k"] = "nodes";
  Set_Prop_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  attributes["type"] = "nodes";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Count stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");

  attributes.clear();
  attributes["k"] = "ways";
  Set_Prop_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["type"] = "ways";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Count stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");

  attributes.clear();
  attributes["k"] = "relations";
  Set_Prop_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["type"] = "relations";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Count stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");

  attributes.clear();
  attributes["k"] = "tags";
  Set_Prop_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  attributes["type"] = "tags";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Properties_Count stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");

  attributes.clear();
  attributes["k"] = "members";
  Set_Prop_Statement stmt5(0, attributes, global_settings);
  stmt.add_statement(&stmt5, "");
  attributes.clear();
  attributes["type"] = "members";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Properties_Count stmt50(0, attributes, global_settings);
  stmt5.add_statement(&stmt50, "");

  stmt.execute(rman);

  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}


void is_tag_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, "_", 7, 14, "1000", global_node_offset);

  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Convert_Statement stmt(0, attributes, global_settings);

  attributes.clear();
  attributes["k"] = "node_key";
  Set_Prop_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  attributes["k"] = "node_key";
  Evaluator_Is_Tag stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  attributes.clear();
  attributes["k"] = "way_key";
  Set_Prop_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["k"] = "way_key";
  Evaluator_Is_Tag stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  attributes.clear();
  attributes["k"] = "relation_key";
  Set_Prop_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["k"] = "relation_key";
  Evaluator_Is_Tag stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  attributes.clear();
  attributes["k"] = "number";
  Set_Prop_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  attributes["k"] = "number";
  Evaluator_Is_Tag stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");

  stmt.execute(rman);

  {
    attributes.clear();
    Print_Statement stmt(0, attributes, global_settings);
    stmt.execute(rman);
  }
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

    std::cout<<"</osm>\n";
  }
  catch (File_Error e)
  {
    std::cerr<<"File error: "<<e.error_number<<' '<<e.origin<<' '<<e.filename<<'\n';
    return 1;
  }
  return 0;
}
