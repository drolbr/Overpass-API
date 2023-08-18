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

#ifndef DE__OSM3S___OVERPASS_API__DATA__ABSTRACT_PROCESSING_H
#define DE__OSM3S___OVERPASS_API__DATA__ABSTRACT_PROCESSING_H

#include "../core/datatypes.h"
#include "../statements/statement.h"
#include "filenames.h"


//-----------------------------------------------------------------------------

template < class Object, class TPredicateA, class TPredicateB >
class And_Predicate
{
public:
  And_Predicate(const TPredicateA& predicate_a_, const TPredicateB& predicate_b_)
      : predicate_a(predicate_a_), predicate_b(predicate_b_) {}
  bool match(const Object& obj) const { return (predicate_a.match(obj) && predicate_b.match(obj)); }
  bool match(const Handle< Object >& h) const { return (predicate_a.match(h) && predicate_b.match(h)); }
  bool match(const Handle< Attic< Object > >& h) const { return (predicate_a.match(h) && predicate_b.match(h)); }

private:
  TPredicateA predicate_a;
  TPredicateB predicate_b;
};

template < class Object, class TPredicateA, class TPredicateB >
class Or_Predicate
{
public:
  Or_Predicate(const TPredicateA& predicate_a_, const TPredicateB& predicate_b_)
      : predicate_a(predicate_a_), predicate_b(predicate_b_) {}
  bool match(const Object& obj) const { return (predicate_a.match(obj) || predicate_b.match(obj)); }
  bool match(const Handle< Object >& h) const { return (predicate_a.match(h) || predicate_b.match(h)); }
  bool match(const Handle< Attic< Object > >& h) const { return (predicate_a.match(h) || predicate_b.match(h)); }

private:
  TPredicateA predicate_a;
  TPredicateB predicate_b;
};

template < class Object, class TPredicateA >
class Not_Predicate
{
public:
  Not_Predicate(const TPredicateA& predicate_a_)
      : predicate_a(predicate_a_) {}
  bool match(const Object& obj) const { return (!predicate_a.match(obj)); }
  bool match(const Handle< Object >& h) const { return (!predicate_a.match(h)); }
  bool match(const Handle< Attic< Object > >& h) const { return (!predicate_a.match(h)); }

private:
  TPredicateA predicate_a;
};

template < class Object >
class Trivial_Predicate
{
public:
  Trivial_Predicate() {}
  bool match(const Object& obj) const { return true; }
  bool match(const Handle< Object >& h) const { return true; }
  bool match(const Handle< Attic< Object > >& h) const { return true; }
  bool possible() const { return true; }
};

//-----------------------------------------------------------------------------

template < class Object >
class Id_Predicate
{
public:
  Id_Predicate(const std::vector< typename Object::Id_Type >& ids_)
    : ids(ids_) {}
  bool match(const Object& obj) const { return std::binary_search(ids.begin(), ids.end(), obj.id); }
  bool match(const Handle< Object >& h) const { return std::binary_search(ids.begin(), ids.end(), h.id()); }
  bool match(const Handle< Attic< Object > >& h) const { return std::binary_search(ids.begin(), ids.end(), h.id()); }
  bool possible() const { return !ids.empty(); }

private:
  const std::vector< typename Object::Id_Type >& ids;
};

//-----------------------------------------------------------------------------

inline bool has_a_child_with_id
    (const Relation_Skeleton& relation, const std::vector< Uint64 >& ids, uint32 type)
{
  for (std::vector< Relation_Entry >::const_iterator it3(relation.members.begin());
      it3 != relation.members.end(); ++it3)
  {
    if (it3->type == type &&
        std::binary_search(ids.begin(), ids.end(), it3->ref))
      return true;
  }
  return false;
}


inline bool has_a_child_with_id_and_role
    (const Relation_Skeleton& relation, const std::vector< Uint64 >& ids, uint32 type, uint32 role_id)
{
  for (std::vector< Relation_Entry >::const_iterator it3(relation.members.begin());
      it3 != relation.members.end(); ++it3)
  {
    if (it3->type == type && it3->role == role_id &&
        std::binary_search(ids.begin(), ids.end(), it3->ref))
      return true;
  }
  return false;
}


inline bool has_a_child_with_id
    (const Way_Skeleton& way, const std::vector< int >* pos, const std::vector< Node::Id_Type >& ids)
{
  if (pos)
  {
    std::vector< int >::const_iterator it3 = pos->begin();
    for (; it3 != pos->end() && *it3 < 0; ++it3)
    {
      if (*it3 + (int)way.nds.size() >= 0 &&
          std::binary_search(ids.begin(), ids.end(), way.nds[*it3 + way.nds.size()]))
        return true;
    }
    for (; it3 != pos->end(); ++it3)
    {
      if (*it3 > 0 && *it3 < (int)way.nds.size()+1 &&
          std::binary_search(ids.begin(), ids.end(), way.nds[*it3-1]))
        return true;
    }
  }
  else
  {
    for (std::vector< Node::Id_Type >::const_iterator it3(way.nds.begin());
        it3 != way.nds.end(); ++it3)
    {
      if (std::binary_search(ids.begin(), ids.end(), *it3))
        return true;
    }
  }
  return false;
}


