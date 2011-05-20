#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "item.h"
#include "print.h"
#include "query.h"
#include "recurse.h"
#include "union.h"

using namespace std;

int main(int argc, char* args[])
{
  if (argc < 3)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute db_dir\n";
    return 0;
  }
  string test_to_execute = args[1];
  set_basedir(args[2]);
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    // Test whether union works well with all relevant statements.
    try
    {
      {
	Nonsynced_Transaction transaction(false, false, args[2], "");
	Resource_Manager rman(transaction);
	
	Union_Statement stmt(0);
	const char* attributes[] = { 0 };
	stmt.set_attributes(attributes);
	
	Id_Query_Statement stmt1(0);
	const char* attributes1[] = { "type", "node", "ref", "2", 0 };
	stmt1.set_attributes(attributes1);
	stmt.add_statement(&stmt1, "");
	
	Recurse_Statement stmt2(0);
	const char* attributes2[] = { "type", "node-relation", 0 };
	stmt2.set_attributes(attributes2);
	stmt.add_statement(&stmt2, "");
	
	Query_Statement stmt3(0);
	const char* attributes3[] = { "type", "way", 0 };
	stmt3.set_attributes(attributes3);
	
	Has_Kv_Statement stmt4(0);
	const char* attributes_kv[] = { "k", "way_key_11", "v", "way_value_2", 0 };
	stmt4.set_attributes(attributes_kv);
	stmt3.add_statement(&stmt4, "");
	
	stmt.add_statement(&stmt3, "");
	
	stmt.execute(rman);
	{
	  Print_Statement stmt(0);
	  const char* attributes[] = { 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    // Test whether union handles properly unsorted, but non-unique content.
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction);
      
      Union_Statement stmt(0);
      const char* attributes[] = { 0 };
      stmt.set_attributes(attributes);
      
      Id_Query_Statement stmt1(0);
      const char* attributes1[] = { "type", "node", "ref", "2", 0 };
      stmt1.set_attributes(attributes1);
      stmt.add_statement(&stmt1, "");
      
      Id_Query_Statement stmt2(0);
      const char* attributes2[] = { "type", "node", "ref", "1", 0 };
      stmt2.set_attributes(attributes2);
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	Print_Statement stmt(0);
	const char* attributes[] = { 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    // Test whether all relevant statements declare properly their output sets.
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction);
      
      Union_Statement stmt(0);
      const char* attributes[] = { "into", "A", 0 };
      stmt.set_attributes(attributes);
      
      Id_Query_Statement stmt1(0);
      const char* attributes1[] = { "type", "node", "ref", "2", "into", "B", 0 };
      stmt1.set_attributes(attributes1);
      stmt.add_statement(&stmt1, "");
      
      Recurse_Statement stmt2(0);
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      stmt2.set_attributes(attributes2);
      stmt.add_statement(&stmt2, "");
      
      Query_Statement stmt3(0);
      const char* attributes3[] = { "type", "way", "into", "D", 0 };
      stmt3.set_attributes(attributes3);
      
      Has_Kv_Statement stmt4(0);
      const char* attributes_kv[] = { "k", "way_key_11", "v", "way_value_2", 0 };
      stmt4.set_attributes(attributes_kv);
      stmt3.add_statement(&stmt4, "");
      
      stmt.add_statement(&stmt3, "");
      
      stmt.execute(rman);
      {
	Print_Statement stmt(0);
	const char* attributes[] = { "from", "A", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    // Test whether union doesn't affect other sets - part 1
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction);
      {
	Id_Query_Statement stmt(0);
	const char* attributes[] = { "type", "way", "ref", "1", "into", "A", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      
      Union_Statement stmt(0);
      const char* attributes[] = { 0 };
      stmt.set_attributes(attributes);
      
      Item_Statement stmt0(0);
      const char* attributes0[] = { "from", "A", 0 };
      stmt0.set_attributes(attributes0);
      stmt.add_statement(&stmt0, "");
      
      Id_Query_Statement stmt1(0);
      const char* attributes1[] = { "type", "node", "ref", "2", "into", "B", 0 };
      stmt1.set_attributes(attributes1);
      stmt.add_statement(&stmt1, "");
      
      Recurse_Statement stmt2(0);
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      stmt2.set_attributes(attributes2);
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	Print_Statement stmt(0);
	const char* attributes[] = { "from", "A", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    // Test whether union doesn't affect other sets - part 2
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction);
      {
	Id_Query_Statement stmt(0);
	const char* attributes[] = { "type", "way", "ref", "1", "into", "A", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      
      Union_Statement stmt(0);
      const char* attributes[] = { 0 };
      stmt.set_attributes(attributes);
      
      Item_Statement stmt0(0);
      const char* attributes0[] = { "from", "A", 0 };
      stmt0.set_attributes(attributes0);
      stmt.add_statement(&stmt0, "");
      
      Id_Query_Statement stmt1(0);
      const char* attributes1[] = { "type", "node", "ref", "2", "into", "B", 0 };
      stmt1.set_attributes(attributes1);
      stmt.add_statement(&stmt1, "");
      
      Recurse_Statement stmt2(0);
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      stmt2.set_attributes(attributes2);
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	Print_Statement stmt(0);
	const char* attributes[] = { "from", "B", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    // Test whether union doesn't affect other sets - part 3
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction);
      {
	Id_Query_Statement stmt(0);
	const char* attributes[] = { "type", "way", "ref", "1", "into", "A", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      
      Union_Statement stmt(0);
      const char* attributes[] = { 0 };
      stmt.set_attributes(attributes);
      
      Item_Statement stmt0(0);
      const char* attributes0[] = { "from", "A", 0 };
      stmt0.set_attributes(attributes0);
      stmt.add_statement(&stmt0, "");
      
      Id_Query_Statement stmt1(0);
      const char* attributes1[] = { "type", "node", "ref", "2", "into", "B", 0 };
      stmt1.set_attributes(attributes1);
      stmt.add_statement(&stmt1, "");
      
      Recurse_Statement stmt2(0);
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      stmt2.set_attributes(attributes2);
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	Print_Statement stmt(0);
	const char* attributes[] = { "from", "C", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  cout<<"</osm>\n";
  return 0;
}
