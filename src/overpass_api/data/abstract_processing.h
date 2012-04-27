/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__DATA__ABSTRACT_PROCESSING_H
#define DE__OSM3S___OVERPASS_API__DATA__ABSTRACT_PROCESSING_H

#include "../core/datatypes.h"
#include "../statements/statement.h"

using namespace std;

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
    
  private:
    TPredicateA predicate_a;
};

template < class TObject >
class Trivial_Predicate
{
  public:
    Trivial_Predicate() {}
    bool match(const TObject& obj) const { return true; }
};

//-----------------------------------------------------------------------------

template < class TObject >
class Id_Predicate
{
  public:
    Id_Predicate(const vector< uint32 >& ids_) : ids(ids_) {}
    bool match(const TObject& obj) const
    {
      return binary_search(ids.begin(), ids.end(), obj.id);
    }
    
  private:
    const vector< uint32 >& ids;
};

//-----------------------------------------------------------------------------

inline bool has_a_child_with_id
    (const Relation_Skeleton& relation, const vector< uint32 >& ids, uint32 type)
{
  for (vector< Relation_Entry >::const_iterator it3(relation.members.begin());
      it3 != relation.members.end(); ++it3)
  {
    if ((it3->type == type) &&
        (binary_search(ids.begin(), ids.end(), it3->ref)))
      return true;
  }
  return false;
}

inline bool has_a_child_with_id
    (const Way_Skeleton& way, const vector< uint32 >& ids)
{
  for (vector< uint32 >::const_iterator it3(way.nds.begin());
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
  Get_Parent_Rels_Predicate(const vector< uint32 >& ids_, uint32 child_type_)
    : ids(ids_), child_type(child_type_) {}
  bool match(const Relation_Skeleton& obj) const
  {
    return has_a_child_with_id(obj, ids, child_type);
  }
  
private:
  const vector< uint32 >& ids;
  uint32 child_type;
};

class Get_Parent_Ways_Predicate
{
public:
  Get_Parent_Ways_Predicate(const vector< uint32 >& ids_)
    : ids(ids_) {}
  bool match(const Way_Skeleton& obj) const
  {
    return has_a_child_with_id(obj, ids);
  }
  
private:
  const vector< uint32 >& ids;
};

//-----------------------------------------------------------------------------

template < class TIndex, class TObject, class TContainer, class TPredicate >
void collect_items_discrete(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const TContainer& req, const TPredicate& predicate,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject, typename TContainer::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< TIndex, TObject, typename TContainer
      ::const_iterator >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      rman.health_check(stmt);
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }
}

template < class TIndex, class TObject, class TContainer, class TPredicate >
void collect_items_range(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const TContainer& req, const TPredicate& predicate,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject, typename TContainer::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< TIndex, TObject, typename TContainer
      ::const_iterator >::Range_Iterator
      it(db.range_begin(req.begin(), req.end()));
	   !(it == db.range_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      rman.health_check(stmt);
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }
}

template < class TIndex, class TObject, class TPredicate >
void collect_items_flat(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties, const TPredicate& predicate,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject > db
      (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
  for (typename Block_Backend< TIndex, TObject >::Flat_Iterator
      it(db.flat_begin()); !(it == db.flat_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      rman.health_check(stmt);
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
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
vector< uint32 > filter_for_ids(const map< TIndex, vector< TObject > >& elems)
{
  vector< uint32 > ids;
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

#endif
