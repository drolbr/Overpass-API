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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__META_COLLECTOR_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__META_COLLECTOR_H

#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "filenames.h"


template< typename Index, typename Id_Type >
struct Meta_Collector
{
public:
  template< typename Object >
  Meta_Collector(const std::map< Index, std::vector< Object > >& items,
      Transaction& transaction, const File_Properties* meta_file_prop = 0);

  Meta_Collector(const Ranges< Index >& used_ranges,
      Transaction& transaction, const File_Properties* meta_file_prop = 0);

  void reset();
  const OSM_Element_Metadata_Skeleton< Id_Type >* get
      (const Index& index, Id_Type ref);
  const OSM_Element_Metadata_Skeleton< Id_Type >* get
      (const Index& index, Id_Type ref, uint64 timestamp);

  ~Meta_Collector()
  {
    delete last_index;
    delete current_index;
    delete range_it;
    delete db_it;
    delete meta_db;
  }

private:
  std::set< Index > used_indices;
  Ranges< Index > used_ranges;
  Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >, typename std::set< Index >::const_iterator >* meta_db;
  typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >, typename std::set< Index >::const_iterator >
      ::Discrete_Iterator* db_it;
  typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >, typename std::set< Index >::const_iterator >
      ::Range_Iterator* range_it;
  Index* current_index;
  Index* last_index;
  std::set< OSM_Element_Metadata_Skeleton< Id_Type > > current_objects;

  void update_current_objects(const Index&);
};


template< typename Index, typename Object >
struct Attic_Meta_Collector
{
public:
  Attic_Meta_Collector(const std::map< Index, std::vector< Attic< Object > > >& items,
                       Transaction& transaction, bool turn_on);

  const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* get
      (const Index& index, typename Object::Id_Type ref, uint64 timestamp = NOW);

private:
  Meta_Collector< Index, typename Object::Id_Type > current;
  Meta_Collector< Index, typename Object::Id_Type > attic;
};


/** Implementation --------------------------------------------------------- */

template< typename Index, typename Object >
void generate_index_query
  (std::set< Index >& indices,
   const std::map< Index, std::vector< Object > >& items)
{
  for (auto it = items.begin(); it != items.end(); ++it)
    indices.insert(it->first);
}


template< typename Index, typename Id_Type >
template< typename Object >
Meta_Collector< Index, Id_Type >::Meta_Collector
    (const std::map< Index, std::vector< Object > >& items,
     Transaction& transaction, const File_Properties* meta_file_prop)
  : meta_db(0), db_it(0), range_it(0), current_index(0), last_index(0)
{
  if (!meta_file_prop)
    return;

  generate_index_query(used_indices, items);
  meta_db = new Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >, typename std::set< Index >::const_iterator >
      (transaction.data_index(meta_file_prop));

  reset();
}


template< typename Index, typename Id_Type >
Meta_Collector< Index, Id_Type >::Meta_Collector
    (const Ranges< Index >& used_ranges_,
     Transaction& transaction, const File_Properties* meta_file_prop)
  : used_ranges(used_ranges_), meta_db(0), db_it(0), range_it(0), current_index(0), last_index(0)
{
  if (!meta_file_prop)
    return;

  meta_db = new Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >, typename std::set< Index >::const_iterator >
      (transaction.data_index(meta_file_prop));

  reset();
}


template< typename Index, typename Id_Type >
void Meta_Collector< Index, Id_Type >::reset()
{
  if (!meta_db)
    return;

  delete db_it;
  db_it = 0;
  delete range_it;
  range_it = 0;
  delete current_index;
  current_index = 0;
  delete last_index;
  last_index = 0;

  if (used_ranges.empty())
  {
    if (!used_indices.empty())
      last_index = new Index(*used_indices.begin());

    db_it = new typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >, typename std::set< Index >::const_iterator >
        ::Discrete_Iterator(meta_db->discrete_begin(used_indices.begin(), used_indices.end()));

    if (!(*db_it == meta_db->discrete_end()))
      current_index = new Index(db_it->index());
    while (!(*db_it == meta_db->discrete_end()) && (*current_index == db_it->index()))
    {
      current_objects.insert(db_it->object());
      ++(*db_it);
    }
  }
  else
  {
    last_index = new Index(used_ranges.begin().lower_bound());
    range_it = new auto(meta_db->range_begin(used_ranges));

    if (!(*range_it == meta_db->range_end()))
      current_index = new Index(range_it->index());
    while (!(*range_it == meta_db->range_end()) && (*current_index == range_it->index()))
    {
      current_objects.insert(range_it->object());
      ++(*range_it);
    }
  }
}


template< typename Index, typename Id_Type >
void Meta_Collector< Index, Id_Type >::update_current_objects(const Index& index)
{
  current_objects.clear();
  if (last_index)
    *last_index = index;

  if (db_it)
  {
    while (!(*db_it == meta_db->discrete_end()) && (db_it->index() < index))
      ++(*db_it);
    if (!(*db_it == meta_db->discrete_end()))
      *current_index = db_it->index();
    while (!(*db_it == meta_db->discrete_end()) && (*current_index == db_it->index()))
    {
      current_objects.insert(db_it->object());
      ++(*db_it);
    }
  }
  else if (range_it)
  {
    while (!(*range_it == meta_db->range_end()) && (range_it->index() < index))
      ++(*range_it);
    if (!(*range_it == meta_db->range_end()))
      *current_index = range_it->index();
    while (!(*range_it == meta_db->range_end()) && (*current_index == range_it->index()))
    {
      current_objects.insert(range_it->object());
      ++(*range_it);
    }
  }
}


template< typename Index, typename Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Meta_Collector< Index, Id_Type >::get
    (const Index& index, Id_Type ref)
{
  if (!meta_db)
    return 0;

  if (current_index && index < *last_index)
    reset();
  if (current_index && *current_index < index)
    update_current_objects(index);

  typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::iterator it
      = current_objects.lower_bound(OSM_Element_Metadata_Skeleton< Id_Type >(ref));
  if (it != current_objects.end() && it->ref == ref)
    return &*it;
  else
    return 0;
}


template< typename Index, typename Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Meta_Collector< Index, Id_Type >::get
    (const Index& index, Id_Type ref, uint64 timestamp)
{
  if (!meta_db)
    return 0;

  if (current_index && index < *last_index)
    reset();
  if (current_index && *current_index < index)
    update_current_objects(index);

  typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::iterator it
      = current_objects.lower_bound(OSM_Element_Metadata_Skeleton< Id_Type >(ref, timestamp));
  if (it == current_objects.begin())
    return 0;
  --it;
  if (it->ref == ref)
    return &*it;
  else
    return 0;
}


template< typename Index, typename Object >
Attic_Meta_Collector< Index, Object >::Attic_Meta_Collector(
    const std::map< Index, std::vector< Attic< Object > > >& items, Transaction& transaction, bool turn_on)
    : current(items, transaction, turn_on ? current_meta_file_properties< Object >() : 0),
    attic(items, transaction, turn_on ? attic_meta_file_properties< Object >() : 0)
{}


template< typename Index, typename Object >
const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* Attic_Meta_Collector< Index, Object >::get(
    const Index& index, typename Object::Id_Type ref, uint64 timestamp)
{
  const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
      = current.get(index, ref, timestamp);
  if (meta)
    return meta;
  return attic.get(index, ref, timestamp);
}


#endif
