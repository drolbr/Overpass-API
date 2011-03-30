#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "print.h"

using namespace std;

typedef unsigned int uint32;

Resource_Manager& perform_id_query(Resource_Manager& rman, string type, uint32 id)
{
  ostringstream buf("");
  buf<<id;
  string id_ = buf.str();
  
  const char* attributes[5];
  attributes[0] = "type";
  attributes[1] = type.c_str();
  attributes[2] = "ref";
  attributes[3] = id_.c_str();
  attributes[4] = 0;

  Id_Query_Statement stmt(1);
  stmt.set_attributes(attributes);  
  stmt.execute(rman);
  
  return rman;
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
  set_basedir(args[3]);
  
  uint32 node_id_upper_limit = 5*pattern_size*pattern_size;
  uint32 way_id_upper_limit = 5*pattern_size*pattern_size;
  uint32 relation_id_upper_limit = 1000;
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      // Print each item alone:
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	// Print nodes:
	Resource_Manager rman;
	perform_id_query(rman, "node", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	// Print ways:
	Resource_Manager rman;
	perform_id_query(rman, "way", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	// Print relations:
	Resource_Manager rman;
	perform_id_query(rman, "relation", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    try
    {
      // Print skeletons:
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	// Print nodes:
	Resource_Manager rman;
	perform_id_query(rman, "node", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { "mode", "skeleton", 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	// Print ways:
	Resource_Manager rman;
	perform_id_query(rman, "way", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { "mode", "skeleton", 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	// Print relations:
	Resource_Manager rman;
	perform_id_query(rman, "relation", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { "mode", "skeleton", 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    try
    {
      // Print ids_only:
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	// Print nodes:
	Resource_Manager rman;
	perform_id_query(rman, "node", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { "mode", "ids_only", 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	// Print ways:
	Resource_Manager rman;
	perform_id_query(rman, "way", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { "mode", "ids_only", 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	// Print relations:
	Resource_Manager rman;
	perform_id_query(rman, "relation", i);
	{
	  Print_Statement stmt(2);
	  const char* attributes[] = { "mode", "ids_only", 0 };
	  stmt.set_attributes(attributes);
	  stmt.execute(rman);
	}
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    try
    {
      // Print all items sorted by id:
      Resource_Manager total_rman;
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	Resource_Manager rman;
	perform_id_query(rman, "node", i);
	if (!rman.sets()["_"].nodes.empty())
	  total_rman.sets()["_"].nodes[rman.sets()["_"].nodes.begin()->first].push_back(rman.sets()["_"].nodes.begin()->second.front());
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	Resource_Manager rman;
	perform_id_query(rman, "way", i);
	if (!rman.sets()["_"].ways.empty())
	  total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	Resource_Manager rman;
	perform_id_query(rman, "relation", i);
	if (!rman.sets()["_"].relations.empty())
	  total_rman.sets()["_"].relations[rman.sets()["_"].relations.begin()->first].push_back(rman.sets()["_"].relations.begin()->second.front());
      }
      {
	Print_Statement stmt(2);
	const char* attributes[] = { "order", "id", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    try
    {
      cout<<"Print all items sorted by quadtile:\n";
      Resource_Manager total_rman;
      for (uint32 i = 10000; i <= node_id_upper_limit; i += 10000)
      {
	Resource_Manager rman;
	perform_id_query(rman, "node", i);
	if (!rman.sets()["_"].nodes.empty())
	  total_rman.sets()["_"].nodes[rman.sets()["_"].nodes.begin()->first].push_back(rman.sets()["_"].nodes.begin()->second.front());
      }
      for (uint32 i = 1000; i <= way_id_upper_limit; i += 1000)
      {
	Resource_Manager rman;
	perform_id_query(rman, "way", i);
	if (!rman.sets()["_"].ways.empty())
	  total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
      }
      for (uint32 i = 4; i <= relation_id_upper_limit; i += 4)
      {
	Resource_Manager rman;
	perform_id_query(rman, "relation", i);
	if (!rman.sets()["_"].relations.empty())
	  total_rman.sets()["_"].relations[rman.sets()["_"].relations.begin()->first].push_back(rman.sets()["_"].relations.begin()->second.front());
      }
      {
	Print_Statement stmt(2);
	const char* attributes[] = { "order", "quadtile", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  cout<<"</osm>\n";
  return 0;
}
