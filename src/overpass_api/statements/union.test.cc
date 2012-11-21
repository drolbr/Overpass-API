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

string to_string_(uint64 val)
{
  ostringstream buf;
  buf<<val;
  return buf.str();
}

int main(int argc, char* args[])
{
  if (argc < 4)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute db_dir node_id_offset\n";
    return 0;
  }
  string test_to_execute = args[1];
  uint64 global_node_offset = atoll(args[3]);
  
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
	
	const char* attributes[] = { 0 };
	Union_Statement stmt(0, convert_c_pairs(attributes));

	string buf = to_string_(2 + global_node_offset);
	const char* attributes1[] = { "type", "node", "ref", buf.c_str(), 0 };
	Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
	stmt.add_statement(&stmt1, "");
	
	const char* attributes2[] = { "type", "node-relation", 0 };
	Recurse_Statement stmt2(0, convert_c_pairs(attributes2));
	stmt.add_statement(&stmt2, "");
	
	const char* attributes3[] = { "type", "way", 0 };
	Query_Statement stmt3(0, convert_c_pairs(attributes3));
	
	const char* attributes_kv[] = { "k", "way_key_11", "v", "way_value_2", 0 };
	Has_Kv_Statement stmt4(0, convert_c_pairs(attributes_kv));
	stmt3.add_statement(&stmt4, "");
	
	stmt.add_statement(&stmt3, "");
	
	stmt.execute(rman);
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes));
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
      
      const char* attributes[] = { 0 };
      Union_Statement stmt(0, convert_c_pairs(attributes));
      
      string buf = to_string_(2 + global_node_offset);
      const char* attributes1[] = { "type", "node", "ref", buf.c_str(), 0 };
      Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
      stmt.add_statement(&stmt1, "");
      
      buf = to_string_(1 + global_node_offset);
      const char* attributes2[] = { "type", "node", "ref", buf.c_str(), 0 };
      Id_Query_Statement stmt2(0, convert_c_pairs(attributes2));
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(0, convert_c_pairs(attributes));
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
      
      const char* attributes[] = { "into", "A", 0 };
      Union_Statement stmt(0, convert_c_pairs(attributes));
      
      string buf = to_string_(2 + global_node_offset);
      const char* attributes1[] = { "type", "node", "ref", buf.c_str(), "into", "B", 0 };
      Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
      stmt.add_statement(&stmt1, "");
      
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      Recurse_Statement stmt2(0, convert_c_pairs(attributes2));
      stmt.add_statement(&stmt2, "");
      
      const char* attributes3[] = { "type", "way", "into", "D", 0 };
      Query_Statement stmt3(0, convert_c_pairs(attributes3));
      
      const char* attributes_kv[] = { "k", "way_key_11", "v", "way_value_2", 0 };
      Has_Kv_Statement stmt4(0, convert_c_pairs(attributes_kv));
      stmt3.add_statement(&stmt4, "");
      
      stmt.add_statement(&stmt3, "");
      
      stmt.execute(rman);
      {
	const char* attributes[] = { "from", "A", 0 };
	Print_Statement stmt(0, convert_c_pairs(attributes));
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
	const char* attributes[] = { "type", "way", "ref", "1", "into", "A", 0 };
	Id_Query_Statement stmt(0, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      
      const char* attributes[] = { 0 };
      Union_Statement stmt(0, convert_c_pairs(attributes));
      
      const char* attributes0[] = { "from", "A", 0 };
      Item_Statement stmt0(0, convert_c_pairs(attributes0));
      stmt.add_statement(&stmt0, "");
      
      string buf = to_string_(2 + global_node_offset);
      const char* attributes1[] = { "type", "node", "ref", buf.c_str(), "into", "B", 0 };
      Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
      stmt.add_statement(&stmt1, "");
      
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      Recurse_Statement stmt2(0, convert_c_pairs(attributes2));
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	const char* attributes[] = { "from", "A", 0 };
	Print_Statement stmt(0, convert_c_pairs(attributes));
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
	const char* attributes[] = { "type", "way", "ref", "1", "into", "A", 0 };
	Id_Query_Statement stmt(0, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      
      const char* attributes[] = { 0 };
      Union_Statement stmt(0, convert_c_pairs(attributes));
      
      const char* attributes0[] = { "from", "A", 0 };
      Item_Statement stmt0(0, convert_c_pairs(attributes0));
      stmt.add_statement(&stmt0, "");
      
      string buf = to_string_(2 + global_node_offset);
      const char* attributes1[] = { "type", "node", "ref", buf.c_str(), "into", "B", 0 };
      Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
      stmt.add_statement(&stmt1, "");
      
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      Recurse_Statement stmt2(0, convert_c_pairs(attributes2));
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	const char* attributes[] = { "from", "B", 0 };
	Print_Statement stmt(0, convert_c_pairs(attributes));
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
	const char* attributes[] = { "type", "way", "ref", "1", "into", "A", 0 };
	Id_Query_Statement stmt(0, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      
      const char* attributes[] = { 0 };
      Union_Statement stmt(0, convert_c_pairs(attributes));
      
      const char* attributes0[] = { "from", "A", 0 };
      Item_Statement stmt0(0, convert_c_pairs(attributes0));
      stmt.add_statement(&stmt0, "");
      
      string buf = to_string_(2 + global_node_offset);
      const char* attributes1[] = { "type", "node", "ref", buf.c_str(), "into", "B", 0 };
      Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
      stmt.add_statement(&stmt1, "");
      
      const char* attributes2[] = { "type", "node-relation", "from", "B", "into", "C", 0 };
      Recurse_Statement stmt2(0, convert_c_pairs(attributes2));
      stmt.add_statement(&stmt2, "");
      
      stmt.execute(rman);
      {
	const char* attributes[] = { "from", "C", 0 };
	Print_Statement stmt(0, convert_c_pairs(attributes));
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
