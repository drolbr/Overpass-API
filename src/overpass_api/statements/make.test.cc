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
#include "id_query.h"
#include "make.h"
#include "print.h"
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
  Tag_Value_Fixed stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = key2;
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  attributes.clear();
  attributes["v"] = value2;
  Tag_Value_Fixed stmt20(0, attributes, global_settings);
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
  Tag_Value_Count stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "ways";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["type"] = "ways";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Count stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = "relations";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["type"] = "relations";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Count stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void add_test(Parsed_Query& global_settings, Transaction& transaction,
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
  Tag_Value_Plus stmt10(0, attributes, global_settings);
  attributes["v"] = value1;
  Tag_Value_Fixed stmt101(0, attributes, global_settings);
  stmt10.add_statement(&stmt101, "");
  attributes["v"] = value2;
  Tag_Value_Fixed stmt102(0, attributes, global_settings);
  stmt10.add_statement(&stmt102, "");
  stmt1.add_statement(&stmt10, "");  
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void multiply_test(Parsed_Query& global_settings, Transaction& transaction,
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
  Tag_Value_Times stmt10(0, attributes, global_settings);
  attributes["v"] = value1;
  Tag_Value_Fixed stmt101(0, attributes, global_settings);
  stmt10.add_statement(&stmt101, "");
  attributes["v"] = value2;
  Tag_Value_Fixed stmt102(0, attributes, global_settings);
  stmt10.add_statement(&stmt102, "");
  stmt1.add_statement(&stmt10, "");  
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void minus_test(Parsed_Query& global_settings, Transaction& transaction,
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
  Tag_Value_Minus stmt10(0, attributes, global_settings);
  attributes["v"] = value1;
  Tag_Value_Fixed stmt101(0, attributes, global_settings);
  stmt10.add_statement(&stmt101, "");
  attributes["v"] = value2;
  Tag_Value_Fixed stmt102(0, attributes, global_settings);
  stmt10.add_statement(&stmt102, "");
  stmt1.add_statement(&stmt10, "");  
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
void divide_test(Parsed_Query& global_settings, Transaction& transaction,
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
  Tag_Value_Divided stmt10(0, attributes, global_settings);
  attributes["v"] = value1;
  Tag_Value_Fixed stmt101(0, attributes, global_settings);
  stmt10.add_statement(&stmt101, "");
  attributes["v"] = value2;
  Tag_Value_Fixed stmt102(0, attributes, global_settings);
  stmt10.add_statement(&stmt102, "");
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
  attributes["k"] = "node_key";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Union_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "way_key";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["k"] = "way_key";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Union_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = "relation_key";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["k"] = "relation_key";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Union_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  
  attributes.clear();
  attributes["k"] = "unused_key";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  attributes["k"] = "unused_key";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Union_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  
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
  attributes["k"] = "node_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Min_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "way_key_7";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["k"] = "way_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Min_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["k"] = "relation_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Min_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  attributes["k"] = "unused_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Min_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  
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
  attributes["k"] = "node_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Max_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "way_key_7";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["k"] = "way_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Max_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["k"] = "relation_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Max_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  attributes["k"] = "unused_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Max_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  
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
  attributes["k"] = "node_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Set_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "way_key_7";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["k"] = "way_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Set_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = "relation_key_7";
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  stmt.add_statement(&stmt3, "");
  attributes.clear();
  attributes["k"] = "relation_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Set_Value stmt30(0, attributes, global_settings);
  stmt3.add_statement(&stmt30, "");
  
  attributes.clear();
  attributes["k"] = "unused_key_7";
  Set_Tag_Statement stmt4(0, attributes, global_settings);
  stmt.add_statement(&stmt4, "");
  attributes.clear();
  attributes["k"] = "unused_key_7";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Set_Value stmt40(0, attributes, global_settings);
  stmt4.add_statement(&stmt40, "");
  
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
  attributes["keytype"] = "id";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Set_Value stmt10(0, attributes, global_settings);
  stmt1.add_statement(&stmt10, "");
  
  attributes.clear();
  attributes["k"] = "type";
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["keytype"] = "type";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_Set_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  stmt.execute(rman);
  
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
}
     
      
template< typename Tag_Value_X_Value >
void any_key_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string type, std::string from, uint64 ref1, uint64 ref2, std::string key1, std::string key2,
    bool show_extra_keys, uint64 global_node_offset)
{
  Resource_Manager rman(transaction, &global_settings);
  prepare_value_test(global_settings, rman, from, ref1, ref2, global_node_offset);
  
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  
  attributes.clear();
  attributes["k"] = key1;
  Set_Tag_Statement stmt1(0, attributes, global_settings);
  attributes.clear();
  attributes["v"] = "val_" + key1;
  Tag_Value_Fixed stmt10(0, attributes, global_settings);
  if (show_extra_keys)
    stmt1.add_statement(&stmt10, "");
  if (key1 != "")
    stmt.add_statement(&stmt1, "");
  
  attributes.clear();
  if (from != "_")
    attributes["from"] = from;
  Set_Tag_Statement stmt2(0, attributes, global_settings);
  stmt.add_statement(&stmt2, "");
  attributes.clear();
  attributes["keytype"] = "generic";
  if (from != "_")
    attributes["from"] = from;
  Tag_Value_X_Value stmt20(0, attributes, global_settings);
  stmt2.add_statement(&stmt20, "");
  
  attributes.clear();
  attributes["k"] = key2;
  Set_Tag_Statement stmt3(0, attributes, global_settings);
  attributes.clear();
  attributes["v"] = "val_" + key2;
  Tag_Value_Fixed stmt30(0, attributes, global_settings);
  if (show_extra_keys)
    stmt3.add_statement(&stmt30, "");
  if (key2 != "")
    stmt.add_statement(&stmt3, "");
  
  stmt.execute(rman);
  
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
      add_test(global_settings, transaction, "test-plus", "sum", "5.5", "3.5");
    if ((test_to_execute == "") || (test_to_execute == "10"))
      add_test(global_settings, transaction, "test-plus", "sum", "1", "0 ");
    if ((test_to_execute == "") || (test_to_execute == "11"))
      add_test(global_settings, transaction, "test-plus", "sum", " 1", "10");
    if ((test_to_execute == "") || (test_to_execute == "12"))
      add_test(global_settings, transaction, "test-plus", "sum", " 1", "2_");
    if ((test_to_execute == "") || (test_to_execute == "13"))
      multiply_test(global_settings, transaction, "test-times", "product", "2", "6.5");
    if ((test_to_execute == "") || (test_to_execute == "14"))
      multiply_test(global_settings, transaction, "test-times", "product", "_2", "7");
    if ((test_to_execute == "") || (test_to_execute == "15"))
      minus_test(global_settings, transaction, "test-minus", "difference", "2", "5");
    if ((test_to_execute == "") || (test_to_execute == "16"))
      minus_test(global_settings, transaction, "test-minus", "difference", "_2", "5");
    if ((test_to_execute == "") || (test_to_execute == "17"))
      divide_test(global_settings, transaction, "test-divided", "quotient", "8", "9");
    if ((test_to_execute == "") || (test_to_execute == "18"))
      divide_test(global_settings, transaction, "test-divided", "quotient", "_8", "9");
    if ((test_to_execute == "") || (test_to_execute == "19"))
      union_value_test(global_settings, transaction, "union-value", "_", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "20"))
      union_value_test(global_settings, transaction, "union-value", "foo", 1, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "21"))
      min_value_test(global_settings, transaction, "min-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "22"))
      min_value_test(global_settings, transaction, "min-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "23"))
      max_value_test(global_settings, transaction, "max-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "24"))
      max_value_test(global_settings, transaction, "max-value", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "25"))
      set_value_test(global_settings, transaction, "value-set", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "26"))
      set_value_test(global_settings, transaction, "value-set", "_", 7, 14, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "27"))
      any_key_test< Tag_Value_Set_Value >(global_settings, transaction, "any-key", "_", 7, 14, "", "",
          true, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "28"))
      any_key_test< Tag_Value_Set_Value >(global_settings, transaction, "any-key", "_", 7, 14, "node_key", "",
          true, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "29"))
      any_key_test< Tag_Value_Set_Value >(global_settings, transaction, "any-key", "_", 7, 14, "", "node_key",
          true, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "30"))
      any_key_test< Tag_Value_Set_Value >(global_settings, transaction, "any-key", "_", 7, 14, "way_key", "node_key",
          true, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "31"))
      any_key_test< Tag_Value_Set_Value >(global_settings, transaction, "any-key", "foo", 7, 14, "way_key", "node_key",
          true, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "32"))
      any_key_test< Tag_Value_Set_Value >(global_settings, transaction, "any-key", "_", 7, 14, "way_key", "node_key",
          false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "33"))
      any_key_test< Tag_Value_Min_Value >(global_settings, transaction, "any-key", "_", 7, 14, "way_key", "node_key",
          false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "34"))
      any_key_test< Tag_Value_Max_Value >(global_settings, transaction, "any-key", "_", 7, 14, "way_key", "node_key",
          false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "35"))
      any_key_test< Tag_Value_Union_Value >(global_settings, transaction, "any-key", "_", 7, 14, "way_key", "node_key",
          false, global_node_offset);
    if ((test_to_execute == "") || (test_to_execute == "36"))
      value_id_type_test(global_settings, transaction, "id-and-type", "_", 1, global_node_offset);

    std::cout<<"</osm>\n";
  }
  catch (File_Error e)
  {
    std::cerr<<"File error: "<<e.error_number<<' '<<e.origin<<' '<<e.filename<<'\n';
    return 1;
  }
  return 0;
}
