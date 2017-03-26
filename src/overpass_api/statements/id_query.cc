/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "id_query.h"


bool Id_Query_Statement::area_query_exists_ = false;

Generic_Statement_Maker< Id_Query_Statement > Id_Query_Statement::statement_maker("id-query");


template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
                   const std::set<Uint64::Id_Type> & ref_ids,
		   std::map< TIndex, std::vector< TObject > >& elems)
{
  std::set< TIndex > req;
  {
    Random_File< uint64, TIndex > random(rman.get_transaction()->random_index(&prop));
    for (std::set<Uint64::Id_Type>::iterator it_ref = ref_ids.begin(); it_ref != ref_ids.end(); it_ref++)
      req.insert(random.get(*it_ref));
  }    
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if (ref_ids.find(it.object().id.val()) != ref_ids.end())
      elems[it.index()].push_back(it.object());
  }
}


template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
                   const std::set<Uint64::Id_Type> & ref_ids,
		   const std::vector< typename TObject::Id_Type >& ids, bool invert_ids,
		   std::map< TIndex, std::vector< TObject > >& elems)
{
  std::set< TIndex > req;
  {
    Random_File< uint64, TIndex > random(rman.get_transaction()->random_index(&prop));
    for (std::set<Uint64::Id_Type>::iterator it_ref = ref_ids.begin(); it_ref != ref_ids.end(); it_ref++)
    {
      if (binary_search(ids.begin(), ids.end(), *it_ref) ^ invert_ids)
        req.insert(random.get(*it_ref));
    }
  }
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if ((ref_ids.find(it.object().id.val()) != ref_ids.end())
        && (binary_search(ids.begin(), ids.end(), it.object().id) ^ invert_ids))
      elems[it.index()].push_back(it.object());
  }
}


void collect_elems_flat(Resource_Manager& rman,
                   const std::set<Uint64::Id_Type> & ref_ids,
		   const std::vector< Area_Skeleton::Id_Type >& ids, bool invert_ids,
		   std::map< Uint31_Index, std::vector< Area_Skeleton > >& elems)
{
  Block_Backend< Uint31_Index, Area_Skeleton > elems_db
      (rman.get_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it = elems_db.flat_begin(); !(it == elems_db.flat_end()); ++it)
  {
    if ((ref_ids.find(it.object().id.val()) != ref_ids.end())
        && (binary_search(ids.begin(), ids.end(), it.object().id) ^ invert_ids))
      elems[it.index()].push_back(it.object());
  }
}


template< class TIndex, class TObject >
void filter_elems(const std::set<Uint64::Id_Type> & ref_ids,
		  std::map< TIndex, std::vector< TObject > >& elems)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (typename std::vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (ref_ids.find(iit->id.val()) != ref_ids.end())
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


//-----------------------------------------------------------------------------

class Id_Query_Constraint : public Query_Constraint
{
  public:
    Id_Query_Constraint(Id_Query_Statement& stmt_) : stmt(&stmt_) {}

    bool delivers_data(Resource_Manager& rman) { return true; }

    bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);
	
    bool get_node_ids
        (Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids);
    bool get_way_ids
        (Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids);
    bool get_relation_ids
        (Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids);
	
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Id_Query_Constraint() {}

  private:
    Id_Query_Statement* stmt;
};


bool Id_Query_Constraint::get_node_ids(Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids)
{
  ids.clear();
  if (stmt->get_type() == Statement::NODE)
  {
    ids.assign(stmt->get_ref_ids().begin(), stmt->get_ref_ids().end());
  }
  return true;
}


bool Id_Query_Constraint::get_way_ids(Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids)
{
  ids.clear();
  if (stmt->get_type() == Statement::WAY)
  {
    ids.assign(stmt->get_ref_ids().begin(), stmt->get_ref_ids().end());
  }
  return true;
}


bool Id_Query_Constraint::get_relation_ids(Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids)
{
  ids.clear();
  if (stmt->get_type() == Statement::RELATION)
  {
    ids.assign(stmt->get_ref_ids().begin(), stmt->get_ref_ids().end());
  }
  return true;
}


bool Id_Query_Constraint::get_ranges(Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges)
{
  std::vector< Node_Skeleton::Id_Type > ids;

  ids.assign(stmt->get_ref_ids().begin(), stmt->get_ref_ids().end());
  std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(ids, rman);

  ranges.clear();
  for (std::vector< Uint32_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
    ranges.insert(std::make_pair(*it, ++Uint32_Index(*it)));

  return true;
}


bool Id_Query_Constraint::get_ranges(Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  std::vector< Uint31_Index > req;
  if (stmt->get_type() == Statement::WAY)
  {
    std::vector< Way_Skeleton::Id_Type > ids;
    ids.assign(stmt->get_ref_ids().begin(), stmt->get_ref_ids().end());
    get_indexes_< Uint31_Index, Way_Skeleton >(ids, rman).swap(req);
  }
  else
  {
    std::vector< Relation_Skeleton::Id_Type > ids;
    ids.assign(stmt->get_ref_ids().begin(), stmt->get_ref_ids().end());
    get_indexes_< Uint31_Index, Relation_Skeleton >(ids, rman).swap(req);
  }

  ranges.clear();
  for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
    ranges.insert(std::make_pair(*it, inc(*it)));

  return true;
}


void Id_Query_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  if (stmt->get_type() == Statement::NODE)
  {
    filter_elems(stmt->get_ref_ids(), into.nodes);
    filter_elems(stmt->get_ref_ids(), into.attic_nodes);
  }
  else
    into.nodes.clear();

  if (stmt->get_type() == Statement::WAY)
  {
    filter_elems(stmt->get_ref_ids(), into.ways);
    filter_elems(stmt->get_ref_ids(), into.attic_ways);
  }
  else
    into.ways.clear();

  if (stmt->get_type() == Statement::RELATION)
  {
    filter_elems(stmt->get_ref_ids(), into.relations);
    filter_elems(stmt->get_ref_ids(), into.attic_relations);
  }
  else
    into.relations.clear();

  if (stmt->get_type() == Statement::AREA)
    filter_elems(stmt->get_ref_ids(), into.areas);
  else
    into.areas.clear();
}

