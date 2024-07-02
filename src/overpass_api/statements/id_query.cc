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
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "../data/idx_from_id.h"
#include "id_query.h"

#include <sstream>


bool Id_Query_Statement::area_query_exists_ = false;


Id_Query_Statement::Statement_Maker Id_Query_Statement::statement_maker;
Id_Query_Statement::Criterion_Maker Id_Query_Statement::criterion_maker;


Statement* Id_Query_Statement::Criterion_Maker::create_criterion(const Token_Node_Ptr& input_tree,
    const std::string& result_type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  Token_Node_Ptr tree_it = input_tree;
  uint line_nr = tree_it->line_col.first;
  std::vector< std::string > ref;

  while (tree_it->token == "," && tree_it->rhs && tree_it->lhs)
  {
    ref.push_back(tree_it.rhs()->token);
    tree_it = tree_it.lhs();
  }

  if (tree_it->token == ":" && tree_it->rhs)
    ref.push_back(tree_it.rhs()->token);
  else
    ref.push_back(tree_it->token);

  std::reverse(ref.begin(), ref.end());

  std::map< std::string, std::string > attributes;
  if (result_type != "nwr" && result_type != "nw" && result_type != "wr" && result_type != "nr")
    attributes["type"] = result_type;
  attributes["into"] = into;

  for (uint i = 0; i < ref.size(); ++i)
  {
    std::ostringstream id;
    if (i == 0)
      id << "ref";
    else
      id << "ref_" << i;
    attributes[id.str()] = ref[i];
  }

  return new Id_Query_Statement(line_nr, attributes, global_settings);
}


void collect_elems_flat(Resource_Manager& rman,
                   const std::vector< uint64 >& ids,
		   std::map< Uint31_Index, std::vector< Area_Skeleton > >& elems)
{
  Block_Backend< Uint31_Index, Area_Skeleton > elems_db
      (rman.get_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it = elems_db.flat_begin(); !(it == elems_db.flat_end()); ++it)
  {
    if (std::binary_search(ids.begin(), ids.end(), it.object().id.val()))
      elems[it.index()].push_back(it.object());
  }
}


