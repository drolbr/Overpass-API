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
#include <memory>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../core/type_meta.h"
#include "filenames.h"
#include "request_context.h"


template< typename Index, typename Id_Type >
struct Meta_File_Reader
{
public:
  template< typename Object >
  Meta_File_Reader(const std::map< Index, std::vector< Object > >& items,
      Transaction& transaction, const File_Properties& meta_file_prop = 0);

  void read_objects(const Index&, std::vector< OSM_Element_Metadata_Skeleton< Id_Type > >&);
  const Index* get_last_index() const { return last_index.get(); }

private:
  std::vector< Index > used_indices;

  std::unique_ptr< Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > > > plain_meta_db;
  std::unique_ptr< typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >
      ::Discrete_Iterator > plain_db_it;

  std::unique_ptr< Block_Backend< Index, Meta_Per_Changeset_Skeleton > > cset_meta_db;
  std::unique_ptr< typename Block_Backend< Index, Meta_Per_Changeset_Skeleton >::Discrete_Iterator > cset_db_it;

  std::unique_ptr< Index > last_index;
};


template< typename Index, typename Id_Type >
struct Meta_Collector
{
public:
  template< typename Object >
  Meta_Collector(const std::map< Index, std::vector< Object > >& items,
      Transaction& transaction, const File_Properties& meta_file_prop);

  const OSM_Element_Metadata_Skeleton< Id_Type >* get
      (const Index& index, Id_Type ref);
  const OSM_Element_Metadata_Skeleton< Id_Type >* get
      (const Index& index, Id_Type ref, uint64 timestamp);

private:
  Meta_File_Reader< Index, Id_Type > db_reader;
  std::vector< OSM_Element_Metadata_Skeleton< Id_Type > > current_objects;
};


template< typename Index, typename Object >
struct Attic_Meta_Collector
{
public:
  Attic_Meta_Collector(const std::map< Index, std::vector< Attic< Object > > >& items, Transaction& transaction);

  const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* get
      (const Index& index, typename Object::Id_Type ref, uint64 timestamp = NOW);

private:
  Meta_Collector< Index, typename Object::Id_Type > current;
  Meta_Collector< Index, typename Object::Id_Type > attic;
};


/** Implementation --------------------------------------------------------- */

template< typename Index, typename Object >
std::vector< Index > generate_index_query
  (const std::map< Index, std::vector< Object > >& items)
{
  std::vector< Index > result;
  for (auto it = items.begin(); it != items.end(); ++it)
    result.push_back(it->first);
  return result;
}


template< typename Index, typename Id_Type >
template< typename Object >
Meta_File_Reader< Index, Id_Type >::Meta_File_Reader
    (const std::map< Index, std::vector< Object > >& items,
     Transaction& transaction, const File_Properties& meta_file_prop)
  : used_indices(generate_index_query(items))
{
  auto meta_idx = transaction.data_index(&meta_file_prop);
  if (meta_idx->get_file_format_version() <= 7620)
  {
    plain_meta_db.reset(
        new Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type > >(meta_idx));
    plain_db_it.reset(new auto(plain_meta_db->discrete_begin(used_indices.begin(), used_indices.end())));
  }
  else
  {
    cset_meta_db.reset(new Block_Backend< Index, Meta_Per_Changeset_Skeleton >(meta_idx));
    cset_db_it.reset(new auto(cset_meta_db->discrete_begin(used_indices.begin(), used_indices.end())));
  }
}


template< typename Index, typename Id_Type >
template< typename Object >
Meta_Collector< Index, Id_Type >::Meta_Collector
    (const std::map< Index, std::vector< Object > >& items,
     Transaction& transaction, const File_Properties& meta_file_prop)
  : db_reader(items, transaction, meta_file_prop) {}


