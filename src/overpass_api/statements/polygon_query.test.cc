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

#include <iomanip>
#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "../output_formats/output_xml.h"
#include "polygon_query.h"
#include "print.h"
#include "query.h"


using namespace std;


void perform_polygon_print(string bounds, Transaction& transaction)
{
  try
  {
    // Select a polygon from the testset that contains one quarter
    // of only one polygon.
    Parsed_Query global_settings;
    global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
    Resource_Manager rman(transaction, &global_settings);
    {
      const char* attributes[] = { "bounds", bounds.c_str(), 0 };
      Polygon_Query_Statement* stmt1 = new Polygon_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "mode", "body", "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


void perform_query_polygon_print(string bounds, string type, Transaction& transaction)
{
  try
  {
    // Select a polygon from the testset that contains one quarter
    // of only one polygon.
    Parsed_Query global_settings;
    global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);
    Resource_Manager rman(transaction, &global_settings);
    {
      const char* attributes[] = { "type", type.c_str(), 0 };
      Query_Statement* stmt1 = new Query_Statement(0, convert_c_pairs(attributes), global_settings);
      {
        const char* attributes[] = { "bounds", bounds.c_str(), 0 };
        Polygon_Query_Statement* stmt2 = new Polygon_Query_Statement(0, convert_c_pairs(attributes), global_settings);
        stmt1->add_statement(stmt2, "");
      }
      if (type == "node")
      {
        const char* attributes[] = { "k", "node_key_5", 0 };
        Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0, convert_c_pairs(attributes), global_settings);
        stmt1->add_statement(stmt2, "");
      }
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "mode", "body", "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->execute(rman);
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


int main(int argc, char* args[])
{
  if (argc < 4)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir\n";
    return 0;
  }
  string test_to_execute = args[1];
  uint pattern_size = 0;
  pattern_size = atoi(args[2]);

  Nonsynced_Transaction transaction(false, false, args[3], "");
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
    
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    ostringstream bounds;
    bounds<<fixed<<setprecision(7)<<30.0 + 11*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 14.9*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 11*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 19*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 14.9*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 23*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 19*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 23*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 23*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 19.1*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 23*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 15*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 19.1*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 11*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 15*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 11*(60.0/2.0/pattern_size)<<' ';
    perform_polygon_print(bounds.str(), transaction);
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    ostringstream bounds;
    bounds<<fixed<<setprecision(7)<<50.0 - (20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + (60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<80.0 - (90.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<105.0 - (120.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<80.0<<' '
          <<fixed<<setprecision(7)<<-120<<' ';
    perform_polygon_print(bounds.str(), transaction);
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    ostringstream bounds;
    bounds<<fixed<<setprecision(7)<<30.0 + 11*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 14.9*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 11*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 19*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 14.9*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 23*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 19*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 23*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 23*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 19.1*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 23*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 15*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 19.1*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 11*(60.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<30.0 + 15*(20.0/2.0/pattern_size)<<' '
          <<fixed<<setprecision(7)<<-120 + 11*(60.0/2.0/pattern_size)<<' ';
    perform_query_polygon_print(bounds.str(), "node", transaction);
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    ostringstream bounds;
    bounds<<fixed<<setprecision(7)<<-10.0<<' '
          <<fixed<<setprecision(7)<<45.0<<' '
          <<fixed<<setprecision(7)<<-10.0<<' '
          <<fixed<<setprecision(7)<<75.0<<' '
          <<fixed<<setprecision(7)<<-10.0 + 90.0/pattern_size<<' '
          <<fixed<<setprecision(7)<<75.0 + 120.0/2.0/pattern_size<<' '
          <<fixed<<setprecision(7)<<-10.0 + 90.0/pattern_size<<' '
          <<fixed<<setprecision(7)<<45.0<<' ';
    perform_query_polygon_print(bounds.str(), "way", transaction);
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    ostringstream bounds;
    bounds<<fixed<<setprecision(7)<<-10.0<<' '
          <<fixed<<setprecision(7)<<45.0<<' '
          <<fixed<<setprecision(7)<<-10.0<<' '
          <<fixed<<setprecision(7)<<75.0<<' '
          <<fixed<<setprecision(7)<<-10.0 + 90.0/pattern_size<<' '
          <<fixed<<setprecision(7)<<75.0 + 120.0/2.0/pattern_size<<' '
          <<fixed<<setprecision(7)<<-10.0 + 90.0/pattern_size<<' '
          <<fixed<<setprecision(7)<<45.0<<' ';
    perform_query_polygon_print(bounds.str(), "relation", transaction);
  }
  
  cout<<"</osm>\n";
  return 0;
}
