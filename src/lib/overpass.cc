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

#include "../overpass_api/core/datatypes.h"
#include "../overpass_api/dispatch/resource_manager.h"
#include "../overpass_api/dispatch/scripting_core.h"
#include "../overpass_api/frontend/print_target.h"
#include "../overpass_api/frontend/web_output.h"
#include "../overpass_api/frontend/user_interface.h"
#include "../overpass_api/statements/osm_script.h"
#include "../overpass_api/statements/statement.h"
#include "../template_db/types.h"
#include "overpass.h"

#include <iostream>
#include <map>

using std::cout;
using std::map;


class Print_Target_C : public Print_Target
{
  public:
    Print_Target_C(Transaction& transaction, vector< Node >& nodes_, vector< Way >& ways_)
        : Print_Target(31, transaction), nodes(nodes_), ways(ways_) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) {}
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) {}
			    
    vector< Node >& nodes;
    vector< Way >& ways;
};


struct Id_Coord
{
  Id_Coord(uint32 id_, double lat_, double lon_) : id(id_), lat(lat_), lon(lon_) {}
  
  bool operator<(const Id_Coord& a) const
  {
    return (id < a.id);
  }
  
  uint32 id;
  double lat;
  double lon;
};


Coord make_coord(double lat, double lon)
{
  Coord result;
  result.lat = lat;
  result.lon = lon;
  return result;
}


struct Real_Handle
{
  Real_Handle() {}
  
  vector< const char* > c_tags;

  vector< Node > nodes;
  vector< Node >::const_iterator nodes_it;
  Overpass_C_Node c_node;

  set< Id_Coord > id_coords;
  
  vector< Way > ways;
  vector< Way >::const_iterator ways_it;
  vector< Coord > coords;
  Overpass_C_Way c_way;
};


int& handle_count()
{
  static int handle_count = 0;
  return handle_count;
}


Dispatcher_Stub*& dispatcher_stub()
{
  static Dispatcher_Stub* dispatcher = 0;
  
  return dispatcher;
}


class Dispatcher_Stub_Semaphore
{
public:
  Dispatcher_Stub_Semaphore() : lock(false)
  {
    bool & global_lock = semaphore();
    if (!global_lock)
    {
      global_lock = true;
      lock = true;
    }
  }
  
  ~Dispatcher_Stub_Semaphore()
  {
    if (lock)
      semaphore() = false;
  }
  
  bool has_lock() { return lock; }
  
private:
  static bool& semaphore()
  {
    static bool lock = false;
    return lock;
  }
  
  bool lock;
};


void alloc_overpass_handle(Overpass_C_Handle** handle)
{
  cerr<<'A';
  *handle = reinterpret_cast< Overpass_C_Handle* >(new Real_Handle());
  ++handle_count();

  Dispatcher_Stub_Semaphore semaphore;
  if (dispatcher_stub() == 0)
  {
    if (semaphore.has_lock())
    {
      try
      {
	int area_level = 0;
	dispatcher_stub() = new Dispatcher_Stub
	    ("", 0, "--- liboverpass ---", area_level, 3600, 1024*1024*1024);
      }
      catch(File_Error e)
      {
	ostringstream temp;
	if (e.origin.substr(e.origin.size()-9) == "::timeout")
	  temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin
	      <<". Probably the server is overcrowded.\n";
	else
	  temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
	
        --handle_count();
	delete reinterpret_cast< Real_Handle* >(*handle);
      }
      catch(Resource_Error e)
      {
	ostringstream temp;
	if (e.timed_out)
	  temp<<"Query timed out in \""<<e.stmt_name<<"\" at line "<<e.line_number
	      <<" after "<<e.runtime<<" seconds.";
	else
	  temp<<"Query run out of memory in \""<<e.stmt_name<<"\" at line "
	      <<e.line_number<<" using about "<<e.size/(1024*1024)<<" MB of RAM.";
	
        --handle_count();
	delete reinterpret_cast< Real_Handle* >(*handle);
      }
      catch(Exit_Error e)
      {
        --handle_count();
	delete reinterpret_cast< Real_Handle* >(*handle);
      }
    }
    else
    {
      while (dispatcher_stub() == 0)
	millisleep(100);
    }
  }
  cerr<<'B';
}


void Print_Target_C::print_item(uint32 ll_upper, const Node_Skeleton& skel,
    const vector< pair< string, string > >* tags,
    const OSM_Element_Metadata_Skeleton* meta,
    const map< uint32, string >* users)
{
  nodes.push_back(Node(skel.id, ll_upper, skel.ll_lower));
  nodes.back().tags = *tags;
}


void Print_Target_C::print_item(uint32 ll_upper, const Way_Skeleton& skel,
    const vector< pair< string, string > >* tags,
    const OSM_Element_Metadata_Skeleton* meta,
    const map< uint32, string >* users)
{
  ways.push_back(Way(skel.id, ll_upper, skel.nds));
  ways.back().tags = *tags;
}


