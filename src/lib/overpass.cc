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
    Print_Target_C(Transaction& transaction, vector< Node >& nodes_)
        : Print_Target(31, transaction), nodes(nodes_) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) {}
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) {}
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) {}
			    
    vector< Node >& nodes;
};


struct Handle_Data
{
  Handle_Data() {}
  
  vector< Node > nodes;
  vector< Node >::const_iterator nodes_it;
  Overpass_C_Node c_node;
  vector< const char* > c_tags;
};


map< Overpass_C_Handle*, Handle_Data >& handle_map()
{
  static map< Overpass_C_Handle*, Handle_Data > static_map;
  return static_map;
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
  *handle = new Overpass_C_Handle();

  (*handle)->num_nodes = 0;
  (*handle)->nodes = 0;
  (*handle)->num_ways = 0;
  (*handle)->ways = 0;
  (*handle)->num_relations = 0;
  (*handle)->relations = 0;

  handle_map()[*handle];
  
  //cerr<<'W';
  Dispatcher_Stub_Semaphore semaphore;
  if (dispatcher_stub() == 0)
  {
    if (semaphore.has_lock())
    {
      //cerr<<'T';
      try
      {
	int area_level = 0;
	//cerr<<'U';
	dispatcher_stub() = new Dispatcher_Stub
	    ("", 0, "--- liboverpass ---", area_level, 3600, 1024*1024*1024);
	//cerr<<'V';
      }
      catch(File_Error e)
      {
	ostringstream temp;
	if (e.origin.substr(e.origin.size()-9) == "::timeout")
	  temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin
	      <<". Probably the server is overcrowded.\n";
	else
	  temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
	//cerr<<temp.str()<<'\n';
	
	handle_map().erase(*handle);
	delete *handle;
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
	//cerr<<temp.str()<<'\n';
	
	handle_map().erase(*handle);
	delete *handle;
      }
      catch(Exit_Error e)
      {
	handle_map().erase(*handle);
	delete *handle;
      }
    }
    else
    {
      //cerr<<'X';
      while (dispatcher_stub() == 0)
      {
	millisleep(100);
        //cerr<<'Y';
      }
    }
    //cerr<<'Z';
  }
  cerr<<'B';
}


void free_overpass_c_tags(uint32 num_tags, char** tags)
{
  if (num_tags > 0)
  {
    for (uint32 i = 0; i < 2*num_tags; ++i)
      free(tags[i]);
  }
  free(tags);
}


void free_overpass_c_node(Overpass_C_Node& node)
{
  free_overpass_c_tags(node.num_tags, node.tags);
}


void free_overpass_c_way(Overpass_C_Way& way)
{
  if (way.num_nodes > 0)
    free(way.nodes);
  free_overpass_c_tags(way.num_tags, way.tags);
}


void free_overpass_c_relation(Overpass_C_Relation& relation)
{
  free_overpass_c_tags(relation.num_tags, relation.tags);
}


void clear_overpass_handle(Overpass_C_Handle* handle)
{
  if (handle)
  {
    for (int i = 0; i < handle->num_nodes; ++i)
      free_overpass_c_node(handle->nodes[i]);
    if (handle->num_nodes > 0)
      free(handle->nodes);
    handle->num_nodes = 0;
    
    for (int i = 0; i < handle->num_ways; ++i)
      free_overpass_c_way(handle->ways[i]);
    if (handle->num_ways > 0)
      free(handle->ways);
    handle->num_ways = 0;

    for (int i = 0; i < handle->num_relations; ++i)
      free_overpass_c_relation(handle->relations[i]);
    if (handle->num_relations > 0)
      free(handle->relations);
    handle->num_relations = 0;
  }
}


char** create_tags(uint32 num_tags, const vector< pair< string, string > >& tags)
{
  char** c_tags = (char**) malloc(2 * num_tags * sizeof(char*));
  for (uint32 i = 0; i < tags.size(); ++i)
  {
    c_tags[2*i] = (char*) malloc((tags[i].first.size() + 1) * sizeof(char));
    strncpy(c_tags[2*i], tags[i].first.c_str(), tags[i].first.size() + 1);
    c_tags[2*i+1] = (char*) malloc((tags[i].second.size() + 1) * sizeof(char));
    strncpy(c_tags[2*i+1], tags[i].second.c_str(), tags[i].second.size() + 1);
  }
  return c_tags;
}


void Print_Target_C::print_item(uint32 ll_upper, const Node_Skeleton& skel,
    const vector< pair< string, string > >* tags,
    const OSM_Element_Metadata_Skeleton* meta,
    const map< uint32, string >* users)
{
  nodes.push_back(Node(skel.id, ll_upper, skel.ll_lower));
  nodes.back().tags = *tags;
}


