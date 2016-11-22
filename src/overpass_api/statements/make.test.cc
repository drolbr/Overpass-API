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
#include "id_query.h"
#include "make.h"
#include "print.h"
#include "set_tag.h"
#include "tag_value.h"
#include "unary_operators.h"
#include "union.h"


void attribute_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string into, std::string type)
{
  Resource_Manager rman(transaction, &global_settings);
        
  std::map< std::string, std::string > attributes;
  attributes["into"] = into;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  stmt.execute(rman);
  
  if (into == "_")
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
  else
  {
    const char* attributes[] = { "from", "target", 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void plain_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key1, std::string value1, std::string key2 = "", std::string value2 = "")
{
  Resource_Manager rman(transaction, &global_settings);
        
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = key1;
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  attributes["v"] = value1;
  Evaluator_Fixed stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = key2;
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  attributes.clear();
  attributes["v"] = value2;
  Evaluator_Fixed stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  if (key2 != "")
    stmt.add_statement(&stmt2, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = "nodes";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  attributes["type"] = "nodes";
  Evaluator_Count stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "ways";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["type"] = "ways";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Count stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = "relations";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["type"] = "relations";
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Count stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  
  attributes.clear();
  attributes["k"] = "tags";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Sum_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  attributes.clear();
  attributes["type"] = "tags";
  Evaluator_Count stmt400(0, attributes, global_settings);
  stmt40.add_statement(&stmt400, "");
  
  attributes.clear();
  attributes["k"] = "members";
  Set_Tag_Statement stmt5(0, attributes, global_settings);
  stmt.add_statement(&stmt5, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Sum_Value stmt50(0, attributes, global_settings);
  stmt5.add_statement(&stmt50, "");
  attributes.clear();
  attributes["type"] = "members";
  Evaluator_Count stmt500(0, attributes, global_settings);
  stmt50.add_statement(&stmt500, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     

template< typename Evaluator_Pair >
void pair_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key, std::string value1, std::string value2)
{
  Resource_Manager rman(transaction, &global_settings);
        
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = key;
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  Evaluator_Pair stmt10(0, attributes, global_settings);
  attributes["v"] = value1;
  Evaluator_Fixed stmt101(0, attributes, global_settings);
  stmt10.add_statement(&stmt101, "");
  attributes["v"] = value2;
  Evaluator_Fixed stmt102(0, attributes, global_settings);
  stmt10.add_statement(&stmt102, "");
  stmt1.add_statement(&stmt10, "");  
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
template< typename Evaluator_Prefix >
void prefix_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string key, std::string value)
{
  Resource_Manager rman(transaction, &global_settings);
        
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = key;
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  Evaluator_Prefix stmt10(0, attributes, global_settings);
  attributes["v"] = value;
  Evaluator_Fixed stmt101(0, attributes, global_settings);
  stmt10.add_statement(&stmt101, "");
  stmt1.add_statement(&stmt10, "");  
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void prepare_value_test(Parsed_Query& global_settings, Resource_Manager& rman,
    std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
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
    
  union_.execute(rman);
}
     
      
void union_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref, ref, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = "node_key";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Union_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  attributes.clear();
  attributes["k"] = "node_key";
  Evaluator_Value stmt100(0, attributes, global_settings);
  stmt10.add_statement(&stmt100, "");
  
  attributes.clear();
  attributes["k"] = "way_key";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Union_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  attributes.clear();
  attributes["k"] = "way_key";
  Evaluator_Value stmt200(0, attributes, global_settings);
  stmt20.add_statement(&stmt200, "");
  
  attributes.clear();
  attributes["k"] = "relation_key";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Union_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  attributes.clear();
  attributes["k"] = "relation_key";
  Evaluator_Value stmt300(0, attributes, global_settings);
  stmt30.add_statement(&stmt300, "");
  
  attributes.clear();
  attributes["k"] = "unused_key";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Union_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  attributes.clear();
  attributes["k"] = "unused_key";
  Evaluator_Value stmt400(0, attributes, global_settings);
  stmt40.add_statement(&stmt400, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void min_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = "node_key_7";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Min_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  attributes.clear();
  attributes["k"] = "node_key_7";
  Evaluator_Value stmt100(0, attributes, global_settings);
  stmt10.add_statement(&stmt100, "");
  
  attributes.clear();
  attributes["k"] = "way_key_7";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Min_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  attributes.clear();
  attributes["k"] = "way_key_7";
  Evaluator_Value stmt200(0, attributes, global_settings);
  stmt20.add_statement(&stmt200, "");
  
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Min_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Evaluator_Value stmt300(0, attributes, global_settings);
  stmt30.add_statement(&stmt300, "");
  
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Min_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Evaluator_Value stmt400(0, attributes, global_settings);
  stmt40.add_statement(&stmt400, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void max_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = "node_key_7";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Max_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  attributes.clear();
  attributes["k"] = "node_key_7";
  Evaluator_Value stmt100(0, attributes, global_settings);
  stmt10.add_statement(&stmt100, "");
  
  attributes.clear();
  attributes["k"] = "way_key_7";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Max_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  attributes.clear();
  attributes["k"] = "way_key_7";
  Evaluator_Value stmt200(0, attributes, global_settings);
  stmt20.add_statement(&stmt200, "");
  
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Max_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Evaluator_Value stmt300(0, attributes, global_settings);
  stmt30.add_statement(&stmt300, "");
  
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Max_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Evaluator_Value stmt400(0, attributes, global_settings);
  stmt40.add_statement(&stmt400, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void set_value_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = "node_key_7";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  attributes.clear();
  attributes["k"] = "node_key_7";
  Evaluator_Value stmt100(0, attributes, global_settings);
  stmt10.add_statement(&stmt100, "");
  
  attributes.clear();
  attributes["k"] = "way_key_7";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  attributes.clear();
  attributes["k"] = "way_key_7";
  Evaluator_Value stmt200(0, attributes, global_settings);
  stmt20.add_statement(&stmt200, "");
  
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Evaluator_Value stmt300(0, attributes, global_settings);
  stmt30.add_statement(&stmt300, "");
  
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Evaluator_Value stmt400(0, attributes, global_settings);
  stmt40.add_statement(&stmt400, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void value_id_type_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref, ref+1, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = "id";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  attributes.clear();
  Evaluator_Id stmt100(0, attributes, global_settings);
  stmt10.add_statement(&stmt100, "");
  
  attributes.clear();
  attributes["k"] = "type";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Evaluator_Set_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  attributes.clear();
  Evaluator_Type stmt200(0, attributes, global_settings);
  stmt20.add_statement(&stmt200, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void key_id_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  if (ref > 0)
    prepare_value_test(global_settings, rman, from, ref, ref+1, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["keytype"] = "id";
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  stmt.add_statement(&stmt1, "");
  attributes.clear();
  
  if (ref > 0)
  {
    if (from != "_")
      attributes["from"] = from;
    Evaluator_Max_Value stmt10(0, attributes, global_settings);
    stmt1.add_statement(&stmt10, "");
    attributes.clear();
    Evaluator_Id stmt100(0, attributes, global_settings);
    stmt10.add_statement(&stmt100, "");
  
    stmt.execute(rman);
  }
  else
  {
    attributes["v"] = "42";
    Evaluator_Fixed stmt10(0, attributes, global_settings);
    stmt1.add_statement(&stmt10, "");
  
    stmt.execute(rman);
  }
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
int main(int argc, char* args[])
{
  if (argc < 5)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir node_id_offset\n";
    return 0;
  }
  string test_to_execute = args[1];
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
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", "5.5", "3.5");
    if ((test_to_execute == "") || (test_to_execute == "35"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", "1", "0 ");
    if ((test_to_execute == "") || (test_to_execute == "36"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", " 1", "10");
    if ((test_to_execute == "") || (test_to_execute == "37"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", " 1", "2_");
    if ((test_to_execute == "") || (test_to_execute == "38"))
      pair_test< Evaluator_Plus >(global_settings, transaction, "test-plus", "sum", "100000000000000000", "1");
    if ((test_to_execute == "") || (test_to_execute == "39"))
      pair_test< Evaluator_Times >(global_settings, transaction, "test-times", "product", "2", "6.5");
    if ((test_to_execute == "") || (test_to_execute == "40"))
      pair_test< Evaluator_Times >(global_settings, transaction, "test-times", "product", "_2", "7");
    if ((test_to_execute == "") || (test_to_execute == "41"))
      pair_test< Evaluator_Minus >(global_settings, transaction, "test-minus", "difference", "2", "5");
    if ((test_to_execute == "") || (test_to_execute == "42"))
      pair_test< Evaluator_Minus >(global_settings, transaction, "test-minus", "difference", "_2", "5");
    if ((test_to_execute == "") || (test_to_execute == "43"))
      pair_test< Evaluator_Minus >(global_settings, transaction, "test-minus", "difference", "100000000000000001", "100000000000000000");
    if ((test_to_execute == "") || (test_to_execute == "44"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "3.14");
    if ((test_to_execute == "") || (test_to_execute == "45"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "-3.");
    if ((test_to_execute == "") || (test_to_execute == "46"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "100000000000000000");
    if ((test_to_execute == "") || (test_to_execute == "47"))
      prefix_test< Evaluator_Negate >(global_settings, transaction, "test-minus", "negation", "one");
    if ((test_to_execute == "") || (test_to_execute == "48"))
      pair_test< Evaluator_Divided >(global_settings, transaction, "test-divided", "quotient", "8", "9");
    if ((test_to_execute == "") || (test_to_execute == "49"))
      pair_test< Evaluator_Divided >(global_settings, transaction, "test-divided", "quotient", "_8", "9");
    if ((test_to_execute == "") || (test_to_execute == "50"))
      union_value_test(global_settings, transaction, "union-value", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "51"))
      union_value_test(global_settings, transaction, "union-value", "foo", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "52"))
      min_value_test(global_settings, transaction, "min-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "53"))
      min_value_test(global_settings, transaction, "min-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "54"))
      max_value_test(global_settings, transaction, "max-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "55"))
      max_value_test(global_settings, transaction, "max-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "56"))
      set_value_test(global_settings, transaction, "value-set", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "57"))
      set_value_test(global_settings, transaction, "value-set", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "58"))
      value_id_type_test(global_settings, transaction, "id-and-type", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "59"))
      key_id_test(global_settings, transaction, "key-id", "_", 0, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "60"))
      key_id_test(global_settings, transaction, "key-id", "_", 1, global_node_offset);

    std::cout<<"</osm>\n";
  }
  catch (File_Error e)
  {
    std::cerr<<"File error: "<<e.error_number<<' '<<e.origin<<' '<<e.filename<<'\n';
    return 1;
  }
  return 0;
}
