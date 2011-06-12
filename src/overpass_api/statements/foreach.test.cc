#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "foreach.h"
#include "id_query.h"
#include "print.h"

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

Resource_Manager& fill_loop_set
    (Resource_Manager& rman, string set_name, uint pattern_size,
     Transaction& transaction)
{
  uint way_id_offset = (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2);
  
  Resource_Manager partial_rman(transaction);
  perform_id_query(partial_rman, "node", 1);
  if (!partial_rman.sets()["_"].nodes.empty())
    rman.sets()[set_name].nodes[partial_rman.sets()["_"].nodes.begin()->first].push_back(partial_rman.sets()["_"].nodes.begin()->second.front());
  perform_id_query(partial_rman, "node", 2);
  if (!partial_rman.sets()["_"].nodes.empty())
    rman.sets()[set_name].nodes[partial_rman.sets()["_"].nodes.begin()->first].push_back(partial_rman.sets()["_"].nodes.begin()->second.front());
  perform_id_query(partial_rman, "node", 3);
  if (!partial_rman.sets()["_"].nodes.empty())
    rman.sets()[set_name].nodes[partial_rman.sets()["_"].nodes.begin()->first].push_back(partial_rman.sets()["_"].nodes.begin()->second.front());
  perform_id_query(partial_rman, "way", 1);
  if (!partial_rman.sets()["_"].ways.empty())
    rman.sets()[set_name].ways[partial_rman.sets()["_"].ways.begin()->first].push_back(partial_rman.sets()["_"].ways.begin()->second.front());
  perform_id_query(partial_rman, "way", way_id_offset + 1);
  if (!partial_rman.sets()["_"].ways.empty())
    rman.sets()[set_name].ways[partial_rman.sets()["_"].ways.begin()->first].push_back(partial_rman.sets()["_"].ways.begin()->second.front());
  perform_id_query(partial_rman, "way", 2*way_id_offset + 1);
  if (!partial_rman.sets()["_"].ways.empty())
    rman.sets()[set_name].ways[partial_rman.sets()["_"].ways.begin()->first].push_back(partial_rman.sets()["_"].ways.begin()->second.front());
  perform_id_query(partial_rman, "relation", 10);
  if (!partial_rman.sets()["_"].relations.empty())
    rman.sets()[set_name].relations[partial_rman.sets()["_"].relations.begin()->first].push_back(partial_rman.sets()["_"].relations.begin()->second.front());
  perform_id_query(partial_rman, "relation", 21);
  if (!partial_rman.sets()["_"].relations.empty())
    rman.sets()[set_name].relations[partial_rman.sets()["_"].relations.begin()->first].push_back(partial_rman.sets()["_"].relations.begin()->second.front());
  perform_id_query(partial_rman, "relation", 32);
  if (!partial_rman.sets()["_"].relations.empty())
    rman.sets()[set_name].relations[partial_rman.sets()["_"].relations.begin()->first].push_back(partial_rman.sets()["_"].relations.begin()->second.front());
  
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
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "_", pattern_size, transaction);
      {
	Foreach_Statement stmt1(0);
	const char* attributes[] = { 0 };
	stmt1.set_attributes(attributes);
	
	Print_Statement stmt2(0);
	const char* attributes_print[] = { 0 };
	stmt2.set_attributes(attributes_print);
	stmt1.add_statement(&stmt2, "");
	
	stmt1.execute(rman);
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
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "_", pattern_size, transaction);
      {
	Foreach_Statement stmt1(0);
	const char* attributes[] = { 0 };
	stmt1.set_attributes(attributes);
	
	stmt1.execute(rman);
      }
      {
	Print_Statement stmt(0);
	const char* attributes_print[] = { 0 };
	stmt.set_attributes(attributes_print);
	stmt.execute(rman);
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
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "A", pattern_size, transaction);
      {
	Foreach_Statement stmt1(0);
	const char* attributes[] = { "from", "A", "into", "B", 0 };
	stmt1.set_attributes(attributes);
	
	Print_Statement stmt2(0);
	const char* attributes_print[] = { "from", "B", 0 };
	stmt2.set_attributes(attributes_print);
	stmt1.add_statement(&stmt2, "");
	
	stmt1.execute(rman);
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
      Nonsynced_Transaction transaction(false, false, args[3], "");
      Resource_Manager rman(transaction);
      fill_loop_set(rman, "A", pattern_size, transaction);
      {
	Foreach_Statement stmt1(0);
	const char* attributes[] = { "from", "A", "into", "B", 0 };
	stmt1.set_attributes(attributes);
	
	stmt1.execute(rman);
      }
      {
	Print_Statement stmt(0);
	const char* attributes_print[] = { "from", "A", 0 };
	stmt.set_attributes(attributes_print);
	stmt.execute(rman);
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