template< class TIndex, class TObject >
void filter_elems(const std::vector< uint64 > & ids,
		  std::map< TIndex, std::vector< TObject > >& elems)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (typename std::vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (std::binary_search(ids.begin(), ids.end(), iit->id.val()))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename Id_Type >
std::vector< Id_Type > filtered_assign(const std::vector< uint64 >& arg)
{
  std::vector< Id_Type > result;
  result.reserve(arg.size());
  for (auto i : arg)
  {
    if (i <= (uint64)Id_Type::max().val())
      result.push_back(Id_Type(static_cast< decltype(result.front().val()) >(i)));
  }
  return result;
}


//-----------------------------------------------------------------------------


class Id_Query_Constraint : public Query_Constraint
{
  public:
    Id_Query_Constraint(Id_Query_Statement& stmt_) : stmt(&stmt_) {}

    Query_Filter_Strategy delivers_data(Resource_Manager& rman) { return prefer_ranges; }

    bool get_ranges(Resource_Manager& rman, Ranges< Uint32_Index >& ranges);
    bool get_ranges(Resource_Manager& rman, Ranges< Uint31_Index >& ranges);

    bool get_node_ids
        (Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids);
    bool get_way_ids
        (Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids);
    bool get_relation_ids
        (Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids);
    bool get_area_ids
        (Resource_Manager& rman, std::vector< Area_Skeleton::Id_Type >& ids);

    void filter(Resource_Manager& rman, Set& into);
    virtual ~Id_Query_Constraint() {}

  private:
    Id_Query_Statement* stmt;
};


bool Id_Query_Constraint::get_node_ids(Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids)
{
  ids = filtered_assign< Node_Skeleton::Id_Type >(stmt->get_refs());
  return true;
}


bool Id_Query_Constraint::get_way_ids(Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids)
{
  ids = filtered_assign< Way_Skeleton::Id_Type >(stmt->get_refs());
  return true;
}


bool Id_Query_Constraint::get_relation_ids(Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids)
{
  ids = filtered_assign< Relation_Skeleton::Id_Type >(stmt->get_refs());
  return true;
}


bool Id_Query_Constraint::get_area_ids(Resource_Manager& rman, std::vector< Area_Skeleton::Id_Type >& ids)
{
  ids = filtered_assign< Area_Skeleton::Id_Type >(stmt->get_refs());
  return true;
}


bool Id_Query_Constraint::get_ranges(Resource_Manager& rman, Ranges< Uint32_Index >& ranges)
{
  std::vector< Node_Skeleton::Id_Type > ids = filtered_assign< Node_Skeleton::Id_Type >(stmt->get_refs());
  Request_Context context(stmt, rman);
  std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(ids, context);

  ranges = Ranges< Uint32_Index >();
  for (std::vector< Uint32_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
    ranges.push_back(*it, ++Uint32_Index(*it));
  ranges.sort();

  return true;
}


bool Id_Query_Constraint::get_ranges(Resource_Manager& rman, Ranges< Uint31_Index >& ranges)
{
  Request_Context context(stmt, rman);
  std::vector< Uint31_Index > req;
  ranges = Ranges< Uint31_Index >();

  {
    std::vector< Way_Skeleton::Id_Type > ids = filtered_assign< Way_Skeleton::Id_Type >(stmt->get_refs());
    get_indexes_< Uint31_Index, Way_Skeleton >(ids, context).swap(req);
    for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
      ranges.push_back(*it, inc(*it));
  }

  {
    std::vector< Relation_Skeleton::Id_Type > ids = filtered_assign< Relation_Skeleton::Id_Type >(stmt->get_refs());
    get_indexes_< Uint31_Index, Relation_Skeleton >(ids, context).swap(req);
    for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
      ranges.push_back(*it, inc(*it));
  }

  ranges.sort();

  return true;
}


void Id_Query_Constraint::filter(Resource_Manager& rman, Set& into)
{
  filter_elems(stmt->get_refs(), into.nodes);
  filter_elems(stmt->get_refs(), into.attic_nodes);

  filter_elems(stmt->get_refs(), into.ways);
  filter_elems(stmt->get_refs(), into.attic_ways);

  filter_elems(stmt->get_refs(), into.relations);
  filter_elems(stmt->get_refs(), into.attic_relations);

  filter_elems(stmt->get_refs(), into.areas);
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
  else if (attributes["type"].empty())
    type = 0;
  else
  {
    type = 0;
    std::ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"id-query\""
	<<" the only allowed values are \"node\", \"way\", \"relation\", or \"area\".";
    add_static_error(temp.str());
  }

  uint64 ref = atoll(attributes["ref"].c_str());

  if (ref > 0)
    refs.push_back(ref);

  for (std::map< std::string, std::string >::iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first.find("ref_") == 0)
    {
      ref = atoll(it->second.c_str());
      if (ref > 0)
        refs.push_back(ref);
    }
  }

  uint64 lower = atoll(attributes["lower"].c_str());
  uint64 upper = atoll(attributes["upper"].c_str());

  if (ref <= 0)
  {
    if (lower == 0 || upper == 0)
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

  if (lower > 0 && upper > 0)
  {
    for (uint64 i = lower; i < upper; ++i)
      refs.push_back(i);
  }

  std::sort(refs.begin(), refs.end());
  refs.erase(std::unique(refs.begin(), refs.end()), refs.end());
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
void get_elements(const std::vector< uint64 >& refs, Statement* stmt, Request_Context& context,
    std::map< Index, std::vector< Skeleton > >& current_result,
    std::map< Index, std::vector< Attic< Skeleton > > >& attic_result)
{
  std::vector< typename Skeleton::Id_Type > ids = filtered_assign< typename Skeleton::Id_Type >(refs);
  std::vector< Index > req = get_indexes_< Index, Skeleton >(ids, context);

  collect_items_discrete< Index, Skeleton >(context, req, Id_Predicate< Skeleton >(ids))
      .swap(current_result, attic_result);

  if (context.get_desired_timestamp() != NOW)
    filter_attic_elements(context, context.get_desired_timestamp(), current_result, attic_result);
}


void Id_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;
  Request_Context context(this, rman);

  if (type == NODE)
    get_elements(refs, this, context, into.nodes, into.attic_nodes);
  else if (type == WAY)
    get_elements(refs, this, context, into.ways, into.attic_ways);
  else if (type == RELATION)
    get_elements(refs, this, context, into.relations, into.attic_relations);
  else if (type == AREA)
  {
    get_elements(refs, this, context, into.ways, into.attic_ways);
    collect_elems_flat(rman, refs, into.areas);
  }

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
