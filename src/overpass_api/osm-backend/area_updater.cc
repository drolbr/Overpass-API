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
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/block_backend_write.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "area_updater.h"


Area_Updater::Area_Updater(Transaction& transaction_)
  : transaction(&transaction_), external_transaction(true),
    total_area_blocks_count(0)
{}

Area_Updater::Area_Updater(std::string db_dir_)
  : transaction(0), external_transaction(false),
    db_dir(db_dir_), total_area_blocks_count(0)
{}

void Area_Updater::add_blocks
    (const std::map< Uint31_Index, std::vector< Area_Block > >& area_blocks_)
{
  for (std::map< Uint31_Index, std::vector< Area_Block > >::const_iterator
    it(area_blocks_.begin()); it != area_blocks_.end(); ++it)
  {
    for (std::vector< Area_Block >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    area_blocks[it->first].push_back(*it2);
    total_area_blocks_count += it->second.size();
  }
}

void Area_Updater::update()
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(Access_Mode::writeable, false, db_dir, "");

  sort(areas_to_insert.begin(), areas_to_insert.end());

  std::map< Uint31_Index, std::set< Area_Skeleton > > locations_to_delete;
  std::map< Uint31_Index, std::set< Area_Block > > blocks_to_delete;
  update_area_ids(locations_to_delete, blocks_to_delete);
  update_members(locations_to_delete, blocks_to_delete);

  std::vector< Tag_Entry< uint32 > > tags_to_delete;
  prepare_delete_tags(tags_to_delete, locations_to_delete);
  update_area_tags_local(tags_to_delete);
  update_area_tags_global(tags_to_delete);

  ids_to_modify.clear();
  areas_to_insert.clear();
  area_blocks.clear();
  total_area_blocks_count = 0;

  if (!external_transaction)
    delete transaction;
}

void Area_Updater::update_area_ids
    (std::map< Uint31_Index, std::set< Area_Skeleton > >& locations_to_delete,
     std::map< Uint31_Index, std::set< Area_Block > >& blocks_to_delete)
{
  std::vector< Uint31_Index > blocks_req;

  // process the areas themselves
  Block_Backend< Uint31_Index, Area_Skeleton > area_locations_db
      (transaction->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it(area_locations_db.flat_begin());
      !(it == area_locations_db.flat_end()); ++it)
  {
    if (ids_to_modify.find(it.object().id) != ids_to_modify.end())
    {
      for (auto it2 = it.object().used_indices.begin(); it2 != it.object().used_indices.end(); ++it2)
        blocks_req.push_back(*it2);
      locations_to_delete[it.index().val()].insert(it.object());
    }
  }
  std::sort(blocks_req.begin(), blocks_req.end());
  blocks_req.erase(std::unique(blocks_req.begin(), blocks_req.end()), blocks_req.end());

  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (transaction->data_index(area_settings().AREA_BLOCKS));
  for (Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      it(area_blocks_db.discrete_begin(blocks_req.begin(), blocks_req.end()));
      !(it == area_blocks_db.discrete_end()); ++it)
  {
    if (ids_to_modify.find(it.object().id) != ids_to_modify.end())
      blocks_to_delete[it.index()].insert(it.object());
  }
}

