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
#include "../output_formats/output_xml.h"
#include "id_query.h"
#include "print.h"
#include "recurse.h"


Resource_Manager& perform_id_query(Resource_Manager& rman, std::string type, uint64 id)
{
  std::ostringstream buf("");
  buf<<id;
  std::string id_ = buf.str();
  Parsed_Query global_settings;

  const char* attributes[5];
  attributes[0] = "type";
  attributes[1] = type.c_str();
  attributes[2] = "ref";
  attributes[3] = id_.c_str();
  attributes[4] = 0;

  Id_Query_Statement stmt(1, convert_c_pairs(attributes), global_settings);
  stmt.execute(rman);

  return rman;
}

int main(int argc, char* args[])
{
  if (argc < 5)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir node_id_offset\n";
    return 0;
  }
  std::string test_to_execute = args[1];
  uint pattern_size = 0;
  pattern_size = atoi(args[2]);
  uint64 global_node_offset = atoll(args[4]);
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      // Collect the nodes of some small ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      Set total;

      for (uint32 i = 1; i <= pattern_size/2; ++i)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "way", i);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->ways.empty())
	  total.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
	const char* attributes[] = { "type", "way-node", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    try
    {
      // Collect the nodes of some large ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);

      uint way_id_offset = 2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2
          + pattern_size*(pattern_size/2-1);
      perform_id_query(total_rman, "way", way_id_offset + 1);
      Set total;
      total_rman.swap_set("_", total);
      {
	Resource_Manager rman(transaction, &global_settings);
	way_id_offset = pattern_size*(pattern_size/2-1);
	perform_id_query(rman, "way", way_id_offset + 1);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->ways.empty())
	  total.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
	const char* attributes[] = { "type", "way-node", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    try
    {
      // Recurse node-way: try a node without ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "node", 1 + global_node_offset);
      {
	const char* attributes[] = { "type", "node-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    try
    {
      // Recurse node-way: try a node with a long way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "node", pattern_size*pattern_size + pattern_size*3/2 + 2 + global_node_offset);
      {
	const char* attributes[] = { "type", "node-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    try
    {
      // Recurse node-way: try an entire bbox of nodes (without using bbox)
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      Set total;

      for (uint i = 0; i < pattern_size/2; ++i)
      {
	for (uint j = 1; j <= pattern_size/2; ++j)
	{
	  Resource_Manager rman(transaction, &global_settings);
	  perform_id_query(rman, "node", pattern_size*i + j + global_node_offset);
          const Set* default_ = rman.get_set("_");
	  if (default_ && !default_->nodes.empty())
	    total.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
	}
      }
      total_rman.swap_set("_", total);
      {
	const char* attributes[] = { "type", "node-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    try
    {
      // Collect the nodes of some relations
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 2);
      Set total;
      total_rman.swap_set("_", total);

      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "relation", 3);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->relations.empty())
	  total.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
	const char* attributes[] = { "type", "relation-node", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "7"))
  {
    try
    {
      // Recurse node-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "node", 2 + global_node_offset);
      {
	const char* attributes[] = { "type", "node-relation", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 8);
      {
	const char* attributes[] = { "type", "relation-way", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "9"))
  {
    try
    {
      // Recurse way-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "way", 1);
      {
	const char* attributes[] = { "type", "way-relation", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "10"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 10);
      {
	const char* attributes[] = { "type", "relation-relation", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "11"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 2);
      {
	const char* attributes[] = { "type", "relation-backwards", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "12"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 1);
      {
	const char* attributes[] = { "type", "down", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "13"))
  {
    try
    {
      // Recurse down
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 6);
      {
	const char* attributes[] = { "type", "down", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "14"))
  {
    try
    {
      // Recurse down-rel with a recursive relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 9);
      {
	const char* attributes[] = { "type", "down-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "15"))
  {
    try
    {
      // Recurse down-rel with a mixed relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 10);
      {
	const char* attributes[] = { "type", "down-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "16"))
  {
    try
    {
      // Recurse up
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "node", pattern_size + 2 + global_node_offset);
      {
	const char* attributes[] = { "type", "up", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "17"))
  {
    try
    {
      // Recurse up
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);
      {
	const char* attributes[] = { "type", "up", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "18"))
  {
    try
    {
      // Recurse up-rel with a node
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "node", 2 + global_node_offset);
      {
	const char* attributes[] = { "type", "up-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "19"))
  {
    try
    {
      // Recurse up-rel with a way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 2);
      {
	const char* attributes[] = { "type", "up-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "20"))
  {
    try
    {
      // Recurse up-rel with a relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 1);
      {
	const char* attributes[] = { "type", "up-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "21"))
  {
    try
    {
      // Recurse down with a way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);
      {
	const char* attributes[] = { "type", "down", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "22"))
  {
    try
    {
      // Recurse down-rel with a way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);
      {
	const char* attributes[] = { "type", "down-rel", 0 };
	Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
      {
	const char* attributes[] = { 0 };
	Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "23"))
  {
    try
    {
      // Collect the nodes of some relations
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 1);

      {
        const char* attributes[] = { "type", "relation-node", "role", "one", 0 };
        Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
      {
        const char* attributes[] = { 0 };
        Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "24"))
  {
    try
    {
      // Recurse node-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "node", 1 + global_node_offset);
      Set total;
      total_rman.swap_set("_", total);

      {
        Resource_Manager rman(transaction, &global_settings);
        perform_id_query(rman, "node", 4 + global_node_offset);
        const Set* default_ = rman.get_set("_");
        if (default_ && !default_->nodes.empty())
          total.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
        const char* attributes[] = { "type", "node-relation", "role", "zero", 0 };
        Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
      {
        const char* attributes[] = { 0 };
        Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "25"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 7);
      {
        const char* attributes[] = { "type", "relation-way", "role", "two", 0 };
        Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
      {
        const char* attributes[] = { 0 };
        Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "26"))
  {
    try
    {
      // Recurse way-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);
      {
        const char* attributes[] = { "type", "way-relation", "role", "two", 0 };
        Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
      {
        const char* attributes[] = { 0 };
        Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "27"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 9);

      {
        const char* attributes[] = { "type", "relation-relation", "role", "one", 0 };
        Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
      {
        const char* attributes[] = { 0 };
        Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "28"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 1);
      Set total;
      total_rman.swap_set("_", total);

      {
        Resource_Manager rman(transaction, &global_settings);
        perform_id_query(rman, "relation", 3);
        const Set* default_ = rman.get_set("_");
        if (default_ && !default_->relations.empty())
          total.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
        const char* attributes[] = { "type", "relation-backwards", "role", "one", 0 };
        Recurse_Statement stmt(2, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
      {
        const char* attributes[] = { 0 };
        Print_Statement stmt(3, convert_c_pairs(attributes), global_settings);
        stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  std::cout<<"</osm>\n";
  return 0;
}
