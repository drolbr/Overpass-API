/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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
  Parsed_Query global_settings;
  Id_Query_Statement(1, { {"type", type}, {"ref", std::to_string(id)} }, global_settings).execute(rman);
  return rman;
}

Resource_Manager& perform_multi_id_query(Resource_Manager& rman, std::string type, const std::vector< uint64 >& ids)
{
  std::map< std::string, std::string > attributes;
  attributes["type"] = type;
  if (!ids.empty())
    attributes["ref"] = std::to_string(ids[0]);
  for (uint i = 1; i < ids.size(); ++i)
    attributes["ref_" + std::to_string(i)] = std::to_string(ids[i]);

  Parsed_Query global_settings;
  Id_Query_Statement(1, attributes, global_settings).execute(rman);
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

      Recurse_Statement(2, { { "type", "way-node" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
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
	perform_id_query(rman, "way", pattern_size*(pattern_size/2-1) + 1);
        const Set* default_ = rman.get_set("_");
	if (default_ && !default_->ways.empty())
	  total.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
      }
      total_rman.swap_set("_", total);

      Recurse_Statement(2, { { "type", "way-node" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    try
    {
      // Recurse node-way: try a node without ways
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "node", 1 + global_node_offset);

      Recurse_Statement(2, { { "type", "node-way" } }, global_settings).execute(rman);
      Print_Statement(3, {}, global_settings).execute(rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    try
    {
      // Recurse node-way: try a node with a long way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "node", pattern_size*pattern_size + pattern_size*3/2 + 2 + global_node_offset);

      Recurse_Statement(2, { { "type", "node-way" } }, global_settings).execute(rman);
      Print_Statement(3, {}, global_settings).execute(rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
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
        std::vector< uint64 > ids;
	for (uint j = 1; j <= pattern_size/2; ++j)
          ids.push_back(pattern_size*i + j + global_node_offset);

	Resource_Manager rman(transaction, &global_settings);
	perform_multi_id_query(rman, "node", ids);
        const Set* default_ = rman.get_set("_");
	if (default_)
        {
          for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator
              it1 = default_->nodes.begin(); it1 != default_->nodes.end(); ++it1)
          {
            for (std::vector< Node_Skeleton >::const_iterator it2 = it1->second.begin();
                it2 != it1->second.end(); ++it2)
              total.nodes[it1->first].push_back(*it2);
          }
        }
      }
      total_rman.swap_set("_", total);

      Recurse_Statement(2, { { "type", "node-way" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
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

      Recurse_Statement(2, { { "type", "relation-node" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "7"))
  {
    try
    {
      // Recurse node-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "node", 2 + global_node_offset);

      Recurse_Statement(2, { { "type", "node-relation" } }, global_settings).execute(rman);
      Print_Statement(3, {}, global_settings).execute(rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 8);

      Recurse_Statement(2, { { "type", "relation-way" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "9"))
  {
    try
    {
      // Recurse way-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      perform_id_query(rman, "way", 1);

      Recurse_Statement(2, { { "type", "way-relation" } }, global_settings).execute(rman);
      Print_Statement(3, {}, global_settings).execute(rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "10"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 10);

      Recurse_Statement(2, { { "type", "relation-relation" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "11"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 2);

      Recurse_Statement(2, { { "type", "relation-backwards" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "12"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 1);

      Recurse_Statement(2, { { "type", "down" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "13"))
  {
    try
    {
      // Recurse down
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 6);

      Recurse_Statement(2, { { "type", "down" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "14"))
  {
    try
    {
      // Recurse down-rel with a recursive relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 9);

      Recurse_Statement(2,  { { "type", "down-rel" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "15"))
  {
    try
    {
      // Recurse down-rel with a mixed relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 10);

      Recurse_Statement(2, { { "type", "down-rel" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "16"))
  {
    try
    {
      // Recurse up
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "node", pattern_size + 2 + global_node_offset);

      Recurse_Statement(2, { { "type", "up" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "17"))
  {
    try
    {
      // Recurse up
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);

      Recurse_Statement(2, { { "type", "up" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "18"))
  {
    try
    {
      // Recurse up-rel with a node
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "node", 2 + global_node_offset);

      Recurse_Statement(2, { { "type", "up-rel" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "19"))
  {
    try
    {
      // Recurse up-rel with a way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 2);

      Recurse_Statement(2, { { "type", "up-rel" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
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
      Recurse_Statement(2, { { "type", "up-rel" } }, global_settings).execute(total_rman);
      }
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "21"))
  {
    try
    {
      // Recurse down with a way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);

      Recurse_Statement(2, { { "type", "down" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "22"))
  {
    try
    {
      // Recurse down-rel with a way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);

      Recurse_Statement(2, { { "type", "down-rel" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "23"))
  {
    try
    {
      // Collect the nodes of some relations
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 1);

      Recurse_Statement(2, { { "type", "relation-node" }, { "role", "one" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
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

      Recurse_Statement(2, { { "type", "node-relation" }, { "role", "zero" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "25"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 7);

      Recurse_Statement(2, { { "type", "relation-way" }, { "role", "two" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "26"))
  {
    try
    {
      // Recurse way-relation
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "way", 1);

      Recurse_Statement(2, { { "type", "way-relation" }, { "role", "two" } }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }
  if ((test_to_execute == "") || (test_to_execute == "27"))
  {
    try
    {
      // Recurse relation-way
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager total_rman(transaction, &global_settings);
      perform_id_query(total_rman, "relation", 9);

      Recurse_Statement(2, { {"type", "relation-relation"}, {"role", "one"} }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
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

      Recurse_Statement(2, { {"type", "relation-backwards"}, {"role", "one"} }, global_settings).execute(total_rman);
      Print_Statement(3, {}, global_settings).execute(total_rman);
    }
    catch (File_Error e) { std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n'; }
  }

  std::cout<<"</osm>\n";
  return 0;
}
