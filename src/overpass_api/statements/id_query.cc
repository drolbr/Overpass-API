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
#include "../data/filenames.h"
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
    
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                          const vector< Node::Id_Type >& ids,
                          bool invert_ids, uint64 timestamp);
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                          int type,
                          const vector< Uint32_Index >& ids,
                          bool invert_ids, uint64 timestamp);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Id_Query_Constraint() {}
    
  private:
    Id_Query_Statement* stmt;
};

bool Id_Query_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint32_Index, Uint32_Index > >& ranges,
     const vector< Node_Skeleton::Id_Type >& ids,
     bool invert_ids, uint64 timestamp)
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
     int type,
     const vector< Uint32_Index >& ids,
     bool invert_ids, uint64 timestamp)
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

void Id_Query_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
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
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  attributes["lower"] = "";
  attributes["upper"] = "";
  
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
    if (lower.val() == 0 || upper.val() == 0)
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


// Nach "data" verschieben ---------------------------


/* Returns for the given set of ids the set of corresponding indexes.
 * For ids where the timestamp is zero, only the current index is returned.
 * For ids where the timestamp is nonzero, all attic indexes are also returned.
 * The function requires that the ids are sorted ascending by id.
 */
template< typename Index, typename Skeleton >
std::pair< std::vector< Index >, std::vector< Index > > get_indexes
    (const std::vector< std::pair< typename Skeleton::Id_Type, uint64 > >& ids,
     Resource_Manager& rman)
{
  std::pair< std::vector< Index >, std::vector< Index > > result;
  
  Random_File< Index > current(rman.get_transaction()->random_index
      (current_skeleton_file_properties< Skeleton >()));
  for (typename std::vector< std::pair< typename Skeleton::Id_Type, uint64 > >::const_iterator
      it = ids.begin(); it != ids.end(); ++it)
    result.first.push_back(current.get(it->first.val()));
  
  std::sort(result.first.begin(), result.first.end());
  result.first.erase(std::unique(result.first.begin(), result.first.end()), result.first.end());
  
  if (rman.get_desired_timestamp() != NOW)
  {
    Random_File< Index > attic_random(rman.get_transaction()->random_index
        (attic_skeleton_file_properties< Skeleton >()));
    std::set< typename Skeleton::Id_Type > idx_list_ids;
    for (typename std::vector< std::pair< typename Skeleton::Id_Type, uint64 > >::const_iterator
        it = ids.begin(); it != ids.end(); ++it)
    {
      if (it->second == 0 || attic_random.get(it->first.val()).val() == 0)
        ;
      else if (attic_random.get(it->first.val()) == 0xff)
        idx_list_ids.insert(it->first.val());
      else
        result.second.push_back(attic_random.get(it->first.val()));
    }
  
    Block_Backend< typename Skeleton::Id_Type, Index > idx_list_db
        (rman.get_transaction()->data_index(attic_idx_list_properties< Skeleton >()));
    for (typename Block_Backend< typename Skeleton::Id_Type, Index >::Discrete_Iterator
        it(idx_list_db.discrete_begin(idx_list_ids.begin(), idx_list_ids.end()));
        !(it == idx_list_db.discrete_end()); ++it)
      result.second.push_back(it.object());
  
    std::sort(result.second.begin(), result.second.end());
    result.second.erase(std::unique(result.second.begin(), result.second.end()), result.second.end());
  }
  
  return result;
}


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


/* Returns for the given set of ids the set of corresponding elements.
 * The function requires that the ids are sorted ascending by id.
 * The function only searches at the provided indexes.
 * Also, the function does always find the oldest object that has not been expired at timestamp
 * (or not yet if timestamp is zero).
 * So the user still needs to filter away objects that have been created after timestamp.
 */
