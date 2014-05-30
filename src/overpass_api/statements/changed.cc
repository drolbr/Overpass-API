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

#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "changed.h"


bool Changed_Statement::area_query_exists_ = false;

Generic_Statement_Maker< Changed_Statement > Changed_Statement::statement_maker("changed");


template< class TIndex, class TObject >
void filter_elems(const std::vector< typename TObject::Id_Type >& ids, map< TIndex, vector< TObject > >& elems)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
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
    
    bool delivers_data() { return true; }
    
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Changed_Constraint() {}
    
  private:
    Changed_Statement* stmt;
};


bool Changed_Constraint::get_ranges(Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  std::vector< Node_Skeleton::Id_Type > ids
      = collect_changed_elements< Uint32_Index, Node_Skeleton >(stmt->get_since(rman), stmt->get_until(rman), rman);
      
  std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(ids, rman);
  
  ranges.clear();
  for (std::vector< Uint32_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
    ranges.insert(std::make_pair(*it, ++Uint32_Index(*it)));
  
  return true;
}


bool Changed_Constraint::get_ranges(Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  std::vector< Way_Skeleton::Id_Type > way_ids = collect_changed_elements< Uint31_Index, Way_Skeleton >
      (stmt->get_since(rman), stmt->get_until(rman), rman);
  std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(way_ids, rman);
      
  ranges.clear();
  for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
    ranges.insert(std::make_pair(*it, inc(*it)));

  std::vector< Relation_Skeleton::Id_Type > rel_ids = collect_changed_elements< Uint31_Index, Relation_Skeleton >
      (stmt->get_since(rman), stmt->get_until(rman), rman);
  get_indexes_< Uint31_Index, Relation_Skeleton >(rel_ids, rman).swap(req);
  
  for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
    ranges.insert(std::make_pair(*it, inc(*it)));

  return true;
}


void Changed_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  {
    std::vector< Node_Skeleton::Id_Type > ids =
        collect_changed_elements< Uint32_Index, Node_Skeleton >
        (stmt->get_since(rman), stmt->get_until(rman), rman);
      
    filter_elems(ids, into.nodes);
    filter_elems(ids, into.attic_nodes);
  }
  {
    std::vector< Way_Skeleton::Id_Type > ids =
        collect_changed_elements< Uint31_Index, Way_Skeleton >
        (stmt->get_since(rman), stmt->get_until(rman), rman);
      
    filter_elems(ids, into.ways);
    filter_elems(ids, into.attic_ways);
  }
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
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_), since(NOW), until(NOW)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["since"] = "auto";
  attributes["until"] = "auto";
  
  Statement::eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);

  string timestamp = attributes["since"];
  if (timestamp != "auto" && timestamp.size() >= 19)
    since = Timestamp(
        atol(timestamp.c_str()), //year
        atoi(timestamp.c_str()+5), //month
        atoi(timestamp.c_str()+8), //day
        atoi(timestamp.c_str()+11), //hour
        atoi(timestamp.c_str()+14), //minute
        atoi(timestamp.c_str()+17) //second
        ).timestamp;
	
  if (timestamp != "auto" && (since == 0 || since == NOW))
  {
    ostringstream temp;
    temp<<"The attribute \"since\" must contain a timestamp exactly in the form \"yyyy-mm-ddThh:mm:ssZ\".";
    add_static_error(temp.str());
  }

  timestamp = attributes["until"];
  if (timestamp.size() >= 19)
    until = Timestamp(
        atol(timestamp.c_str()), //year
        atoi(timestamp.c_str()+5), //month
        atoi(timestamp.c_str()+8), //day
        atoi(timestamp.c_str()+11), //hour
        atoi(timestamp.c_str()+14), //minute
        atoi(timestamp.c_str()+17) //second
        ).timestamp;
	
  if (timestamp != "auto" && (until == 0 || until == NOW))
  {
    ostringstream temp;
    temp<<"The attribute \"until\" must contain a timestamp exactly in the form \"yyyy-mm-ddThh:mm:ssZ\".";
    add_static_error(temp.str());
  }
}


uint64 Changed_Statement::get_since(Resource_Manager& rman) const
{
  if (since == until)
    return std::min(since, rman.get_desired_timestamp());
  else if (since == NOW)
    return rman.get_desired_timestamp();
  else
    return since;
}


uint64 Changed_Statement::get_until(Resource_Manager& rman) const
{
  if (since == until)
    return std::max(until, rman.get_desired_timestamp());
  else if (until == NOW)
    return rman.get_desired_timestamp();
  else
    return until;
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
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


Query_Constraint* Changed_Statement::get_query_constraint()
{
  constraints.push_back(new Changed_Constraint(*this));
  return constraints.back();
}
