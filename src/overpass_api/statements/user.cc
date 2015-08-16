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

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/collect_members.h"
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

    bool delivers_data(Resource_Manager& rman) { return false; }
    
    bool get_ranges(Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    bool get_ranges(Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~User_Constraint() {}
    
  private:
    User_Statement* user;
};


template< typename TIndex, typename TObject >
void user_filter_map
    (map< TIndex, vector< TObject > >& modify,
     Resource_Manager& rman, set< Uint32_Index > user_ids, File_Properties* file_properties)
{
  if (modify.empty())
    return;
  
  Meta_Collector< TIndex, typename TObject::Id_Type > meta_collector
      (modify, *rman.get_transaction(), file_properties, false);
      
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      const OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >* meta_skel
	  = meta_collector.get(it->first, iit->id);
      if ((meta_skel) && (user_ids.find(meta_skel->user_id) != user_ids.end()))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename TIndex, typename TObject >
void user_filter_map_attic
    (map< TIndex, vector< TObject > >& modify,
     Resource_Manager& rman, set< Uint32_Index > user_ids,
     File_Properties* current_file_properties, File_Properties* attic_file_properties)
{
  if (modify.empty())
    return;
  
  Meta_Collector< TIndex, typename TObject::Id_Type > current_meta_collector
      (modify, *rman.get_transaction(), current_file_properties, false);
  Meta_Collector< TIndex, typename TObject::Id_Type > attic_meta_collector
      (modify, *rman.get_transaction(), attic_file_properties, false);
      
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      const OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >* meta_skel
	  = current_meta_collector.get(it->first, iit->id);
      if (!meta_skel || !(meta_skel->timestamp < iit->timestamp))
        meta_skel = attic_meta_collector.get(it->first, iit->id, iit->timestamp);
      if ((meta_skel) && (user_ids.find(meta_skel->user_id) != user_ids.end()))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


void User_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  set< Uint32_Index > user_ids = user->get_ids(*rman.get_transaction());
  
  user_filter_map(into.nodes, rman, user_ids, meta_settings().NODES_META);
  user_filter_map(into.ways, rman, user_ids, meta_settings().WAYS_META);
  user_filter_map(into.relations, rman, user_ids, meta_settings().RELATIONS_META);
  
  if (timestamp != NOW)
  {
    user_filter_map_attic(into.attic_nodes, rman, user_ids,
			  meta_settings().NODES_META, attic_settings().NODES_META);
    user_filter_map_attic(into.attic_ways, rman, user_ids,
			  meta_settings().WAYS_META, attic_settings().WAYS_META);
    user_filter_map_attic(into.attic_relations, rman, user_ids,
			  meta_settings().RELATIONS_META, attic_settings().RELATIONS_META);
  }
  
  into.areas.clear();
}

//-----------------------------------------------------------------------------


Generic_Statement_Maker< User_Statement > User_Statement::statement_maker("user");


User_Statement::User_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["uid"] = "";
  attributes["name"] = "";
  attributes["type"] = "";
  
  for (map<string, string>::const_iterator it = input_attributes.begin();
      it != input_attributes.end(); ++it)
  {
    if (it->first.find("name_") == 0 ||
        it->first.find("uid_")  == 0)
      attributes[it->first] = "";
  }

  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);

  string user_name = attributes["name"];
  uint32 user_id = atoll(attributes["uid"].c_str());

  if (user_name != "")
    user_names.insert(user_name);

  if (user_id != 0)
    user_ids.insert(user_id);

  for (std::map<string, string>::iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first.find("name_") == 0)
    {
      if (it->second != "")
        user_names.insert(it->second);
    }
    if (it->first.find("uid_") == 0)
    {
      user_id = atoll(it->second.c_str());
      if (user_id != 0)
        user_ids.insert(user_id);
    }
  }

  if (!(user_ids.empty() ^ user_names.empty()))
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


