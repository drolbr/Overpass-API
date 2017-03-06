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

#include "../../template_db/block_backend.h"
#include "map_to_area.h"


Generic_Statement_Maker< Map_To_Area_Statement > Map_To_Area_Statement::statement_maker("map-to-area");

bool Map_To_Area_Statement::is_used_ = false;

Map_To_Area_Statement::Map_To_Area_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  is_used_ = true;

  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);

}

std::vector< Area_Skeleton::Id_Type > get_area_ids_for_ways(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  std::vector<Area::Id_Type> area_ids;
  for (std::map<Uint31_Index, std::vector<Way_Skeleton> >::const_iterator it =
      ways.begin(); it != ways.end(); ++it)
  {
    for (std::vector<Way_Skeleton>::const_iterator sit = it->second.begin();
        sit != it->second.end(); ++sit)
    {
      area_ids.push_back(sit->id.val() + 2400000000u);
    }
  }

  return area_ids;
}

std::vector< Area_Skeleton::Id_Type > get_area_ids_for_attic_ways(std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways)
{
  std::vector<Area::Id_Type> area_ids;
  for (std::map<Uint31_Index, std::vector<Attic<Way_Skeleton> > >::const_iterator it =
      attic_ways.begin(); it != attic_ways.end(); ++it)
  {
    for (std::vector<Attic<Way_Skeleton> >::const_iterator sit = it->second.begin();
        sit != it->second.end(); ++sit)
    {
      area_ids.push_back(sit->id.val() + 2400000000u);
    }
  }

  return area_ids;
}

std::vector< Area_Skeleton::Id_Type > get_area_ids_for_relations(const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels)
{
  std::vector<Area::Id_Type> area_ids;
  for (std::map<Uint31_Index, std::vector<Relation_Skeleton> >::const_iterator it =
      rels.begin(); it != rels.end(); ++it)
  {
    for (std::vector<Relation_Skeleton>::const_iterator sit = it->second.begin();
        sit != it->second.end(); ++sit)
    {
      area_ids.push_back(sit->id.val() + 3600000000u);
    }
  }

  return area_ids;
}

std::vector< Area_Skeleton::Id_Type > get_area_ids_for_attic_relations(std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels)
{
  std::vector<Area::Id_Type> area_ids;
  for (std::map<Uint31_Index, std::vector<Attic<Relation_Skeleton> > >::const_iterator it =
      attic_rels.begin(); it != attic_rels.end(); ++it)
  {
    for (std::vector<Attic<Relation_Skeleton> >::const_iterator sit =
        it->second.begin(); sit != it->second.end(); ++sit)
    {
      area_ids.push_back(sit->id.val() + 3600000000u);
    }
  }

  return area_ids;
}

void collect_elems_flat(Resource_Manager& rman,
           const std::vector< Area_Skeleton::Id_Type >& ids,
           std::map< Uint31_Index, std::vector< Area_Skeleton > >& elems)
{
  if (ids.size() == 0)
    return;

  std::vector<Area_Skeleton::Id_Type>::const_iterator lower = min_element(ids.begin(), ids.end());
  std::vector<Area_Skeleton::Id_Type>::const_iterator upper = max_element(ids.begin(), ids.end());

  Block_Backend< Uint31_Index, Area_Skeleton > elems_db
      (rman.get_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it = elems_db.flat_begin(); !(it == elems_db.flat_end()); ++it)
  {
    if (!(it.object().id < (*lower).val()) && it.object().id < ((*upper).val() + 1) &&
        binary_search(ids.begin(), ids.end(), it.object().id))
    {
      elems[it.index()].push_back(it.object());
    }
  }
}

void Map_To_Area_Statement::execute(Resource_Manager& rman)
{
  Set into;
  std::vector< Area_Skeleton::Id_Type > idx;

  std::vector< Area_Skeleton::Id_Type > ways_idx = get_area_ids_for_ways(rman.sets()[input].ways);
  std::vector< Area_Skeleton::Id_Type > rels_idx = get_area_ids_for_relations(rman.sets()[input].relations);
  std::vector< Area_Skeleton::Id_Type > attic_ways_idx = get_area_ids_for_attic_ways(rman.sets()[input].attic_ways);
  std::vector< Area_Skeleton::Id_Type > attic_rels_idx = get_area_ids_for_attic_relations(rman.sets()[input].attic_relations);

  idx.insert(idx.end(), ways_idx.begin(), ways_idx.end());
  idx.insert(idx.end(), rels_idx.begin(), rels_idx.end());
  idx.insert(idx.end(), attic_ways_idx.begin(), attic_ways_idx.end());
  idx.insert(idx.end(), attic_rels_idx.begin(), attic_rels_idx.end());

  sort(idx.begin(), idx.end());
  idx.erase(unique(idx.begin(), idx.end()), idx.end());

  collect_elems_flat(rman, idx, into.areas);

  transfer_output(rman, into);
  rman.health_check(*this);
}
