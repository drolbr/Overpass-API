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

#include "aggregators.h"
#include "binary_operators.h"
#include "id_query.h"
#include "if.h"
#include "print.h"
#include "tag_value.h"
#include "unary_operators.h"


template< typename T >
std::string to_string_(T val)
{
  std::ostringstream buf;
  buf<<val;
  return buf.str();
}


void execute_base_test_case(const char* id_attributes[], Statement& condition,
    Resource_Manager& rman, Parsed_Query& global_settings)
{
  {
    Id_Query_Statement stmt(0, convert_c_pairs(id_attributes), global_settings);
    stmt.execute(rman);
  }
  {
    const char* attributes[] = { 0 };
    If_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    
    stmt.add_statement(&condition, "");
    
    const char* attributes1[] = { "type", "way", "ref", "2", 0 };
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


int main(int argc, char* args[])
{
  if (argc < 4)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute db_dir node_id_offset\n";
    return 0;
  }
  std::string test_to_execute = args[1];
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
	Nonsynced_Transaction transaction(false, false, args[2], "");
	Resource_Manager rman(transaction, &global_settings);
        
        const char* attributes0[] = { 0 };
        Evaluator_Min_Value stmt0(0, convert_c_pairs(attributes0), global_settings);
          
        const char* attributes01[] = { 0 };
        Evaluator_Id stmt01(0, convert_c_pairs(attributes01), global_settings);
        stmt0.add_statement(&stmt01, "");
          
        const char* id_attributes[] = { "type", "way", "ref", "1", 0 };
        execute_base_test_case(id_attributes, stmt0, rman, global_settings);	
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
    // An empty complete must copy the input set to the output set
    
    try
    {
      {
	Nonsynced_Transaction transaction(false, false, args[2], "");
	Resource_Manager rman(transaction, &global_settings);
        
        const char* attributes0[] = { 0 };
        Evaluator_Not_Equal stmt0(0, convert_c_pairs(attributes0), global_settings);
          
        const char* attributes01[] = { 0 };
        Evaluator_Min_Value stmt01(0, convert_c_pairs(attributes01), global_settings);
        stmt0.add_statement(&stmt01, "");
          
        const char* attributes011[] = { 0 };
        Evaluator_Id stmt011(0, convert_c_pairs(attributes011), global_settings);
        stmt01.add_statement(&stmt011, "");
          
        const char* attributes02[] = { "v", "1", 0 };
        Evaluator_Fixed stmt02(0, convert_c_pairs(attributes02), global_settings);
        stmt0.add_statement(&stmt02, "");
          
        const char* id_attributes[] = { "type", "way", "ref", "1", 0 };
        execute_base_test_case(id_attributes, stmt0, rman, global_settings);	
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
    // An empty complete must copy the input set to the output set
    
    try
    {
      {
	Nonsynced_Transaction transaction(false, false, args[2], "");
	Resource_Manager rman(transaction, &global_settings);
        
        const char* attributes0[] = { "from", "a", 0 };
        Evaluator_Min_Value stmt0(0, convert_c_pairs(attributes0), global_settings);
          
        const char* attributes01[] = { 0 };
        Evaluator_Id stmt01(0, convert_c_pairs(attributes01), global_settings);
        stmt0.add_statement(&stmt01, "");
          
        const char* id_attributes[] = { "type", "way", "ref", "1", "into", "a", 0 };
        execute_base_test_case(id_attributes, stmt0, rman, global_settings);	
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
    // An empty complete must copy the input set to the output set
    
    try
    {
      {
	Nonsynced_Transaction transaction(false, false, args[2], "");
	Resource_Manager rman(transaction, &global_settings);
        
        const char* attributes0[] = { 0 };
        Evaluator_Not_Equal stmt0(0, convert_c_pairs(attributes0), global_settings);
          
        const char* attributes01[] = { "from", "a", 0 };
        Evaluator_Min_Value stmt01(0, convert_c_pairs(attributes01), global_settings);
        stmt0.add_statement(&stmt01, "");
          
        const char* attributes011[] = { 0 };
        Evaluator_Id stmt011(0, convert_c_pairs(attributes011), global_settings);
        stmt01.add_statement(&stmt011, "");
          
        const char* attributes02[] = { "v", "1", 0 };
        Evaluator_Fixed stmt02(0, convert_c_pairs(attributes02), global_settings);
        stmt0.add_statement(&stmt02, "");
          
        {
          const char* attributes[] = { "type", "way", "ref", "3", 0 };
          Id_Query_Statement stmt(0, convert_c_pairs(attributes), global_settings);
          stmt.execute(rman);
        }
        const char* id_attributes[] = { "type", "way", "ref", "1", "into", "a", 0 };
        execute_base_test_case(id_attributes, stmt0, rman, global_settings);	
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
