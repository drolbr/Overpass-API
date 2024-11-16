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

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../../template_db/block_backend.h"
#include "../../template_db/block_backend_write.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"



void process_user_data(Transaction& transaction, std::map< uint32, std::string >& user_by_id,
		       std::map< uint32, std::vector< uint32 > >& idxs_by_user_id)
{
  {
    std::map< Uint32_Index, std::set< User_Data > > db_to_delete;
    std::map< Uint32_Index, std::set< User_Data > > db_to_insert;

    for (std::map< uint32, std::string >::const_iterator it = user_by_id.begin();
        it != user_by_id.end(); ++it)
    {
      User_Data user_data;
      user_data.id = it->first;
      db_to_delete[Uint32_Index(it->first & 0xffffff00)].insert(user_data);
    }
    for (std::map< uint32, std::string >::const_iterator it = user_by_id.begin();
        it != user_by_id.end(); ++it)
    {
      User_Data user_data;
      user_data.id = it->first;
      user_data.name = it->second;
      db_to_insert[Uint32_Index(it->first & 0xffffff00)].insert(user_data);
    }
    user_by_id.clear();

    Block_Backend< Uint32_Index, User_Data > user_db
        (transaction.data_index(meta_settings().USER_DATA));
    user_db.update(db_to_delete, db_to_insert);
  }
  {
    std::map< Uint32_Index, std::set< Uint31_Index > > db_to_delete;
    std::map< Uint32_Index, std::set< Uint31_Index > > db_to_insert;

    for (std::map< uint32, std::vector< uint32 > >::const_iterator it = idxs_by_user_id.begin();
        it != idxs_by_user_id.end(); ++it)
    {
      std::set< Uint31_Index >& ins = db_to_delete[it->first];
      for (std::vector< uint32 >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
	ins.insert(Uint31_Index(*it2));
    }
    for (std::map< uint32, std::vector< uint32 > >::const_iterator it = idxs_by_user_id.begin();
        it != idxs_by_user_id.end(); ++it)
    {
      std::set< Uint31_Index >& ins = db_to_insert[it->first];
      for (std::vector< uint32 >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
	ins.insert(Uint31_Index(*it2));
    }

    Block_Backend< Uint32_Index, Uint31_Index > user_db
        (transaction.data_index(meta_settings().USER_INDICES));
    user_db.update(db_to_delete, db_to_insert);
  }
}


//-----------------------------------------------------------------------------


Transaction_Collection::Transaction_Collection
    (Access_Mode access_mode, bool use_shadow,
     const std::string& db_dir, const std::vector< std::string >& file_name_extensions_)
     : file_name_extensions(file_name_extensions_)
{
  for (std::vector< std::string >::const_iterator it = file_name_extensions.begin();
      it != file_name_extensions.end(); ++it)
    transactions.push_back(new Nonsynced_Transaction(access_mode, use_shadow, db_dir, *it));
}

Transaction_Collection::~Transaction_Collection()
{
  for (std::vector< Transaction* >::const_iterator it = transactions.begin();
      it != transactions.end(); ++it)
    delete(*it);
}

void Transaction_Collection::remove_referred_files(const File_Properties& file_prop)
{
  for (std::vector< std::string >::const_iterator it = file_name_extensions.begin();
      it != file_name_extensions.end(); ++it)
  {
    remove((transactions.front()->get_db_dir()
           + file_prop.get_file_name_trunk() + *it
           + file_prop.get_data_suffix()
           + file_prop.get_index_suffix()).c_str());
    remove((transactions.front()->get_db_dir()
           + file_prop.get_file_name_trunk() + *it
           + file_prop.get_data_suffix()).c_str());
  }
}

void rename_referred_file(const std::string& db_dir, const std::string& from, const std::string& to,
			  const File_Properties& file_prop)
{
  rename((db_dir
      + file_prop.get_file_name_trunk() + from
      + file_prop.get_data_suffix()
      + file_prop.get_index_suffix()).c_str(),
	 (db_dir
      + file_prop.get_file_name_trunk() + to
      + file_prop.get_data_suffix()
      + file_prop.get_index_suffix()).c_str());
  rename((db_dir
      + file_prop.get_file_name_trunk() + from
      + file_prop.get_data_suffix()).c_str(),
	 (db_dir
      + file_prop.get_file_name_trunk() + to
      + file_prop.get_data_suffix()).c_str());
}


//-----------------------------------------------------------------------------


namespace
{
  template< typename Meta >
  struct Meta_Skel_By_Chgst
  {
    bool operator()(const Meta& lhs, const Meta& rhs)
    {
      return lhs.changeset < rhs.changeset;
    }
  };
}


// Assert: data_by_id is already sorted
template< typename Index, typename Skeleton >
Meta_By_Changeset_Timeless< Index > meta_from_fresh_data(const Data_By_Id< Skeleton >& data_by_id)
{
  std::map< Index, std::vector< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > > current;
  std::map< Index, std::vector< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > > attic;
  
  for (auto it = data_by_id.data.begin(); it != data_by_id.data.end(); ++it)
  {
    auto next_it = it+1;
    if (it->idx == Index(0u))
    {
      auto idx = it->idx;
      if (it != data_by_id.data.begin())
      {
        auto prev_it = it-1;
        if (prev_it->elem.id == it->elem.id)
          idx = prev_it->idx;
      }
      attic[idx].push_back(it->meta);
    }
    if (next_it != data_by_id.data.end() && next_it->elem.id == it->elem.id)
      attic[it->idx].push_back(it->meta);
    else
      current[it->idx].push_back(it->meta);
  }
  
  Meta_By_Changeset_Timeless< Index > result;
  
  for (auto& i : current)
  {
    std::sort(
        i.second.begin(), i.second.end(),
        Meta_Skel_By_Chgst< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >());
    auto& to = result.current[i.first];
    for (const auto& j : i.second)
    {
      if (to.empty() || to.back().get_changeset() != j.changeset)
        to.push_back(Meta_Per_Changeset_Skeleton(j.changeset, false, j.user_id));
      to.back().add_ref({ j.ref.val(), j.version, j.timestamp });
    }
  }
  for (auto& i : attic)
  {
    std::sort(
        i.second.begin(), i.second.end(),
        Meta_Skel_By_Chgst< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >());
    auto& to = result.attic[i.first];
    for (const auto& j : i.second)
    {
      if (to.empty() || to.back().get_changeset() != j.changeset)
        to.push_back(Meta_Per_Changeset_Skeleton(j.changeset, false, j.user_id));
      to.back().add_ref({ j.ref.val(), j.version, j.timestamp });
    }
  }
  
  return result;
}


template
Meta_By_Changeset_Timeless< Node::Index > meta_from_fresh_data(const Data_By_Id< Node_Skeleton >& data_by_id);


//-----------------------------------------------------------------------------

namespace
{
  template< typename Index, typename Unused1, typename Unused2 >
  std::vector< Index > merge_idxs(
      const std::map< Index, Unused1 >& to_add, const std::vector< std::pair< Unused2, Index > >& extra_idxs)
  {
    std::vector< Index > result;

    for (const auto& i : extra_idxs)
      result.push_back(i.second);
    for (const auto& i : to_add)
      result.push_back(i.first);

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
  }


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


  template< typename Skeleton >
  const typename Data_By_Id< Skeleton >::Entry* get_entry(const Data_By_Id< Skeleton >& data_by_id, uint64_t ref)
  {
    auto it = std::lower_bound(data_by_id.data.begin(), data_by_id.data.end(), ref,
        [](const typename Data_By_Id< Skeleton >::Entry& entry, uint64_t ref)
        { return entry.elem.id.val() < ref; });
    return (it != data_by_id.data.end() && it->elem.id.val() == ref) ? &*it : nullptr;
  }
}


template< typename Index, typename Skeleton >
Meta_By_Changeset_Delta< Index > load_and_process_current(
    const std::vector< std::pair< typename Skeleton::Id_Type, Index > >& extra_idxs,
    Transaction& transaction, const File_Properties& cur_meta_file_properties,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge,
    Data_By_Id< Skeleton >& data_by_id)
{
  Meta_By_Changeset_Delta< Index > result;
  
  std::vector< Index > req = merge_idxs(to_merge, extra_idxs);
  Block_Backend< Index, Meta_Per_Changeset_Skeleton, typename std::vector< Index >::const_iterator > meta_db(
      transaction.data_index(&cur_meta_file_properties));
  auto db_it = meta_db.discrete_begin(req.begin(), req.end());
  
  auto extra_it = to_merge.begin();
  
  for (auto idx : req)
  {
    auto& loc_to_add = result.to_add[idx];
    auto& loc_to_del = result.to_remove[idx];
    
    while (extra_it != to_merge.end() && extra_it->first < idx)
      ++extra_it;  // Should never happen, but prevent infinite loop
    while (!(db_it == meta_db.discrete_end()) && db_it.index() < idx)
      ++db_it;  // Should never happen, but prevent infinite loop
    
    if (extra_it != to_merge.end() && extra_it->first == idx)
      std::sort(extra_it->second.begin(), extra_it->second.end());

    while (!(db_it == meta_db.discrete_end()) && db_it.index() == idx)
    {
      if (!db_it.object().get_is_redacted())
      {
        Meta_Per_Changeset_Skeleton* new_entries = nullptr;
        if (extra_it != to_merge.end() && extra_it->first == idx)
        {
          auto merge_it = std::lower_bound(extra_it->second.begin(), extra_it->second.end(), db_it.object());
          if (merge_it != extra_it->second.end() && merge_it->get_changeset() == db_it.object().get_changeset())
            new_entries = &*merge_it;
        }
        
        const auto& refs = db_it.object().get_refs();
        auto ref_it = refs.begin();
        const typename Data_By_Id< Skeleton >::Entry* ptr_new_obj = nullptr;
        while (ref_it != refs.end())
        {
          ptr_new_obj = get_entry(data_by_id, ref_it->ref);
          if (ptr_new_obj)
            break;
          ++ref_it;
        }
        
        if (ptr_new_obj)
        {
          Meta_Per_Changeset_Skeleton combined(
              db_it.object().get_changeset(), db_it.object().get_is_redacted(), db_it.object().get_user_id());
          Meta_Per_Changeset_Skeleton attic(
              db_it.object().get_changeset(), db_it.object().get_is_redacted(), db_it.object().get_user_id());

          for (auto it = refs.begin(); it != ref_it; ++it)
            combined.add_ref(*it);
          
          while (ref_it != refs.end())
          {
            ptr_new_obj = get_entry(data_by_id, ref_it->ref);
            if (ptr_new_obj)
              attic.add_ref(*ref_it);
            else
              combined.add_ref(*ref_it);

            ++ref_it;
          }
          loc_to_del.push_back(attic);
          
          if (new_entries)
            new_entries->move_refs_to(combined);
          
          loc_to_add.push_back(combined);
        }
        else if (new_entries)
        {
          Meta_Per_Changeset_Skeleton combined = db_it.object();
          new_entries->move_refs_to(combined);
          
          loc_to_del.push_back(*new_entries);
          loc_to_add.push_back(combined);
        }
      }
      
      ++db_it;
    }
    
    if (extra_it != to_merge.end() && extra_it->first == idx)
    {
      for (auto i : extra_it->second)
      {
        if (!i.get_refs().empty())
          loc_to_add.push_back(i);
      }
    }
    
    std::sort(loc_to_del.begin(), loc_to_del.end());
    std::sort(loc_to_add.begin(), loc_to_add.end());
  }
  
  return result;
}


template
Meta_By_Changeset_Delta< Node::Index > load_and_process_current< Node::Index, Node_Skeleton >(
    const std::vector< std::pair< Node_Skeleton::Id_Type, Node::Index > >& extra_idxs,
    Transaction& transaction, const File_Properties& cur_meta_file_properties,
    std::map< Node::Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge,
    Data_By_Id< Node_Skeleton >& data_by_id);


template< typename Index >
std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > merge_meta(
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& lhs,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& rhs)
{
  for (auto& i : rhs)
  {
    auto& to = lhs[i.first];
    auto it_to = to.begin();
    
    for (auto& j : i.second)
    {
      while (it_to != to.end() && it_to->get_changeset() < j.get_changeset())
        ++it_to;
      if (it_to != to.end() && it_to->get_changeset() == j.get_changeset())
        j.move_refs_to(*it_to);
    }
    for (auto& j : i.second)
    {
      if (!j.get_refs().empty())
        to.push_back(j);
    }

    std::sort(to.begin(), to.end());
  }
  return lhs;
}


template
std::map< Node::Index, std::vector< Meta_Per_Changeset_Skeleton > > merge_meta(
    std::map< Node::Index, std::vector< Meta_Per_Changeset_Skeleton > >&& lhs,
    std::map< Node::Index, std::vector< Meta_Per_Changeset_Skeleton > >&& rhs);


template< typename Index >
Meta_By_Changeset_Delta< Index > load_and_process_attic(
    Transaction& transaction, const File_Properties& attic_meta_file_properties,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge)
{
  Meta_By_Changeset_Delta< Index > result;
  
  std::vector< Index > req = extract_idxs(to_merge);
  Block_Backend< Index, Meta_Per_Changeset_Skeleton, typename std::vector< Index >::const_iterator > meta_db(
      transaction.data_index(&attic_meta_file_properties));
  auto db_it = meta_db.discrete_begin(req.begin(), req.end());
  
  auto extra_it = to_merge.begin();
  
  for (auto idx : req)
  {
    auto& loc_to_add = result.to_add[idx];
    auto& loc_to_del = result.to_remove[idx];
    
    while (extra_it != to_merge.end() && extra_it->first < idx)
      ++extra_it;  // Should never happen, but prevent infinite loop
    while (!(db_it == meta_db.discrete_end()) && db_it.index() < idx)
      ++db_it;  // Should never happen, but prevent infinite loop
      
    std::sort(extra_it->second.begin(), extra_it->second.end());

    while (!(db_it == meta_db.discrete_end()) && db_it.index() == idx)
    {
      if (!db_it.object().get_is_redacted())
      {
        Meta_Per_Changeset_Skeleton* new_entries = nullptr;
        if (extra_it != to_merge.end() && extra_it->first == idx)
        {
          auto merge_it = std::lower_bound(extra_it->second.begin(), extra_it->second.end(), db_it.object());
          if (merge_it != extra_it->second.end() && merge_it->get_changeset() == db_it.object().get_changeset()
              && !merge_it->get_refs().empty())
            new_entries = &*merge_it;
        }

        if (new_entries)
        {
          Meta_Per_Changeset_Skeleton combined = db_it.object();
          new_entries->move_refs_to(combined);
          
          loc_to_del.push_back(*new_entries);
          loc_to_add.push_back(combined);
        }
      }
      
      ++db_it;
    }
    
    if (extra_it != to_merge.end() && extra_it->first == idx)
    {
      for (auto i : extra_it->second)
      {
        if (!i.get_refs().empty())
          loc_to_add.push_back(i);
      }
    }
    
    std::sort(loc_to_del.begin(), loc_to_del.end());
    std::sort(loc_to_add.begin(), loc_to_add.end());
  }
  
  return result;
}


template
Meta_By_Changeset_Delta< Node::Index > load_and_process_attic(
    Transaction& transaction, const File_Properties& attic_meta_file_properties,
    std::map< Node::Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge);
