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

#include <iomanip>
#include <iostream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "../output_formats/output_xml.h"
#include "around.h"
#include "binary_operators.h"
#include "filter.h"
#include "id_query.h"
#include "print.h"
#include "query.h"
#include "tag_value.h"
#include "testing_tools.h"
#include "union.h"



void perform_around_print(uint pattern_size, std::string radius, uint64 global_node_offset,
			  Transaction& transaction)
{
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  try
  {
    Resource_Manager rman(transaction, &global_settings);
    Id_Query_Statement(0, Attr()("type", "node")("ref", to_string(
        2*pattern_size*pattern_size + 1 + global_node_offset)).kvs(), global_settings).execute(rman);
    Around_Statement(0, Attr()("radius", radius).kvs(), global_settings).execute(rman);
    Print_Statement(0, Attr()("order", "id").kvs(), global_settings).execute(rman);
  }
  catch (File_Error e)
  {
    std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


void perform_coord_print(uint pattern_size, std::string radius, uint64 global_node_offset,
                          Transaction& transaction)
{
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  try
  {
    Resource_Manager rman(transaction, &global_settings);
    {
      std::ostringstream buf;
      buf<<std::setprecision(14)<<(47.9 + 0.1/pattern_size);
      std::string lat = buf.str();
      buf.str("");
      buf<<std::setprecision(14)<<(-0.2 + 0.2/pattern_size);
      std::string lon = buf.str();

      Around_Statement(0, Attr()("radius", radius)("lat", lat)("lon", lon).kvs(), global_settings).execute(rman);
    }
    Print_Statement(0, Attr()("order", "id").kvs(), global_settings).execute(rman);
  }
  catch (File_Error e)
  {
    std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


void perform_polyline_print(uint pattern_size, std::string polyline,
    uint64 global_node_offset, Transaction& transaction)
{
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  try
  {
    std::string radius = to_string(200000./pattern_size);

    Resource_Manager rman(transaction, &global_settings);
    Around_Statement(0, Attr()("radius", radius)("polyline", polyline).kvs(),
                     global_settings).execute(rman);
    Print_Statement(0, Attr()("order", "id").kvs(), global_settings).execute(rman);
  }
  catch (File_Error e)
  {
    std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


void perform_polyline_in_query_print(uint pattern_size,
    std::string radius, std::string type, std::string polyline,
    uint64 global_node_offset, Transaction& transaction)
{
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  Statement_Container stmt_cont(global_settings);
  try
  {
    Resource_Manager rman(transaction, &global_settings);
    Query_Statement query(0, Attr()("type", type).kvs(), global_settings);
    stmt_cont.add_stmt(new Around_Statement(0, Attr()("radius", radius)("polyline", polyline).kvs(),
        global_settings), &query);
    Statement* stmt = stmt_cont.add_stmt(new Filter_Statement(0, Attr().kvs(),
        global_settings), &query);
    stmt = stmt_cont.add_stmt(new Evaluator_Less(0, Attr().kvs(), global_settings), stmt);
    stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), stmt);
    stmt_cont.add_stmt(new Evaluator_Fixed(0, Attr()("v", to_string(pattern_size*pattern_size/2)).kvs(),
        global_settings), stmt);
    query.execute(rman);
    Print_Statement(0, Attr()("order", "id").kvs(), global_settings).execute(rman);
  }
  catch (File_Error e)
  {
    std::cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
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

  Nonsynced_Transaction transaction(false, false, args[3], "");
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
    perform_around_print(pattern_size, "20.01", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    perform_around_print(pattern_size, "200.1", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    perform_around_print(pattern_size, "2001", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    Resource_Manager rman(transaction, &global_settings);
    Id_Query_Statement(0, Attr()("type", "node")("into", "foo")("ref", to_string(
        2*pattern_size*pattern_size + 1 + global_node_offset)).kvs(), global_settings).execute(rman);
    Around_Statement(0, Attr()("radius", "200.1")("from", "foo").kvs(), global_settings).execute(rman);
    Print_Statement(0, Attr()("order", "id").kvs(), global_settings).execute(rman);
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    Resource_Manager rman(transaction, &global_settings);
    Id_Query_Statement(0, Attr()("type", "node")("ref", to_string(
        2*pattern_size*pattern_size + 1 + global_node_offset)).kvs(), global_settings).execute(rman);
    Around_Statement(0, Attr()("radius", "200.1")("into", "foo").kvs(), global_settings).execute(rman);
    Print_Statement(0, Attr()("order", "id")("from", "foo").kvs(), global_settings).execute(rman);
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    Resource_Manager rman(transaction, &global_settings);
    Statement_Container stmt_cont(global_settings);
    {
      Union_Statement stmt1(0, Attr().kvs(), global_settings);
      stmt_cont.add_stmt(new Id_Query_Statement(0, Attr()("type", "node")("ref", to_string(
          2*pattern_size*pattern_size + 1 + global_node_offset)).kvs(), global_settings), &stmt1);
      stmt_cont.add_stmt(new Id_Query_Statement(0, Attr()("type", "node")("ref", to_string(
          3*pattern_size*pattern_size + global_node_offset)).kvs(), global_settings), &stmt1);
      stmt1.execute(rman);
    }
    Around_Statement(0, Attr()("radius", "200.1").kvs(), global_settings).execute(rman);
    Print_Statement(0, Attr()("order", "id").kvs(), global_settings).execute(rman);
  }

  if ((test_to_execute == "") || (test_to_execute == "7"))
    perform_coord_print(pattern_size, "20.01", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "8"))
    perform_coord_print(pattern_size, "200.1", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "9"))
    perform_coord_print(pattern_size, "2001", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "10"))
    perform_polyline_print(pattern_size, "51.1,6.9,50.9,7.1", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "11"))
    perform_polyline_print(pattern_size, "51.,7.,50.9,6.9", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "12"))
    perform_polyline_print(pattern_size, "50.95,6.9,51.,7.,50.9,6.95", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "13"))
    perform_polyline_print(pattern_size, "50.9,6.9,51.,7.", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "14"))
    perform_polyline_print(pattern_size, "51.1,6.9,50.9,7.1,50.8,7.1,50.7,7.05", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "15"))
    perform_polyline_print(pattern_size, "51.1,6.8,51.1,6.9,50.9,7.1,50.8,7.1", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "16"))
    perform_polyline_print(pattern_size, "51.05,6.7,51.1,6.8,51.1,6.9,50.9,7.1", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "17"))
    perform_polyline_in_query_print(pattern_size, to_string(200000./pattern_size), "way", "51.1,6.9,50.9,7.1",
        global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "18"))
    perform_polyline_in_query_print(pattern_size, to_string(200000./pattern_size), "relation", "51.1,6.9,50.9,7.1",
        global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "19"))
    perform_polyline_in_query_print(pattern_size, "0.", "way",
        to_string(51.+1./pattern_size) + "," + to_string(7.+1./pattern_size) + ","
        + to_string(51.+2./pattern_size) + "," + to_string(7.+1./pattern_size),
        global_node_offset, transaction);

  std::cout<<"</osm>\n";
  return 0;
}
