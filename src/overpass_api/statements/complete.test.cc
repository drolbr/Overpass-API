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
#include "bbox_query.h"
#include "complete.h"
#include "id_query.h"
#include "item.h"
#include "print.h"
#include "query.h"
#include "recurse.h"
#include "union.h"


template< typename T >
std::string to_string_(T val)
{
  std::ostringstream buf;
  buf<<val;
  return buf.str();
}


void fill_set_with_all_types(Resource_Manager& rman, uint64 global_node_offset,
    Parsed_Query& global_settings, const std::string& set_name)
{
  const char* attributes[] = { "into", set_name.c_str(), 0 };
  Union_Statement stmt(0, convert_c_pairs(attributes), global_settings);

  std::string buf1 = to_string_(1 + global_node_offset);
  const char* attributes1[] = { "type", "node", "ref", buf1.c_str(), 0 };
  Id_Query_Statement stmt1(0, convert_c_pairs(attributes1), global_settings);
  stmt.add_statement(&stmt1, "");

  std::string buf2 = to_string_(2);
  const char* attributes2[] = { "type", "way", "ref", buf2.c_str(), 0 };
  Id_Query_Statement stmt2(0, convert_c_pairs(attributes2), global_settings);
  stmt.add_statement(&stmt2, "");

  std::string buf3 = to_string_(3);
  const char* attributes3[] = { "type", "relation", "ref", buf3.c_str(), 0 };
  Id_Query_Statement stmt3(0, convert_c_pairs(attributes3), global_settings);
  stmt.add_statement(&stmt3, "");

  stmt.execute(rman);
}


int main(int argc, char* args[])
{
  if (argc < 5)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir node_id_offset\n";
    return 0;
  }
  std::string test_to_execute = args[1];
  uint pattern_size = atoi(args[2]);
  uint64 global_node_offset = atoll(args[4]);
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    // An empty complete must copy the input set to the output set

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "a");

	{
	  const char* attributes[] = { "from", "a", "into", "b", 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { "from", "b", 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
    // An empty complete with equal input and output shall do nothing

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "_");

	{
	  const char* attributes[] = { 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);
	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
    // A complete statement with empty loop result shall copy the input set to the output set

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "a");

	{
	  const char* attributes[] = { "from", "a", "into", "b", 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);

          std::string buf1 = to_string_(global_node_offset);
          const char* attributes1[] = { "type", "node", "ref", buf1.c_str(), "into", "a", 0 };
          Id_Query_Statement stmt1(0, convert_c_pairs(attributes1), global_settings);
          stmt.add_statement(&stmt1, "");

	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { "from", "b", 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
    // A complete statement with empty loop result shall copy the input set to the output set

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "_");

	{
	  const char* attributes[] = { 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);

          std::string buf1 = to_string_(global_node_offset);
          const char* attributes1[] = { "type", "node", "ref", buf1.c_str(), 0 };
          Id_Query_Statement stmt1(0, convert_c_pairs(attributes1), global_settings);
          stmt.add_statement(&stmt1, "");

	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    // A complete statement with loop result equal to input shall copy the input set to the output set

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "_");

	{
	  const char* attributes[] = { "into", "b", 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);

          const char* attributes0[] = { 0 };
          Union_Statement stmt0(0, convert_c_pairs(attributes0), global_settings);
          stmt.add_statement(&stmt0, "");

          std::string buf1 = to_string_(1 + global_node_offset);
          const char* attributes1[] = { "type", "node", "ref", buf1.c_str(), 0 };
          Id_Query_Statement stmt1(0, convert_c_pairs(attributes1), global_settings);
          stmt0.add_statement(&stmt1, "");

          std::string buf2 = to_string_(2);
          const char* attributes2[] = { "type", "way", "ref", buf2.c_str(), 0 };
          Id_Query_Statement stmt2(0, convert_c_pairs(attributes2), global_settings);
          stmt0.add_statement(&stmt2, "");

          std::string buf3 = to_string_(3);
          const char* attributes3[] = { "type", "relation", "ref", buf3.c_str(), 0 };
          Id_Query_Statement stmt3(0, convert_c_pairs(attributes3), global_settings);
          stmt0.add_statement(&stmt3, "");

	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { "from", "b", 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    // A complete statement with equal input and output set name shall keep the original input as output both inside and after the loop

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "_");

	{
	  const char* attributes[] = { 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);

	  const char* attributes0[] = { 0 };
	  Print_Statement stmt0(0, convert_c_pairs(attributes0), global_settings);
          stmt.add_statement(&stmt0, "");

          std::string buf1 = to_string_(4 + global_node_offset);
          const char* attributes1[] = { "type", "node", "ref", buf1.c_str(), 0 };
          Id_Query_Statement stmt1(0, convert_c_pairs(attributes1), global_settings);
          stmt.add_statement(&stmt1, "");

	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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
  if ((test_to_execute == "") || (test_to_execute == "7"))
  {
    // Collect with multiple but limited rounds of growth

    try
    {
      {
	Nonsynced_Transaction transaction(Access_Mode::readonly, false, args[3], "");
	Resource_Manager rman(transaction, &global_settings);

        fill_set_with_all_types(rman, global_node_offset, global_settings, "_");

	{
	  const char* attributes[] = { 0 };
	  Complete_Statement stmt(0, convert_c_pairs(attributes), global_settings);

	  const char* attributes0[] = { 0 };
	  Print_Statement stmt0(0, convert_c_pairs(attributes0), global_settings);
          stmt.add_statement(&stmt0, "");

          const char* attributes1[] = { "type", "way-node", 0 };
          Recurse_Statement stmt1(0, convert_c_pairs(attributes1), global_settings);
          stmt.add_statement(&stmt1, "");

          const char* attributes2[] = { "type", "way", 0 };
          Query_Statement stmt2(0, convert_c_pairs(attributes2), global_settings);
          stmt.add_statement(&stmt2, "");

          const char* attributes21[] = { "type", "node-way", 0 };
          Recurse_Statement stmt21(0, convert_c_pairs(attributes21), global_settings);
          stmt2.add_statement(&stmt21, "");

          std::string north = to_string_(51. + 10./pattern_size);
          std::string east = to_string_(7. + 10./pattern_size);
          const char* attributes22[] = { "s", "51.0", "w", "7.0", "n", north.c_str(), "e", east.c_str(), 0 };
          Bbox_Query_Statement stmt22(0, convert_c_pairs(attributes22), global_settings);
          stmt2.add_statement(&stmt22, "");

	  stmt.execute(rman);
	}
	{
	  const char* attributes[] = { 0 };
	  Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
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

  std::cout<<"</osm>\n";
  return 0;
}
