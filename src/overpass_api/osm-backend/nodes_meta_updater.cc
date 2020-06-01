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


#include "nodes_meta_updater.h"


// Preconditions: none
// Remark: from several objects with the same id and timestamp only the highest version number is preserved
std::vector< New_Object_Meta_Context > objectlist_by_id(const Data_By_Id< Node_Skeleton >& new_data)
{
  Perflog_Tree perflog("objectlist_by_id");

  std::vector< New_Object_Meta_Context > result;
  result.reserve(new_data.data.size());

  for (const auto& item : new_data.data)
    result.push_back(New_Object_Meta_Context(item.meta, item.idx, item.elem.ll_lower));

  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  for (decltype(result.size()) i = 0; i+1 < result.size(); ++i)
  {
    if (result[i+1].meta.ref == result[i].meta.ref)
    {
      result[i].idx_of_next_version = result[i+1].own_idx;
      result[i].end_timestamp = result[i+1].meta.timestamp;
    }
  }

  return result;
}


std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
    add_new_current_nodes(const std::vector< New_Object_Meta_Context >& new_obj_context)
{
  Perflog_Tree perflog("add_new_current_nodes");

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > result;

  for (const auto& obj : new_obj_context)
  {
    if (obj.end_timestamp == 0)
      result[obj.own_idx].insert(obj.meta);
  }

  return result;
}


void write_nodes_mapfile(
    const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& new_meta,
    Transaction& transaction)
{
  Perflog_Tree perflog("write_nodes_mapfile");
  uint32 entry_count = 0;

  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_map_positions;
  for (const auto& per_idx : new_meta)
  {
    for (const auto& entry : per_idx.second)
      new_map_positions.push_back(std::make_pair(entry.ref, per_idx.first));

    entry_count += per_idx.second.size();
  }
  std::sort(
      new_map_positions.begin(), new_map_positions.end(),
      [](
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& lhs,
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& rhs)
      { return lhs.first < rhs.first; } );

  std::cerr<<"#idx:"<<new_meta.size()<<" #objs:"<<entry_count<<' ';

  {
    Perflog_Tree perflog("update_map_positions");
    update_map_positions(new_map_positions, transaction, *osm_base_settings().NODES);
  }
}


std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > read_current_mapfile(
    Transaction& transaction, const std::vector< New_Object_Meta_Context >& new_obj_context)
{
  Perflog_Tree perflog("read_current_mapfile");

  std::vector< Node_Skeleton::Id_Type > ids_to_update;
  ids_to_update.reserve(new_obj_context.size());
  for (const auto& i : new_obj_context)
  {
    if (ids_to_update.empty() || !(ids_to_update.back() == i.meta.ref))
      ids_to_update.push_back(i.meta.ref);
  }
  std::cerr<<"#id:"<<ids_to_update.size()<<' ';

  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_map_positions;
  {
    Perflog_Tree perflog("get_existing_map_positions");
    get_existing_map_positions(ids_to_update, transaction, *osm_base_settings().NODES)
        .swap(existing_map_positions);
  }

  std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > result;
  for (const auto& i : existing_map_positions)
    result[i.second].push_back(i.first);
  return result;
}


class Attic_Mapfile_IO
{
public:
  Attic_Mapfile_IO(Transaction& transaction_) :transaction(transaction_) {}
  std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > read_attic_mapfile(
      const std::vector< New_Object_Meta_Context >& new_obj_context);
  void write_attic_nodes_mapfile(
      const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& new_meta);

private:
  Transaction& transaction;
  std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists;
};


void Attic_Mapfile_IO::write_attic_nodes_mapfile(
    const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& new_meta)
{
  Perflog_Tree perflog("write_attic_nodes_mapfile");
  uint32 entry_count = 0;

  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_map_positions;
  for (const auto& per_idx : new_meta)
  {
    for (const auto& entry : per_idx.second)
    {
      if (new_map_positions.empty() || !(new_map_positions.back().first == entry.ref)
          || !(new_map_positions.back().second == per_idx.first))
        new_map_positions.push_back(std::make_pair(entry.ref, per_idx.first));
    }

    entry_count += per_idx.second.size();
  }
  std::sort(
      new_map_positions.begin(), new_map_positions.end(),
      [](
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& lhs,
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& rhs)
      { return lhs.first < rhs.first || !(rhs.first < lhs.first && lhs.second < rhs.second); } );

  std::cerr<<"#idx:"<<new_meta.size()<<" #objs:"<<entry_count<<' ';

  std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists;
  {
    auto from = new_map_positions.begin();
    auto to = new_map_positions.begin();
    while (from != new_map_positions.end())
    {
      auto i_exist = existing_idx_lists.find(from->first);
      if (i_exist != existing_idx_lists.end())
      {
        auto id = from->first;
        while (from != new_map_positions.end() && from->first == id)
        {
          if (i_exist->second.find(from->second) == i_exist->second.end())
            break;
          ++from;
        }
        if (from != new_map_positions.end() && from->first == id)
        {
          auto idx_list = i_exist->second;
          while (from != new_map_positions.end() && from->first == id)
          {
            idx_list.insert(from->second);
            ++from;
          }
          new_attic_idx_lists.insert(std::make_pair(id, idx_list));
          // keep entry in exising_idx_lists to mark it for deletion
          // Remove entries from new_map_positions by not copying.
        }
        else
          // All indices of the new objects are already present.
          // They are, as intended, deleted from new_map_positions by not copying.
          // The entry in exising_idx_lists shall be kept and therefore removed from the deletion list existing_idx_lists
          existing_idx_lists.erase(i_exist);
      }
      else
      {
        std::set< Uint31_Index > idx_list;
        auto next = from;
        ++next;
        while (next != new_map_positions.end() && from->first == next->first)
        {
          idx_list.insert(next->second);
        }
        if (idx_list.empty())
          *to = *from;
        else
        {
          idx_list.insert(from->second);
          new_attic_idx_lists.insert(std::make_pair(from->first, idx_list));
          *to = std::make_pair(from->first, 0xff);
        }
        ++from;
        ++to;
      }
    }
    new_map_positions.erase(to, new_map_positions.end());
  }

  {
    Perflog_Tree perflog("update_map_positions");
    update_map_positions(new_map_positions, transaction, *attic_settings().NODES);
  }
  {
    Perflog_Tree perflog("update_idx_lists");
    update_elements(
        existing_idx_lists, new_attic_idx_lists, transaction, *attic_settings().NODE_IDX_LIST);
  }
}