template< typename Index, typename Skeleton >
void get_elements
    (const std::vector< std::pair< typename Skeleton::Id_Type, uint64 > >& ids,
     const std::pair< std::vector< Index >, std::vector< Index > > & idx_set,
     Resource_Manager& rman,
     std::map< Index, vector< Skeleton > >& current,
     std::map< Index, vector< Attic< Skeleton > > >& attic)
{
  std::vector< Index > joint_idx_set(idx_set.first.size() + idx_set.second.size());
  joint_idx_set.erase(std::set_union
      (idx_set.first.begin(), idx_set.first.end(), idx_set.second.begin(), idx_set.second.end(),
       joint_idx_set.begin()), joint_idx_set.end());
  
  current.clear();
  attic.clear();
  
  Id_Pair_First_Comparator< typename Skeleton::Id_Type > first_comp;
  
  if (rman.get_desired_timestamp() == NOW)
  {
    // Get the current elements.
    Block_Backend< Index, Skeleton, typename std::vector< Index >::const_iterator >
        current_db(rman.get_transaction()->data_index(current_skeleton_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, Skeleton, typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = current_db.discrete_begin(idx_set.first.begin(), idx_set.first.end());
        !(it == current_db.discrete_end()); ++it)
    {
      if (std::binary_search(ids.begin(), ids.end(), make_pair(it.object().id, 0), first_comp))
        current[it.index()].push_back(it.object());
    }
  }
  else
  {
    // Set up a dictionary to store for each requested element the oldest that would fit.
    std::vector< Attic_Skeleton_By_Id< Index, Skeleton > > attic_dict;
    for (typename std::vector< std::pair< typename Skeleton::Id_Type, uint64 > >::const_iterator
        it = ids.begin(); it != ids.end(); ++it)
      attic_dict.push_back(Attic_Skeleton_By_Id< Index, Skeleton >
          (it->first, it->second == 0ull ? 0xffffffffffffffffull : it->second));
    sort(attic_dict.begin(), attic_dict.end());
    
    // Get the current elements and put each to the dictionary.
    Block_Backend< Index, Skeleton, typename std::vector< Index >::const_iterator >
        current_db(rman.get_transaction()->data_index(current_skeleton_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, Skeleton, typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = current_db.discrete_begin(idx_set.first.begin(), idx_set.first.end());
        !(it == current_db.discrete_end()); ++it)
    {
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict
          = std::upper_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object().id, NOW));
      if (it_dict != attic_dict.end() && it_dict->id == it.object().id)
      {
        it_dict->index = it.index();
        it_dict->elem = Attic< Skeleton >(it.object(), NOW);
      }
    }
  
    // Get the attic elements and put each to the dictionary if it is the best hit.
    Block_Backend< Index, Attic< Skeleton >, typename std::vector< Index >::const_iterator >
        attic_db(rman.get_transaction()->data_index(attic_skeleton_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, Attic< Skeleton >, typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = attic_db.discrete_begin(idx_set.second.begin(), idx_set.second.end());
        !(it == attic_db.discrete_end()); ++it)
    {
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict
          = std::upper_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object().id, it.object().timestamp));
      if (it_dict != attic_dict.end() && it_dict->id == it.object().id
          && (it_dict->elem.id == typename Skeleton::Id_Type() || it.object().timestamp < it_dict->elem.timestamp))
      {
        it_dict->index = it.index();
        it_dict->elem = it.object();
      }
    }
    
    // Remove elements that have been deleted at the given point of time
    Block_Backend< Index, Attic< typename Skeleton::Id_Type >, typename std::vector< Index >::const_iterator >
        undeleted_db(rman.get_transaction()->data_index(attic_undeleted_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, Attic< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = undeleted_db.discrete_begin(joint_idx_set.begin(), joint_idx_set.end());
        !(it == undeleted_db.discrete_end()); ++it)
    {
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict
          = std::upper_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object(), it.object().timestamp));
      if (it_dict != attic_dict.end() && it_dict->id == it.object()
          && (it_dict->elem.id == typename Skeleton::Id_Type() || it.object().timestamp < it_dict->elem.timestamp))
      {
        it_dict->elem.id = typename Skeleton::Id_Type();
        it_dict->index = 0xffu;
      }
    }
    
    // Confirm elements that are backed by meta data
    // Update element's expiration timestamp if a meta exists that is older than the current
    // expiration date and younger than timestamp
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        attic_meta_db(rman.get_transaction()->data_index
          (attic_meta_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = attic_meta_db.discrete_begin(joint_idx_set.begin(), joint_idx_set.end());
        !(it == attic_meta_db.discrete_end()); ++it)
    {
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict
          = std::lower_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object().ref, 0xffffffffffffffffull));
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict_end
          = std::upper_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object().ref, 0ull));
      while (it_dict != it_dict_end)
      {
        if (it_dict->timestamp >= it.object().timestamp)
          it_dict->meta_confirmed |= (it_dict->index == it.index());
        else
          it_dict->elem.timestamp = std::min(it_dict->elem.timestamp, it.object().timestamp);
        ++it_dict;
      }
    }
    
    // Same thing with current meta data
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        meta_db(rman.get_transaction()->data_index
          (current_meta_file_properties< Skeleton >()));
        
    for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = meta_db.discrete_begin(idx_set.first.begin(), idx_set.first.end());
        !(it == meta_db.discrete_end()); ++it)
    {
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict
          = std::lower_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object().ref, 0xffffffffffffffffull));
      typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict_end
          = std::upper_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it.object().ref, 0ull));
      while (it_dict != it_dict_end)
      {
        if (it_dict->timestamp >= it.object().timestamp)
          it_dict->meta_confirmed |= (it_dict->index == it.index());
        else
          it_dict->elem.timestamp = std::min(it_dict->elem.timestamp, it.object().timestamp);
        ++it_dict;
      }
    }
    
    // Filter the current elements such that only those not superseded by older remain.
    for (typename std::map< Index, vector< Skeleton > >::iterator it_idx = current.begin();
         it_idx != current.end(); ++it_idx)
    {
      vector< Skeleton > filtered;
      for (typename vector< Skeleton >::iterator it = it_idx->second.begin();
           it != it_idx->second.end(); ++it)
      {
        typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::iterator it_dict
            = std::lower_bound(attic_dict.begin(), attic_dict.end(),
                             Attic_Skeleton_By_Id< Index, Skeleton >(it->id, 0xffffffffffffffffull));
        while (it_dict != attic_dict.end() && it_dict->id == it->id)
        {
          if (it_dict->elem.id == typename Skeleton::Id_Type() && it_dict->index == 0u && it_dict->meta_confirmed)
          {
            if (it_dict->elem.timestamp == 0xffffffffffffffffull)
              filtered.push_back(*it);
            else
            {
              it_dict->index = it_idx->first;
              it_dict->elem = Attic< Skeleton >(*it, it_dict->elem.timestamp);
            }
          }
          ++it_dict;
        }
      }
      it_idx->second.swap(filtered);
    }
  
    // Copy the found attic elements to attic or current.
    for (typename std::vector< Attic_Skeleton_By_Id< Index, Skeleton > >::const_iterator
         it = attic_dict.begin(); it != attic_dict.end(); ++it)
    {
      if (!(it->elem.id == typename Skeleton::Id_Type()) && it->meta_confirmed)
      {
        if (it->elem.timestamp == NOW)
          current[it->index].push_back(it->elem);
        else
          attic[it->index].push_back(it->elem);
      }
    }
  }
}


