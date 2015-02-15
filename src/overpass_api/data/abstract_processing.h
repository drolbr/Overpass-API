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

#ifndef DE__OSM3S___OVERPASS_API__DATA__ABSTRACT_PROCESSING_H
#define DE__OSM3S___OVERPASS_API__DATA__ABSTRACT_PROCESSING_H

#include "../core/datatypes.h"
#include "../statements/statement.h"
#include "collect_items.h"
#include "filenames.h"


//-----------------------------------------------------------------------------

template < class TObject, class TPredicateA, class TPredicateB >
class And_Predicate
{
  public:
    And_Predicate(const TPredicateA& predicate_a_, const TPredicateB& predicate_b_)
        : predicate_a(predicate_a_), predicate_b(predicate_b_) {}
    bool match(const TObject& obj) const
    {
      return (predicate_a.match(obj) && predicate_b.match(obj));
    }
    bool match(const typename TObject::Id_Type id) const
    {
      return (predicate_a.match(id) && predicate_b.match(id));
    }
    
  private:
    TPredicateA predicate_a;
    TPredicateB predicate_b;
};

template < class TObject, class TPredicateA, class TPredicateB >
class Or_Predicate
{
  public:
    Or_Predicate(const TPredicateA& predicate_a_, const TPredicateB& predicate_b_)
        : predicate_a(predicate_a_), predicate_b(predicate_b_) {}
    bool match(const TObject& obj) const
    {
      return (predicate_a.match(obj) || predicate_b.match(obj));
    }
    bool match(const typename TObject::Id_Type id) const
    {
      return (predicate_a.match(id) || predicate_b.match(id));
    }
    
  private:
    TPredicateA predicate_a;
    TPredicateB predicate_b;
};

template < class TObject, class TPredicateA >
class Not_Predicate
{
  public:
    Not_Predicate(const TPredicateA& predicate_a_)
        : predicate_a(predicate_a_) {}
    bool match(const TObject& obj) const
    {
      return (!predicate_a.match(obj));
    }
    bool match(const typename TObject::Id_Type id) const
    {
      return (!predicate_a.match(id));
    }
    
  private:
    TPredicateA predicate_a;
};

template < class TObject >
class Trivial_Predicate
{
  public:
    Trivial_Predicate() {}
    bool match(const TObject& obj) const { return true; }
    bool match(const typename TObject::Id_Type id) const { return true; }
};

//-----------------------------------------------------------------------------

template < class TObject >
class Id_Predicate
{
  public:
    Id_Predicate(const vector< typename TObject::Id_Type >& ids_)
      : ids(ids_) {}
    bool match(const TObject& obj) const
    {
      return binary_search(ids.begin(), ids.end(), obj.id);
    }
    bool match(const typename TObject::Id_Type id) const
    {
      return binary_search(ids.begin(), ids.end(), id);
    }
    
  private:
    const vector< typename TObject::Id_Type >& ids;
};

//-----------------------------------------------------------------------------

inline bool has_a_child_with_id
    (const Relation_Skeleton& relation, const vector< Uint64 >& ids, uint32 type)
{
  for (vector< Relation_Entry >::const_iterator it3(relation.members.begin());
      it3 != relation.members.end(); ++it3)
  {
    if (it3->type == type &&
        binary_search(ids.begin(), ids.end(), it3->ref))
      return true;
  }
  return false;
}


inline bool has_a_child_with_id_and_role
    (const Relation_Skeleton& relation, const vector< Uint64 >& ids, uint32 type, uint32 role_id)
{
  for (vector< Relation_Entry >::const_iterator it3(relation.members.begin());
      it3 != relation.members.end(); ++it3)
  {
    if (it3->type == type && it3->role == role_id &&
        binary_search(ids.begin(), ids.end(), it3->ref))
      return true;
  }
  return false;
}


inline bool has_a_child_with_id
    (const Way_Skeleton& way, const vector< Node::Id_Type >& ids)
{
  for (vector< Node::Id_Type >::const_iterator it3(way.nds.begin());
      it3 != way.nds.end(); ++it3)
  {
    if (binary_search(ids.begin(), ids.end(), *it3))
      return true;
  }
  return false;
}


class Get_Parent_Rels_Predicate
{
public:
  Get_Parent_Rels_Predicate(const vector< Uint64 >& ids_, uint32 child_type_)
    : ids(ids_), child_type(child_type_) {}
  bool match(const Relation_Skeleton& obj) const
  {
    return has_a_child_with_id(obj, ids, child_type);
  }
  
private:
  const vector< Uint64 >& ids;
  uint32 child_type;
};


class Get_Parent_Rels_Role_Predicate
{
public:
  Get_Parent_Rels_Role_Predicate(const vector< Uint64 >& ids_, uint32 child_type_, uint32 role_id_)
    : ids(ids_), child_type(child_type_), role_id(role_id_) {}
  bool match(const Relation_Skeleton& obj) const
  {
    return has_a_child_with_id_and_role(obj, ids, child_type, role_id);
  }
  
private:
  const vector< Uint64 >& ids;
  uint32 child_type;
  uint32 role_id;
};