set< Uint32_Index > get_user_ids(const set< string >& user_names, Transaction& transaction)
{
  set< Uint32_Index > ids;

  Block_Backend< Uint32_Index, User_Data > user_db
      (transaction.data_index(meta_settings().USER_DATA));
  for (Block_Backend< Uint32_Index, User_Data >::Flat_Iterator
      user_it = user_db.flat_begin(); !(user_it == user_db.flat_end()); ++user_it)
  {
    if (user_names.find(user_it.object().name) != user_names.end())
      ids.insert(user_it.object().id);
  }
  return ids;
}


set< Uint32_Index > User_Statement::get_ids(Transaction& transaction)
{
  if (!user_names.empty())
    user_ids = get_user_ids(user_names, transaction);
  
  return user_ids;
}


void calc_ranges
  (set< pair< Uint32_Index, Uint32_Index > >& node_req,
   set< pair< Uint31_Index, Uint31_Index > >& other_req,
   set< Uint32_Index > user_ids, Transaction& transaction)
{
  
  Block_Backend< Uint32_Index, Uint31_Index > user_db
      (transaction.data_index(meta_settings().USER_INDICES));
  for (Block_Backend< Uint32_Index, Uint31_Index >::Discrete_Iterator
      user_it = user_db.discrete_begin(user_ids.begin(), user_ids.end());
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
  calc_ranges(ranges, nonnodes, user->get_ids(*rman.get_transaction()), *rman.get_transaction());
  return true;
}


bool User_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  set< pair< Uint32_Index, Uint32_Index > > nodes;
  calc_ranges(nodes, ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());
  return true;
}


void User_Statement::calc_ranges
    (set< pair< Uint32_Index, Uint32_Index > >& node_req,
     set< pair< Uint31_Index, Uint31_Index > >& other_req,
     Transaction& transaction)
{
  if (!user_names.empty())
    user_ids = get_user_ids(user_names, transaction);

  ::calc_ranges(node_req, other_req, user_ids, transaction);
}


void User_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  if ((result_type == "") || (result_type == "node"))
  {
    User_Constraint constraint(*this);
    set< pair< Uint32_Index, Uint32_Index > > ranges;
    constraint.get_ranges(rman, ranges);
    get_elements_by_id_from_db< Uint32_Index, Node_Skeleton >
        (into.nodes, into.attic_nodes,
         vector< Node::Id_Type >(), false, rman.get_desired_timestamp(), ranges, *this, rman,
         *osm_base_settings().NODES, *attic_settings().NODES);  
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);
    constraint.filter(*this, rman, into, rman.get_desired_timestamp());
  }
  
  if ((result_type == "") || (result_type == "way"))
  {
    User_Constraint constraint(*this);
    set< pair< Uint31_Index, Uint31_Index > > ranges;
    constraint.get_ranges(rman, ranges);
    get_elements_by_id_from_db< Uint31_Index, Way_Skeleton >
        (into.ways, into.attic_ways,
         vector< Way::Id_Type >(), false, rman.get_desired_timestamp(), ranges, *this, rman,
         *osm_base_settings().WAYS, *attic_settings().WAYS);  
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.ways, into.attic_ways);
    constraint.filter(*this, rman, into, rman.get_desired_timestamp());
  }
  
  if ((result_type == "") || (result_type == "relation"))
  {
    User_Constraint constraint(*this);
    set< pair< Uint31_Index, Uint31_Index > > ranges;
    constraint.get_ranges(rman, ranges);
    get_elements_by_id_from_db< Uint31_Index, Relation_Skeleton >
        (into.relations, into.attic_relations,
         vector< Relation::Id_Type >(), false, rman.get_desired_timestamp(), ranges, *this, rman,
         *osm_base_settings().RELATIONS, *attic_settings().RELATIONS);  
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.relations, into.attic_relations);
    constraint.filter(*this, rman, into, rman.get_desired_timestamp());
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}


Query_Constraint* User_Statement::get_query_constraint()
{
  constraints.push_back(new User_Constraint(*this));
  return constraints.back();
}