template< typename Index, typename Id_Type >
void Meta_File_Reader< Index, Id_Type >::read_objects(
    const Index& index, std::vector< OSM_Element_Metadata_Skeleton< Id_Type > >& current_objects)
{
  if (!last_index)
    last_index.reset(new Index(index));
  else
  {
    if (!(*last_index < index))
    {
      if (plain_meta_db)
        *plain_db_it = plain_meta_db->discrete_begin(used_indices.begin(), used_indices.end());
      else
        *cset_db_it = cset_meta_db->discrete_begin(used_indices.begin(), used_indices.end());
    }
    *last_index = index;
  }

  if (plain_db_it)
  {
    while (!(*plain_db_it == plain_meta_db->discrete_end()) && (plain_db_it->index() < index))
      ++(*plain_db_it);
    while (!(*plain_db_it == plain_meta_db->discrete_end()) && (index == plain_db_it->index()))
    {
      current_objects.push_back(plain_db_it->object());
      ++(*plain_db_it);
    }
  }
  else
  {
    while (!(*cset_db_it == cset_meta_db->discrete_end()) && (cset_db_it->index() < index))
      ++(*cset_db_it);
    while (!(*cset_db_it == cset_meta_db->discrete_end()) && (cset_db_it->index() == index))
    {
      const auto& cset = cset_db_it->object();
      for (auto i : cset.get_refs())
        current_objects.push_back(OSM_Element_Metadata_Skeleton< Id_Type >(cset, i));
      ++(*cset_db_it);      
    }
  }
}


template< typename Index, typename Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Meta_Collector< Index, Id_Type >::get
    (const Index& index, Id_Type ref)
{
  if (!db_reader.get_last_index() || !(index == *db_reader.get_last_index()))
  {
    current_objects.clear();
    db_reader.read_objects(index, current_objects);
    std::sort(current_objects.begin(), current_objects.end());
  }

  auto it = std::lower_bound(
      current_objects.begin(), current_objects.end(), OSM_Element_Metadata_Skeleton< Id_Type >(ref));
  if (it != current_objects.end() && it->ref == ref)
    return &*it;
  else
    return 0;
}


template< typename Index, typename Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Meta_Collector< Index, Id_Type >::get
    (const Index& index, Id_Type ref, uint64 timestamp)
{
  if (!db_reader.get_last_index() || !(index == *db_reader.get_last_index()))
  {
    current_objects.clear();
    db_reader.read_objects(index, current_objects);
    std::sort(current_objects.begin(), current_objects.end());
  }

  auto it = std::lower_bound(
      current_objects.begin(), current_objects.end(), OSM_Element_Metadata_Skeleton< Id_Type >(ref, timestamp));
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
    const std::map< Index, std::vector< Attic< Object > > >& items, Transaction& transaction)
    : current(items, transaction, *current_meta_file_properties< Object >()),
    attic(items, transaction, *attic_meta_file_properties< Object >())
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


template< typename Index, typename Skeleton >
class Idx_Cached_Meta_Collector
{
public:
  Idx_Cached_Meta_Collector(
      const std::map< Index, std::vector< Skeleton > >& current_items,
      const std::map< Index, std::vector< Attic< Skeleton > > >& attic_items,
      uint64_t timestamp, Request_Context& context);
  
  const Full_Monotype_Meta* get(Index idx, uint64_t ref);
  
private:
  std::vector< Index > current_req;
  std::vector< Index > attic_req;

  std::unique_ptr< Block_Backend< Index, Meta_Per_Changeset_Skeleton > > current_db;
  std::unique_ptr< typename Block_Backend< Index, Meta_Per_Changeset_Skeleton >::Discrete_Iterator > current_db_it;
  std::unique_ptr< Block_Backend< Index, Meta_Per_Changeset_Skeleton > > attic_db;
  std::unique_ptr< typename Block_Backend< Index, Meta_Per_Changeset_Skeleton >::Discrete_Iterator > attic_db_it;

  bool idx_valid;
  Index ref_idx;
  std::vector< Full_Monotype_Meta > cache;
  
  uint64_t timestamp;
  const std::map< Index, std::vector< Skeleton > >& current_items;
  const std::map< Index, std::vector< Attic< Skeleton > > >& attic_items;  
};


template< typename Index >
class Chunked_Meta_Collector
{
public:
  template< typename Skeleton >
  void prefetch_chunk(
      const std::map< Index, std::vector< Skeleton > >& current_items,
      const std::map< Index, std::vector< Attic< Skeleton > > >& attic_items,
      uint64_t lower_id_bound, uint64_t upper_id_bound, uint64_t timestamp,
      Request_Context& context);
  
  const Full_Monotype_Meta* get(Index idx, uint64_t ref) const;
  
private:
  std::map< Index, std::vector< Full_Monotype_Meta > > chunk;
};


#endif
