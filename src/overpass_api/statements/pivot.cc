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
#include "pivot.h"
#include "query.h"


Pivot_Statement::Statement_Maker Pivot_Statement::statement_maker;
Pivot_Statement::Criterion_Maker Pivot_Statement::criterion_maker;


Statement* Pivot_Statement::Criterion_Maker::create_criterion(const Token_Node_Ptr& tree_it,
    const std::string& type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  uint line_nr = tree_it->line_col.first;
  std::string from = "_";
  if (tree_it->rhs && tree_it->token == ".")
      from = tree_it.rhs()->token;
  
  std::map< std::string, std::string > attributes;
  attributes["from"] = from;
  attributes["into"] = into;
  return new Pivot_Statement(line_nr, attributes, global_settings);
}


template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
		   const std::vector< typename TObject::Id_Type >& ids,
		   std::map< TIndex, std::vector< TObject > >& elems)
{
  std::set< TIndex > req;
  {
    Random_File< typename TObject::Id_Type, TIndex > random(rman.get_transaction()->random_index(&prop));
    for (typename std::vector< typename TObject::Id_Type >::const_iterator
        it = ids.begin(); it != ids.end(); ++it)
      req.insert(random.get(it->val()));
  }
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if (binary_search(ids.begin(), ids.end(), it.object().id))
      elems[it.index()].push_back(it.object());
  }
}


template< class TIndex, class TObject >
void filter_elems(const std::vector< typename TObject::Id_Type >& ids,
                  std::map< TIndex, std::vector< TObject > >& elems)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (typename std::vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (binary_search(ids.begin(), ids.end(), iit->id))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


std::vector< Node::Id_Type > get_node_pivot_ids(const std::map< Uint31_Index, std::vector< Area_Skeleton > >& areas)
{
  std::vector< Node::Id_Type > pivot_ids;
  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator it = areas.begin();
      it != areas.end(); ++it)
  {
    for (std::vector< Area_Skeleton >::const_iterator sit = it->second.begin(); sit != it->second.end(); ++sit)
    {
      if (sit->id.val() < 2400000000u)
        pivot_ids.push_back(sit->id.val());
    }
  }
  sort(pivot_ids.begin(), pivot_ids.end());
  return pivot_ids;
}


std::vector< Way::Id_Type > get_way_pivot_ids(const std::map< Uint31_Index, std::vector< Area_Skeleton > >& areas)
{
  std::vector< Way::Id_Type > pivot_ids;
  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator it = areas.begin();
      it != areas.end(); ++it)
  {
    for (std::vector< Area_Skeleton >::const_iterator sit = it->second.begin(); sit != it->second.end(); ++sit)
    {
      if (sit->id.val() > 2400000000u && sit->id.val() < 3600000000u)
        pivot_ids.push_back(sit->id.val() - 2400000000u);
    }
  }
  sort(pivot_ids.begin(), pivot_ids.end());
  return pivot_ids;
}


std::vector< Relation::Id_Type > get_relation_pivot_ids(const std::map< Uint31_Index, std::vector< Area_Skeleton > >& areas)
{
  std::vector< Relation::Id_Type > pivot_ids;
  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator it = areas.begin();
      it != areas.end(); ++it)
  {
    for (std::vector< Area_Skeleton >::const_iterator sit = it->second.begin(); sit != it->second.end(); ++sit)
    {
      if (sit->id.val() > 3600000000u)
        pivot_ids.push_back(sit->id.val() - 3600000000u);
    }
  }
  sort(pivot_ids.begin(), pivot_ids.end());
  return pivot_ids;
}


//-----------------------------------------------------------------------------


class Pivot_Constraint : public Query_Constraint
{
  public:
    Pivot_Constraint(Pivot_Statement& stmt_) : stmt(&stmt_) {}

    bool delivers_data(Resource_Manager& rman) { return true; }

    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
                          const std::vector< Node::Id_Type >& ids,
                          bool invert_ids);
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
                          int type,
                          const std::vector< Uint32_Index >& ids,
                          bool invert_ids);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Pivot_Constraint() {}

  private:
    Pivot_Statement* stmt;
};


