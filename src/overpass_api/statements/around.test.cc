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

#include <iomanip>
#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "../output_formats/output_xml.h"
#include "around.h"
#include "id_query.h"
#include "print.h"
#include "union.h"



void perform_around_print(uint pattern_size, std::string radius, uint64 global_node_offset,
			  Transaction& transaction)
{
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  try
  {
    Resource_Manager rman(transaction, &global_settings);
    {
      std::ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1 + global_node_offset);
      std::string buf_ = buf.str();

      const char* attributes[] = { "type", "node", "ref", buf_.c_str(), 0 };
      Id_Query_Statement* stmt1 = new Id_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "radius", radius.c_str(), 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
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

      const char* attributes[] = { "radius", radius.c_str(), "lat", lat.c_str(), "lon", lon.c_str(),  0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
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
    {
      std::ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1 + global_node_offset);
      std::string buf_ = buf.str();

      const char* attributes[] = { "type", "node", "into", "foo", "ref", buf_.c_str(), 0 };
      Id_Query_Statement* stmt1 = new Id_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "radius", "200.1", "from", "foo", 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    Resource_Manager rman(transaction, &global_settings);
    {
      std::ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1 + global_node_offset);
      std::string buf_ = buf.str();

      const char* attributes[] = { "type", "node", "ref", buf_.c_str(), 0 };
      Id_Query_Statement* stmt1 = new Id_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "radius", "200.1", "into", "foo", 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", "from", "foo", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    Resource_Manager rman(transaction, &global_settings);
    {
      std::ostringstream buf1, buf2;
      buf1<<(2*pattern_size*pattern_size + 1 + global_node_offset);
      buf2<<(3*pattern_size*pattern_size + global_node_offset);
      std::string buf1_ = buf1.str();
      std::string buf2_ = buf2.str();

      const char* attributes[] = { 0 };
      Union_Statement* stmt1 = new Union_Statement(0, convert_c_pairs(attributes), global_settings);
      {
	const char* attributes[] = { "type", "node", "ref", buf1_.c_str(), 0 };
	Id_Query_Statement* stmt2 = new Id_Query_Statement(0, convert_c_pairs(attributes), global_settings);
	stmt1->add_statement(stmt2, "");
      }
      {
	const char* attributes[] = { "type", "node", "ref", buf2_.c_str(), 0 };
	Id_Query_Statement* stmt2 = new Id_Query_Statement(0, convert_c_pairs(attributes), global_settings);
	stmt1->add_statement(stmt2, "");
      }
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "radius", "200.1", 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
  }

  if ((test_to_execute == "") || (test_to_execute == "7"))
    perform_coord_print(pattern_size, "20.01", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "8"))
    perform_coord_print(pattern_size, "200.1", global_node_offset, transaction);
  if ((test_to_execute == "") || (test_to_execute == "9"))
    perform_coord_print(pattern_size, "2001", global_node_offset, transaction);

  std::cout<<"</osm>\n";
  return 0;
}