void Area_Updater::update_members
    (const std::map< Uint31_Index, std::set< Area_Skeleton > >& locations_to_delete,
     const std::map< Uint31_Index, std::set< Area_Block > >& blocks_to_delete)
{
  std::map< Uint31_Index, std::set< Area_Skeleton > > locations_to_insert;
  for (std::vector< std::pair< Area_Location, Uint31_Index > >::const_iterator
      it(areas_to_insert.begin()); it != areas_to_insert.end(); ++it)
    locations_to_insert[it->second].insert(Area_Skeleton(it->first));

  Block_Backend< Uint31_Index, Area_Skeleton > area_locations
      (transaction->data_index(area_settings().AREAS));
  area_locations.update(locations_to_delete, locations_to_insert);

  std::map< Uint31_Index, std::set< Area_Block > > blocks_to_insert;
  for (std::map< Uint31_Index, std::vector< Area_Block > >::const_iterator
      it(area_blocks.begin()); it != area_blocks.end(); ++it)
  {
    for (std::vector< Area_Block >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      blocks_to_insert[it->first].insert(*it2);
  }

  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (transaction->data_index(area_settings().AREA_BLOCKS));
  area_blocks_db.update(blocks_to_delete, blocks_to_insert);
}

void Area_Updater::prepare_delete_tags
    (std::vector< Tag_Entry< uint32 > >& tags_to_delete,
     const std::map< Uint31_Index, std::set< Area_Skeleton > >& to_delete)
{
  // make indices appropriately coarse
  std::map< uint32, std::set< Area::Id_Type > > to_delete_coarse;
  for (std::map< Uint31_Index, std::set< Area_Skeleton > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    std::set< Area::Id_Type >& handle(to_delete_coarse[it->first.val() & 0xffffff00]);
    for (std::set< Area_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      handle.insert(it2->id);
  }

  // formulate range query
  Ranges< Tag_Index_Local > ranges;
  for (auto it = to_delete_coarse.begin(); it != to_delete_coarse.end(); ++it)
    ranges.push_back({ it->first, "", "" }, { it->first + 1, "", "" });
  ranges.sort();
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, Uint32_Index > areas_db
      (transaction->data_index(area_settings().AREA_TAGS_LOCAL));
  Tag_Index_Local current_index;
  Tag_Entry< uint32 > tag_entry;
  current_index.index = 0xffffffff;
  std::set< Area::Id_Type >* handle = 0;
  for (auto it = areas_db.range_begin(ranges); !(it == areas_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	tags_to_delete.push_back(tag_entry);
      current_index = it.index();
      tag_entry.index = it.index().index;
      tag_entry.key = it.index().key;
      tag_entry.value = it.index().value;
      tag_entry.ids.clear();
    
      handle = &to_delete_coarse[it.index().index];
    }

    if (handle->find(it.object().val()) != handle->end())
      tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}


Area_Location* binary_search_for_id
    (std::vector< std::pair< Area_Location, Uint31_Index > >& vect, uint32 id)
{
  uint32 lower(0);
  uint32 upper(vect.size());

  while (upper > lower)
  {
    uint32 pos((upper + lower)/2);
    if (id < vect[pos].first.id)
      upper = pos;
    else if (vect[pos].first.id == id)
      return &(vect[pos].first);
    else
      lower = pos + 1;
  }
  return 0;
}

void Area_Updater::update_area_tags_local
    (const std::vector< Tag_Entry< uint32 > >& tags_to_delete)
{
  std::map< Tag_Index_Local, std::set< Uint32_Index > > db_to_delete;
  std::map< Tag_Index_Local, std::set< Uint32_Index > > db_to_insert;

  for (std::vector< Tag_Entry< uint32 > >::const_iterator it(tags_to_delete.begin());
      it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Local index;
    index.index = it->index;
    index.key = it->key;
    index.value = it->value;

    std::set< Uint32_Index > area_ids;
    for (std::vector< uint32 >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
      area_ids.insert(*it2);

    db_to_delete[index] = area_ids;
  }

  std::vector< std::pair< Area_Location, Uint31_Index > >::const_iterator
      rit(areas_to_insert.begin());
  for (std::set< Area::Id_Type >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((rit != areas_to_insert.end()) && (*it == rit->first.id))
    {
      Tag_Index_Local index;
      index.index = rit->second.val() & 0xffffff00;

      for (std::vector< std::pair< std::string, std::string > >::const_iterator
	  it2(rit->first.tags.begin()); it2 != rit->first.tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(rit->first.id);
	db_to_delete[index].insert(rit->first.id);
      }
      ++rit;
    }
  }

  Block_Backend< Tag_Index_Local, Uint32_Index > areas_db
      (transaction->data_index(area_settings().AREA_TAGS_LOCAL));
  areas_db.update(db_to_delete, db_to_insert);
}

void Area_Updater::update_area_tags_global
    (const std::vector< Tag_Entry< uint32 > >& tags_to_delete)
{
  std::map< Tag_Index_Global, std::set< Uint32_Index > > db_to_delete;
  std::map< Tag_Index_Global, std::set< Uint32_Index > > db_to_insert;

  for (std::vector< Tag_Entry< uint32 > >::const_iterator it(tags_to_delete.begin());
      it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Global index;
    index.key = it->key;
    index.value = it->value;

    for (std::vector< uint32 >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
      db_to_delete[index].insert(*it2);
  }

  std::vector< std::pair< Area_Location, Uint31_Index > >::const_iterator
      rit(areas_to_insert.begin());
  for (std::set< Area::Id_Type >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((rit != areas_to_insert.end()) && (*it == rit->first.id))
    {
      Tag_Index_Global index;

      for (std::vector< std::pair< std::string, std::string > >::const_iterator
	  it2(rit->first.tags.begin()); it2 != rit->first.tags.end(); ++it2)
      {
	index.key = it2->first;
	index.value = it2->second;
	db_to_insert[index].insert(rit->first.id);
	db_to_delete[index].insert(rit->first.id);
      }
      ++rit;
    }
  }

  Block_Backend< Tag_Index_Global, Uint32_Index > areas_db
      (transaction->data_index(area_settings().AREA_TAGS_GLOBAL));
  areas_db.update(db_to_delete, db_to_insert);
}
