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

#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "item.h"
#include "print.h"
#include "query.h"
#include "recurse.h"
#include "difference.h"

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
  uint64 size = atoll(args[2]);
  uint64 global_node_offset = atoll(args[3]);
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    // Test whether difference works well with an empty base set
    try
    {
      {
	Nonsynced_Transaction transaction(false, false, args[2], "");
	Resource_Manager rman(transaction);
	
	const char* attributes[] = { 0 };
	Difference_Statement stmt(0, convert_c_pairs(attributes));

	string buf = to_string_(size * size);
	const char* attributes1[] = { "type", "node", "ref", buf.c_str(), 0 };
	Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
	stmt.add_statement(&stmt1, "");
	
        buf = to_string_(2 + global_node_offset);
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
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    // Test whether difference works well with an empty set to substract
    try
    {
      {
        Nonsynced_Transaction transaction(false, false, args[2], "");
        Resource_Manager rman(transaction);
        
        const char* attributes[] = { 0 };
        Difference_Statement stmt(0, convert_c_pairs(attributes));

        string buf = to_string_(2 + global_node_offset);
        const char* attributes1[] = { "type", "node", "ref", buf.c_str(), 0 };
        Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
        stmt.add_statement(&stmt1, "");
        
        buf = to_string_(size * size);
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
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    // Test whether difference works well with both sets non empty
    try
    {
      {
        Nonsynced_Transaction transaction(false, false, args[2], "");
        Resource_Manager rman(transaction);
        
        const char* attributes[] = { 0 };
        Difference_Statement stmt(0, convert_c_pairs(attributes));

        string buf = to_string_(2 + global_node_offset);
        const char* attributes1[] = { "type", "node", "ref", buf.c_str(), 0 };
        Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
        stmt.add_statement(&stmt1, "");
        
        buf = to_string_(2 + global_node_offset);
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
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    // Test whether difference works well with both sets non empty for ways and relations
    try
    {
      {
        Nonsynced_Transaction transaction(false, false, args[2], "");
        Resource_Manager rman(transaction);
        
        const char* attributes[] = { 0 };
        Difference_Statement stmt(0, convert_c_pairs(attributes));

        const char* attributes1[] = { "type", "way", "lower", "1", "upper", "4", 0 };
        Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
        stmt.add_statement(&stmt1, "");
        
        const char* attributes2[] = { "type", "way", "lower", "2", "upper", "3", 0 };
        Id_Query_Statement stmt2(0, convert_c_pairs(attributes2));
        stmt.add_statement(&stmt2, "");
        
        stmt.execute(rman);
        {
          const char* attributes[] = { 0 };
          Print_Statement stmt(0, convert_c_pairs(attributes));
          stmt.execute(rman);
        }
      }
      {
        Nonsynced_Transaction transaction(false, false, args[2], "");
        Resource_Manager rman(transaction);
        
        const char* attributes[] = { 0 };
        Difference_Statement stmt(0, convert_c_pairs(attributes));

        const char* attributes1[] = { "type", "relation", "lower", "1", "upper", "4", 0 };
        Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
        stmt.add_statement(&stmt1, "");
        
        const char* attributes2[] = { "type", "relation", "lower", "2", "upper", "3", 0 };
        Id_Query_Statement stmt2(0, convert_c_pairs(attributes2));
        stmt.add_statement(&stmt2, "");
        
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
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    // Test whether all relevant statements declare properly their output sets.
    try
    {
      {
        Nonsynced_Transaction transaction(false, false, args[2], "");
        Resource_Manager rman(transaction);
        
        const char* attributes[] = { "into", "A", 0 };
        Difference_Statement stmt(0, convert_c_pairs(attributes));

        const char* attributes1[] = { "type", "way", "lower", "1", "upper", "4", "into", "B", 0 };
        Id_Query_Statement stmt1(0, convert_c_pairs(attributes1));
        stmt.add_statement(&stmt1, "");
        
        const char* attributes2[] = { "type", "way", "lower", "2", "upper", "3", "into", "C", 0 };
        Id_Query_Statement stmt2(0, convert_c_pairs(attributes2));
        stmt.add_statement(&stmt2, "");
        
        stmt.execute(rman);
        {
          const char* attributes[] = { "from", "A", 0 };
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
  
  cout<<"</osm>\n";
  return 0;
}
