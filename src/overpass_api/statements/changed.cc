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
#include "changed.h"


bool Changed_Statement::area_query_exists_ = false;


Changed_Statement::Statement_Maker Changed_Statement::statement_maker;
Changed_Statement::Criterion_Maker Changed_Statement::criterion_maker;


Statement* Changed_Statement::Criterion_Maker::create_criterion(const Token_Node_Ptr& tree_it,
    const std::string& type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  std::string since;
  std::string until;
  uint line_nr = tree_it->line_col.first;

  if (tree_it->token == ":" && tree_it->rhs)
  {
    since = decode_json(tree_it.rhs()->token, error_output);
    until = since;
  }
  else if (tree_it->token == "," && tree_it->lhs && tree_it.lhs()->token == ":" && tree_it.lhs()->rhs
      && tree_it->rhs)
  {
    since = decode_json(tree_it.lhs().rhs()->token, error_output);
    until = decode_json(tree_it.rhs()->token, error_output);
  }
  else if (tree_it->token == "changed")
  {
    since = "auto";
    until = "auto";
  }
  else if (tree_it->token == ":")
  {
    if (error_output)
      error_output->add_parse_error("Date required after \"changed\" with colon",
          tree_it->line_col.first);
    return 0;
  }
  else
  {
    if (error_output)
      error_output->add_parse_error("Unexpected token \"" + tree_it->token + "\" after \"changed\"",
          tree_it->line_col.first);
    return 0;
  }

  std::map< std::string, std::string > attributes;
  attributes["since"] = since;
  attributes["until"] = until;
  return new Changed_Statement(line_nr, attributes, global_settings);
}