//-----------------------------------------------------------------------------

Id_Query_Statement::Id_Query_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  attributes["lower"] = "";
  attributes["upper"] = "";
  
  for (std::map<std::string, std::string>::const_iterator it = input_attributes.begin();
      it != input_attributes.end(); ++it)
  {
    if (it->first.find("ref_") == 0)
      attributes[it->first] = "";
  }

  Statement::eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);

  if (attributes["type"] == "node")
    type = Statement::NODE;
  else if (attributes["type"] == "way")
    type = Statement::WAY;
  else if (attributes["type"] == "relation")
    type = Statement::RELATION;
  else if (attributes["type"] == "area")
  {
    type = Statement::AREA;
    area_query_exists_ = true;
  }
  else
  {
    type = 0;
    std::ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"id-query\""
	<<" the only allowed values are \"node\", \"way\", \"relation\", or \"area\".";
    add_static_error(temp.str());
  }

  ref = atoll(attributes["ref"].c_str());

  if (ref.val() > 0)
    ref_ids.insert(ref.val());

  for (std::map<std::string, std::string>::iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first.find("ref_") == 0)
    {
      ref = atoll(it->second.c_str());
      if (ref.val() > 0)
        ref_ids.insert(ref.val());
    }
  }

  lower = atoll(attributes["lower"].c_str());
  upper = atoll(attributes["upper"].c_str());

  if (ref.val() <= 0)
  {
    if (lower.val() == 0 || upper.val() == 0)
    {
      std::ostringstream temp;
      temp<<"For the attribute \"ref\" of the element \"id-query\""
	  <<" the only allowed values are positive integers.";
      add_static_error(temp.str());
    }
    ++upper;
  }
  else
  {
    lower = ref;
    upper = ++ref;
  }

  if (lower.val() > 0 && upper.val() > 0)
  {
    for (Uint64::Id_Type i = lower.val(); i < upper.val(); ++i)
      ref_ids.insert(i);
  }
}


// Nach "data" verschieben ---------------------------


template< typename Id_Type >
struct Id_Pair_First_Comparator
{
  bool operator()(const std::pair< Id_Type, uint64 >& lhs,
                  const std::pair< Id_Type, uint64 >& rhs) const
  {
    return (lhs.first < rhs.first);
  }
};


template< typename Id_Type >
struct Id_Pair_Full_Comparator
{
  bool operator()(const std::pair< Id_Type, uint64 >& lhs,
                  const std::pair< Id_Type, uint64 >& rhs) const
  {
    if (lhs.first < rhs.first)
      return true;
    if (rhs.first < lhs.first)
      return false;
    return (lhs.second < rhs.second);
  }
};


template< typename Index, typename Skeleton >
struct Attic_Skeleton_By_Id
{
  Attic_Skeleton_By_Id(const typename Skeleton::Id_Type& id_, uint64 timestamp_)
      : id(id_), timestamp(timestamp_), index(0u), meta_confirmed(false),
        elem(Skeleton(), 0xffffffffffffffffull) {}

  typename Skeleton::Id_Type id;
  uint64 timestamp;
  Index index;
  bool meta_confirmed;
  Attic< Skeleton > elem;

  bool operator<(const Attic_Skeleton_By_Id& rhs) const
  {
    if (id < rhs.id)
      return true;
    if (rhs.id < id)
      return false;
    return (rhs.timestamp < timestamp);
  }
};


template< typename Index, typename Skeleton >
void get_elements(const std::set<Uint64::Id_Type> & ref_ids, Statement* stmt, Resource_Manager& rman,
    std::map< Index, std::vector< Skeleton > >& current_result,
    std::map< Index, std::vector< Attic< Skeleton > > >& attic_result)
{
  std::vector< typename Skeleton::Id_Type > ids;
  ids.assign(ref_ids.begin(), ref_ids.end());
  std::vector< Index > req = get_indexes_< Index, Skeleton >(ids, rman);

  if (rman.get_desired_timestamp() == NOW)
    collect_items_discrete(stmt, rman, *current_skeleton_file_properties< Skeleton >(), req,
        Id_Predicate< Skeleton >(ids), current_result);
  else
  {
    collect_items_discrete_by_timestamp(stmt, rman, req,
        Id_Predicate< Skeleton >(ids), current_result, attic_result);
    filter_attic_elements(rman, rman.get_desired_timestamp(), current_result, attic_result);
  }
}


void Id_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;

  if (type == NODE)
    get_elements(ref_ids, this, rman, into.nodes, into.attic_nodes);
  else if (type == WAY)
    get_elements(ref_ids, this, rman, into.ways, into.attic_ways);
  else if (type == RELATION)
    get_elements(ref_ids, this, rman, into.relations, into.attic_relations);
  else if (type == AREA)
    collect_elems_flat(rman, ref_ids, std::vector< Area_Skeleton::Id_Type >(), true, into.areas);

  transfer_output(rman, into);
  rman.health_check(*this);
}

Id_Query_Statement::~Id_Query_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

Query_Constraint* Id_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Id_Query_Constraint(*this));
  return constraints.back();
}
