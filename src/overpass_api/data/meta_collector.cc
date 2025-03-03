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

#include "meta_collector.h"


template
class Idx_Cached_Meta_Collector< Uint32_Index, Node_Skeleton >;
template
class Idx_Cached_Meta_Collector< Uint31_Index, Way_Skeleton >;
template
class Idx_Cached_Meta_Collector< Uint31_Index, Relation_Skeleton >;


template
class Chunked_Meta_Collector< Uint32_Index >;
template
void Chunked_Meta_Collector< Uint32_Index >::prefetch_chunk< Node_Skeleton >(
    const std::map< Uint32_Index, std::vector< Node_Skeleton > >& current_items,
    const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_items,
    uint64_t lower_id_bound, uint64_t upper_id_bound, uint64_t timestamp,
    Request_Context& context);

template
class Chunked_Meta_Collector< Uint31_Index >;
template
void Chunked_Meta_Collector< Uint31_Index >::prefetch_chunk< Way_Skeleton >(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& current_items,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_items,
    uint64_t lower_id_bound, uint64_t upper_id_bound, uint64_t timestamp,
    Request_Context& context);
template
void Chunked_Meta_Collector< Uint31_Index >::prefetch_chunk< Relation_Skeleton >(
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& current_items,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_items,
    uint64_t lower_id_bound, uint64_t upper_id_bound, uint64_t timestamp,
    Request_Context& context);


namespace
{
  template< typename Index, typename Unused >
  std::vector< Index > extract_idxs(
      const std::map< Index, Unused >& to_add)
  {
    std::vector< Index > result;

    for (const auto& i : to_add)
      result.push_back(i.first);

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
  }
  
  
  template< typename Index, typename Unused1, typename Unused2 >
  std::vector< Index > extract_idxs(
      const std::map< Index, Unused1 >& to_add_1,
      const std::map< Index, Unused2 >& to_add_2)
  {
    std::vector< Index > result;

    for (const auto& i : to_add_1)
      result.push_back(i.first);
    for (const auto& i : to_add_2)
      result.push_back(i.first);

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
  }
  
  
  template< typename Index, typename Discrete_Iterator >
  void eval_idx_for_chunk(
      Index idx, uint64_t timestamp,
      std::vector< Full_Monotype_Meta >& to, Discrete_Iterator& db_it, const Discrete_Iterator& db_end)
  {
    while (!(db_it == db_end) && db_it.index() < idx)
      ++db_it;
    while (!(db_it == db_end) && db_it.index() == idx)
    {
      auto& changeset = db_it.object();
      for (const auto& entry : changeset.get_refs())
      {
        if (entry.timestamp <= timestamp)
        {
          auto to_it = std::lower_bound(to.begin(), to.end(), Full_Monotype_Meta{ entry.ref });
          if (to_it != to.end() && to_it->ref == entry.ref && to_it->version < entry.version)
            *to_it = { entry.ref, entry.version, entry.timestamp,
                changeset.get_is_redacted(), changeset.get_changeset(), changeset.get_user_id() };
        }
      }
      ++db_it;
    }
  }
  
  
  template< typename Index >
  void eval_db_for_chunk(
      const std::vector< Index >& req,
      Block_Backend< Index, Meta_Per_Changeset_Skeleton, typename std::vector< Index >::const_iterator >& db,
      std::map< Index, std::vector< Full_Monotype_Meta > >& chunk,
      uint64_t lower_id_bound, uint64_t upper_id_bound, uint64_t timestamp)
  {
    auto db_it = db.discrete_begin(req.begin(), req.end());
    
    for (auto idx : req)
    {
      auto& to = chunk[idx];
      
      while (!(db_it == db.discrete_end()) && db_it.index() == idx)
      {
        auto& changeset = db_it.object();
        for (const auto& entry : changeset.get_refs())
        {
          if (lower_id_bound <= entry.ref && entry.ref <= upper_id_bound && entry.timestamp <= timestamp)
          {
            auto to_it = std::lower_bound(to.begin(), to.end(), Full_Monotype_Meta{ entry.ref });
            if (to_it != to.end() && to_it->ref == entry.ref && to_it->version < entry.version)
              *to_it = { entry.ref, entry.version, entry.timestamp,
                  changeset.get_is_redacted(), changeset.get_changeset(), changeset.get_user_id() };
          }
        }
        ++db_it;
      }
    }
  }
  
  
  template< typename Container >
  void populate_chunk(
      const Container& item_container, 
      std::vector< Full_Monotype_Meta >& to,
      uint64_t lower_id_bound, uint64_t upper_id_bound)
  {
    for (const auto& item : item_container)
    {
      if (lower_id_bound <= item.id.val() && item.id.val() <= upper_id_bound)
        to.push_back(Full_Monotype_Meta{ item.id.val() });
    }
  }
}