template< class TIndex, class TObject >
void filter_elems(const std::vector< typename TObject::Id_Type >& ids, std::map< TIndex, std::vector< TObject > >& elems)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (typename std::vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (std::binary_search(ids.begin(), ids.end(), iit->id))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename Index, typename Skeleton >
std::vector< typename Skeleton::Id_Type > collect_changed_elements
    (uint64 since,
     uint64 until,
     Resource_Manager& rman)
{
  std::set< std::pair< Timestamp, Timestamp > > range;
  range.insert(std::make_pair(Timestamp(since), Timestamp(until)));

  std::vector< typename Skeleton::Id_Type > ids;

  Block_Backend< Timestamp, Change_Entry< typename Skeleton::Id_Type > > changelog_db
      (rman.get_transaction()->data_index(changelog_file_properties< Skeleton >()));
  for (typename Block_Backend< Timestamp, Change_Entry< typename Skeleton::Id_Type > >::Range_Iterator
      it = changelog_db.range_begin(Default_Range_Iterator< Timestamp >(range.begin()),
            Default_Range_Iterator< Timestamp >(range.end()));
      !(it == changelog_db.range_end()); ++it)
    ids.push_back(it.object().elem_id);

  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}


//-----------------------------------------------------------------------------

class Changed_Constraint : public Query_Constraint
{
  public:
    Changed_Constraint(Changed_Statement& stmt_) : stmt(&stmt_) {}

    bool delivers_data(Resource_Manager& rman) { return true; }

//     bool get_ranges
//         (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges);
//     bool get_ranges
//         (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);

    bool get_node_ids
        (Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids);
    bool get_way_ids
        (Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids);
    bool get_relation_ids
        (Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids);

    void filter(Resource_Manager& rman, Set& into);
    virtual ~Changed_Constraint() {}

  private:
    Changed_Statement* stmt;
};


bool Changed_Constraint::get_node_ids(Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids)
{
  ids = collect_changed_elements< Uint32_Index, Node_Skeleton >(
      stmt->get_since(rman), stmt->get_until(rman), rman);
  return true;
}


bool Changed_Constraint::get_way_ids(Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids)
{
  ids = collect_changed_elements< Uint31_Index, Way_Skeleton >(
      stmt->get_since(rman), stmt->get_until(rman), rman);
  return true;
}


bool Changed_Constraint::get_relation_ids(Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids)
{
  ids = collect_changed_elements< Uint31_Index, Relation_Skeleton >(
      stmt->get_since(rman), stmt->get_until(rman), rman);
  return true;
}


// bool Changed_Constraint::get_ranges(Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges)
// {
//   std::vector< Node_Skeleton::Id_Type > ids
//       = collect_changed_elements< Uint32_Index, Node_Skeleton >(stmt->get_since(rman), stmt->get_until(rman), rman);
//
//   std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(ids, rman);
//
//   ranges.clear();
//   for (std::vector< Uint32_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
//     ranges.insert(std::make_pair(*it, ++Uint32_Index(*it)));
//
//   return true;
// }


// bool Changed_Constraint::get_ranges(Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
// {
//   std::vector< Way_Skeleton::Id_Type > way_ids = collect_changed_elements< Uint31_Index, Way_Skeleton >
//       (stmt->get_since(rman), stmt->get_until(rman), rman);
//   std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(way_ids, rman);
//
//   ranges.clear();
//   for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
//     ranges.insert(std::make_pair(*it, inc(*it)));
//
//   std::vector< Relation_Skeleton::Id_Type > rel_ids = collect_changed_elements< Uint31_Index, Relation_Skeleton >
//       (stmt->get_since(rman), stmt->get_until(rman), rman);
//   get_indexes_< Uint31_Index, Relation_Skeleton >(rel_ids, rman).swap(req);
//
//   for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
//     ranges.insert(std::make_pair(*it, inc(*it)));
//
//   return true;
// }


void Changed_Constraint::filter(Resource_Manager& rman, Set& into)
{
  if (!stmt->trivial())
  {
    std::vector< Node_Skeleton::Id_Type > ids =
        collect_changed_elements< Uint32_Index, Node_Skeleton >
        (stmt->get_since(rman), stmt->get_until(rman), rman);

    filter_elems(ids, into.nodes);
    filter_elems(ids, into.attic_nodes);
  }

  if (!stmt->trivial())
  {
    std::vector< Way_Skeleton::Id_Type > ids =
        collect_changed_elements< Uint31_Index, Way_Skeleton >
        (stmt->get_since(rman), stmt->get_until(rman), rman);

    filter_elems(ids, into.ways);
    filter_elems(ids, into.attic_ways);
  }

  if (!stmt->trivial())
  {
    std::vector< Relation_Skeleton::Id_Type > ids =
        collect_changed_elements< Uint31_Index, Relation_Skeleton >
        (stmt->get_since(rman), stmt->get_until(rman), rman);

    filter_elems(ids, into.relations);
    filter_elems(ids, into.attic_relations);
  }

  into.areas.clear();
}

//-----------------------------------------------------------------------------

Changed_Statement::Changed_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), since(NOW), until(NOW), behave_trivial(false)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";
  attributes["since"] = "auto";
  attributes["until"] = "auto";

  Statement::eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);

  if (attributes["since"] == "init")
    behave_trivial = true;

  if (!behave_trivial && ((attributes["since"] == "auto") ^ (attributes["until"] == "auto")))
  {
    std::ostringstream temp;
    temp<<"The attributes \"since\" and \"until\" must be set either both or none.";
    add_static_error(temp.str());
  }

  std::string timestamp = attributes["since"];
  if (timestamp.size() >= 19)
    since = Timestamp(timestamp).timestamp;

  if (!behave_trivial && attributes["since"] != "auto" && (since == 0 || since == NOW))
    add_static_error("The attribute \"since\" must contain a timestamp exactly in the form \"yyyy-mm-ddThh:mm:ssZ\".");

  timestamp = attributes["until"];
  if (timestamp.size() >= 19)
    until = Timestamp(timestamp).timestamp;

  if (!behave_trivial && attributes["until"] != "auto" && (until == 0 || until == NOW))
    add_static_error("The attribute \"until\" must contain a timestamp exactly in the form \"yyyy-mm-ddThh:mm:ssZ\".");
}