bool Pivot_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
     const std::vector< Node_Skeleton::Id_Type >& ids,
     bool invert_ids)
{
  const Set* input_set = rman.get_set(stmt->get_input());
  std::vector< Node::Id_Type > pivot_ids;
  if (input_set)
    pivot_ids = get_node_pivot_ids(input_set->areas);
  std::vector< Node::Id_Type > intersect_ids(pivot_ids.size());
  if (ids.empty())
    pivot_ids.swap(intersect_ids);
  else if (!invert_ids)
    intersect_ids.erase(set_intersection
          (pivot_ids.begin(), pivot_ids.end(),
           ids.begin(), ids.end(),
          intersect_ids.begin()), intersect_ids.end());
  else
    intersect_ids.erase(set_difference
          (pivot_ids.begin(), pivot_ids.end(),
           ids.begin(), ids.end(),
          intersect_ids.begin()), intersect_ids.end());
  collect_elems(rman, *osm_base_settings().NODES, intersect_ids, into.nodes);
		
  return true;
}


bool Pivot_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
     int type,
     const std::vector< Uint32_Index >& ids,
     bool invert_ids)
{
  const Set* input_set = rman.get_set(stmt->get_input());
  
  if (type == QUERY_WAY)
  {
    std::vector< Way::Id_Type > pivot_ids;
    if (input_set)
      pivot_ids = get_way_pivot_ids(input_set->areas);
    std::vector< Way::Id_Type > intersect_ids(pivot_ids.size());
    if (ids.empty())
      pivot_ids.swap(intersect_ids);
    else if (!invert_ids)
      intersect_ids.erase(set_intersection
          (pivot_ids.begin(), pivot_ids.end(),
           ids.begin(), ids.end(),
          intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (pivot_ids.begin(), pivot_ids.end(),
           ids.begin(), ids.end(),
          intersect_ids.begin()), intersect_ids.end());
    collect_elems(rman, *osm_base_settings().WAYS, intersect_ids, into.ways);
  }
  else if (type == QUERY_RELATION)
  {
    std::vector< Relation::Id_Type > pivot_ids;
    if (input_set)
      pivot_ids = get_relation_pivot_ids(input_set->areas);
    std::vector< Relation::Id_Type > intersect_ids(pivot_ids.size());
    if (ids.empty())
      pivot_ids.swap(intersect_ids);
    else if (!invert_ids)
      intersect_ids.erase(set_intersection
          (pivot_ids.begin(), pivot_ids.end(),
           ids.begin(), ids.end(),
          intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (pivot_ids.begin(), pivot_ids.end(),
           ids.begin(), ids.end(),
          intersect_ids.begin()), intersect_ids.end());
    collect_elems(rman, *osm_base_settings().RELATIONS, intersect_ids, into.relations);
  }

  return true;
}

void Pivot_Constraint::filter(Resource_Manager& rman, Set& into)
{
  const Set* input_set = rman.get_set(stmt->get_input());
  
  if (input_set)
  {
    filter_elems(get_node_pivot_ids(input_set->areas), into.nodes);
    filter_elems(get_way_pivot_ids(input_set->areas), into.ways);
    filter_elems(get_relation_pivot_ids(input_set->areas), into.relations);
  }
  else
  {
    into.nodes.clear();
    into.ways.clear();
    into.relations.clear();
  }
  into.attic_nodes.clear();
  into.attic_ways.clear();
  into.attic_relations.clear();
  into.areas.clear();
  into.deriveds.clear();
}

//-----------------------------------------------------------------------------

Pivot_Statement::Pivot_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";

  Statement::eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);
}


void Pivot_Statement::execute(Resource_Manager& rman)
{
  Set into;

  const Set* input_set = rman.get_set(input);
  if (input_set)
  {
    collect_elems(rman, *osm_base_settings().NODES, get_node_pivot_ids(input_set->areas), into.nodes);
    collect_elems(rman, *osm_base_settings().WAYS, get_way_pivot_ids(input_set->areas), into.ways);
    collect_elems(rman, *osm_base_settings().RELATIONS, get_relation_pivot_ids(input_set->areas), into.relations);
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}


Pivot_Statement::~Pivot_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


Query_Constraint* Pivot_Statement::get_query_constraint()
{
  constraints.push_back(new Pivot_Constraint(*this));
  return constraints.back();
}
