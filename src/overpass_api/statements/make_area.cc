/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../data/collect_members.h"
#include "../osm-backend/area_updater.h"
#include "make_area.h"
#include "print.h"


bool Make_Area_Statement::is_used_ = false;

Generic_Statement_Maker< Make_Area_Statement > Make_Area_Statement::statement_maker("make-area");

Make_Area_Statement::Make_Area_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  is_used_ = true;

  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["pivot"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);
  pivot = attributes["pivot"];
}


std::pair< uint32, Uint64 > Make_Area_Statement::detect_pivot(const Set& pivot)
{
  uint32 pivot_type(0);
  Node::Id_Type pivot_id(0ull);
  std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator
      nit(pivot.nodes.begin());
  while ((pivot_id.val() == 0) && (nit != pivot.nodes.end()))
  {
    if (nit->second.size() > 0)
    {
      pivot_id = nit->second.front().id;
      pivot_type = NODE;
    }
    ++nit;
  }
  std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
      wit(pivot.ways.begin());
  while ((pivot_id.val() == 0) && (wit != pivot.ways.end()))
  {
    if (wit->second.size() > 0)
    {
      pivot_id = Uint64(wit->second.front().id.val());
      pivot_type = WAY;
    }
    ++wit;
  }
  std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      rit(pivot.relations.begin());
  while ((pivot_id.val() == 0) && (rit != pivot.relations.end()))
  {
    if (rit->second.size() > 0)
    {
      pivot_id = Uint64(rit->second.front().id.val());
      pivot_type = RELATION;
    }
    ++rit;
  }

  return std::make_pair(pivot_type, pivot_id);
}

Node::Id_Type Make_Area_Statement::check_node_parity(const Set& pivot)
{
  std::set< Node::Id_Type > node_parity_control;
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
    it(pivot.ways.begin()); it != pivot.ways.end(); ++it)
  {
    for (std::vector< Way_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      if (it2->nds.size() < 2)
	continue;
      std::pair< std::set< Node::Id_Type >::iterator, bool > npp(node_parity_control.insert
          (it2->nds.front()));
      if (!npp.second)
	node_parity_control.erase(npp.first);
      npp = node_parity_control.insert
          (it2->nds.back());
      if (!npp.second)
	node_parity_control.erase(npp.first);
    }
  }
  if (node_parity_control.size() > 0)
    return *(node_parity_control.begin());
  return Node::Id_Type(0ull);
}


