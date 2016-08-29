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
#include "foreach.h"
#include "id_query.h"
#include "print.h"

using namespace std;

Resource_Manager& perform_id_query(Resource_Manager& rman, string type, uint64 id)
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

Resource_Manager& fill_loop_set
    (Resource_Manager& rman, string set_name, uint pattern_size, uint64 global_node_offset,
     Transaction& transaction)
{
  uint way_id_offset = (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2);
  
  Resource_Manager partial_rman(transaction);
  perform_id_query(partial_rman, "node", 1 + global_node_offset);
  if (!partial_rman.sets()["_"].nodes.empty())
    rman.sets()[set_name].nodes[partial_rman.sets()["_"].nodes.begin()->first].push_back(partial_rman.sets()["_"].nodes.begin()->second.front());
  perform_id_query(partial_rman, "node", 2 + global_node_offset);
  if (!partial_rman.sets()["_"].nodes.empty())
    rman.sets()[set_name].nodes[partial_rman.sets()["_"].nodes.begin()->first].push_back(partial_rman.sets()["_"].nodes.begin()->second.front());
  perform_id_query(partial_rman, "node", 3 + global_node_offset);
  if (!partial_rman.sets()["_"].nodes.empty())
    rman.sets()[set_name].nodes[partial_rman.sets()["_"].nodes.begin()->first].push_back(partial_rman.sets()["_"].nodes.begin()->second.front());
/*  perform_id_query(partial_rman, "way", 1);
  if (!partial_rman.sets()["_"].ways.empty())
    rman.sets()[set_name].ways[partial_rman.sets()["_"].ways.begin()->first].push_back(partial_rman.sets()["_"].ways.begin()->second.front());*/
  perform_id_query(partial_rman, "way", way_id_offset + 1);
  if (!partial_rman.sets()["_"].ways.empty())
    rman.sets()[set_name].ways[partial_rman.sets()["_"].ways.begin()->first].push_back(partial_rman.sets()["_"].ways.begin()->second.front());
  perform_id_query(partial_rman, "way", 2*way_id_offset + 1);
  if (!partial_rman.sets()["_"].ways.empty())
    rman.sets()[set_name].ways[partial_rman.sets()["_"].ways.begin()->first].push_back(partial_rman.sets()["_"].ways.begin()->second.front());
/*  perform_id_query(partial_rman, "relation", 10);
  if (!partial_rman.sets()["_"].relations.empty())
    rman.sets()[set_name].relations[partial_rman.sets()["_"].relations.begin()->first].push_back(partial_rman.sets()["_"].relations.begin()->second.front());*/
  perform_id_query(partial_rman, "relation", 21);
  if (!partial_rman.sets()["_"].relations.empty())
    rman.sets()[set_name].relations[partial_rman.sets()["_"].relations.begin()->first].push_back(partial_rman.sets()["_"].relations.begin()->second.front());
  perform_id_query(partial_rman, "relation", 32);
  if (!partial_rman.sets()["_"].relations.empty())
    rman.sets()[set_name].relations[partial_rman.sets()["_"].relations.begin()->first].push_back(partial_rman.sets()["_"].relations.begin()->second.front());
  
  return rman;
}

int main(int argc, char* args[])
{
  if (argc < 5)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir node_id_offset\n";
    return 0;
  }
  string test_to_execute = args[1];
  uint pattern_size = 0;
  pattern_size = atoi(args[2]);
  uint64 global_node_offset = atoll(args[4]);
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "_", pattern_size, global_node_offset, transaction);
      {
	const char* attributes[] = { 0 };
	Foreach_Statement stmt1(0, convert_c_pairs(attributes));
	
	const char* attributes_print[] = { 0 };
	Print_Statement stmt2(0, convert_c_pairs(attributes_print));
	stmt1.add_statement(&stmt2, "");
	
	stmt1.execute(rman);
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
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "_", pattern_size, global_node_offset, transaction);
      {
	const char* attributes[] = { 0 };
	Foreach_Statement stmt1(0, convert_c_pairs(attributes));
	stmt1.execute(rman);
      }
      {
	const char* attributes_print[] = { 0 };
	Print_Statement stmt(0, convert_c_pairs(attributes_print));
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
    try
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "A", pattern_size, global_node_offset, transaction);
      {
	const char* attributes[] = { "from", "A", "into", "B", 0 };
	Foreach_Statement stmt1(0, convert_c_pairs(attributes));
	
	const char* attributes_print[] = { "from", "B", 0 };
	Print_Statement stmt2(0, convert_c_pairs(attributes_print));
	stmt1.add_statement(&stmt2, "");
	
	stmt1.execute(rman);
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
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "A", pattern_size, global_node_offset, transaction);
      {
	const char* attributes[] = { "from", "A", "into", "B", 0 };
	Foreach_Statement stmt1(0, convert_c_pairs(attributes));
	stmt1.execute(rman);
      }
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
  
  cout<<"</osm>\n";
  return 0;
}