void overpass_bbox(Overpass_C_Handle* handle,
		   double south, double west, double north, double east)
{
  cerr<<'C';
  handle_map()[handle].nodes.clear();
  
  ostringstream out;
  out<</*"(way("<<south<<","<<west<<","<<north<<","<<east<<");>;"*/
      "node("<<south<<","<<west<<","<<north<<","<<east<<");"/*");"*/"out qt;";
  cout<<out.str()<<'\n';
  
  cerr<<'G';
//   Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(0/*&error_output*/);

  Statement::Factory stmt_factory;
  Output_Handle output_handle("---");
  get_statement_stack()->clear();
  if (!parse_and_validate(stmt_factory, out.str(), 0/*&error_output*/, parser_execute))
  {
    cout<<"parser error\n";
    return;
  }
  vector< Statement* > statement_stack;
  statement_stack.swap(*get_statement_stack());
  
  cerr<<'H';
  Osm_Script_Statement* osm_script = 0;
  cerr<<'K';
  if (!statement_stack.empty())
    osm_script = dynamic_cast< Osm_Script_Statement* >(statement_stack.front());
  cerr<<'L';
  if (osm_script)
  {
    cerr<<'M';
    osm_script->set_factory(&stmt_factory, &output_handle);
    cerr<<'N';
    Resource_Manager* r = &dispatcher_stub()->resource_manager();
    cerr<<r<<'S';
    Transaction* t =  r->get_transaction();
    cerr<<'Q';
    vector< Node >& nodes = handle_map()[handle].nodes;
    cerr<<'R';
    Print_Target_C* p = new Print_Target_C
        (*t/**dispatcher_stub()->resource_manager().get_transaction()*/,
	 nodes/*handle_map()[handle].nodes*/);
    cerr<<'P';
    output_handle.set_print_target(p/*new Print_Target_C
        (*dispatcher_stub()->resource_manager().get_transaction(),
	 handle_map()[handle].nodes)*/);
    cerr<<'O';
  }
    
  cerr<<'I';
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
      it != statement_stack.end(); ++it)
    (*it)->execute(dispatcher_stub()->resource_manager());

//   vector< Overpass_C_Node >& nodes
//       = (dynamic_cast< Print_Target_C* >
//       (&output_handle.get_print_target(0, *handle_map()[handle]->resource_manager().get_transaction())))->nodes;
//   handle->num_nodes = nodes.size();
//   handle->nodes = (Overpass_C_Node*) malloc(nodes.size() * sizeof(Overpass_C_Node));
//   for (uint32 i = 0; i < nodes.size(); ++i)
//     handle->nodes[i] = nodes[i];
    
  cerr<<'J';
  handle_map()[handle].nodes_it = handle_map()[handle].nodes.begin();
  
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
  map< Overpass_C_Handle*, Handle_Data >::const_iterator it = handle_map().find(handle);
  if (it == handle_map().end())
    return 0;
  if (it->second.nodes_it != it->second.nodes.end())
    return 1;
  return 0;
}


// The library keeps ownership of the pointer
Overpass_C_Node* next_node_overpass_handle(Overpass_C_Handle* handle)
{
  map< Overpass_C_Handle*, Handle_Data >::iterator it = handle_map().find(handle);
  if (it == handle_map().end() || it->second.nodes_it == it->second.nodes.end())
    return 0;
  
  it->second.c_node.id = it->second.nodes_it->id;
  it->second.c_node.lat = Node::lat(it->second.nodes_it->ll_upper, it->second.nodes_it->ll_lower_);
  it->second.c_node.lon = Node::lon(it->second.nodes_it->ll_upper, it->second.nodes_it->ll_lower_);
  it->second.c_node.num_tags = it->second.nodes_it->tags.size();
  if (it->second.c_node.num_tags > 0)
  {
    it->second.c_tags.resize(2*it->second.c_node.num_tags);
    for (uint i = 0; i < it->second.nodes_it->tags.size(); ++i)
    {
      it->second.c_tags[2*i] = it->second.nodes_it->tags[i].first.c_str();
      it->second.c_tags[2*i+1] = it->second.nodes_it->tags[i].second.c_str();
    }
    it->second.c_node.tags = (char**) &it->second.c_tags[0];
  }
  
  ++it->second.nodes_it;
  return &(it->second.c_node);
}


void free_overpass_handle(Overpass_C_Handle* handle)
{
//   if (handle)
//     clear_overpass_handle(handle);

  cerr<<'E';
  handle_map().erase(handle);
  delete handle;
  if (handle_map().empty())
    delete dispatcher_stub();
  dispatcher_stub() = 0;
  cerr<<'F';
}