std::pair< Node::Id_Type, Uint32_Index > Make_Area_Statement::create_area_blocks
    (std::map< Uint31_Index, std::vector< Area_Block > >& areas, bool& wraps_around_date_line,
     uint32 id, const Set& pivot)
{
  std::vector< Node > nodes;
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator
      it(pivot.nodes.begin()); it != pivot.nodes.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      nodes.push_back(Node(it2->id.val(), it->first.val(), it2->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());

  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
      it(pivot.ways.begin()); it != pivot.ways.end(); ++it)
  {
    for (std::vector< Way_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      wraps_around_date_line ^= add_way_to_area_blocks(make_geometry(*it2, nodes), id, areas);
  }

  return std::make_pair< uint32, uint32 >(0, 0);
}


uint32 Make_Area_Statement::shifted_lat(uint32 ll_index, uint64 coord)
{
  return ::ilat(ll_index | (coord>>32), coord & 0xffffffff);
}


int32 Make_Area_Statement::lon_(uint32 ll_index, uint64 coord)
{
  return ::ilon(ll_index | (coord>>32), coord & 0xffffffff);
}

void Make_Area_Statement::add_segment_blocks
    (std::map< Uint31_Index, std::vector< Area_Block > >& area_blocks, uint32 id)
{
  /* We use that more northern segments always have bigger indices.
    Thus we can collect each block's end points and add them, if they do not
    cancel out, to the next block further northern.*/
  for (std::map< Uint31_Index, std::vector< Area_Block > >::const_iterator
    it = area_blocks.begin(); it != area_blocks.end(); ++it)
  {
    if (area_blocks.size() > 384*1024)
    {
      area_blocks.clear();
      break;
    }

    std::set< int32 > lons;

    for (std::vector< Area_Block >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (it2->coors.empty())
	continue;

      const uint64& ll_front(it2->coors.front());
      int32 lon_front(lon_(ll_front>>32, ll_front & 0xffffffffull));
      const uint64& ll_back(it2->coors.back());
      int32 lon_back(lon_(ll_back>>32, ll_back & 0xffffffffull));
      if (lons.find(lon_front) == lons.end())
	lons.insert(lon_front);
      else
	lons.erase(lon_front);
      if (lons.find(lon_back) == lons.end())
	lons.insert(lon_back);
      else
	lons.erase(lon_back);
    }

    if (lons.empty())
      continue;

    uint32 current_idx(it->first.val());

    // calc lat
    uint32 lat(shifted_lat(it->first.val(), 0) + 16*65536);
    int32 lon(lon_(it->first.val(), 0));
    uint32 northern_ll_upper(::ll_upper(lat, lon) ^ 0x40000000);
    // insert lons
    std::vector< Area_Block >& northern_block(area_blocks[northern_ll_upper]);
    for (std::set< int32 >::const_iterator it2(lons.begin()); it2 != lons.end(); ++it2)
    {
      int32 from(*it2);
      ++it2;
      int32 to(*it2);
      std::vector< uint64 > coors;
      coors.push_back
          ((((uint64)(::ll_upper(lat, from) ^ 0x40000000))<<32) | ::ll_lower(lat, from));
      coors.push_back
          ((((uint64)(::ll_upper(lat, to) ^ 0x40000000))<<32) | ::ll_lower(lat, to));
      Area_Block new_block(id, coors);
      northern_block.push_back(new_block);
    }

    it = area_blocks.find(Uint31_Index(current_idx));
  }
}


void add_south_pole_line
    (std::map< Uint31_Index, std::vector< Area_Block > >& area_blocks, uint32 id)
{
  int south_delta = 0;

  for (std::map< Uint31_Index, std::vector< Area_Block > >::const_iterator
    it = area_blocks.begin(); it != area_blocks.end(); ++it)
  {
    if (::lat(it->first.val(), 0u) < 0)
      ++south_delta;
    else
      --south_delta;
  }
  if (south_delta < 0)
    return;

  for (int32 lon = -1800000000 + 0x100000; lon < 1800000000 - 0x100000; lon += 0x100000)
  {
    int32 from = lon & 0xfff00000;

    std::vector< uint64 > coors;
    coors.push_back
        ((((uint64)(::ll_upper(10000000, from) ^ 0x40000000))<<32) | ::ll_lower(10000000, from));
    coors.push_back
        ((((uint64)(::ll_upper(10000000, from + 0xfffff) ^ 0x40000000))<<32)
            | ::ll_lower(10000000, from + 0xfffff));

    area_blocks[(::ll_upper(10000000, lon) & 0xffffff00) ^ 0x40000000].push_back(Area_Block(id, coors));
  }

  int32 to = (-1800000000 & 0xfff00000) + 0xfffff;

  std::vector< uint64 > coors;
  coors.push_back
      ((((uint64)(::ll_upper(10000000, -1800000000) ^ 0x40000000))<<32) | ::ll_lower(10000000, -1800000000));
  coors.push_back
      ((((uint64)(::ll_upper(10000000, to) ^ 0x40000000))<<32) | ::ll_lower(10000000, to));

  area_blocks[(::ll_upper(10000000, -1800000000) & 0xffffff00) ^ 0x40000000].push_back(Area_Block(id, coors));

  int32 from = 1800000000 & 0xfff00000;

  coors.clear();
  coors.push_back
      ((((uint64)(::ll_upper(10000000, from) ^ 0x40000000))<<32) | ::ll_lower(10000000, from));
  coors.push_back
      ((((uint64)(::ll_upper(10000000, 1800000000) ^ 0x40000000))<<32) | ::ll_lower(10000000, 1800000000));

  area_blocks[(::ll_upper(10000000, 1800000000) & 0xffffff00) ^ 0x40000000].push_back(Area_Block(id, coors));
}


void Make_Area_Statement::execute(Resource_Manager& rman)
{
  Set into;

  // detect pivot element
  Set* pivot_set = rman.get_set(pivot);
  if (!pivot_set)
  {
    transfer_output(rman, into);
    return;
  }
  std::pair< uint32, Uint64 > pivot_pair(detect_pivot(*pivot_set));
  int pivot_type(pivot_pair.first);
  uint32 pivot_id(pivot_pair.second.val());

  if (pivot_type == 0)
  {
    transfer_output(rman, into);
    return;
  }

  //formulate range query to query tags of the pivot
  std::set< Uint31_Index > coarse_indices;
  std::set< std::pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  if (pivot_type == NODE)
    coarse_indices.insert(pivot_set->nodes.begin()->first.val() & 0x7fffff00);
  else if (pivot_type == WAY)
    coarse_indices.insert(pivot_set->ways.begin()->first.val() & 0x7fffff00);
  else if (pivot_type == RELATION)
    coarse_indices.insert(pivot_set->relations.begin()->first.val() & 0x7fffff00);

  formulate_range_query(range_set, coarse_indices);

  // iterate over the result
  std::vector< std::pair< std::string, std::string > > new_tags;
  File_Properties* file_prop = 0;
  if (pivot_type == NODE)
    file_prop = osm_base_settings().NODE_TAGS_LOCAL;
  else if (pivot_type == WAY)
    file_prop = osm_base_settings().WAY_TAGS_LOCAL;
  else if (pivot_type == RELATION)
    file_prop = osm_base_settings().RELATION_TAGS_LOCAL;
  Block_Backend< Tag_Index_Local, Uint32_Index > items_db
      (rman.get_transaction()->data_index(file_prop));
  Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
      tag_it(items_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  for (; !(tag_it == items_db.range_end()); ++tag_it)
  {
    if (tag_it.object().val() == pivot_id)
      new_tags.push_back(std::make_pair(tag_it.index().key, tag_it.index().value));
  }

  if (pivot_type == WAY)
    pivot_id += 2400000000u;
  else if (pivot_type == RELATION)
    pivot_id += 3600000000u;

  Set* input_set = rman.get_set(input);
  if (!input_set)
  {
    transfer_output(rman, into);
    return;
  }

  // check node parity
  Node::Id_Type odd_id(check_node_parity(*input_set));
  if (!(odd_id == Node::Id_Type(0ull)))
  {
    std::ostringstream temp;
    temp<<"make-area: Node "<<odd_id.val()
        <<" is contained in an odd number of ways.\n";
    runtime_remark(temp.str());
  }

  // create area blocks
  std::map< Uint31_Index, std::vector< Area_Block > > area_blocks;
  bool wraps_around_date_line = false;
  std::pair< Node::Id_Type, Way::Id_Type > odd_pair
    (create_area_blocks(area_blocks, wraps_around_date_line, pivot_id, *input_set));
  if (!(odd_pair.first == Node::Id_Type(0ull)))
  {
    std::ostringstream temp;
    temp<<"make-area: Node "<<odd_pair.first.val()
        <<" referred by way "<<odd_pair.second.val()
        <<" is not contained in set \""<<input<<"\".\n";
    runtime_remark(temp.str());
  }

  if (!(odd_id == Node::Id_Type(0ull)) || !(odd_pair.first == Node::Id_Type(0ull)))
  {
    transfer_output(rman, into);
    return;
  }

  if (wraps_around_date_line)
    add_south_pole_line(area_blocks, pivot_id);

  add_segment_blocks(area_blocks, pivot_id);

  std::vector< uint32 > used_indices;
  for (std::map< Uint31_Index, std::vector< Area_Block > >::const_iterator
      it(area_blocks.begin()); it != area_blocks.end(); ++it)
    used_indices.push_back(it->first.val());
  sort(used_indices.begin(), used_indices.end());

  Area_Location new_location(pivot_id, used_indices);
  new_location.tags = new_tags;
  Uint31_Index new_index(new_location.calc_index());

  if (new_index.val() == 0)
  {
    transfer_output(rman, into);
    return;
  }

  if (rman.area_updater())
  {
    Area_Updater* area_updater = dynamic_cast< Area_Updater* >(rman.area_updater());
    area_updater->set_area(new_index, new_location);
    area_updater->add_blocks(area_blocks);
    area_updater->commit();
  }

  into.areas[new_index].push_back(Area_Skeleton(new_location));

  transfer_output(rman, into);
  rman.health_check(*this);
}
