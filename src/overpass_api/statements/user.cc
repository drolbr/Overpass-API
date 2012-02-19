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

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "meta_collector.h"
#include "user.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

class User_Constraint : public Query_Constraint
{
  public:
    User_Constraint(User_Statement& user_) : user(&user_) {}
    bool get_ranges(Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    bool get_ranges(Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~User_Constraint() {}
    
  private:
    User_Statement* user;
};

template< typename TIndex, typename TObject >
void user_filter_map
    (map< TIndex, vector< TObject > >& modify,
     Resource_Manager& rman, uint32 user_id, File_Properties* file_properties)
{
  if (modify.empty())
    return;
  Meta_Collector< TIndex, TObject > meta_collector
      (modify, *rman.get_transaction(), file_properties, false);
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      const OSM_Element_Metadata_Skeleton* meta_skel
	  = meta_collector.get(it->first, iit->id);
      if ((meta_skel) && (meta_skel->user_id == user_id))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

void User_Constraint::filter(Resource_Manager& rman, Set& into)
{
  uint32 user_id = user->get_id(*rman.get_transaction());
  user_filter_map(into.nodes, rman, user_id, meta_settings().NODES_META);
  user_filter_map(into.ways, rman, user_id, meta_settings().WAYS_META);
  user_filter_map(into.relations, rman, user_id, meta_settings().RELATIONS_META);
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< User_Statement > User_Statement::statement_maker("user");

User_Statement::User_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["uid"] = "";
  attributes["name"] = "";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["into"];
  user_name = attributes["name"];
  user_id = atoll(attributes["uid"].c_str());
  if (((user_id == 0) && (user_name == "")) ||
      ((user_id != 0) && (user_name != "")))
  {
    ostringstream temp;
    temp<<"Exactly one of the two attributes \"name\" and \"uid\" must be set.";
    add_static_error(temp.str());
  }
  result_type = attributes["type"];
}

User_Statement::~User_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

uint32 get_user_id(const string& user_name, Transaction& transaction)
{
  Block_Backend< Uint32_Index, User_Data > user_db
      (transaction.data_index(meta_settings().USER_DATA));
  for (Block_Backend< Uint32_Index, User_Data >::Flat_Iterator
      user_it = user_db.flat_begin(); !(user_it == user_db.flat_end()); ++user_it)
  {
    if (user_it.object().name == user_name)
      return user_it.object().id;
  }
  return 0;
}

uint32 User_Statement::get_id(Transaction& transaction)
{
  if (user_name != "")
    user_id = get_user_id(user_name, transaction);  
  
  return user_id;
}

void calc_ranges
  (set< pair< Uint32_Index, Uint32_Index > >& node_req,
   set< pair< Uint31_Index, Uint31_Index > >& other_req,
   uint32 user_id, Transaction& transaction)
{
  set< Uint32_Index > req;
  req.insert(user_id);
  
  Block_Backend< Uint32_Index, Uint31_Index > user_db
      (transaction.data_index(meta_settings().USER_INDICES));
  for (Block_Backend< Uint32_Index, Uint31_Index >::Discrete_Iterator
      user_it = user_db.discrete_begin(req.begin(), req.end());
      !(user_it == user_db.discrete_end()); ++user_it)
  {
    if ((user_it.object().val() & 0x80000000) == 0)
    {
      node_req.insert(make_pair(Uint32_Index(user_it.object().val()),
			        Uint32_Index(user_it.object().val() + 0x100)));
      other_req.insert(make_pair(Uint31_Index(user_it.object().val()),
			         Uint31_Index(user_it.object().val() + 0x100)));
    }
    else if ((user_it.object().val() & 0xff) == 0)
      other_req.insert(make_pair(Uint31_Index(user_it.object().val()),
			         Uint31_Index(user_it.object().val() + 0x100)));
    else      
      other_req.insert(make_pair(Uint31_Index(user_it.object().val()),
			         Uint31_Index(user_it.object().val() + 1)));
  }
}

bool User_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  set< pair< Uint31_Index, Uint31_Index > > nonnodes;
  calc_ranges(ranges, nonnodes, user->get_id(*rman.get_transaction()), *rman.get_transaction());
  return true;
}

bool User_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  set< pair< Uint32_Index, Uint32_Index > > nodes;
  calc_ranges(nodes, ranges, user->get_id(*rman.get_transaction()), *rman.get_transaction());
  return true;
}

void User_Statement::calc_ranges
    (set< pair< Uint32_Index, Uint32_Index > >& node_req,
     set< pair< Uint31_Index, Uint31_Index > >& other_req,
     Transaction& transaction)
{
  if (user_name != "")
    user_id = get_user_id(user_name, transaction);  

  ::calc_ranges(node_req, other_req, user_id, transaction);
}

void User_Statement::forecast()
{
}

void User_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();
  
