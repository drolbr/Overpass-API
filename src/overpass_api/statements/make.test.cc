/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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


#include "make.h"
#include "print.h"


void attribute_test(Parsed_Query& global_settings, Transaction& transaction,
    std::string into, std::string type)
{
  Resource_Manager rman(transaction, &global_settings);
        
  std::map< std::string, std::string > attributes;
  attributes["into"] = into;
  attributes["type"] = type;
  Make_Statement stmt(0, attributes, global_settings);
  stmt.execute(rman);
  
  if (into == "_")
  {
    const char* attributes[] = { 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
  else
  {
    const char* attributes[] = { "from", "target", 0 };
    Print_Statement stmt(0, convert_c_pairs(attributes), global_settings);
    stmt.execute(rman);
  }
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
  //uint64 global_node_offset = atoll(args[4]);

  Nonsynced_Transaction transaction(false, false, args[3], "");
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
  
  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
    attribute_test(global_settings, transaction, "_", "one");
  if ((test_to_execute == "") || (test_to_execute == "2"))
    attribute_test(global_settings, transaction, "_", "two");
  if ((test_to_execute == "") || (test_to_execute == "3"))
    attribute_test(global_settings, transaction, "target", "into_target");

  std::cout<<"</osm>\n";
  return 0;
}
