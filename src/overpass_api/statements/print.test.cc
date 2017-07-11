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
    std::cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir global_node_offset\n";
    return 0;
  }
  std::string test_to_execute = args[1];
  uint pattern_size = 0;
  pattern_size = atoi(args[2]);
  uint64 global_node_offset = atoll(args[4]);
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

  uint32 node_id_upper_limit = 5*pattern_size*pattern_size;
  uint32 way_id_upper_limit = 5*pattern_size*pattern_size;
  uint32 relation_id_upper_limit = 1000;

  Nonsynced_Transaction transaction(false, false, args[3], "");

  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      // Print each item alone:
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	// Print nodes:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "node", i + global_node_offset);
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	// Print ways:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "way", i);
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	// Print relations:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "relation", i);
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
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
      // Print skeletons:
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	// Print nodes:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "node", i + global_node_offset);
	{
	  const char* attributes[] = { "mode", "skeleton", 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	// Print ways:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "way", i);
	{
	  const char* attributes[] = { "mode", "skeleton", 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	// Print relations:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "relation", i);
	{
	  const char* attributes[] = { "mode", "skeleton", 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
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
      // Print ids_only:
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	// Print nodes:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "node", i + global_node_offset);
	{
	  const char* attributes[] = { "mode", "ids_only", 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	// Print ways:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "way", i);
	{
	  const char* attributes[] = { "mode", "ids_only", 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	// Print relations:
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "relation", i);
	{
	  const char* attributes[] = { "mode", "ids_only", 0 };
	  Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
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
      // Print all items sorted by id:
      Resource_Manager total_rman(transaction, &global_settings);
      Set total;
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "node", i + global_node_offset);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->nodes.empty())
	  total.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "way", i);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->ways.empty())
	  total.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "relation", i);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->relations.empty())
	  total.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
	const char* attributes[] = { "order", "id", 0 };
	Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
	stmt.execute(total_rman);
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
      std::cout<<"Print all items sorted by quadtile:\n";
      Resource_Manager total_rman(transaction, &global_settings);
      Set total;
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "node", i + global_node_offset);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->nodes.empty())
	  total.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "way", i);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->ways.empty())
	  total.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	Resource_Manager rman(transaction, &global_settings);
	perform_id_query(rman, "relation", i);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->relations.empty())
	  total.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());
      }
      total_rman.swap_set("_", total);
      {
	const char* attributes[] = { "order", "quadtile", 0 };
	Print_Statement stmt(2, convert_c_pairs(attributes), global_settings);
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