std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > Attic_Mapfile_IO::read_attic_mapfile(
    const std::vector< New_Object_Meta_Context >& new_obj_context)
{
  Perflog_Tree perflog("read_attic_mapfile");

  std::vector< Node_Skeleton::Id_Type > ids_to_update;
  Node_Skeleton::Id_Type recent_ref = 0ull;
  ids_to_update.reserve(new_obj_context.size());
  for (const auto& i : new_obj_context)
  {
    if (!(i.meta.ref == recent_ref) && (ids_to_update.empty() || !(ids_to_update.back() == i.meta.ref)))
    {
      if (!i.replaced_timestamp)
        ids_to_update.push_back(i.meta.ref);
      recent_ref = i.meta.ref;
    }
  }
  std::cerr<<"#id:"<<ids_to_update.size()<<' ';

  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > existing_attic_map_positions;
  {
    Perflog_Tree perflog("get_existing_map_positions");
    get_existing_map_positions(ids_to_update, transaction, *attic_settings().NODES)
        .swap(existing_attic_map_positions);
  }
  {
    Perflog_Tree perflog("get_existing_idx_lists");
    get_existing_idx_lists(ids_to_update, existing_attic_map_positions,
            transaction, *attic_settings().NODE_IDX_LIST).swap(existing_idx_lists);
  }

  std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > result;
  for (const auto& i : existing_attic_map_positions)
    result[i.second].push_back(i.first);
  for (const auto& i : existing_idx_lists)
  {
    for (const auto& j : i.second)
      result[j].push_back(i.first);
  }
  return result;
}


template< typename Element_Skeleton >
std::map< Uint31_Index, std::set< Element_Skeleton > > get_existing_meta
    (const std::map< Uint31_Index, std::vector< typename Element_Skeleton::Id_Type > >& ids_per_idx,
     Transaction& transaction, const File_Properties& file_properties)
{
  std::set< Uint31_Index > req;
  for (const auto& i : ids_per_idx)
    req.insert(i.first);

  std::map< Uint31_Index, std::set< Element_Skeleton > > result;
  typename std::map< Uint31_Index, std::vector< typename Element_Skeleton::Id_Type > >::const_iterator ids_it
      = ids_per_idx.begin();

  Block_Backend< Uint31_Index, Element_Skeleton > db(transaction.data_index(&file_properties));
  for (typename Block_Backend< Uint31_Index, Element_Skeleton >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    while (ids_it != ids_per_idx.end() && !(ids_it->first == it.index()))
      ++ids_it;
    if (std::binary_search(ids_it->second.begin(), ids_it->second.end(), it.object().ref))
      result[it.index()].insert(it.object());
  }

  return result;
}


std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > read_nodes_meta(
    Transaction& transaction,
    const std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > >& ids_per_idx,
    std::vector< New_Object_Meta_Context >& new_obj_context)
{
  Perflog_Tree perflog("read_nodes_meta");

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > existing_meta;
  {
    Perflog_Tree perflog("get_existing_meta");
    get_existing_meta< OSM_Element_Metadata_Skeleton< Node::Id_Type > >(
        ids_per_idx, transaction, *meta_settings().NODES_META).swap(existing_meta);
  }

  for (auto& idx : existing_meta)
  {
    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > local_result;
    for (const auto& obj : idx.second)
    {
      auto new_obj_it = std::lower_bound(new_obj_context.begin(), new_obj_context.end(),
          New_Object_Meta_Context(obj, 0u, 0));
      if (new_obj_it != new_obj_context.end() && new_obj_it->meta.ref == obj.ref)
      {
        new_obj_it->replaced_idx = idx.first;
        new_obj_it->replaced_timestamp = obj.timestamp;
        local_result.insert(obj);
      }
      if (new_obj_it != new_obj_context.begin())
      {
        --new_obj_it;
        if (new_obj_it->meta.ref == obj.ref)
        {
          new_obj_it->idx_of_next_version = idx.first;
          new_obj_it->end_timestamp = obj.timestamp;
        }
      }
    }
    idx.second.swap(local_result);
  }

  return existing_meta;
}


