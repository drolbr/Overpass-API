/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "print.h"
#include "recurse.h"

using namespace std;

Resource_Manager& perform_id_query(Resource_Manager& rman, string type, uint32 id)
{
  ostringstream buf("");
  buf<<id;
  string id_ = buf.str();
  
  const char* attributes[5];
  attributes[0] = "type";
  attributes[1] = type.c_str();
  attributes[2] = "ref";
  attributes[3] = id_.c_str();
  attributes[4] = 0;
  
  Id_Query_Statement stmt(1, convert_c_pairs(attributes));
  stmt.execute(rman);
  
  return rman;
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
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      // Collect the nodes of some small ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      for (uint32 i = 1; i <= pattern_size/2; ++i)
      {
	Resource_Manager rman(transaction);
	perform_id_query(rman, "way", i);
	if (!rman.sets()["_"].ways.empty())
	  total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
      }
      {
	const char* attributes[] = { "type", "way-node", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
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
    try
    {
      // Collect the nodes of some large ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      uint way_id_offset = 2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2
          + pattern_size*(pattern_size/2-1);
      perform_id_query(total_rman, "way", way_id_offset + 1);
      {
	Resource_Manager rman(transaction);
	way_id_offset = pattern_size*(pattern_size/2-1);
	perform_id_query(rman, "way", way_id_offset + 1);
	if (!rman.sets()["_"].ways.empty())
	  total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
      }
      {
	const char* attributes[] = { "type", "way-node", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
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
    try
    {
      // Recurse node-way: try a node without ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      perform_id_query(rman, "node", 1);
      {
	const char* attributes[] = { "type", "node-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
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
    try
    {
      // Recurse node-way: try a node with a long way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      perform_id_query(rman, "node", pattern_size*pattern_size + pattern_size*3/2 + 2);
      {
	const char* attributes[] = { "type", "node-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
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
    try
    {
      // Recurse node-way: try an entire bbox of nodes (without using bbox)
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      for (uint i = 0; i < pattern_size/2; ++i)
      {
	for (uint j = 1; j <= pattern_size/2; ++j)
	{
	  Resource_Manager rman(transaction);
	  perform_id_query(rman, "node", pattern_size*i + j);
	  if (!rman.sets()["_"].nodes.empty())
	    total_rman.sets()["_"].nodes[rman.sets()["_"].nodes.begin()->first].push_back(rman.sets()["_"].nodes.begin()->second.front());
	}
      }
      {
	const char* attributes[] = { "type", "node-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
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
    try
    {
      // Collect the nodes of some relations
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 2);
      {
	Resource_Manager rman(transaction);
	perform_id_query(rman, "relation", 3);
	if (!rman.sets()["_"].relations.empty())
	  total_rman.sets()["_"].relations[rman.sets()["_"].relations.begin()->first].push_back(rman.sets()["_"].relations.begin()->second.front());
      }
      {
	const char* attributes[] = { "type", "relation-node", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "7"))
  {
    try
    {
      // Recurse node-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      perform_id_query(rman, "node", 2);
      {
	const char* attributes[] = { "type", "node-relation", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 8);
      {
	const char* attributes[] = { "type", "relation-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "9"))
  {
    try
    {
      // Recurse way-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      perform_id_query(rman, "way", 1);
      {
	const char* attributes[] = { "type", "way-relation", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "10"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 10);
      {
	const char* attributes[] = { "type", "relation-relation", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "11"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 2);
      {
	const char* attributes[] = { "type", "relation-backwards", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "12"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 1);
      {
	const char* attributes[] = { "type", "down", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "13"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 6);
      {
	const char* attributes[] = { "type", "down", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "14"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 9);
      {
	const char* attributes[] = { "type", "down-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "15"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction);
      perform_id_query(total_rman, "relation", 10);
      {
	const char* attributes[] = { "type", "down-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes));
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes));
	stmt.execute(total_rman);
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
