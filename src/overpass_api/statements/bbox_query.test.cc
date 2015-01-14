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
#include "bbox_query.h"
#include "print.h"

using namespace std;

void perform_bbox_print(string south, string north, string west, string east,
			Transaction& transaction)
{
  Parsed_Query global_settings;
  try
  {
    // Select a bbox from the testset that contains one quarter
    // of only one bbox.
    Resource_Manager rman(transaction);
    {
      const char* attributes[] =
          { "s", south.c_str(), "n", north.c_str(),
            "w", west.c_str(), "e", east.c_str(), 0 };
      Bbox_Query_Statement* stmt1 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
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
  Parsed_Query global_settings;
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
    perform_bbox_print("-10.0", "8.0", "-15.0", "9.0", transaction);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    perform_bbox_print("-10.0", "-1.0", "-15.0", "-3.0", transaction);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    perform_bbox_print("-10.0", "-1.0", "93.0", "105.0", transaction);
  if ((test_to_execute == "") || (test_to_execute == "4"))
    perform_bbox_print("-10.0", "-1.0", "93.0", "-3.0", transaction);
  if ((test_to_execute == "") || (test_to_execute == "5"))
    perform_bbox_print("-10.0", "-1.0", "-15.0", "-15.0", transaction);
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    double lon_offset = (105.0-(-15.0))/pattern_size/2;
    ostringstream west_ss;
    west_ss<<fixed<<setprecision(7)<<(-15.0 + lon_offset);
    perform_bbox_print("-10.0", "-1.0", west_ss.str(), west_ss.str(), transaction);
  }
  if ((test_to_execute == "") || (test_to_execute == "7"))
    perform_bbox_print("-10.0", "-10.0", "-15.0", "-3.0", transaction);
  if ((test_to_execute == "") || (test_to_execute == "8"))
  {
    double lat_offset = (80.0-(-10.0))/pattern_size/2;
    ostringstream south_ss;
    south_ss<<fixed<<setprecision(7)<<(-10.0 + lat_offset);
    perform_bbox_print(south_ss.str(), south_ss.str(), "-15.0", "-3.0", transaction);
  }
  
  cout<<"</osm>\n";
  return 0;
}

// Test cases are:
// - bbox with positive lon, with negative lon, both, lon wrap
//   ew line, ns line, point
