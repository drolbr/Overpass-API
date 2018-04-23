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
#include "foreach.h"
#include "id_query.h"
#include "print.h"
#include "testing_tools.h"


Resource_Manager& perform_id_query(Resource_Manager& rman, std::string type, uint64 id)
{
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  std::ostringstream buf("");
  buf<<id;

  Id_Query_Statement(1, Attr()("type", type)("ref", buf.str()).kvs(), global_settings).execute(rman);

  return rman;
}

Resource_Manager& fill_loop_set
    (Resource_Manager& rman, std::string set_name, uint pattern_size, uint64 global_node_offset,
     Transaction& transaction)
{
  uint way_id_offset = (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2);

  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  Resource_Manager partial_rman(transaction, &global_settings);
  perform_id_query(partial_rman, "node", 1 + global_node_offset);
  Set target;
  rman.swap_set(set_name, target);

  const Set* default_ = partial_rman.get_set("_");
  if (default_ && !default_->nodes.empty())
    target.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
  perform_id_query(partial_rman, "node", 2 + global_node_offset);
  if (default_ && !default_->nodes.empty())
    target.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
  perform_id_query(partial_rman, "node", 3 + global_node_offset);
  if (default_ && !default_->nodes.empty())
    target.nodes[default_->nodes.begin()->first].push_back(default_->nodes.begin()->second.front());
/*  perform_id_query(partial_rman, "way", 1);
  if (!default_->ways.empty())
    target.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());*/
  perform_id_query(partial_rman, "way", way_id_offset + 1);
  if (default_ && !default_->ways.empty())
    target.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
  perform_id_query(partial_rman, "way", 2*way_id_offset + 1);
  if (default_ && !default_->ways.empty())
    target.ways[default_->ways.begin()->first].push_back(default_->ways.begin()->second.front());
/*  perform_id_query(partial_rman, "relation", 10);
  if (!default_->relations.empty())
    target.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());*/
  perform_id_query(partial_rman, "relation", 21);
  if (default_ && !default_->relations.empty())
    target.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());
  perform_id_query(partial_rman, "relation", 32);
  if (default_ && !default_->relations.empty())
    target.relations[default_->relations.begin()->first].push_back(default_->relations.begin()->second.front());

  rman.swap_set(set_name, target);
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
  Statement_Container stmt_cont(global_settings);

  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  try
  {
    if ((test_to_execute == "") || (test_to_execute == "1"))
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      fill_loop_set(rman, "_", pattern_size, global_node_offset, transaction);

      Foreach_Statement stmt(0, Attr().kvs(), global_settings);
      stmt_cont.add_stmt(new Print_Statement(0, Attr().kvs(), global_settings), &stmt);
      stmt.execute(rman);
    }
    if ((test_to_execute == "") || (test_to_execute == "2"))
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      fill_loop_set(rman, "_", pattern_size, global_node_offset, transaction);

      Foreach_Statement(0, Attr().kvs(), global_settings).execute(rman);
      Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
    }
    if ((test_to_execute == "") || (test_to_execute == "3"))
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      fill_loop_set(rman, "A", pattern_size, global_node_offset, transaction);

      Foreach_Statement stmt(0, Attr()("from", "A")("into", "B").kvs(), global_settings);
      stmt_cont.add_stmt(new Print_Statement(0, Attr()("from", "B").kvs(), global_settings), &stmt);
      stmt.execute(rman);
    }
    if ((test_to_execute == "") || (test_to_execute == "4"))
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction, &global_settings);
      fill_loop_set(rman, "A", pattern_size, global_node_offset, transaction);

      Foreach_Statement(0, Attr()("from", "A")("into", "B").kvs(), global_settings).execute(rman);
      Print_Statement(0, Attr()("from", "A").kvs(), global_settings).execute(rman);
    }
  }
  catch (File_Error e)
  {
    std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }

  std::cout<<"</osm>\n";
  return 0;
}
