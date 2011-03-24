#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "print.h"
#include "recurse.h"

using namespace std;

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
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      // Collect the nodes of some small ways
      Resource_Manager total_rman;
      for (uint32 i = 1; i <= pattern_size/2; ++i)
      {
	Resource_Manager rman;
	perform_id_query(rman, "way", i);
	if (!rman.sets()["_"].ways.empty())
	  total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
      }
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "way-node", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    try
    {
      // Collect the nodes of some large ways
      Resource_Manager total_rman;
      uint way_id_offset = 2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2
          + pattern_size*(pattern_size/2-1);
      perform_id_query(total_rman, "way", way_id_offset + 1);
      {
	Resource_Manager rman;
	way_id_offset = pattern_size*(pattern_size/2-1);
	perform_id_query(rman, "way", way_id_offset + 1);
	if (!rman.sets()["_"].ways.empty())
	  total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
      }
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "way-node", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    try
    {
      // Recurse node-way: try a node without ways
      Resource_Manager rman;
      perform_id_query(rman, "node", 1);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "node-way", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
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
      // Recurse node-way: try a node with a long way
      Resource_Manager rman;
      perform_id_query(rman, "node", pattern_size*pattern_size + pattern_size*3/2 + 2);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "node-way", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
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
      // Recurse node-way: try an entire bbox of nodes (without using bbox)
      Resource_Manager total_rman;
      for (uint i = 0; i < pattern_size/2; ++i)
      {
	for (uint j = 1; j <= pattern_size/2; ++j)
	{
	  Resource_Manager rman;
	  perform_id_query(rman, "node", pattern_size*i + j);
	  if (!rman.sets()["_"].nodes.empty())
	    total_rman.sets()["_"].nodes[rman.sets()["_"].nodes.begin()->first].push_back(rman.sets()["_"].nodes.begin()->second.front());
	}
      }
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "node-way", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    try
    {
      // Collect the nodes of some relations
      Resource_Manager total_rman;
      perform_id_query(total_rman, "relation", 2);
      {
	Resource_Manager rman;
	perform_id_query(rman, "relation", 3);
	if (!rman.sets()["_"].relations.empty())
	  total_rman.sets()["_"].relations[rman.sets()["_"].relations.begin()->first].push_back(rman.sets()["_"].relations.begin()->second.front());
      }
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "relation-node", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
  if ((test_to_execute == "") || (test_to_execute == "7"))
  {
    try
    {
      // Recurse node-relation
      Resource_Manager rman;
      perform_id_query(rman, "node", 2);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "node-relation", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
  {
    try
    {
      // Recurse relation-way
      Resource_Manager total_rman;
      perform_id_query(total_rman, "relation", 8);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "relation-way", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
  if ((test_to_execute == "") || (test_to_execute == "9"))
  {
    try
    {
      // Recurse way-relation
      Resource_Manager rman;
      perform_id_query(rman, "way", 1);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "way-relation", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
	stmt.set_attributes(attributes);
	stmt.execute(rman);
      }
    }
    catch (File_Error e)
    {
      cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "10"))
  {
    try
    {
      // Recurse relation-way
      Resource_Manager total_rman;
      perform_id_query(total_rman, "relation", 10);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "relation-relation", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
  if ((test_to_execute == "") || (test_to_execute == "11"))
  {
    try
    {
      // Recurse relation-way
      Resource_Manager total_rman;
      perform_id_query(total_rman, "relation", 2);
      {
	Recurse_Statement stmt(2);
	const char* attributes[] = { "type", "relation-backwards", 0 };
	stmt.set_attributes(attributes);
	stmt.execute(total_rman);
      }
      {
	Print_Statement stmt(3);
	const char* attributes[] = { 0 };
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