class Get_Parent_Rels_Predicate
{
public:
  Get_Parent_Rels_Predicate(const std::vector< Uint64 >& ids_, uint32 child_type_)
    : ids(ids_), child_type(child_type_) {}
  bool match(const Relation_Skeleton& obj) const
  { return has_a_child_with_id(obj, ids, child_type); }
  bool match(const Handle< Relation_Skeleton >& h) const
  { return has_a_child_with_id(h.object(), ids, child_type); }
  bool match(const Handle< Attic< Relation_Skeleton > >& h) const
  { return has_a_child_with_id(h.object(), ids, child_type); }

private:
  const std::vector< Uint64 >& ids;
  uint32 child_type;
};


class Get_Parent_Rels_Role_Predicate
{
public:
  Get_Parent_Rels_Role_Predicate(const std::vector< Uint64 >& ids_, uint32 child_type_, uint32 role_id_)
    : ids(ids_), child_type(child_type_), role_id(role_id_) {}
  bool match(const Relation_Skeleton& obj) const
  { return has_a_child_with_id_and_role(obj, ids, child_type, role_id); }
  bool match(const Handle< Relation_Skeleton >& h) const
  { return has_a_child_with_id_and_role(h.object(), ids, child_type, role_id); }
  bool match(const Handle< Attic< Relation_Skeleton > >& h) const
  { return has_a_child_with_id_and_role(h.object(), ids, child_type, role_id); }

private:
  const std::vector< Uint64 >& ids;
  uint32 child_type;
  uint32 role_id;
};


class Get_Parent_Ways_Predicate
{
public:
  Get_Parent_Ways_Predicate(const std::vector< Node::Id_Type >& ids_, const std::vector< int >* pos_)
    : ids(ids_), pos(pos_) {}
  bool match(const Way_Skeleton& obj) const { return has_a_child_with_id(obj, pos, ids); }
  bool match(const Handle< Way_Skeleton >& h) const { return has_a_child_with_id(h.object(), pos, ids); }
  bool match(const Handle< Attic< Way_Skeleton > >& h) const { return has_a_child_with_id(h.object(), pos, ids); }

private:
  const std::vector< Node::Id_Type >& ids;
  const std::vector< int >* pos;
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
    (std::map< TIndex, std::vector< Attic< TObject > > >& attic_result,
     std::map< TIndex, std::vector< TObject > >& result,
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
void filter_items(const TPredicate& predicate, std::map< TIndex, std::vector< TObject > >& data)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = data.begin();
  it != data.end(); ++it)
  {
    std::vector< TObject > local_into;
    for (typename std::vector< TObject >::const_iterator iit = it->second.begin();
    iit != it->second.end(); ++iit)
    {
      if (predicate.match(*iit))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

template< class TIndex, class TObject >
std::vector< typename TObject::Id_Type > filter_for_ids(const std::map< TIndex, std::vector< TObject > >& elems)
{
  std::vector< typename TObject::Id_Type > ids;
  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator it = elems.begin();
  it != elems.end(); ++it)
  {
    for (typename std::vector< TObject >::const_iterator iit = it->second.begin();
    iit != it->second.end(); ++iit)
    ids.push_back(iit->id);
  }
  std::sort(ids.begin(), ids.end());

  return ids;
}

//-----------------------------------------------------------------------------


template< typename TObject >
struct Compare_By_Id
{
  bool operator()(const TObject& lhs, const TObject& rhs) { return lhs.id < rhs.id; }
};


template< class TIndex, class TObject >
bool indexed_set_union(std::map< TIndex, std::vector< TObject > >& result,
		       const std::map< TIndex, std::vector< TObject > >& summand)
{
  bool result_has_grown = false;

  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
      it = summand.begin(); it != summand.end(); ++it)
  {
    if (it->second.empty())
      continue;

    std::vector< TObject >& target = result[it->first];
    if (target.empty())
    {
      target = it->second;
      result_has_grown = true;
      continue;
    }

    if (it->second.size() == 1 && target.size() > 64)
    {
      typename std::vector< TObject >::iterator it_target
          = std::lower_bound(target.begin(), target.end(), it->second.front());
      if (it_target == target.end())
      {
        target.push_back(it->second.front());
        result_has_grown = true;
      }
      else if (!(*it_target == it->second.front()))
      {
        target.insert(it_target, it->second.front());
        result_has_grown = true;
      }
    }
    else
    {
      std::vector< TObject > other;
      other.swap(target);
      std::set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
                back_inserter(target), Compare_By_Id< TObject >());

      result_has_grown |= (target.size() > other.size());
    }
  }

  return result_has_grown;
}

//-----------------------------------------------------------------------------

template< class TIndex, class TObject >
void indexed_set_difference(std::map< TIndex, std::vector< TObject > >& result,
                            const std::map< TIndex, std::vector< TObject > >& to_substract)
{
  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
      it = to_substract.begin(); it != to_substract.end(); ++it)
  {
    std::vector< TObject > other;
    other.swap(result[it->first]);
    std::sort(other.begin(), other.end());
    std::set_difference(other.begin(), other.end(), it->second.begin(), it->second.end(),
                   back_inserter(result[it->first]));
  }
}

#endif