void overpass_bbox(Overpass_C_Handle* handle,
		   double south, double west, double north, double east,
		   char* condition)
{
  cerr<<'C';
  Real_Handle& handle_data = *reinterpret_cast< Real_Handle* >(handle);
  handle_data.nodes.clear();
  handle_data.ways.clear();
  handle_data.id_coords.clear();
  
  ostringstream out;
  out<<"(way("<<south<<","<<west<<","<<north<<","<<east<<")"<<condition<<";>;"
      "node("<<south<<","<<west<<","<<north<<","<<east<<")"<<condition<<";);out qt;";
  cout<<out.str()<<'\n';
  
//   Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(0/*&error_output*/);

  Statement::Factory stmt_factory;
  Output_Handle output_handle("---");
  
  static int parser_lock = 0;
  while (parser_lock > 0)
    millisleep(100);
  ++parser_lock;
  get_statement_stack()->clear();
  if (!parse_and_validate(stmt_factory, out.str(), 0/*&error_output*/, parser_execute))
  {
    --parser_lock;
    cout<<"parser error\n";
    cerr<<'E';
    return;
  }
  vector< Statement* > statement_stack;
  statement_stack.swap(*get_statement_stack());
  --parser_lock;
  cerr<<'a';
  Resource_Manager rman(*dispatcher_stub()->resource_manager().get_transaction(), 0, 0); 
  cerr<<'b';
  
  Osm_Script_Statement* osm_script = 0;
  if (!statement_stack.empty())
    osm_script = dynamic_cast< Osm_Script_Statement* >(statement_stack.front());
  cerr<<'c';
  if (osm_script)
  {
    cerr<<'d';
    osm_script->set_factory(&stmt_factory, &output_handle);
    cerr<<'g';
    output_handle.set_print_target(new Print_Target_C
        (*rman.get_transaction(), handle_data.nodes, handle_data.ways));
    cerr<<'h';
  }
    
  cerr<<'e';
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
      it != statement_stack.end(); ++it)
    (*it)->execute(rman);

  cerr<<'f';
  handle_data.nodes_it = handle_data.nodes.begin();
  handle_data.ways_it = handle_data.ways.begin();
  
  if (osm_script)
    osm_script->set_factory(0, 0);
  cerr<<'D';
}


// Returns
// 0 if no elements are left
// 1 if next element is a node
// 2 if next element is a way
int has_next_overpass_handle(Overpass_C_Handle* handle)
{
  Real_Handle& handle_data = *reinterpret_cast< Real_Handle* >(handle);
  if (handle_data.nodes_it != handle_data.nodes.end())
    return 1;
  else if (handle_data.ways_it != handle_data.ways.end())
    return 2;
  else
    return 0;
}


// The library keeps ownership of the pointer
Overpass_C_Node* next_node_overpass_handle(Overpass_C_Handle* handle)
{
  Real_Handle& handle_data = *reinterpret_cast< Real_Handle* >(handle);
  
  handle_data.c_node.id = handle_data.nodes_it->id;
  handle_data.c_node.lat = ::lat(handle_data.nodes_it->ll_upper, handle_data.nodes_it->ll_lower_);
  handle_data.c_node.lon = ::lon(handle_data.nodes_it->ll_upper, handle_data.nodes_it->ll_lower_);
  handle_data.c_node.num_tags = handle_data.nodes_it->tags.size();
  if (handle_data.c_node.num_tags > 0)
  {
    handle_data.c_tags.resize(2*handle_data.c_node.num_tags);
    for (uint i = 0; i < handle_data.nodes_it->tags.size(); ++i)
    {
      handle_data.c_tags[2*i] = handle_data.nodes_it->tags[i].first.c_str();
      handle_data.c_tags[2*i+1] = handle_data.nodes_it->tags[i].second.c_str();
    }
    handle_data.c_node.tags = (char**) &handle_data.c_tags[0];
  }
  
  ++handle_data.nodes_it;
  return &(handle_data.c_node);
}


// The library keeps ownership of the pointer
Overpass_C_Way* next_way_overpass_handle(Overpass_C_Handle* handle)
{
  Real_Handle& handle_data = *reinterpret_cast< Real_Handle* >(handle);
  
  handle_data.c_way.id = handle_data.ways_it->id;
  
  handle_data.coords.clear();
  if (handle_data.id_coords.empty())
  {
    for (vector< Node >::const_iterator it = handle_data.nodes.begin(); it != handle_data.nodes.end(); ++it)
      handle_data.id_coords.insert(Id_Coord(it->id,
	  ::lat(it->ll_upper, it->ll_lower_), ::lon(it->ll_upper, it->ll_lower_)));
  }
  for (vector< uint32 >::const_iterator it = handle_data.ways_it->nds.begin();
       it != handle_data.ways_it->nds.end(); ++it)
  {
    set< Id_Coord >::const_iterator cit = handle_data.id_coords.find(Id_Coord(*it, 0, 0));
    if (cit != handle_data.id_coords.end())
      handle_data.coords.push_back(make_coord(cit->lat, cit->lon));
  }
  handle_data.c_way.num_coords = handle_data.coords.size();
  if (handle_data.c_way.num_coords > 0)
    handle_data.c_way.coords = &handle_data.coords[0];
  
  handle_data.c_way.num_tags = handle_data.ways_it->tags.size();
  if (handle_data.c_way.num_tags > 0)
  {
    handle_data.c_tags.resize(2*handle_data.c_way.num_tags);
    for (uint i = 0; i < handle_data.ways_it->tags.size(); ++i)
    {
      handle_data.c_tags[2*i] = handle_data.ways_it->tags[i].first.c_str();
      handle_data.c_tags[2*i+1] = handle_data.ways_it->tags[i].second.c_str();
    }
    handle_data.c_way.tags = (char**) &handle_data.c_tags[0];
  }
  
  ++handle_data.ways_it;
  return &(handle_data.c_way);
}


void free_overpass_handle(Overpass_C_Handle* handle)
{
  cerr<<'F';
  delete reinterpret_cast< Real_Handle* >(handle);
  if (--handle_count() == 0)
  {
    cerr<<'z';
    delete dispatcher_stub();
    dispatcher_stub() = 0;
  }
  cerr<<'G';
}
