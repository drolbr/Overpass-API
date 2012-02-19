/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iomanip>
#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "around.h"
#include "id_query.h"
#include "print.h"
#include "union.h"

using namespace std;

void perform_around_print(uint pattern_size, string radius, Transaction& transaction)
{
  try
  {
    Resource_Manager rman(transaction);
    {
      ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1);
      char* buf_str = new char[40];
      strncpy(buf_str, buf.str().c_str(), 40);
      
      const char* attributes[] = { "type", "node", "ref", buf_str, 0 };
      Id_Query_Statement* stmt1 = new Id_Query_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
      
      delete[] buf_str;
    }
    {
      const char* attributes[] = { "radius", radius.c_str(), 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
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
    perform_around_print(pattern_size, "20.01", transaction);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    perform_around_print(pattern_size, "200.1", transaction);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    perform_around_print(pattern_size, "2001", transaction);
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    Resource_Manager rman(transaction);
    {
      ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1);
      char* buf_str = new char[40];
      strncpy(buf_str, buf.str().c_str(), 40);
      
      const char* attributes[] = { "type", "node", "into", "foo", "ref", buf_str, 0 };
      Id_Query_Statement* stmt1 = new Id_Query_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
      
      delete[] buf_str;
    }
    {
      const char* attributes[] = { "radius", "200.1", "from", "foo", 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    Resource_Manager rman(transaction);
    {
      ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1);
      char* buf_str = new char[40];
      strncpy(buf_str, buf.str().c_str(), 40);
      
      const char* attributes[] = { "type", "node", "ref", buf_str, 0 };
      Id_Query_Statement* stmt1 = new Id_Query_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
      
      delete[] buf_str;
    }
    {
      const char* attributes[] = { "radius", "200.1", "into", "foo", 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", "from", "foo", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    Resource_Manager rman(transaction);
    {
      ostringstream buf1, buf2;
      buf1<<(2*pattern_size*pattern_size + 1);
      buf2<<(3*pattern_size*pattern_size);
      char* buf_str_1 = new char[40];
      char* buf_str_2 = new char[40];
      strncpy(buf_str_1, buf1.str().c_str(), 40);
      strncpy(buf_str_2, buf2.str().c_str(), 40);
      
      const char* attributes[] = { 0 };
      Union_Statement* stmt1 = new Union_Statement(0, convert_c_pairs(attributes));
      {
	const char* attributes[] = { "type", "node", "ref", buf_str_1, 0 };
	Id_Query_Statement* stmt2 = new Id_Query_Statement(0, convert_c_pairs(attributes));
	stmt1->add_statement(stmt2, "");
      }
      {
	const char* attributes[] = { "type", "node", "ref", buf_str_2, 0 };
	Id_Query_Statement* stmt2 = new Id_Query_Statement(0, convert_c_pairs(attributes));
	stmt1->add_statement(stmt2, "");
      }
      stmt1->execute(rman);
      
      delete[] buf_str_1;
      delete[] buf_str_2;
    }
    {
      const char* attributes[] = { "radius", "200.1", 0 };
      Around_Statement* stmt1 = new Around_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
    {
      const char* attributes[] = { "order", "id", 0 };
      Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes));
      stmt1->execute(rman);
    }
  }
  
  cout<<"</osm>\n";
  return 0;
}
