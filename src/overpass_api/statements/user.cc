/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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
#include "../data/bbox_filter.h"
#include "../data/collect_members.h"
#include "../data/meta_collector.h"
#include "user.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>


//-----------------------------------------------------------------------------

class User_Constraint : public Query_Constraint
{
  public:
    User_Constraint(User_Statement& user_) : user(&user_) {}

    Query_Filter_Strategy delivers_data(Resource_Manager& rman) override
    { return user->get_criterion() == User_Statement::last ? ids_required : prefer_ranges; }

    bool get_ranges(Resource_Manager& rman, Ranges< Uint31_Index >& ranges) override;
    bool get_ranges(Resource_Manager& rman, Ranges< Uint32_Index >& ranges) override;

    bool get_node_ids(Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids) override;
    bool get_way_ids(Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids) override;
    bool get_relation_ids(Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids) override;

    void filter(const Statement& query, Resource_Manager& rman, Set& into) override;
    virtual ~User_Constraint() {}

  private:
    User_Statement* user;
};


template< typename TIndex, typename TObject >
void user_filter_map
    (std::map< TIndex, std::vector< TObject > >& modify,
     Resource_Manager& rman, const std::set< Uint32_Index >& user_ids, File_Properties* file_properties)
{
  if (modify.empty())
    return;

  Meta_Collector< TIndex, typename TObject::Id_Type > meta_collector
      (modify, *rman.get_transaction(), file_properties);

  for (auto it = modify.begin(); it != modify.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (auto iit = it->second.begin();
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
    (std::map< TIndex, std::vector< TObject > >& modify,
     Resource_Manager& rman, const std::set< Uint32_Index >& user_ids,
     File_Properties* current_file_properties, File_Properties* attic_file_properties)
{
  if (modify.empty())
    return;

  Meta_Collector< TIndex, typename TObject::Id_Type > current_meta_collector
      (modify, *rman.get_transaction(), current_file_properties);
  Meta_Collector< TIndex, typename TObject::Id_Type > attic_meta_collector
      (modify, *rman.get_transaction(), attic_file_properties);

  for (auto it = modify.begin(); it != modify.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (auto iit = it->second.begin();
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


template< typename Index, typename Object >
std::vector< typename Object::Id_Type > touched_ids_by_users(
    Resource_Manager& rman, const Ranges< Index >& ranges, const std::set< Uint32_Index >& user_ids)
{
  std::vector< typename Object::Id_Type > result;
  
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > cur_meta_db(
      rman.get_transaction()->data_index(current_meta_file_properties< Object >()));
    for (auto it = cur_meta_db.range_begin(ranges); !(it == cur_meta_db.range_end()); ++it)
    {
      if (user_ids.find(it.object().user_id) != user_ids.end()
          && (result.empty() || !(result.back() == it.object().ref)))
        result.push_back(it.object().ref);
    }
  }
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > cur_meta_db(
      rman.get_transaction()->data_index(attic_meta_file_properties< Object >()));
    for (auto it = cur_meta_db.range_begin(ranges); !(it == cur_meta_db.range_end()); ++it)
    {
      if (user_ids.find(it.object().user_id) != user_ids.end()
          && (result.empty() || !(result.back() == it.object().ref)))
        result.push_back(it.object().ref);
    }
  }
  
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  
  return result;
}


void calc_ranges_32
  (Ranges< Uint32_Index >& node_req,
   const std::set< Uint32_Index >& user_ids, Transaction& transaction)
{

  Block_Backend< Uint32_Index, Uint31_Index, std::set< Uint32_Index >::const_iterator > user_db
      (transaction.data_index(meta_settings().USER_INDICES));
  for (auto user_it = user_db.discrete_begin(user_ids.begin(), user_ids.end());
      !(user_it == user_db.discrete_end()); ++user_it)
  {
    if ((user_it.object().val() & 0x80000000) == 0)
      node_req.push_back(Uint32_Index(user_it.object().val()), Uint32_Index(user_it.object().val() + 0x100));
  }
  node_req.sort();
}


void calc_ranges_31
  (Ranges< Uint31_Index >& other_req,
   const std::set< Uint32_Index >& user_ids, Transaction& transaction)
{

  Block_Backend< Uint32_Index, Uint31_Index, std::set< Uint32_Index >::const_iterator > user_db
      (transaction.data_index(meta_settings().USER_INDICES));
  for (auto user_it = user_db.discrete_begin(user_ids.begin(), user_ids.end());
      !(user_it == user_db.discrete_end()); ++user_it)
  {
    if ((user_it.object().val() & 0x80000000) == 0)
      other_req.push_back(Uint31_Index(user_it.object().val()), Uint31_Index(user_it.object().val() + 0x100));
    else if ((user_it.object().val() & 0xff) == 0)
      other_req.push_back(Uint31_Index(user_it.object().val()), Uint31_Index(user_it.object().val() + 0x100));
    else
      other_req.push_back(Uint31_Index(user_it.object().val()), Uint31_Index(user_it.object().val() + 1));
  }
  other_req.sort();
}


void calc_ranges_both
  (Ranges< Uint32_Index >& node_req, Ranges< Uint31_Index >& other_req,
   const std::set< Uint32_Index >& user_ids, Transaction& transaction)
{

  Block_Backend< Uint32_Index, Uint31_Index, std::set< Uint32_Index >::const_iterator > user_db
      (transaction.data_index(meta_settings().USER_INDICES));
  for (auto user_it = user_db.discrete_begin(user_ids.begin(), user_ids.end());
      !(user_it == user_db.discrete_end()); ++user_it)
  {
    if ((user_it.object().val() & 0x80000000) == 0)
    {
      node_req.push_back(Uint32_Index(user_it.object().val()), Uint32_Index(user_it.object().val() + 0x100));
      other_req.push_back(Uint31_Index(user_it.object().val()), Uint31_Index(user_it.object().val() + 0x100));
    }
    else if ((user_it.object().val() & 0xff) == 0)
      other_req.push_back(Uint31_Index(user_it.object().val()), Uint31_Index(user_it.object().val() + 0x100));
    else
      other_req.push_back(Uint31_Index(user_it.object().val()), Uint31_Index(user_it.object().val() + 1));
  }
  node_req.sort();
  other_req.sort();
}


void User_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  std::set< Uint32_Index > user_ids = user->get_ids(*rman.get_transaction());

  if (user->get_criterion() == User_Statement::last)
  {
    user_filter_map(into.nodes, rman, user_ids, meta_settings().NODES_META);
    user_filter_map(into.ways, rman, user_ids, meta_settings().WAYS_META);
    user_filter_map(into.relations, rman, user_ids, meta_settings().RELATIONS_META);

    if (!into.attic_nodes.empty())
      user_filter_map_attic(
          into.attic_nodes, rman, user_ids, meta_settings().NODES_META, attic_settings().NODES_META);

    if (!into.attic_ways.empty())
      user_filter_map_attic(
          into.attic_ways, rman, user_ids, meta_settings().WAYS_META, attic_settings().WAYS_META);

    if (!into.attic_relations.empty())
      user_filter_map_attic(
          into.attic_relations, rman, user_ids, meta_settings().RELATIONS_META, attic_settings().RELATIONS_META);
  }
  else
  {
    Ranges< Uint32_Index > node_ranges;
    Ranges< Uint31_Index > other_ranges;
    calc_ranges_both(node_ranges, other_ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());

    if (!into.nodes.empty() || !into.attic_nodes.empty())
      Timeless< Uint32_Index, Node_Skeleton >{ into.nodes, into.attic_nodes }.filter_by_id(
          touched_ids_by_users< Uint32_Index, Node_Skeleton >(
              rman, node_ranges, user->get_ids(*rman.get_transaction())))
          .swap(into.nodes, into.attic_nodes);
    if (!into.ways.empty() || !into.attic_ways.empty())
      Timeless< Uint31_Index, Way_Skeleton >{ into.ways, into.attic_ways }.filter_by_id(
          touched_ids_by_users< Uint31_Index, Way_Skeleton >(
              rman, other_ranges, user->get_ids(*rman.get_transaction())))
          .swap(into.ways, into.attic_ways);
    if (!into.nodes.empty() || !into.attic_nodes.empty())
      Timeless< Uint31_Index, Relation_Skeleton >{ into.relations, into.attic_relations }.filter_by_id(
          touched_ids_by_users< Uint31_Index, Relation_Skeleton >(
              rman, other_ranges, user->get_ids(*rman.get_transaction())))
          .swap(into.relations, into.attic_relations);
  }

  into.areas.clear();
}

//-----------------------------------------------------------------------------


User_Statement::Statement_Maker User_Statement::statement_maker;
User_Statement::Criterion_Maker User_Statement::criterion_maker;


Statement* User_Statement::Criterion_Maker::create_criterion(const Token_Node_Ptr& input_tree,
    const std::string& result_type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  Token_Node_Ptr tree_it = input_tree;
  uint line_nr = tree_it->line_col.first;
  std::vector< std::string > users;

  while (tree_it->token == "," && tree_it->rhs && tree_it->lhs)
  {
    users.push_back(tree_it.rhs()->token);
    tree_it = tree_it.lhs();
  }

  if (tree_it->token == ":" && tree_it->rhs)
    users.push_back(tree_it.rhs()->token);

  std::reverse(users.begin(), users.end());

  std::map< std::string, std::string > attributes;
  attributes["into"] = into;
  attributes["type"] = result_type;

  std::string prefix;
  if (tree_it->lhs)
  {
    if (tree_it.lhs()->token == "uid_touched" || tree_it.lhs()->token == "user_touched")
      attributes["criterion"] = "touched";

    if (tree_it.lhs()->token == "user" || tree_it.lhs()->token == "user_touched")
    {
      for (std::vector< std::string >::iterator it = users.begin(); it != users.end(); ++it)
        *it = decode_json(*it, error_output);
      prefix = "name";
    }
    else
      prefix = "uid";
  }

  std::vector< std::string >::const_iterator it = users.begin();
  if (it != users.end())
    attributes[prefix] = *it;
  for (uint i = 0; it != users.end(); ++it)
  {
    std::ostringstream id;
    id<<prefix<<"_"<<++i;
    attributes[id.str()] = *it;
  }

  return new User_Statement(line_nr, attributes, global_settings);
}


User_Statement::User_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), bbox_limitation(&global_settings.get_global_bbox_limitation()), criterion(last)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";
  attributes["uid"] = "";
  attributes["name"] = "";
  attributes["type"] = "";
  attributes["criterion"] = "last";

  for (std::map<std::string, std::string>::const_iterator it = input_attributes.begin();
      it != input_attributes.end(); ++it)
  {
    if (it->first.find("name_") == 0 || it->first.find("uid_") == 0)
      attributes[it->first] = "";
  }

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);

  std::string user_name = attributes["name"];
  uint32 user_id = atoll(attributes["uid"].c_str());

  if (user_name != "")
    user_names.insert(user_name);

  if (user_id != 0)
    user_ids.insert(user_id);

  for (std::map<std::string, std::string>::iterator it = attributes.begin();
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
    add_static_error("Exactly one of the two attributes \"name\" and \"uid\" must be set.");

  if (attributes["criterion"] == "touched")
    criterion = touched;
  else if (attributes["criterion"] != "last")
    add_static_error("Atttribute \"criterion\" must have the value \"last\" or \"touched\".");

  result_type = attributes["type"];
}


User_Statement::~User_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


std::set< Uint32_Index > get_user_ids(const std::set< std::string >& user_names, Transaction& transaction)
{
  std::set< Uint32_Index > ids;

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


std::set< Uint32_Index > User_Statement::get_ids(Transaction& transaction)
{
  if (!user_names.empty())
    user_ids = get_user_ids(user_names, transaction);

  return user_ids;
}


bool User_Constraint::get_ranges(Resource_Manager& rman, Ranges< Uint32_Index >& ranges)
{
  if (user->get_criterion() == User_Statement::last)
  {
    calc_ranges_32(ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());
    return true;
  }
  return false;
}


bool User_Constraint::get_ranges(Resource_Manager& rman, Ranges< Uint31_Index >& ranges)
{
  if (user->get_criterion() == User_Statement::last)
  {
    calc_ranges_31(ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());
    return true;
  }
  return false;
}


bool User_Constraint::get_node_ids(Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids)
{
  if (user->get_criterion() == User_Statement::touched)
  {
    Ranges< Uint32_Index > node_ranges;
    calc_ranges_32(node_ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());

    touched_ids_by_users< Uint32_Index, Node_Skeleton >(
        rman, node_ranges, user->get_ids(*rman.get_transaction())).swap(ids);
    return true;
  }
  return false;
}


bool User_Constraint::get_way_ids(Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids)
{
  if (user->get_criterion() == User_Statement::touched)
  {
    Ranges< Uint31_Index > other_ranges;
    calc_ranges_31(other_ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());

    touched_ids_by_users< Uint31_Index, Way_Skeleton >(
        rman, other_ranges, user->get_ids(*rman.get_transaction())).swap(ids);
    return true;
  }
  return false;
}


bool User_Constraint::get_relation_ids(Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids)
{
  if (user->get_criterion() == User_Statement::touched)
  {
    Ranges< Uint31_Index > other_ranges;
    calc_ranges_31(other_ranges, user->get_ids(*rman.get_transaction()), *rman.get_transaction());

    touched_ids_by_users< Uint31_Index, Relation_Skeleton >(
        rman, other_ranges, user->get_ids(*rman.get_transaction())).swap(ids);
    return true;
  }
  return false;
}


void User_Statement::execute(Resource_Manager& rman)
{
  Set into;
  User_Constraint constraint(*this);

  if ((result_type == "") || (result_type == "node"))
  {
    Ranges< Uint32_Index > ranges;
    constraint.get_ranges(rman, ranges);
    get_elements_from_db< Uint32_Index, Node_Skeleton >(
        into.nodes, into.attic_nodes, ranges, *this, rman);
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);
  }

  if ((result_type == "") || (result_type == "way"))
  {
    Ranges< Uint31_Index > ranges;
    constraint.get_ranges(rman, ranges);
    get_elements_from_db< Uint31_Index, Way_Skeleton >(
        into.ways, into.attic_ways, ranges, *this, rman);
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.ways, into.attic_ways);
  }

  if ((result_type == "") || (result_type == "relation"))
  {
    Ranges< Uint31_Index > ranges;
    constraint.get_ranges(rman, ranges);
    get_elements_from_db< Uint31_Index, Relation_Skeleton >(
        into.relations, into.attic_relations, ranges, *this, rman);
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.relations, into.attic_relations);
  }

  if (bbox_limitation)
  {
    Bbox_Filter filter(*bbox_limitation);
    filter.filter(into);
    constraint.filter(*this, rman, into);
    filter.filter(*this, rman, into);
  }
  else
    constraint.filter(*this, rman, into);

  transfer_output(rman, into);
  rman.health_check(*this);
}


Query_Constraint* User_Statement::get_query_constraint()
{
  constraints.push_back(new User_Constraint(*this));
  return constraints.back();
}