uint64 Changed_Statement::get_since(Resource_Manager& rman) const
{
  if (since == NOW && until == NOW)
    // We have zero arguments on changed.
  {
    if (rman.get_diff_from_timestamp() != rman.get_diff_to_timestamp())
      // We are on diff mode
      return rman.get_diff_from_timestamp() + (rman.get_diff_from_timestamp() == NOW ? 0 : 1);
    else if (rman.get_desired_timestamp() != NOW)
      // We are in attic mode
      return rman.get_desired_timestamp() + 1;
    else
      // We are in present mode.
      // Trigger an empty result on purpose
      return NOW;
  }
  else if (since == until)
    // We have one argument on changed only.
  {
    if (rman.get_diff_from_timestamp() != rman.get_diff_to_timestamp())
      // We are on diff mode
    {
      if (since == rman.get_diff_from_timestamp() || since == rman.get_diff_to_timestamp())
	// If the only parameter is equal to the time range then silently ignore it
	return rman.get_diff_from_timestamp() + 1;
      else
        // Trigger an empty result on purpose
        return NOW;
    }
    else if (rman.get_desired_timestamp() != NOW)
      // We are in attic mode
      return std::min(since, rman.get_desired_timestamp()) + 1;
    else
      // We are in present mode.
      return since + 1;
  }
  else
    // We have two arguments on changed.
    return since + 1;
}


uint64 Changed_Statement::get_until(Resource_Manager& rman) const
{
  if (since == NOW && until == NOW)
    // We have zero arguments on changed.
  {
    if (rman.get_diff_from_timestamp() != rman.get_diff_to_timestamp())
      // We are on diff mode
      return rman.get_diff_to_timestamp() + (rman.get_diff_to_timestamp() == NOW ? 0 : 1);
    else if (rman.get_desired_timestamp() != NOW)
      // We are in attic mode
      return NOW;
    else
      // We are in present mode.
      // Trigger an empty result on purpose
      return NOW;
  }
  else if (since == until)
    // We have one argument on changed only.
  {
    if (rman.get_diff_from_timestamp() != rman.get_diff_to_timestamp())
      // We are on diff mode
    {
      if (since == rman.get_diff_from_timestamp() || since == rman.get_diff_to_timestamp())
	// If the only parameter is equal to the time range then silently ignore it
	return rman.get_diff_to_timestamp() + (rman.get_diff_to_timestamp() == NOW ? 0 : 1);
      else
        // Trigger an empty result on purpose
        return NOW;
    }
    else if (rman.get_desired_timestamp() != NOW)
      // We are in attic mode
      return std::max(until, rman.get_desired_timestamp()) + 1;
    else
      // We are in present mode.
      return NOW;
  }
  else
    // We have two arguments on changed.
    return until + 1;
}


template< typename Index, typename Skeleton >
void get_elements(Changed_Statement& stmt, Resource_Manager& rman,
    std::map< Index, std::vector< Skeleton > >& current_result,
    std::map< Index, std::vector< Attic< Skeleton > > >& attic_result)
{
  std::vector< typename Skeleton::Id_Type > ids =
      collect_changed_elements< Index, Skeleton >(stmt.get_since(rman), stmt.get_until(rman), rman);
  std::vector< Index > req = get_indexes_< Index, Skeleton >(ids, rman);

  if (rman.get_desired_timestamp() == NOW)
    collect_items_discrete(&stmt, rman, *current_skeleton_file_properties< Skeleton >(), req,
        Id_Predicate< Skeleton >(ids), current_result);
  else
  {
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        Id_Predicate< Skeleton >(ids), current_result, attic_result);
    filter_attic_elements(rman, rman.get_desired_timestamp(), current_result, attic_result);
  }
}


void Changed_Statement::execute(Resource_Manager& rman)
{
  Set into;

  get_elements(*this, rman, into.nodes, into.attic_nodes);
  get_elements(*this, rman, into.ways, into.attic_ways);
  get_elements(*this, rman, into.relations, into.attic_relations);
  into.areas.clear();

  transfer_output(rman, into);
  rman.health_check(*this);
}


Changed_Statement::~Changed_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


Query_Constraint* Changed_Statement::get_query_constraint()
{
  constraints.push_back(new Changed_Constraint(*this));
  return constraints.back();
}