  map< Uint32_Index, vector< Node_Skeleton > >& nodes
      (rman.sets()[output].nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways
      (rman.sets()[output].ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations
      (rman.sets()[output].relations);
  map< Uint31_Index, vector< Area_Skeleton > >& areas
      (rman.sets()[output].areas);

  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();

  set< pair< Uint32_Index, Uint32_Index > > node_req;
  set< pair< Uint31_Index, Uint31_Index > > other_req;
  calc_ranges(node_req, other_req, *rman.get_transaction());
  
  stopwatch.stop(Stopwatch::NO_DISK);
  uint nodes_count = 0;
  
  if ((result_type == "") || (result_type == "node"))
  {
    Meta_Collector< Uint32_Index, Node_Skeleton > meta_collector
        (node_req, *rman.get_transaction(), meta_settings().NODES_META);
    Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
        (rman.get_transaction()->data_index(osm_base_settings().NODES));
    for (Block_Backend< Uint32_Index, Node_Skeleton >::Range_Iterator
        it(nodes_db.range_begin
        (Default_Range_Iterator< Uint32_Index >(node_req.begin()),
	 Default_Range_Iterator< Uint32_Index >(node_req.end())));
        !(it == nodes_db.range_end()); ++it)
    {
      if (++nodes_count >= 64*1024)
      {
        nodes_count = 0;
        rman.health_check(*this);
      }
    
      const OSM_Element_Metadata_Skeleton* meta_skel
          = meta_collector.get(it.index(), it.object().id);
      if ((meta_skel) && (meta_skel->user_id == user_id))
        nodes[it.index()].push_back(it.object());
    }
    
    stopwatch.add(Stopwatch::NODES, nodes_db.read_count());
    stopwatch.stop(Stopwatch::NODES);
  }
  
  uint ways_count = 0;
  
  if ((result_type == "") || (result_type == "way"))
  {
    Meta_Collector< Uint31_Index, Way_Skeleton > meta_collector
        (other_req, *rman.get_transaction(), meta_settings().WAYS_META);
    Block_Backend< Uint31_Index, Way_Skeleton > ways_db
        (rman.get_transaction()->data_index(osm_base_settings().WAYS));
    for (Block_Backend< Uint31_Index, Way_Skeleton >::Range_Iterator
        it(ways_db.range_begin
        (Default_Range_Iterator< Uint31_Index >(other_req.begin()),
	 Default_Range_Iterator< Uint31_Index >(other_req.end())));
        !(it == ways_db.range_end()); ++it)
    {
      if (++ways_count >= 64*1024)
      {
        ways_count = 0;
        rman.health_check(*this);
      }
    
      const OSM_Element_Metadata_Skeleton* meta_skel
          = meta_collector.get(it.index(), it.object().id);
      if ((meta_skel) && (meta_skel->user_id == user_id))
        ways[it.index()].push_back(it.object());
    }
    
    stopwatch.add(Stopwatch::WAYS, ways_db.read_count());
    stopwatch.stop(Stopwatch::WAYS);
  }
  
  uint relations_count = 0;
  
  if ((result_type == "") || (result_type == "relation"))
  {
    Meta_Collector< Uint31_Index, Relation_Skeleton > meta_collector
        (other_req, *rman.get_transaction(), meta_settings().RELATIONS_META);
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
        (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Range_Iterator
        it(relations_db.range_begin
        (Default_Range_Iterator< Uint31_Index >(other_req.begin()),
	 Default_Range_Iterator< Uint31_Index >(other_req.end())));
        !(it == relations_db.range_end()); ++it)
    {
      if (++relations_count >= 64*1024)
      {
        relations_count = 0;
        rman.health_check(*this);
      }
    
      const OSM_Element_Metadata_Skeleton* meta_skel
          = meta_collector.get(it.index(), it.object().id);
      if ((meta_skel) && (meta_skel->user_id == user_id))
        relations[it.index()].push_back(it.object());
    }
    
    stopwatch.add(Stopwatch::RELATIONS, relations_db.read_count());
    stopwatch.stop(Stopwatch::RELATIONS);
  }
  
  stopwatch.report(get_name());
  rman.health_check(*this);
}

Query_Constraint* User_Statement::get_query_constraint()
{
  constraints.push_back(new User_Constraint(*this));
  return constraints.back();
}
