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

#include <cmath>
#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "around.h"
#include "bbox_query.h"
#include "id_query.h"
#include "item.h"
#include "query.h"
#include "print.h"
#include "recurse.h"
#include "union.h"

using namespace std;

template < class TStatement >
class SProxy
{
  public:
    SProxy() : stmt_(0) {}
    
    ~SProxy()
    {
      if (stmt_)
	delete stmt_;
    }
    
    SProxy& operator()(const string& k, const string& v)
    {
      attributes[k] = v;
      return *this;
    }
    
    TStatement& stmt()
    {
      if (!stmt_)
        stmt_ = new TStatement(0, attributes);
      
      return *stmt_;
    }
    
  private:
    TStatement* stmt_;
    map< string, string > attributes;
};

void perform_print(Resource_Manager& rman, string from = "_")
{
  SProxy< Print_Statement >()("order", "id")("from", from).stmt().execute(rman);
}

void perform_query(string type, string key, string value, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key)("v", value).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key)("v", value).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "b");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set", "a").stmt(), "");
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["b"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["b"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["b"].relations))
    {
      cout<<"Sets \"_\" and \"b\" differ:\n";
      perform_print(rman, "b");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query
    (string type, string key1, string value1, string key2, string value2,
     string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k", key2)("v", value2).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "b");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set", "a").stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k", key2)("v", value2).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["b"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["b"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["b"].relations))
    {
      cout<<"Sets \"_\" and \"b\" differ:\n";
      perform_print(rman, "b");
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "c");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k", key2)("v", value2).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "d");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set", "a").stmt(), "");
      SProxy< Item_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("set", "c").stmt(), "");
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["d"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["d"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["d"].relations))
    {
      cout<<"Sets \"_\" and \"d\" differ:\n";
      perform_print(rman, "d");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query
    (string type, string key1, string value1, string key2, string value2,
     string key3, string value3, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k", key2)("v", value2).stmt(), "");
      SProxy< Has_Kv_Statement > stmt4;
      stmt1.stmt().add_statement(&stmt4("k", key3)("v", value3).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "b");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k", key2)("v", value2).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "c");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set", "a").stmt(), "");
      SProxy< Item_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("set", "b").stmt(), "");
      SProxy< Has_Kv_Statement > stmt4;
      stmt1.stmt().add_statement(&stmt4("k", key3)("v", value3).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["c"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["c"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["c"].relations))
    {
      cout<<"Sets \"_\" and \"c\" differ:\n";
      perform_print(rman, "c");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_regex_query
    (string type, string key, string value, string key2, string regex2, bool straight2,
     string key3, string regex3, bool straight3, 
     string key4, string value4, bool straight4, 
     string key5, string value5, bool straight5, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      if (key != "")
        stmt1.stmt().add_statement(&stmt2("k", key)("v", value).stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      if (key2 != "")
	stmt1.stmt().add_statement
	    (&stmt3("k", key2)("regv", regex2)("modv", straight2 ? "" : "not").stmt(), "");
      SProxy< Has_Kv_Statement > stmt4;
      if (key3 != "")
	stmt1.stmt().add_statement
	    (&stmt4("k", key3)("regv", regex3)("modv", straight3 ? "" : "not").stmt(), "");
      SProxy< Has_Kv_Statement > stmt5;
      if (key4 != "")
	stmt1.stmt().add_statement
	    (&stmt5("k", key4)("v", value4)("modv", straight4 ? "" : "not").stmt(), "");
      SProxy< Has_Kv_Statement > stmt6;
      if (key5 != "")
	stmt1.stmt().add_statement
	    (&stmt6("k", key5)("v", value5)("modv", straight5 ? "" : "not").stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query_with_around
    (string id_type, string type, string key1, string value1, string db_dir, uint pattern_size,
     bool big_radius = false)
{
  string radius = "200.1";
  if (id_type == "node" && type == "way")
  {
    ostringstream out;
    if (big_radius)
      out<<(120.0/pattern_size)/360.0*40*1000*1000*cos(-10.0/90.0*acos(0))*0.5;
    else
      out<<(120.0/pattern_size)/360.0*40*1000*1000*cos(-10.0/90.0*acos(0))*0.1;
    radius = out.str();
  }
  else if (id_type == "node" && type == "relation")
  {
    ostringstream out;
    if (big_radius)
      out<<(120.0/pattern_size)/360.0*40*1000*1000*cos(-10.0/90.0*acos(0))*0.9;
    else
      out<<(120.0/pattern_size)/360.0*40*1000*1000*cos(-10.0/90.0*acos(0))*1.2;
    radius = out.str();
  }
  else if (id_type == "way")
  {
    ostringstream out;
    if (type == "node")
      out<<(120.0/pattern_size)/360.0*40*1000*1000*1.1;
    else
      out<<1;
    radius = out.str();
  }
  else if (id_type == "relation")
  {
    ostringstream out;
    if (type == "node")
      out<<(120.0/pattern_size)/360.0*40*1000*1000*1.1;
    else
      out<<1;
    radius = out.str();
  }
  
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      ostringstream buf;
      if (id_type == "node" && type == "node")
        buf<<(2*pattern_size*pattern_size + 1);
      else if (id_type == "node" && type == "relation")
      {
	if (big_radius)
	  buf<<(pattern_size*pattern_size + pattern_size*3/2 + 1);
	else
	  buf<<(pattern_size*pattern_size + 2);
      }
      else if (id_type == "way")
      {
	if (type == "node")
	  buf<<(pattern_size*pattern_size/2 + pattern_size/2 - 2 + 1);
	else
	  buf<<(2*(pattern_size*pattern_size/2 + pattern_size/2 - 2) - pattern_size + 1);
      }
      else if (id_type == "relation")
      {
	if (type == "node")
	  buf<<16;
	else
	  buf<<17;
      }
      else if (big_radius)
	buf<<(pattern_size*pattern_size + 2*pattern_size);
      else
	buf<<(pattern_size*pattern_size + pattern_size - 1);
      SProxy< Id_Query_Statement >()("type", id_type)("ref", buf.str()).stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      if (value1 != "")
        stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      else if (key1 != "")
	stmt1.stmt().add_statement(&stmt2("k", key1).stmt(), "");
      SProxy< Around_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("radius", radius).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    if (key1 == "")
      return;
    {
      ostringstream buf;
      if (id_type == "node" && type == "node")
	buf<<(2*pattern_size*pattern_size + 1);
      else if (id_type == "node" && big_radius)
	buf<<(pattern_size*pattern_size + 2*pattern_size);
      else
	buf<<(pattern_size*pattern_size + pattern_size - 1);
      SProxy< Id_Query_Statement >()("type", id_type)("ref", buf.str())("into", "a")
          .stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "b");
      SProxy< Has_Kv_Statement > stmt2;
      if (value1 != "")
        stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      else
	stmt1.stmt().add_statement(&stmt2("k", key1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "c");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set", "b").stmt(), "");
      SProxy< Around_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("radius", radius)("from", "a").stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["c"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["c"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["c"].relations))
    {
      cout<<"Sets \"_\" and \"c\" differ:\n";
      perform_print(rman, "c");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query_with_bbox
    (string type, string key1, string value1,
     string south, string north, string west, string east, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      SProxy< Bbox_Query_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("n", north)("s", south)("e", east)("w", west).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k", key1)("v", value1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type)("into", "b");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set", "a").stmt(), "");
      SProxy< Bbox_Query_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("n", north)("s", south)("e", east)("w", west).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["b"].nodes) ||
      (rman.sets()["_"].ways != rman.sets()["b"].ways) ||
      (rman.sets()["_"].relations != rman.sets()["b"].relations))
    {
      cout<<"Sets \"_\" and \"b\" differ:\n";
      perform_print(rman, "b");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


template< class T >
string to_string(T d)
{
  ostringstream out;
  out<<d;
  return out.str();
}


void perform_multi_query_with_bbox
    (string type, string key1, string value1, string key2, string value2,
     int regex, bool straight2,
     double south, double north, double west, double east, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", type);
      SProxy< Has_Kv_Statement > stmt2;
      if (value1 == "")
        stmt1.stmt().add_statement(&stmt2("k", key1).stmt(), "");
      else
      {
        if (regex & 0x1)
          stmt1.stmt().add_statement(&stmt2("k", key2)("regv", value1).stmt(), "");
        else
          stmt1.stmt().add_statement(&stmt2("k", key2)("v", value1).stmt(), "");
      }
      SProxy< Has_Kv_Statement > stmt4;
      if (regex & 0x2)
        stmt1.stmt().add_statement(&stmt4("k", key2)("regv", value2)
	    ("modv", straight2 ? "" : "not").stmt(), "");
      else
        stmt1.stmt().add_statement(&stmt4("k", key2)("v", value2)
	    ("modv", straight2 ? "" : "not").stmt(), "");
      SProxy< Bbox_Query_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("n", to_string(north))("s", to_string(south))
          ("e", to_string(east))("w", to_string(west)).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


void perform_query_with_recurse
    (string query_type, string recurse_type, string key1, string value1,
     double south, double north, double west, double east, bool double_recurse,
     int pattern_size, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Id_Query_Statement > stmt1;
      if (recurse_type == "way-node")
        stmt1("type", "way")("ref", to_string((pattern_size-1)*pattern_size/2)).stmt().execute(rman);
      else if (recurse_type == "relation-node")
	stmt1("type", "relation")("ref", "11").stmt().execute(rman);
      else if (recurse_type == "relation-way")
	stmt1("type", "relation")("ref", "6").stmt().execute(rman);
      else if (recurse_type == "relation-relation")
	stmt1("type", "relation")("ref", "9").stmt().execute(rman);
      else if (recurse_type == "node-way")
	stmt1("type", "node")("lower", to_string(pattern_size*pattern_size - 4))
	    ("upper", to_string(pattern_size*pattern_size - 1)).stmt().execute(rman);
      else if (recurse_type == "node-relation")
	stmt1("type", "node")("lower", to_string(pattern_size))
	    ("upper", to_string(pattern_size + 2)).stmt().execute(rman);
      else if (recurse_type == "way-relation")
	stmt1("type", "way")("ref", "1").stmt().execute(rman);
      else if (recurse_type == "relation-backwards")
	stmt1("type", "relation")("ref", "1").stmt().execute(rman);
      else if (recurse_type == "down")
	stmt1("type", "relation")("ref", "10").stmt().execute(rman);
      else if (recurse_type == "down-rel")
	stmt1("type", "relation")("ref", "9").stmt().execute(rman);
      else if (recurse_type == "up" || recurse_type == "up-rel")
      {
	SProxy< Id_Query_Statement > stmt2;
	SProxy< Union_Statement > stmt3;
	stmt3.stmt().add_statement(&stmt1("type", "node")("ref", "2").stmt(), "");
	stmt3.stmt().add_statement(&stmt2("type", "way")("ref",
	    to_string(pattern_size*pattern_size/2 - 1)).stmt(), "");
	stmt3.stmt().execute(rman);
      }
    }
    if (double_recurse)
    {
      SProxy< Id_Query_Statement > stmt1;
      if (recurse_type == "way-node")
	stmt1("type", "way")("ref", to_string((pattern_size-1)*pattern_size/2 - 1))("into", "elem2")
          .stmt().execute(rman);
      else if (recurse_type == "relation-node")
	stmt1("type", "relation")("ref", "1")("into", "elem2")
          .stmt().execute(rman);
      else if (recurse_type == "relation-way")
	stmt1("type", "relation")("ref", "6")("into", "elem2")
          .stmt().execute(rman);
      else if (recurse_type == "relation-relation")
	stmt1("type", "relation")("ref", "10")("into", "elem2").stmt().execute(rman);
      else if (recurse_type == "node-way")
	stmt1("type", "node")("into", "elem2")
	    ("lower", to_string(pattern_size*pattern_size - pattern_size - 6))
	    ("upper", to_string(pattern_size*pattern_size - pattern_size - 3)).stmt().execute(rman);
      else if (recurse_type == "node-relation")
	stmt1("type", "node")("into", "elem2")
	    ("ref", to_string(pattern_size*pattern_size)).stmt().execute(rman);
      else if (recurse_type == "way-relation")
	stmt1("type", "way")("ref", "2")("into", "elem2").stmt().execute(rman);
      else if (recurse_type == "relation-backwards")
	stmt1("type", "relation")("ref", "2")("into", "elem2").stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", query_type);
      
      SProxy< Recurse_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("type", recurse_type).stmt(), "");

      SProxy< Has_Kv_Statement > stmt3;
      if (key1 != "")
	stmt1.stmt().add_statement(&stmt3("k", key1)("v", value1).stmt(), "");

      SProxy< Bbox_Query_Statement > stmt4;
      if (south <= 90.0 && north <= 90.0)
      {
	stmt1.stmt().add_statement
	    (&stmt4("n", to_string(north))("s", to_string(south))
	           ("w", to_string(west))("e", to_string(east)).stmt(), "");
      }

      SProxy< Recurse_Statement > stmt5;
      if (double_recurse)
        stmt1.stmt().add_statement(&stmt5("type", recurse_type)("from", "elem2").stmt(), "");

      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query_with_id_query
    (string query_type, string key1, string value1,
     double south, double north, double west, double east, bool double_id_query,
     int pattern_size, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type", query_type);
      
      SProxy< Id_Query_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2
          ("type", query_type)("lower", "1")("upper", "10").stmt(), "");

      SProxy< Has_Kv_Statement > stmt3;
      if (key1 != "")
	stmt1.stmt().add_statement(&stmt3("k", key1)("v", value1).stmt(), "");

      SProxy< Bbox_Query_Statement > stmt4;
      if (south <= 90.0 && north <= 90.0)
      {
	stmt1.stmt().add_statement
	    (&stmt4("n", to_string(north))("s", to_string(south))
	           ("w", to_string(west))("e", to_string(east)).stmt(), "");
      }

      SProxy< Id_Query_Statement > stmt5;
      if (double_id_query)
	stmt1.stmt().add_statement(&stmt5
	    ("type", query_type)("lower", "9")("upper", "12").stmt(), "");

      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
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
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  // Test queries for nodes.
  if ((test_to_execute == "") || (test_to_execute == "1"))
    // Test a key and value which appears only locally
    perform_query("node", "node_key_11", "node_value_2", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    // Test a key and value which appears almost everywhere
    perform_query("node", "node_key_5", "node_value_5", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    // Test a key only which has multiple values
    perform_query("node", "node_key_11", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "4"))
    // Test a key only which has only one value
    perform_query("node", "node_key_15", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "5"))
    // Test a key only which doesn't appear at all
    perform_query("node", "nowhere", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "6"))
    // Test a key intersected with a small key and value pair
    perform_query("node", "node_key_7", "", "node_key_11", "node_value_8", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "7"))
    // Test a key intersected with a large key and value pair
    perform_query("node", "node_key_7", "", "node_key_15", "node_value_15", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "8"))
    // Test a bbox combined with a local key-value pair
    perform_query_with_bbox("node", "node_key_11", "node_value_2",
			    "51.0", "51.2", "7.0", "8.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "9"))
    // Test a bbox combined with a global key-value pair
    perform_query_with_bbox("node", "node_key_5", "node_value_5",
			    "-10.0", "-1.0", "-15.0", "-3.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "10"))
    // Test a bbox combined with a global key-value pair
    perform_query_with_bbox("node", "node_key_7", "",
			    "-10.0", "-1.0", "-15.0", "-3.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "11"))
    // Test three key-values intersected
    perform_query("node", "node_key_5", "node_value_5", "node_key_7", "node_value_0",
		  "node_key_15", "node_value_15", args[3]);
		  
  // Test queries for ways.
  if ((test_to_execute == "") || (test_to_execute == "12"))
    // Test a key and value which appears only locally
    perform_query("way", "way_key_11", "way_value_2", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "13"))
    // Test a key and value which appears almost everywhere
    perform_query("way", "way_key_5", "way_value_5", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "14"))
    // Test a key only which has multiple values
    perform_query("way", "way_key_11", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "15"))
    // Test a key only which has only one value
    perform_query("way", "way_key_15", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "16"))
    // Test a key only which doesn't appear at all
    perform_query("way", "nowhere", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "17"))
    // Test a key intersected with a small key and value pair
    perform_query("way", "way_key_7", "", "way_key_11", "way_value_8", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "18"))
    // Test a key intersected with a large key and value pair
    perform_query("way", "way_key_7", "", "way_key_15", "way_value_15", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "19"))
    // Test three key-values intersected
    perform_query("way", "way_key_5", "way_value_5", "way_key_7", "way_value_0",
		  "way_key_15", "way_value_15", args[3]);

  // Test queries for relations.
  if ((test_to_execute == "") || (test_to_execute == "20"))
    // Test a key and value which appears only locally
    perform_query("relation", "relation_key_11", "relation_value_2", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "21"))
    // Test a key and value which appears almost everywhere
    perform_query("relation", "relation_key_2/4", "relation_value_1", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "22"))
    // Test a key only which has multiple values
    perform_query("relation", "relation_key_2/4", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "23"))
    // Test a key only which has only one value
    perform_query("relation", "relation_key_5", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "24"))
    // Test a key only which doesn't appear at all
    perform_query("relation", "nowhere", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "25"))
    // Test two key-values intersected. This tests also whether
    // relations with index zero appear in the results.
    perform_query("relation", "relation_key_2/4", "relation_value_0",
		  "relation_key_5", "relation_value_5", args[3]);

  if ((test_to_execute == "") || (test_to_execute == "26"))
    // Test an around combined with a local key-value pair
    perform_query_with_around("node", "node", "node_key_11", "", args[3], pattern_size);
  if ((test_to_execute == "") || (test_to_execute == "27"))
    // Test an around combined with a global key-value pair
    perform_query_with_around("node", "node", "node_key_7", "node_value_1", args[3], pattern_size);

  if ((test_to_execute == "") || (test_to_execute == "28"))
    // Test a bbox combined with a global key-value pair, yielding diagonal ways.
    perform_query_with_bbox("way", "way_key_5", "way_value_5",
			    "12.5", "35.0", "-15.0", "45.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "29"))
    // Test a bbox combined with a global key-value pair, yielding horizontal and vertical ways.
    perform_query_with_bbox("way", "way_key_5", "way_value_5",
			    "57.5", "80.0", "75.0", "105.0", args[3]);

  if ((test_to_execute == "") || (test_to_execute == "30"))
    // Test a bbox combined with a global key-value pair, yielding diagonal ways.
    perform_query_with_bbox("relation", "relation_key_2/4", "",
			    "12.5", "35.0", "-15.0", "45.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "31"))
    // Test a bbox combined with a global key-value pair, yielding horizontal and vertical ways.
    perform_query_with_bbox("relation", "relation_key_2/4", "",
			    "57.5", "80.0", "75.0", "105.0", args[3]);

  if ((test_to_execute == "") || (test_to_execute == "32"))
    // Test regular expressions: A simple string
    perform_regex_query("node", "", "",
			"node_key_11", "^node_value_2$", true,
			"", "", true, "", "", true, "", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "33"))
    // Test regular expressions: An extended regular expression
    perform_regex_query("node", "", "",
			"node_key_11", "^node_.?alue_2$", true,
			"", "", true, "", "", true, "", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "34"))
    // Test regular expressions: Two regular expressions
    perform_regex_query("relation", "", "",
			"relation_key_2/4", "^relation_.*_0$", true,
			"relation_key_5", "^relation_.*_5$", true,
			"", "", true, "", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "35"))
    // Test regular expressions: A regular expression and a key-value pair
    perform_regex_query("relation",
			"relation_key_2/4", "relation_value_0",
			"relation_key_5", "^relation_.*_5$", true,
			"", "", true, "", "", true, "", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "36"))
    // Test regular expressions: A regular expression and a key only
    perform_regex_query("way", "way_key_7", "", "way_key_11", "^way_.*_8$", true,
			"", "", true, "", "", true, "", "", true, args[3]);

  if ((test_to_execute == "") || (test_to_execute == "37"))
    // Test an around collecting ways from nodes
    perform_query_with_around("node", "way", "", "", args[3], pattern_size);
  if ((test_to_execute == "") || (test_to_execute == "38"))
    // Test an around collecting ways from nodes
    perform_query_with_around("node", "way", "", "", args[3], pattern_size, true);

  if ((test_to_execute == "") || (test_to_execute == "39"))
    // Test an around collecting relations from nodes based on node membership
    perform_query_with_around("node", "relation", "", "", args[3], pattern_size);
  if ((test_to_execute == "") || (test_to_execute == "40"))
    // Test an around collecting relations from nodes based on way membership
    perform_query_with_around("node", "relation", "", "", args[3], pattern_size, true);

  if ((test_to_execute == "") || (test_to_execute == "41"))
    // Test an around collecting relations from nodes based on node membership
    perform_query_with_around("way", "node", "", "", args[3], pattern_size);
  if ((test_to_execute == "") || (test_to_execute == "42"))
    // Test an around collecting relations from nodes based on way membership
    perform_query_with_around("way", "way", "", "", args[3], pattern_size);
  
  if ((test_to_execute == "") || (test_to_execute == "43"))
    // Test an around collecting relations from nodes based on node membership
    perform_query_with_around("relation", "node", "", "", args[3], pattern_size);
  if ((test_to_execute == "") || (test_to_execute == "44"))
    // Test an around collecting relations from nodes based on way membership
    perform_query_with_around("relation", "way", "", "", args[3], pattern_size);

  if ((test_to_execute == "") || (test_to_execute == "45"))
    // Test a simple has-k-not-v
    perform_regex_query("relation", "", "", "", "", true, "", "", true,
			"relation_key_7", "relation_value_2", false,
			"", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "46"))
    // Test a simple has-k-not-v guarded by another has-kv
    perform_regex_query("node", "node_key", "node_few",
			"", "", true, "", "", true,
			"node_key_7", "node_value_0", false,
			"", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "47"))
    // Test two has-k-not-v
    perform_regex_query("relation", "", "", "", "", true, "", "", true,
			"relation_key_7", "relation_value_0", false,
			"relation_key_7", "relation_value_2", false, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "48"))
    // Test a simple has-k-not-reg-v
    perform_regex_query("relation", "", "",
			"relation_key_7", "^relation_value_2$", false,
			"", "", true, "", "", true, "", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "49"))
    // Test a simple has-k-not-reg-v guarded by another has-kv
    perform_regex_query("node", "node_key", "node_few",
			"node_key_7", "^node_value_0$", false,
			"", "", true, "", "", true, "", "", true, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "50"))
    // Test two has-k-not-reg-v
    perform_regex_query("relation", "", "",
			"relation_key_7", "^relation_value_0$", false,
			"relation_key_7", "^relation_value_2$", false,
			"", "", true, "", "", true, args[3]);

  // Test recurse of type way-node as subquery
  if ((test_to_execute == "") || (test_to_execute == "51"))
    perform_query_with_recurse("node", "way-node", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "52"))
    perform_query_with_recurse("node", "way-node", "node_key_5", "node_value_5",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "53"))
    perform_query_with_recurse("node", "way-node", "", "", 51.5, 52.0, 7.5, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "54"))
    perform_query_with_recurse("node", "way-node", "node_key_5", "node_value_5",
			       51.5, 52.0, 7.5, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "55"))
    perform_query_with_recurse("node", "way-node", "", "", 100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);
  
  // Test recurse of type relation-node as subquery
  if ((test_to_execute == "") || (test_to_execute == "56"))
    perform_query_with_recurse("node", "relation-node", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "57"))
    perform_query_with_recurse("node", "relation-node", "node_key_5", "node_value_5",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "58"))
    perform_query_with_recurse("node", "relation-node", "", "", 51.5, 52.0, 7.5, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "59"))
    perform_query_with_recurse("node", "relation-node", "node_key_5", "node_value_5",
			       51.5, 52.0, 7.5, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "60"))
    perform_query_with_recurse("node", "relation-node", "", "", 100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);
  
  // Test recurse of type relation-way as subquery
  if ((test_to_execute == "") || (test_to_execute == "61"))
    perform_query_with_recurse("way", "relation-way", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "62"))
    perform_query_with_recurse("way", "relation-way", "way_key_2/4", "way_value_1",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "63"))
    perform_query_with_recurse("way", "relation-way", "", "",
			       51.0, 51.0 + 2.0/pattern_size, 7.0, 7.5, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "64"))
    perform_query_with_recurse("way", "relation-way", "way_key_2/4", "way_value_1",
			       51.0, 51.0 + 2.0/pattern_size, 7.0, 7.5, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "65"))
    perform_query_with_recurse("way", "relation-way", "", "", 100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);
  
  // Test recurse of type relation-relation as subquery
  if ((test_to_execute == "") || (test_to_execute == "66"))
    perform_query_with_recurse("relation", "relation-relation", "", "",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "67"))
    perform_query_with_recurse("relation", "relation-relation",
			       "relation_key_2/4", "relation_value_0",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "68"))
    perform_query_with_recurse("relation", "relation-relation", "", "",
			       51.75, 52.0, 7.0, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "69"))
    perform_query_with_recurse("relation", "relation-relation",
			       "relation_key_2/4", "relation_value_0",
			       51.75, 52.0, 7.0, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "70"))
    perform_query_with_recurse("relation", "relation-relation", "", "",
			       100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);

  // Test recurse of type relation-relation as subquery
  if ((test_to_execute == "") || (test_to_execute == "71"))
    perform_query_with_recurse("way", "node-way", "", "",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "72"))
    perform_query_with_recurse("way", "node-way", "way_key_2/4", "way_value_0",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "73"))
    perform_query_with_recurse("way", "node-way", "", "",
			       51.0, 51.5, 8.0 - 3.0/pattern_size, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "74"))
    perform_query_with_recurse("way", "node-way", "way_key_2/4", "way_value_0",
			       51.0, 51.5, 8.0 - 3.0/pattern_size, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "75"))
    perform_query_with_recurse("way", "node-way", "", "",
			       100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);

  // Test recurse of type relation-relation as subquery
  if ((test_to_execute == "") || (test_to_execute == "76"))
    perform_query_with_recurse("relation", "node-relation", "", "",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "77"))
    perform_query_with_recurse("relation", "node-relation", "relation_key_2/4", "relation_value_0",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "78"))
    perform_query_with_recurse("relation", "node-relation", "", "",
			       52.0 - 1.0/pattern_size, 52.0, 8.0 - 1.0/pattern_size, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "79"))
    perform_query_with_recurse("relation", "node-relation", "relation_key_2/4", "relation_value_0",
			       52.0 - 1.0/pattern_size, 52.0, 8.0 - 1.0/pattern_size, 8.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "80"))
    perform_query_with_recurse("relation", "node-relation", "", "",
			       100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);

  // Test recurse of type relation-relation as subquery
  if ((test_to_execute == "") || (test_to_execute == "81"))
    perform_query_with_recurse("relation", "way-relation", "", "",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "82"))
    perform_query_with_recurse("relation", "way-relation", "relation_key_5", "relation_value_5",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "83"))
    perform_query_with_recurse("relation", "way-relation", "", "",
			       51.0, 51.0 + 2.0/pattern_size, 7.0, 7.0 + 1.0/pattern_size, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "84"))
    perform_query_with_recurse("relation", "way-relation", "relation_key_5", "relation_value_5",
			       51.0, 51.0 + 2.0/pattern_size, 7.0, 7.0 + 1.0/pattern_size, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "85"))
    perform_query_with_recurse("relation", "way-relation", "", "",
			       100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);

  // Test recurse of type relation-relation as subquery
  if ((test_to_execute == "") || (test_to_execute == "86"))
    perform_query_with_recurse("relation", "relation-backwards", "", "",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "87"))
    perform_query_with_recurse("relation", "relation-backwards",
			       "relation_key_2/4", "relation_value_0",
			       100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "88"))
    perform_query_with_recurse("relation", "relation-backwards", "", "",
			       51.0, 51.0 + 1.0/pattern_size, 7.0, 7.0 + 1.0/pattern_size, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "89"))
    perform_query_with_recurse("relation", "relation-backwards",
			       "relation_key_2/4", "relation_value_0",
			       51.0, 51.0 + 1.0/pattern_size, 7.0, 7.0 + 1.0/pattern_size, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "90"))
    perform_query_with_recurse("relation", "relation-backwards", "", "",
			       100.0, 100.0, 0.0, 0.0, true,
			       pattern_size, args[3]);
			       
  // Test id-query type node as subquery
  if ((test_to_execute == "") || (test_to_execute == "91"))
    perform_query_with_id_query("node", "", "", 100.0, 100.0, 0.0, 0.0, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "92"))
    perform_query_with_id_query("node", "node_key_5", "node_value_5",
			        100.0, 100.0, 0.0, 0.0, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "93"))
    perform_query_with_id_query("node", "", "",
				51.0, 51.5, 7.0, 7.0 + 5.0/pattern_size, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "94"))
    perform_query_with_id_query("node", "node_key_5", "node_value_5",
				51.0, 51.5, 7.0, 7.0 + 5.0/pattern_size, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "95"))
    perform_query_with_id_query("node", "", "", 100.0, 100.0, 0.0, 0.0, true,
			        pattern_size, args[3]);
			       
  // Test id-query type way as subquery
  if ((test_to_execute == "") || (test_to_execute == "96"))
    perform_query_with_id_query("way", "", "", 100.0, 100.0, 0.0, 0.0, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "97"))
    perform_query_with_id_query("way", "way_key_5", "way_value_5",
			        100.0, 100.0, 0.0, 0.0, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "98"))
    perform_query_with_id_query("way", "", "",
				51.0, 51.5, 7.0, 7.0 + 5.0/pattern_size, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "99"))
    perform_query_with_id_query("way", "way_key_5", "way_value_5",
				51.0, 51.5, 7.0, 7.0 + 5.0/pattern_size, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "100"))
    perform_query_with_id_query("way", "", "", 100.0, 100.0, 0.0, 0.0, true,
			        pattern_size, args[3]);
			       
  // Test id-query type relation as subquery
  if ((test_to_execute == "") || (test_to_execute == "101"))
    perform_query_with_id_query("relation", "", "", 100.0, 100.0, 0.0, 0.0, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "102"))
    perform_query_with_id_query("relation", "relation_key_5", "relation_value_5",
			        100.0, 100.0, 0.0, 0.0, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "103"))
    perform_query_with_id_query("relation", "", "",
				51.0, 51.5, 7.0, 7.0 + 6.0/pattern_size, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "104"))
    perform_query_with_id_query("relation", "relation_key_5", "relation_value_5",
				51.0, 51.5, 7.0, 7.0 + 6.0/pattern_size, false,
			        pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "105"))
    perform_query_with_id_query("relation", "", "", 100.0, 100.0, 0.0, 0.0, true,
			        pattern_size, args[3]);
  
  // Test recurse of type down as subquery
  if ((test_to_execute == "") || (test_to_execute == "106"))
    perform_query_with_recurse("node", "down", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "107"))
    perform_query_with_recurse("way", "down", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "108"))
    perform_query_with_recurse("relation", "down", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  
  // Test recurse of type down-rel as subquery
  if ((test_to_execute == "") || (test_to_execute == "109"))
    perform_query_with_recurse("node", "down-rel", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "110"))
    perform_query_with_recurse("way", "down-rel", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "111"))
    perform_query_with_recurse("relation", "down-rel", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  
  // Test recurse of type up as subquery
  if ((test_to_execute == "") || (test_to_execute == "112"))
    perform_query_with_recurse("node", "up", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "113"))
    perform_query_with_recurse("way", "up", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "114"))
    perform_query_with_recurse("relation", "up", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  
  // Test recurse of type up-rel as subquery
  if ((test_to_execute == "") || (test_to_execute == "115"))
    perform_query_with_recurse("node", "up-rel", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "116"))
    perform_query_with_recurse("way", "up-rel", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "117"))
    perform_query_with_recurse("relation", "up-rel", "", "", 100.0, 100.0, 0.0, 0.0, false,
			       pattern_size, args[3]);
    
  // Test combination of keys-only with several other elements
  if ((test_to_execute == "") || (test_to_execute == "118"))
    perform_multi_query_with_bbox("node", "node_key_5", "", "node_key_7", "", 0, true,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "119"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_7", "", 0, true,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "120"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_5", "", 0, true,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "121"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_5", ".", 2, true,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "122"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_7", "node_value_0", 0, false,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "123"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_7", "node_value_0", 2, false,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "124"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_11", "node_value", 2, false,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "125"))
    perform_multi_query_with_bbox("node", "node_key_7", "", "node_key_7", "value_0", 2, true,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
  if ((test_to_execute == "") || (test_to_execute == "126"))
    perform_multi_query_with_bbox("node", "node_key_7", "node_....._0", "node_key_7", "value_", 3, true,
				  30.0 + 9.9/pattern_size, 30.0 + 10.1/pattern_size, -120.0, -60.0, args[3]);
    
  cout<<"</osm>\n";
  return 0;
}