class Get_Parent_Ways_Predicate
{
public:
  Get_Parent_Ways_Predicate(const vector< Node::Id_Type >& ids_)
    : ids(ids_) {}
  bool match(const Way_Skeleton& obj) const
  {
    return has_a_child_with_id(obj, ids);
  }
  
private:
  const vector< Node::Id_Type >& ids;
};


//-----------------------------------------------------------------------------


template < typename Attic_Skeleton >
struct Attic_Comparator
{
public:
  bool operator()(const Attic_Skeleton& lhs, const Attic_Skeleton& rhs)
  {
    if (lhs.id < rhs.id)
      return true;
    if (rhs.id < lhs.id)
      return false;
    return (lhs.timestamp < rhs.timestamp);
  }
};


template < class TIndex, class TObject >
void keep_only_least_younger_than
    (map< TIndex, vector< Attic< TObject > > >& attic_result,
     map< TIndex, vector< TObject > >& result,
     uint64 timestamp)
{
  std::map< typename TObject::Id_Type, uint64 > timestamp_per_id;
  
  for (typename std::map< TIndex, std::vector< Attic< TObject > > >::iterator
      it = attic_result.begin(); it != attic_result.end(); ++it)
  {
    std::sort(it->second.begin(), it->second.end(), Attic_Comparator< Attic< TObject > >());
    typename std::vector< Attic< TObject > >::iterator it_from = it->second.begin();
    typename std::vector< Attic< TObject > >::iterator it_to = it->second.begin();
    while (it_from != it->second.end())
    {
      if (it_from->timestamp <= timestamp)
        ++it_from;
      else
      {
        *it_to = *it_from;
        if (timestamp_per_id[it_to->id] == 0 || timestamp_per_id[it_to->id] > it_to->timestamp)
          timestamp_per_id[it_to->id] = it_to->timestamp;
        ++it_from;
        while (it_from != it->second.end() && it_from->id == it_to->id)
          ++it_from;
        ++it_to;
      }
    }
    it->second.erase(it_to, it->second.end());
  }
  
  for (typename std::map< TIndex, std::vector< Attic< TObject > > >::iterator
      it = attic_result.begin(); it != attic_result.end(); ++it)
  {
    typename std::vector< Attic< TObject > >::iterator it_from = it->second.begin();
    typename std::vector< Attic< TObject > >::iterator it_to = it->second.begin();
    while (it_from != it->second.end())
    {
      if (timestamp_per_id[it_from->id] == it_from->timestamp)
      {
        *it_to = *it_from;
        ++it_to;
      }
      ++it_from;
    }
    it->second.erase(it_to, it->second.end());
  }
  
  for (typename std::map< TIndex, std::vector< TObject > >::iterator
      it = result.begin(); it != result.end(); ++it)
  {
    typename std::vector< TObject >::iterator it_from = it->second.begin();
    typename std::vector< TObject >::iterator it_to = it->second.begin();
    while (it_from != it->second.end())
    {
      if (timestamp_per_id.find(it_from->id) == timestamp_per_id.end())
      {
        *it_to = *it_from;
        ++it_to;
      }
      ++it_from;
    }
    it->second.erase(it_to, it->second.end());
  }
}


//-----------------------------------------------------------------------------

template < class TIndex, class TObject, class TPredicate >
void filter_items(const TPredicate& predicate, map< TIndex, vector< TObject > >& data)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = data.begin();
  it != data.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
    iit != it->second.end(); ++iit)
    {
      if (predicate.match(*iit))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

template< class TIndex, class TObject >
vector< typename TObject::Id_Type > filter_for_ids(const map< TIndex, vector< TObject > >& elems)
{
  vector< typename TObject::Id_Type > ids;
  for (typename map< TIndex, vector< TObject > >::const_iterator it = elems.begin();
  it != elems.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
    iit != it->second.end(); ++iit)
    ids.push_back(iit->id);
  }
  sort(ids.begin(), ids.end());
  
  return ids;
}

//-----------------------------------------------------------------------------

template< class TIndex, class TObject >
void indexed_set_union(map< TIndex, vector< TObject > >& result,
		       map< TIndex, vector< TObject > >& summand)
{
  for (typename map< TIndex, vector< TObject > >::iterator
      it = summand.begin(); it != summand.end(); ++it)
  {
    sort(it->second.begin(), it->second.end());
    vector< TObject > other;
    other.swap(result[it->first]);
    sort(other.begin(), other.end());
    set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
	      back_inserter(result[it->first]));
  }
}

//-----------------------------------------------------------------------------

template< class TIndex, class TObject >
void indexed_set_difference(map< TIndex, vector< TObject > >& result,
                            map< TIndex, vector< TObject > >& to_substract)
{
  for (typename map< TIndex, vector< TObject > >::iterator
      it = to_substract.begin(); it != to_substract.end(); ++it)
  {
    sort(it->second.begin(), it->second.end());
    vector< TObject > other;
    other.swap(result[it->first]);
    sort(other.begin(), other.end());
    set_difference(other.begin(), other.end(), it->second.begin(), it->second.end(),
                   back_inserter(result[it->first]));
  }
}

//-----------------------------------------------------------------------------

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


#endif