template< typename Index, typename Skeleton >
Idx_Cached_Meta_Collector< Index, Skeleton >::Idx_Cached_Meta_Collector(
    const std::map< Index, std::vector< Skeleton > >* current_items_,
    const std::map< Index, std::vector< Attic< Skeleton > > >* attic_items_,
    uint64_t timestamp_, Request_Context& context)
    : current_req(extract_idxs(
          current_items_ ? *current_items_ : decltype(*current_items_){},
          attic_items_ ? *attic_items_ : decltype(*attic_items_){})),
      attic_req(extract_idxs(attic_items_ ? *attic_items_ : decltype(*attic_items_){})),
      current_db(current_req.empty() ? nullptr
          : new Block_Backend< Index, Meta_Per_Changeset_Skeleton >(
                context.data_index(current_meta_file_properties< Skeleton >()))),
      attic_db((current_req.empty() && attic_req.empty()) ? nullptr
          : new Block_Backend< Index, Meta_Per_Changeset_Skeleton >(
                context.data_index(attic_meta_file_properties< Skeleton >()))),
      idx_valid(false), ref_idx((uint32_t)0),
      timestamp(timestamp_), current_items(current_items_), attic_items(attic_items_)
{
  if (current_db)
    current_db_it.reset(new auto(current_db->discrete_begin(current_req.begin(), current_req.end())));
  if (attic_db)
    attic_db_it.reset(new auto(attic_db->discrete_begin(attic_req.begin(), attic_req.end())));
}


template< typename Index, typename Skeleton >
const Full_Monotype_Meta* Idx_Cached_Meta_Collector< Index, Skeleton >::get(Index idx, uint64_t ref)
{
  if (idx_valid && idx < ref_idx)
  {
    if (current_db)
      *current_db_it = current_db->discrete_begin(current_req.begin(), current_req.end());
    if (attic_db)
      *attic_db_it = attic_db->discrete_begin(attic_req.begin(), attic_req.end());
  }
  if (!idx_valid || !(ref_idx == idx))
  {
    cache.clear();

    if (current_items)
    {
      auto current_it = current_items->find(idx);
      if (current_it != current_items->end())
      {
        for (const auto& item : current_it->second)
          cache.push_back(Full_Monotype_Meta{ item.id.val() });
      }
    }
    if (attic_items)
    {
      auto attic_it = attic_items->find(idx);
      if (attic_it != attic_items->end())
      {
        for (const auto& item : attic_it->second)
          cache.push_back(Full_Monotype_Meta{ item.id.val() });
      }
    }
    std::sort(cache.begin(), cache.end());

    if (current_db_it)
      eval_idx_for_chunk(idx, timestamp, cache, *current_db_it, current_db->discrete_end());
    if (attic_db_it)
      eval_idx_for_chunk(idx, timestamp, cache, *attic_db_it, attic_db->discrete_end());
  }
  idx_valid = true;
  ref_idx = idx;
  
  auto ref_it = std::lower_bound(cache.begin(), cache.end(), Full_Monotype_Meta{ ref });
  if (ref_it == cache.end() || ref_it->ref != ref)
    return nullptr;
  return &*ref_it;
}


template< typename Index >
template< typename Skeleton >
void Chunked_Meta_Collector< Index >::prefetch_chunk(
    const std::map< Index, std::vector< Skeleton > >& current_items,
    const std::map< Index, std::vector< Attic< Skeleton > > >& attic_items,
    uint64_t lower_id_bound, uint64_t upper_id_bound, uint64_t timestamp,
    Request_Context& context)
{
  chunk.clear();

  if (!current_items.empty() || !attic_items.empty())
  {
    std::vector< Index > req = extract_idxs(current_items, attic_items);  
    
    auto current_items_it = current_items.begin();
    auto attic_items_it = attic_items.begin();
    for (auto idx : req)
    {
      auto& to = chunk[idx];
      
      while (current_items_it != current_items.end() && current_items_it->first < idx)
        ++current_items_it;
      if (current_items_it != current_items.end() && current_items_it->first == idx)
        populate_chunk(current_items_it->second, to, lower_id_bound, upper_id_bound);
      
      while (attic_items_it != attic_items.end() && attic_items_it->first < idx)
        ++attic_items_it;
      if (attic_items_it != attic_items.end() && attic_items_it->first == idx)
        populate_chunk(attic_items_it->second, to, lower_id_bound, upper_id_bound);

      std::sort(to.begin(), to.end());
    }

    Block_Backend< Index, Meta_Per_Changeset_Skeleton, typename std::vector< Index >::const_iterator > db(
        context.data_index(current_meta_file_properties< Skeleton >()));
    eval_db_for_chunk(req, db, chunk, lower_id_bound, upper_id_bound, timestamp);
  }

  if (!attic_items.empty())
  {
    std::vector< Index > req = extract_idxs(attic_items);
    Block_Backend< Index, Meta_Per_Changeset_Skeleton, typename std::vector< Index >::const_iterator > db(
        context.data_index(attic_meta_file_properties< Skeleton >()));
    eval_db_for_chunk(req, db, chunk, lower_id_bound, upper_id_bound, timestamp);
  }
  
//   for (auto i : chunk)
//     for (auto j : i.second)
//       std::cout<<"DEBUG prefetch_chunk "<<std::hex<<i.first.val()<<'\t'<<std::dec<<j.ref<<'\t'<<j.version<<'\t'<<Timestamp(j.timestamp).str()<<'\t'<<j.is_redacted<<'\t'<<j.changeset<<'\t'<<j.uid<<'\n';
}


template< typename Index >
const Full_Monotype_Meta* Chunked_Meta_Collector< Index >::get(Index idx, uint64_t ref) const
{
  auto idx_it = chunk.find(idx);
  if (idx_it == chunk.end())
    return nullptr;
  auto ref_it = std::lower_bound(idx_it->second.begin(), idx_it->second.end(), Full_Monotype_Meta{ ref });
  if (ref_it == idx_it->second.end() || ref_it->ref != ref)
    return nullptr;
  return &*ref_it;
}
