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
#include "id_query.h"

using namespace std;

bool Id_Query_Statement::area_query_exists_ = false;

Generic_Statement_Maker< Id_Query_Statement > Id_Query_Statement::statement_maker("id-query");


template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
		   Uint64 lower, Uint64 upper,
		   map< TIndex, vector< TObject > >& elems)
{
  set< TIndex > req;
  {
    Random_File< TIndex > random(rman.get_transaction()->random_index(&prop));
    for (Uint64 i = lower; i < upper; ++i)
      req.insert(random.get(i.val()));
  }    
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if (it.object().id.val() >= lower.val() && it.object().id.val() < upper.val())
      elems[it.index()].push_back(it.object());
  }
}


template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
		   Uint64 lower, Uint64 upper,
		   const vector< typename TObject::Id_Type >& ids, bool invert_ids,
		   map< TIndex, vector< TObject > >& elems)
{
  set< TIndex > req;
  {
    Random_File< TIndex > random(rman.get_transaction()->random_index(&prop));
    for (typename TObject::Id_Type i = lower.val(); i.val() < upper.val(); ++i)
    {
      if (binary_search(ids.begin(), ids.end(), i) ^ invert_ids)
        req.insert(random.get(i.val()));
    }
  }    
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if (!(it.object().id.val() < lower.val()) && it.object().id.val() < upper.val()
        && (binary_search(ids.begin(), ids.end(), it.object().id) ^ invert_ids))
      elems[it.index()].push_back(it.object());
  }
}


void collect_elems_flat(Resource_Manager& rman,
		   Area_Skeleton::Id_Type lower, Area_Skeleton::Id_Type upper,
		   const vector< Area_Skeleton::Id_Type >& ids, bool invert_ids,
		   map< Uint31_Index, vector< Area_Skeleton > >& elems)
{
  Block_Backend< Uint31_Index, Area_Skeleton > elems_db
      (rman.get_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it = elems_db.flat_begin(); !(it == elems_db.flat_end()); ++it)
  {
    if (!(it.object().id < lower) && it.object().id < upper
        && (binary_search(ids.begin(), ids.end(), it.object().id) ^ invert_ids))
      elems[it.index()].push_back(it.object());
  }
}


template< class TIndex, class TObject >
void filter_elems(Uint64 lower, Uint64 upper,
		  map< TIndex, vector< TObject > >& elems)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (iit->id.val() >= lower.val() && iit->id.val() < upper.val())
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
    
    bool delivers_data() { return true; }
    
    bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
		  const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		  const vector< Node_Skeleton::Id_Type >& ids, bool invert_ids);
    bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  int type, const vector< Uint32_Index >& ids, bool invert_ids);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Id_Query_Constraint() {}
    
  private:
    Id_Query_Statement* stmt;
};

bool Id_Query_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint32_Index, Uint32_Index > >& ranges,
     const vector< Node_Skeleton::Id_Type >& ids, bool invert_ids)
{
  if (stmt->get_type() == Statement::NODE)
  {
    if (ids.empty())
      collect_elems(rman, *osm_base_settings().NODES, stmt->get_lower(), stmt->get_upper(),
		    into.nodes);
    else
      collect_elems(rman, *osm_base_settings().NODES, stmt->get_lower(), stmt->get_upper(),
		    ids, invert_ids, into.nodes);
  }
		    
  return true;
}

bool Id_Query_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint31_Index, Uint31_Index > >& ranges,
     int type, const vector< Uint32_Index >& ids, bool invert_ids)
{
  if (stmt->get_type() == Statement::WAY)
  {
    if (ids.empty())
      collect_elems(rman, *osm_base_settings().WAYS, stmt->get_lower(), stmt->get_upper(),
		    into.ways);
    else
      collect_elems(rman, *osm_base_settings().WAYS, stmt->get_lower(), stmt->get_upper(),
		    ids, invert_ids, into.ways);
    return true;
  }
  else if (stmt->get_type() == Statement::RELATION)
  {
    if (ids.empty())
      collect_elems(rman, *osm_base_settings().RELATIONS, stmt->get_lower(), stmt->get_upper(),
		    into.relations);
    else
      collect_elems(rman, *osm_base_settings().RELATIONS, stmt->get_lower(), stmt->get_upper(),
		    ids, invert_ids, into.relations);
    return true;
  }
  else if (stmt->get_type() == Statement::AREA)
  {
    if (ids.empty())
      collect_elems_flat(rman, stmt->get_lower().val(), stmt->get_upper().val(), ids, true, into.areas);
    else
      collect_elems_flat(rman, stmt->get_lower().val(), stmt->get_upper().val(), ids, invert_ids, into.areas);
    return true;
  }

  return false;
}

void Id_Query_Constraint::filter(Resource_Manager& rman, Set& into)
{
  if (stmt->get_type() == Statement::NODE)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.nodes);
  else
    into.nodes.clear();

  if (stmt->get_type() == Statement::WAY)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.ways);
  else
    into.ways.clear();
  
  if (stmt->get_type() == Statement::RELATION)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.relations);
  else
    into.relations.clear();
  
  if (stmt->get_type() == Statement::AREA)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.areas);
  else
    into.areas.clear();
}

//-----------------------------------------------------------------------------

Id_Query_Statement::Id_Query_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  attributes["lower"] = "";
  attributes["upper"] = "";
  
  Statement::eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["into"];
  
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
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"id-query\""
	<<" the only allowed values are \"node\", \"way\", \"relation\", or \"area\".";
    add_static_error(temp.str());
  }
  
  ref = atoll(attributes["ref"].c_str());
  lower = atoll(attributes["lower"].c_str());
  upper = atoll(attributes["upper"].c_str());
  if (ref.val() <= 0)
  {
    if (lower.val() == 0 || upper.val() == 1)
    {
      ostringstream temp;
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
}

void Id_Query_Statement::forecast()
{
}

void Id_Query_Statement::execute(Resource_Manager& rman)
{
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

  if (type == NODE)
    collect_elems(rman, *osm_base_settings().NODES, lower, upper, nodes);
  else if (type == WAY)
    collect_elems(rman, *osm_base_settings().WAYS, lower, upper, ways);
  else if (type == RELATION)
    collect_elems(rman, *osm_base_settings().RELATIONS, lower, upper, relations);
  else if (type == AREA)
    collect_elems_flat(rman, lower.val(), upper.val(), vector< Area_Skeleton::Id_Type >(), true, areas);

  rman.health_check(*this);
}

Id_Query_Statement::~Id_Query_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

Query_Constraint* Id_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Id_Query_Constraint(*this));
  return constraints.back();
}
