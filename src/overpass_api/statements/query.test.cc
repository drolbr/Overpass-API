#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "bbox_query.h"
#include "query.h"
#include "print.h"

using namespace std;

void perform_print(Resource_Manager& rman)
{
  Print_Statement stmt(0);
  const char* attributes[] = { "order", "id", 0 };
  stmt.set_attributes(attributes);
  stmt.execute(rman);
}

void perform_query(string type, string key, string value)
{
  try
  {
    Resource_Manager rman;
    {
      Query_Statement stmt1(0);
      const char* attributes[] = { "type", type.c_str(), 0 };
      stmt1.set_attributes(attributes);
      
      Has_Kv_Statement stmt2(0);
      const char* attributes_kv[] = { "k", key.c_str(), "v", value.c_str(), 0 };
      stmt2.set_attributes(attributes_kv);
      stmt1.add_statement(&stmt2, "");
      
      stmt1.execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query
    (string type, string key1, string value1, string key2, string value2)
{
  try
  {
    Resource_Manager rman;
    {
      Query_Statement stmt1(0);
      const char* attributes[] = { "type", type.c_str(), 0 };
      stmt1.set_attributes(attributes);
      
      Has_Kv_Statement stmt2(0);
      const char* attributes_kv1[] = { "k", key1.c_str(), "v", value1.c_str(), 0 };
      stmt2.set_attributes(attributes_kv1);
      stmt1.add_statement(&stmt2, "");
      
      Has_Kv_Statement stmt3(0);
      const char* attributes_kv2[] = { "k", key2.c_str(), "v", value2.c_str(), 0 };
      stmt3.set_attributes(attributes_kv2);
      stmt1.add_statement(&stmt3, "");
      
      stmt1.execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query
    (string type, string key1, string value1, string key2, string value2,
     string key3, string value3)
{
  try
  {
    Resource_Manager rman;
    {
      Query_Statement stmt1(0);
      const char* attributes[] = { "type", type.c_str(), 0 };
      stmt1.set_attributes(attributes);
      
      Has_Kv_Statement stmt2(0);
      const char* attributes_kv1[] = { "k", key1.c_str(), "v", value1.c_str(), 0 };
      stmt2.set_attributes(attributes_kv1);
      stmt1.add_statement(&stmt2, "");
      
      Has_Kv_Statement stmt3(0);
      const char* attributes_kv2[] = { "k", key2.c_str(), "v", value2.c_str(), 0 };
      stmt3.set_attributes(attributes_kv2);
      stmt1.add_statement(&stmt3, "");
      
      Has_Kv_Statement stmt4(0);
      const char* attributes_kv3[] = { "k", key3.c_str(), "v", value3.c_str(), 0 };
      stmt4.set_attributes(attributes_kv3);
      stmt1.add_statement(&stmt4, "");
      
      stmt1.execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query_with_bbox
    (string type, string key1, string value1,
     string south, string north, string west, string east)
{
  try
  {
    Resource_Manager rman;
    {
      Query_Statement stmt1(0);
      const char* attributes[] = { "type", type.c_str(), 0 };
      stmt1.set_attributes(attributes);
      
      Has_Kv_Statement stmt2(0);
      const char* attributes_kv[] = { "k", key1.c_str(), "v", value1.c_str(), 0 };
      stmt2.set_attributes(attributes_kv);
      stmt1.add_statement(&stmt2, "");
      
      Bbox_Query_Statement stmt3(0);
      const char* attributes_bbox[] =
          { "n", north.c_str(), "s", south.c_str(),
	    "e", east.c_str(), "w", west.c_str(), 0 };
      stmt3.set_attributes(attributes_bbox);
      stmt1.add_statement(&stmt3, "");
      
      stmt1.execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

int main(int argc, char* args[])
{
  if (argc < 4)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir\n";
    return 0;
  }
  string test_to_execute = args[1];
  uint pattern_size = 0;
  pattern_size = atoi(args[2]);
  set_basedir(args[3]);
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  // Test queries for nodes.
  if ((test_to_execute == "") || (test_to_execute == "1"))
    // Test a key and value which appears only locally
    perform_query("node", "node_key_11", "node_value_2");
  if ((test_to_execute == "") || (test_to_execute == "2"))
    // Test a key and value which appears almost everywhere
    perform_query("node", "node_key_5", "node_value_5");
  if ((test_to_execute == "") || (test_to_execute == "3"))
    // Test a key only which has multiple values
    perform_query("node", "node_key_11", "");
  if ((test_to_execute == "") || (test_to_execute == "4"))
    // Test a key only which has only one value
    perform_query("node", "node_key_15", "");
  if ((test_to_execute == "") || (test_to_execute == "5"))
    // Test a key only which doesn't appear at all
    perform_query("node", "nowhere", "");
  if ((test_to_execute == "") || (test_to_execute == "6"))
    // Test a key intersected with a small key and value pair
    perform_query("node", "node_key_7", "", "node_key_11", "node_value_8");
  if ((test_to_execute == "") || (test_to_execute == "7"))
    // Test a key intersected with a large key and value pair
    perform_query("node", "node_key_7", "", "node_key_15", "node_value_15");
  if ((test_to_execute == "") || (test_to_execute == "8"))
    // Test a bbox combined with a local key-value pair
    perform_query_with_bbox("node", "node_key_11", "node_value_2",
			    "51.0", "51.2", "7.0", "8.0");
  if ((test_to_execute == "") || (test_to_execute == "9"))
    // Test a bbox combined with a global key-value pair
    perform_query_with_bbox("node", "node_key_5", "node_value_5",
			    "-10.0", "-1.0", "-15.0", "-3.0");
  if ((test_to_execute == "") || (test_to_execute == "10"))
    // Test a bbox combined with a global key-value pair
    perform_query_with_bbox("node", "node_key_7", "",
			    "-10.0", "-1.0", "-15.0", "-3.0");
  if ((test_to_execute == "") || (test_to_execute == "11"))
    // Test three key-values intersected
    perform_query("node", "node_key_5", "node_value_5", "node_key_7", "node_value_0",
		  "node_key_15", "node_value_15");
  
  // Test queries for ways.
  if ((test_to_execute == "") || (test_to_execute == "12"))
    // Test a key and value which appears only locally
    perform_query("way", "way_key_11", "way_value_2");
  if ((test_to_execute == "") || (test_to_execute == "13"))
    // Test a key and value which appears almost everywhere
    perform_query("way", "way_key_5", "way_value_5");
  if ((test_to_execute == "") || (test_to_execute == "14"))
    // Test a key only which has multiple values
    perform_query("way", "way_key_11", "");
  if ((test_to_execute == "") || (test_to_execute == "15"))
    // Test a key only which has only one value
    perform_query("way", "way_key_15", "");
  if ((test_to_execute == "") || (test_to_execute == "16"))
    // Test a key only which doesn't appear at all
    perform_query("way", "nowhere", "");
  if ((test_to_execute == "") || (test_to_execute == "17"))
    // Test a key intersected with a small key and value pair
    perform_query("way", "way_key_7", "", "way_key_11", "way_value_8");
  if ((test_to_execute == "") || (test_to_execute == "18"))
    // Test a key intersected with a large key and value pair
    perform_query("way", "way_key_7", "", "way_key_15", "way_value_15");
  if ((test_to_execute == "") || (test_to_execute == "19"))
    // Test three key-values intersected
    perform_query("way", "way_key_5", "way_value_5", "way_key_7", "way_value_0",
		  "way_key_15", "way_value_15");

  // Test queries for relations.
  if ((test_to_execute == "") || (test_to_execute == "20"))
    // Test a key and value which appears only locally
    perform_query("relation", "relation_key_11", "relation_value_2");
  if ((test_to_execute == "") || (test_to_execute == "21"))
    // Test a key and value which appears almost everywhere
    perform_query("relation", "relation_key_2/4", "relation_value_1");
  if ((test_to_execute == "") || (test_to_execute == "22"))
    // Test a key only which has multiple values
    perform_query("relation", "relation_key_2/4", "");
  if ((test_to_execute == "") || (test_to_execute == "23"))
    // Test a key only which has only one value
    perform_query("relation", "relation_key_5", "");
  if ((test_to_execute == "") || (test_to_execute == "24"))
    // Test a key only which doesn't appear at all
    perform_query("relation", "nowhere", "");
  if ((test_to_execute == "") || (test_to_execute == "25"))
    // Test two key-values intersected. This tests also whether
    // relations with index zero appear in the results.
    perform_query("relation", "relation_key_2/4", "relation_value_0",
		  "relation_key_5", "relation_value_5");

  cout<<"</osm>\n";
  return 0;
  
  // Test queries for relations.
  /*
  type: node, way, relation
  for relations: also read zero key relations
    */
  
  Query_Statement stmt_5(5), stmt_6(6);
  
  Resource_Manager rman;
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "name", "v", "Grenze Jagdhaus", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "name", "v", "", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "shelter", "v", "yes", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "way", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "name", "v", "Morianstraße", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "shelter", "v", "yes", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "relation", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "type", "v", "route", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "route", "v", "bus", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "ref", "v", "", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2", "n", "51.3", "w", "7.0", "e", "7.3", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "skeleton", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  return 0;
}