// Nach "data" verschieben ---------------------------


void Id_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  if (type == NODE)
  {
    std::vector< std::pair< Node_Skeleton::Id_Type, uint64 > > ids;
    for (uint64 i = lower.val(); i < upper.val(); ++i)
      ids.push_back(make_pair(i, rman.get_desired_timestamp()));
    std::pair< std::vector< Uint32_Index >, std::vector< Uint32_Index > > req
        = get_indexes< Uint32_Index, Node_Skeleton >(ids, rman);
        
    get_elements(ids, req, rman, into.nodes, into.attic_nodes);
  }
  else if (type == WAY)
  {
    std::vector< std::pair< Way_Skeleton::Id_Type, uint64 > > ids;
    for (uint64 i = lower.val(); i < upper.val(); ++i)
      ids.push_back(make_pair(i, rman.get_desired_timestamp()));
    std::pair< std::vector< Uint31_Index >, std::vector< Uint31_Index > > req
        = get_indexes< Uint31_Index, Way_Skeleton >(ids, rman);
        
    get_elements(ids, req, rman, into.ways, into.attic_ways);
  }
  else if (type == RELATION)
  {
    std::vector< std::pair< Relation_Skeleton::Id_Type, uint64 > > ids;
    for (uint64 i = lower.val(); i < upper.val(); ++i)
      ids.push_back(make_pair(i, rman.get_desired_timestamp()));
    std::pair< std::vector< Uint31_Index >, std::vector< Uint31_Index > > req
        = get_indexes< Uint31_Index, Relation_Skeleton >(ids, rman);
        
    get_elements(ids, req, rman, into.relations, into.attic_relations);
  }
  else if (type == AREA)
    collect_elems_flat(rman, lower.val(), upper.val(), vector< Area_Skeleton::Id_Type >(), true, into.areas);

  transfer_output(rman, into);
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