void read_attic_nodes_meta(
    Transaction& transaction,
    const std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > >& ids_per_idx,
    std::vector< New_Object_Meta_Context >& new_obj_context)
{
  Perflog_Tree perflog("read_attic_nodes_meta");

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > existing_meta;
  {
    Perflog_Tree perflog("get_existing_meta");
    get_existing_meta< OSM_Element_Metadata_Skeleton< Node::Id_Type > >(
        ids_per_idx, transaction, *attic_settings().NODES_META).swap(existing_meta);
  }

  for (auto& idx : existing_meta)
  {
    for (const auto& obj : idx.second)
    {
      auto new_obj_it = std::lower_bound(new_obj_context.begin(), new_obj_context.end(),
          New_Object_Meta_Context(obj, 0u, 0));
      if (new_obj_it != new_obj_context.end() && new_obj_it->meta.ref == obj.ref)
      {
        if (new_obj_it->replaced_timestamp < obj.timestamp)
        {
          new_obj_it->replaced_idx = idx.first;
          new_obj_it->replaced_timestamp = obj.timestamp;
        }
      }
      if (new_obj_it != new_obj_context.begin())
      {
        --new_obj_it;
        // assert: new_obj_it->end_timestamp != 0
        // because end_timestamp is set for all but the most recent version of an object
        // and we cannot arrive at the most recent version of an object here.
        if (new_obj_it->meta.ref == obj.ref &&
            (obj.timestamp < new_obj_it->end_timestamp))
        {
          new_obj_it->idx_of_next_version = idx.first;
          new_obj_it->end_timestamp = obj.timestamp;
        }
      }
    }
  }
}


std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
    add_new_attic_nodes(
        const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& current_to_attic,
        const std::vector< New_Object_Meta_Context >& new_obj_context)
{
  Perflog_Tree perflog("add_new_attic_nodes");

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > result
      = current_to_attic;

  for (const auto& obj : new_obj_context)
  {
    if (obj.end_timestamp != 0)
      result[obj.own_idx].insert(obj.meta);
  }

  return result;
}


/* Invariants:
 * - The current nodes meta file contains for each id at most one entry.
 * - The attic nodes meta file for each id has only an entry if an entry for that id exists in current
 *   and then all those entries have older timestamp than the current entry.
 * - If there is an entry for a given id at index idx in the current nodes meta file
 *   then there is an entry in the nodes meta map file mapping the id to idx.
 * - If there are one or more entries for a given id at index idx in the attic nodes meta file
 *   then there is an entry in the nodes meta map file or the idx list file mapping the id to idx resp idxs.
 * Preconditions: none
 */
std::vector< New_Object_Meta_Context > read_and_update_meta(
    Transaction& transaction, const Data_By_Id< Node_Skeleton >& new_data, meta_modes strategy)
{
  Perflog_Tree perflog("read_and_update_meta");

  std::vector< New_Object_Meta_Context > new_obj_context = objectlist_by_id(new_data);
  std::cerr<<"#objs:"<<new_obj_context.size()<<' ';

  std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > current_ids_per_idx
      = read_current_mapfile(transaction, new_obj_context);
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > current_to_attic
      = read_nodes_meta(transaction, current_ids_per_idx, new_obj_context);

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_meta
      = add_new_current_nodes(new_obj_context);

  {
    Perflog_Tree perflog("update_elements");
    update_elements(current_to_attic, new_meta, transaction, *meta_settings().NODES_META);
  }

  write_nodes_mapfile(new_meta, transaction);

  if (strategy == keep_attic)
  {
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_attic
        = add_new_attic_nodes(current_to_attic, new_obj_context);

    Attic_Mapfile_IO attic_mapfile(transaction);
    std::map< Uint31_Index, std::vector< Node_Skeleton::Id_Type > > attic_ids_per_idx
        = attic_mapfile.read_attic_mapfile(new_obj_context);
    read_attic_nodes_meta(transaction, current_ids_per_idx, new_obj_context);

    {
      Perflog_Tree perflog("update_elements");
      update_elements(decltype(new_attic)(), new_attic, transaction, *attic_settings().NODES_META);
    }

    attic_mapfile.write_attic_nodes_mapfile(new_attic);
  }

  return new_obj_context;
}
/* Postconditions:
 * - For every entry that has existed before the function call in the current or attic nodes meta file
 *   there is still an entry in the current or attic nodes meta call;
 *   entries may have moved from current to attic due to the age rule.
 * - For every entry in new_data there is an entry
 *   with the same id, timestamp, and idx in the current nodes meta file or attic nodes meta file;
 *   the age rule governs which is in which of the two files.
 */
