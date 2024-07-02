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

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "../data/filter_by_tags.h"
#include "../data/filter_ids_by_tags.h"
#include "../data/idx_from_id.h"
#include "../data/meta_collector.h"
#include "../data/regular_expression.h"
#include "../data/tags_global_reader.h"
#include "area_query.h"
#include "bbox_query.h"
#include "query.h"

#include <algorithm>
#include <sstream>


//-----------------------------------------------------------------------------

bool Query_Statement::area_query_exists_ = false;

Generic_Statement_Maker< Query_Statement > Query_Statement::statement_maker("query");

Query_Statement::Query_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), global_bbox_statement(0)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";
  attributes["type"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);
  if (attributes["type"] == "node")
    type = QUERY_NODE;
  else if (attributes["type"] == "way")
    type = QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = QUERY_RELATION;
  else if (attributes["type"] == "derived")
    type = QUERY_DERIVED;
  else if (attributes["type"] == "nwr")
    type = (QUERY_NODE | QUERY_WAY | QUERY_RELATION);
  else if (attributes["type"] == "nw")
    type = (QUERY_NODE | QUERY_WAY);
  else if (attributes["type"] == "wr")
    type = (QUERY_WAY | QUERY_RELATION);
  else if (attributes["type"] == "nr")
    type = (QUERY_NODE | QUERY_RELATION);
  else if (attributes["type"] == "area")
  {
    type = QUERY_AREA | QUERY_WAY | QUERY_CLOSED_WAY;
    area_query_exists_ = true;
  }
  else
  {
    type = 0;
    std::ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"query\""
        <<" the only allowed values are \"node\", \"way\", \"relation\", \"nwr\", \"nw\", \"wr\", \"nr\", or \"area\".";
    add_static_error(temp.str());
  }

  if (global_settings.get_global_bbox_limitation().valid())
  {
    global_bbox_statement = new Bbox_Query_Statement(global_settings.get_global_bbox_limitation());
    constraints.push_back(global_bbox_statement->get_query_constraint());
  }
}


Query_Statement::~Query_Statement()
{
  delete global_bbox_statement;
}


void Query_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());

  Has_Kv_Statement* has_kv(dynamic_cast<Has_Kv_Statement*>(statement));
  if (has_kv)
  {
    substatements.push_back(statement);

    if (has_kv->get_value() != "")
    {
      if (has_kv->get_straight())
        key_values.push_back(std::make_pair< std::string, std::string >
	    (has_kv->get_key(), has_kv->get_value()));
      else
        key_nvalues.push_back(std::make_pair< std::string, std::string >
	    (has_kv->get_key(), has_kv->get_value()));
    }
    else if (has_kv->get_key_regex())
    {
      if (has_kv->get_straight())
	regkey_regexes.push_back(std::make_pair< Regular_Expression*, Regular_Expression* >
            (has_kv->get_key_regex(), has_kv->get_regex()));
      else
	regkey_nregexes.push_back(std::make_pair< Regular_Expression*, Regular_Expression* >
            (has_kv->get_key_regex(), has_kv->get_regex()));
    }
    else if (has_kv->get_regex())
    {
      if (has_kv->get_straight())
	key_regexes.push_back(std::make_pair< std::string, Regular_Expression* >
            (has_kv->get_key(), has_kv->get_regex()));
      else
	key_nregexes.push_back(std::make_pair< std::string, Regular_Expression* >
            (has_kv->get_key(), has_kv->get_regex()));
    }
    else
      keys.push_back(has_kv->get_key());
    return;
  }

  Query_Constraint* constraint = statement->get_query_constraint();
  if (constraint)
  {
    constraints.push_back(constraint);
    substatements.push_back(statement);
  }
  else
    substatement_error(get_name(), statement);
}


void Query_Statement::get_elements_by_id_from_db
    (std::map< Uint31_Index, std::vector< Area_Skeleton > >& elements,
     const std::vector< Area_Skeleton::Id_Type >& ids, bool invert_ids,
     Resource_Manager& rman, File_Properties& file_prop)
{
  Request_Context context(this, rman);
  if (!invert_ids)
    collect_items_flat(context, file_prop,
			Id_Predicate< Area_Skeleton >(ids), elements);
  else
    collect_items_flat(context, file_prop,
			Not_Predicate< Area_Skeleton, Id_Predicate< Area_Skeleton > >
			(Id_Predicate< Area_Skeleton >(ids)), elements);
}


template< typename TIndex, typename TObject >
void clear_empty_indices
    (std::map< TIndex, std::vector< TObject > >& modify)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = modify.begin();
      it != modify.end();)
  {
    if (!it->second.empty())
    {
      ++it;
      continue;
    }
    typename std::map< TIndex, std::vector< TObject > >::iterator next_it = it;
    if (++next_it == modify.end())
    {
      modify.erase(it);
      break;
    }
    else
    {
      TIndex idx = next_it->first;
      modify.erase(it);
      it = modify.find(idx);
    }
  }
}


template< typename Id_Type >
void filter_ids_by_ntags
  (const std::map< std::string, std::pair< std::vector< Regular_Expression* >, std::vector< std::string > > >& keys,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   uint32 coarse_index,
   std::vector< Id_Type >& new_ids)
{
  std::vector< Id_Type > removed_ids;
  std::string last_key, last_value;
  bool key_relevant = false;
  bool valid = false;
  std::map< std::string, std::pair< std::vector< Regular_Expression* >, std::vector< std::string > > >::const_iterator
      key_it = keys.begin();

  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if (tag_it.index().key != last_key)
    {
      last_value = void_tag_value() + " ";

      if (key_relevant)
      {
        ++key_it;
        sort(removed_ids.begin(), removed_ids.end());
        removed_ids.erase(unique(removed_ids.begin(), removed_ids.end()), removed_ids.end());
        new_ids.erase(set_difference(new_ids.begin(), new_ids.end(),
                      removed_ids.begin(), removed_ids.end(), new_ids.begin()), new_ids.end());
      }
      key_relevant = false;

      last_key = tag_it.index().key;
      while (key_it != keys.end() && last_key > key_it->first)
        ++key_it;

      if (key_it == keys.end())
        break;

      if (last_key == key_it->first)
      {
        key_relevant = true;
        removed_ids.clear();
      }
    }

    if (key_relevant)
    {
      if (tag_it.index().value != last_value)
      {
        valid = false;
        for (std::vector< Regular_Expression* >::const_iterator rit = key_it->second.first.begin();
            rit != key_it->second.first.end(); ++rit)
          valid |= (tag_it.index().value != void_tag_value() && (*rit)->matches(tag_it.index().value));
        for (std::vector< std::string >::const_iterator rit = key_it->second.second.begin();
            rit != key_it->second.second.end(); ++rit)
          valid |= (*rit == tag_it.index().value);
        last_value = tag_it.index().value;
      }

      if (valid)
        removed_ids.push_back(tag_it.object());
    }

    ++tag_it;
  }
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
    ++tag_it;

  sort(removed_ids.begin(), removed_ids.end());
  removed_ids.erase(unique(removed_ids.begin(), removed_ids.end()), removed_ids.end());
  new_ids.erase(set_difference(new_ids.begin(), new_ids.end(),
                removed_ids.begin(), removed_ids.end(), new_ids.begin()), new_ids.end());

  sort(new_ids.begin(), new_ids.end());
}


template< typename Id_Type >
void filter_ids_by_ntags
  (const std::map< std::string, std::pair< std::vector< Regular_Expression* >, std::vector< std::string > > >& keys,
   uint64 timestamp,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   const Block_Backend< Tag_Index_Local, Attic< Id_Type > >& attic_items_db,
   typename Block_Backend< Tag_Index_Local, Attic< Id_Type > >::Range_Iterator& attic_tag_it,
   uint32 coarse_index,
   std::vector< Id_Type >& new_ids)
{
  for (std::map< std::string, std::pair< std::vector< Regular_Expression* >, std::vector< std::string > > >::const_iterator
      key_it = keys.begin(); key_it != keys.end(); ++key_it)
  {
    std::map< Id_Type, std::pair< uint64, uint64 > > timestamps;
    for (typename std::vector< Id_Type >::const_iterator it = new_ids.begin(); it != new_ids.end(); ++it)
      timestamps[*it];

    while ((!(tag_it == items_db.range_end())) &&
        ((tag_it.index().index) & 0x7fffff00) == coarse_index &&
        tag_it.index().key < key_it->first)
      ++tag_it;
    while ((!(attic_tag_it == attic_items_db.range_end())) &&
        ((attic_tag_it.index().index) & 0x7fffff00) == coarse_index &&
        attic_tag_it.index().key < key_it->first)
      ++attic_tag_it;

    bool valid = false;
    std::string last_value = void_tag_value() + " ";
    while ((!(tag_it == items_db.range_end())) &&
        ((tag_it.index().index) & 0x7fffff00) == coarse_index &&
        tag_it.index().key == key_it->first)
    {
      if (std::binary_search(new_ids.begin(), new_ids.end(), tag_it.object()))
      {
        std::pair< uint64, uint64 >& timestamp_ref = timestamps[tag_it.object()];
        timestamp_ref.second = NOW;

        if (tag_it.index().value != last_value)
        {
          valid = false;
          for (std::vector< Regular_Expression* >::const_iterator rit = key_it->second.first.begin();
              rit != key_it->second.first.end(); ++rit)
            valid |= (tag_it.index().value != void_tag_value() && (*rit)->matches(tag_it.index().value));
          for (std::vector< std::string >::const_iterator rit = key_it->second.second.begin();
              rit != key_it->second.second.end(); ++rit)
            valid |= (*rit == tag_it.index().value);
          last_value = tag_it.index().value;
        }

        if (valid)
          timestamp_ref.first = NOW;
      }
      ++tag_it;
    }

    last_value = void_tag_value() + " ";
    while ((!(attic_tag_it == attic_items_db.range_end())) &&
        ((attic_tag_it.index().index) & 0x7fffff00) == coarse_index &&
        attic_tag_it.index().key == key_it->first)
    {
      if (std::binary_search(new_ids.begin(), new_ids.end(), Id_Type(attic_tag_it.object())))
      {
        std::pair< uint64, uint64 >& timestamp_ref = timestamps[attic_tag_it.object()];
        if (timestamp < attic_tag_it.object().timestamp &&
            (timestamp_ref.second == 0 || timestamp_ref.second > attic_tag_it.object().timestamp))
          timestamp_ref.second = attic_tag_it.object().timestamp;

        if (attic_tag_it.index().value != last_value)
        {
          valid = false;
          if (attic_tag_it.index().value != void_tag_value())
          {
            for (std::vector< Regular_Expression* >::const_iterator rit = key_it->second.first.begin();
                rit != key_it->second.first.end(); ++rit)
              valid |= (attic_tag_it.index().value != void_tag_value()
                  && (*rit)->matches(attic_tag_it.index().value));
            for (std::vector< std::string >::const_iterator rit = key_it->second.second.begin();
                rit != key_it->second.second.end(); ++rit)
              valid |= (*rit == attic_tag_it.index().value);
          }
          last_value = attic_tag_it.index().value;
        }

        if (valid && timestamp < attic_tag_it.object().timestamp &&
            (timestamp_ref.first == 0 || timestamp_ref.first > attic_tag_it.object().timestamp))
          timestamp_ref.first = attic_tag_it.object().timestamp;
      }
      ++attic_tag_it;
    }

    new_ids.clear();
    new_ids.reserve(timestamps.size());
    for (typename std::map< Id_Type, std::pair< uint64, uint64 > >::const_iterator
        it = timestamps.begin(); it != timestamps.end(); ++it)
    {
      if (!(0 < it->second.first && it->second.first <= it->second.second))
        new_ids.push_back(it->first);
    }
  }
  while ((!(tag_it == items_db.range_end())) &&
      ((tag_it.index().index) & 0x7fffff00) == coarse_index)
    ++tag_it;
  while ((!(attic_tag_it == attic_items_db.range_end())) &&
      ((attic_tag_it.index().index) & 0x7fffff00) == coarse_index)
    ++attic_tag_it;
}


template< class TIndex, class TObject >
void filter_by_ids(
    const std::map< uint32, std::vector< typename TObject::Id_Type > >& ids_by_coarse,
    std::map< TIndex, std::vector< TObject > >& items,
    std::map< TIndex, std::vector< Attic< TObject > > >* attic_items)
{
  std::map< TIndex, std::vector< TObject > > result;
  std::map< TIndex, std::vector< Attic< TObject > > > attic_result;

  typename std::map< TIndex, std::vector< TObject > >::const_iterator item_it = items.begin();

  if (!attic_items)
  {
    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::const_iterator
        it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    {
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->first))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (std::binary_search(it->second.begin(), it->second.end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }
  else
  {
    typename std::map< TIndex, std::vector< Attic< TObject > > >::const_iterator attic_item_it
        = attic_items->begin();

    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::const_iterator
        it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    {
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->first))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (std::binary_search(it->second.begin(), it->second.end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }

      while ((attic_item_it != attic_items->end()) &&
          ((attic_item_it->first.val() & 0x7fffff00) == it->first))
      {
        for (typename std::vector< Attic< TObject > >::const_iterator eit = attic_item_it->second.begin();
             eit != attic_item_it->second.end(); ++eit)
        {
          if (std::binary_search(it->second.begin(), it->second.end(), eit->id))
            attic_result[attic_item_it->first.val()].push_back(*eit);
        }
        ++attic_item_it;
      }
    }
  }

  items.swap(result);
  if (attic_items)
    attic_items->swap(attic_result);
}


template< class TIndex, class TObject >
void Query_Statement::filter_by_tags
    (std::map< TIndex, std::vector< TObject > >& items,
     std::map< TIndex, std::vector< Attic< TObject > > >* attic_items,
     uint64 timestamp, const File_Properties& file_prop, const File_Properties* attic_file_prop,
     Resource_Manager& rman, Transaction& transaction)
{
  if (keys.empty() && key_values.empty() && key_regexes.empty() && regkey_regexes.empty()
      && key_nregexes.empty() && key_nvalues.empty())
    return;

  // generate set of relevant coarse indices
  std::map< uint32, std::vector< typename TObject::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(ids_by_coarse, items);
  if (timestamp != NOW)
    generate_ids_by_coarse(ids_by_coarse, *attic_items);

  // formulate range query
  Ranges< Tag_Index_Local > ranges = formulate_range_query(ids_by_coarse);

  // prepare straight keys
  std::map< std::string, std::pair< std::string, std::vector< Regular_Expression* > > > key_union;
  for (std::vector< std::string >::const_iterator it = keys.begin(); it != keys.end();
      ++it)
    key_union[*it];
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = key_values.begin(); it != key_values.end();
      ++it)
    key_union[it->first].first = it->second;
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator
      it = key_regexes.begin(); it != key_regexes.end(); ++it)
    key_union[it->first].second.push_back(it->second);

  // iterate over the result
  std::map< TIndex, std::vector< TObject > > result;
  std::map< TIndex, std::vector< Attic< TObject > > > attic_result;
  uint coarse_count = 0;

  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  auto tag_it = items_db.range_begin(ranges);

  if (timestamp == NOW)
  {
    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator
        it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
      filter_ids_by_tags(key_union, regkey_regexes, items_db, tag_it, it->first, it->second);
    }
  }
  else
  {
    Block_Backend< Tag_Index_Local, Attic< typename TObject::Id_Type > > attic_items_db
        (transaction.data_index(attic_file_prop));
    auto attic_tag_it = attic_items_db.range_begin(ranges);

    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator
        it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
      filter_ids_by_tags(key_union, regkey_regexes, timestamp, items_db, tag_it, attic_items_db, attic_tag_it,
                         it->first, it->second);
    }
  }

  if (key_nregexes.empty() && key_nvalues.empty())
  {
    filter_by_ids(ids_by_coarse, items, attic_items);
    return;
  }

  // prepare negated keys
  std::map< std::string, std::pair< std::vector< Regular_Expression* >, std::vector< std::string > > > nkey_union;
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator
      it = key_nregexes.begin(); it != key_nregexes.end(); ++it)
    nkey_union[it->first].first.push_back(it->second);
  for (std::vector< std::pair< std::string, std::string > >::const_iterator
      it = key_nvalues.begin(); it != key_nvalues.end(); ++it)
    nkey_union[it->first].second.push_back(it->second);

  // iterate over the result
  result.clear();
  attic_result.clear();
  coarse_count = 0;
  auto ntag_it = items_db.range_begin(ranges);

  if (timestamp == NOW)
  {
    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator
        it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
      filter_ids_by_ntags(nkey_union, items_db, ntag_it, it->first, it->second);
    }
  }
  else
  {
    Block_Backend< Tag_Index_Local, Attic< typename TObject::Id_Type > > attic_items_db
        (transaction.data_index(attic_file_prop));
    auto attic_ntag_it = attic_items_db.range_begin(ranges);

    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator
        it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
      filter_ids_by_ntags(nkey_union, timestamp, items_db, ntag_it, attic_items_db, attic_ntag_it,
                          it->first, it->second);
    }
  }

  filter_by_ids(ids_by_coarse, items, attic_items);
}


template< class TIndex, class TObject >
void Query_Statement::filter_by_tags
    (std::map< TIndex, std::vector< TObject > >& items,
     const File_Properties& file_prop,
     Resource_Manager& rman, Transaction& transaction)
{
  if (keys.empty() && key_values.empty() && key_regexes.empty() && regkey_regexes.empty()
      && key_nregexes.empty() && key_nvalues.empty())
    return;

  // generate set of relevant coarse indices
  std::map< uint32, std::vector< typename TObject::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(ids_by_coarse, items);

  // formulate range query
  Ranges< Tag_Index_Local > ranges = formulate_range_query(ids_by_coarse);

  // prepare straight keys
  std::map< std::string, std::pair< std::string, std::vector< Regular_Expression* > > > key_union;
  for (std::vector< std::string >::const_iterator it = keys.begin(); it != keys.end();
      ++it)
    key_union[*it];
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = key_values.begin(); it != key_values.end();
      ++it)
    key_union[it->first].first = it->second;
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator
      it = key_regexes.begin(); it != key_regexes.end(); ++it)
    key_union[it->first].second.push_back(it->second);

  // iterate over the result
  std::map< TIndex, std::vector< TObject > > result;
  std::map< TIndex, std::vector< Attic< TObject > > > attic_result;
  uint coarse_count = 0;

  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  auto tag_it = items_db.range_begin(ranges);

  typename std::map< TIndex, std::vector< TObject > >::const_iterator item_it
      = items.begin();

  {
    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator it = ids_by_coarse.begin();
        it != ids_by_coarse.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
      std::vector< typename TObject::Id_Type >& ids_by_coarse_ = it->second;

      filter_ids_by_tags_old(ids_by_coarse_, key_union, regkey_regexes, items_db, tag_it, it->first);

      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->first))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
	     eit != item_it->second.end(); ++eit)
        {
          if (std::binary_search(ids_by_coarse_.begin(), ids_by_coarse_.end(), eit->id))
	    result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }

  items.swap(result);

  if (key_nregexes.empty() && key_nvalues.empty())
    return;

  // prepare negated keys
  std::map< std::string, std::pair< std::vector< Regular_Expression* >, std::vector< std::string > > > nkey_union;
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator
      it = key_nregexes.begin(); it != key_nregexes.end(); ++it)
    nkey_union[it->first].first.push_back(it->second);
  for (std::vector< std::pair< std::string, std::string > >::const_iterator
      it = key_nvalues.begin(); it != key_nvalues.end(); ++it)
    nkey_union[it->first].second.push_back(it->second);

  // iterate over the result
  result.clear();
  attic_result.clear();
  coarse_count = 0;
  auto ntag_it = items_db.range_begin(ranges);
  item_it = items.begin();

  {
    for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator it = ids_by_coarse.begin();
        it != ids_by_coarse.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
      std::vector< typename TObject::Id_Type >& ids_by_coarse_ = it->second;

      sort(ids_by_coarse_.begin(), ids_by_coarse_.end());

      filter_ids_by_ntags(nkey_union, items_db, ntag_it, it->first, ids_by_coarse_);

      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->first))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (std::binary_search(ids_by_coarse_.begin(), ids_by_coarse_.end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }

  items.swap(result);
}


template< typename Conditions, typename Tags, typename Single_Match >
bool matches_all_conditions(const Conditions& conds, const Tags& tags, const Single_Match& lambda)
{
  for (auto it_c = conds.begin(); it_c != conds.end(); ++it_c)
  {
    auto it_tags = tags.begin();
    for (; it_tags != tags.end(); ++it_tags)
    {
      if (lambda(*it_c, *it_tags))
        break;
    }
    if (it_tags == tags.end())
      return false;
  }
  return true;
}


template< typename Conditions, typename Tags, typename Single_Match >
bool matches_some_conditions(const Conditions& conds, const Tags& tags, const Single_Match& lambda)
{
  for (auto it_c = conds.begin(); it_c != conds.end(); ++it_c)
  {
    for (auto it_tags = tags.begin(); it_tags != tags.end(); ++it_tags)
    {
      if (lambda(*it_c, *it_tags))
        return true;
    }
  }
  return false;
}


void Query_Statement::filter_by_tags(std::map< Uint31_Index, std::vector< Derived_Structure > >& items)
{
  typedef std::pair< std::string, std::string > Tag_Pair;

  for (std::map< Uint31_Index, std::vector< Derived_Structure > >::iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    std::vector< Derived_Structure > result;
    for (std::vector< Derived_Structure >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      if (matches_all_conditions(key_values, it_elem->tags,
          [](const std::pair< std::string, std::string >& cond, const Tag_Pair& tag)
              { return tag.first == cond.first && tag.second == cond.second; })
          && matches_all_conditions(keys, it_elem->tags,
          [](const std::string& cond, const Tag_Pair& tag)
              { return tag.first == cond; })
          && matches_all_conditions(key_regexes, it_elem->tags,
          [](const std::pair< std::string, Regular_Expression* >& cond, const Tag_Pair& tag)
              { return tag.first == cond.first && cond.second->matches(tag.second); })
          && !matches_some_conditions(key_nvalues, it_elem->tags,
          [](const std::pair< std::string, std::string >& cond, const Tag_Pair& tag)
              { return tag.first == cond.first && cond.second == tag.second; })
          && !matches_some_conditions(key_nregexes, it_elem->tags,
          [](const std::pair< std::string, Regular_Expression* >& cond, const Tag_Pair& tag)
              { return tag.first == cond.first && cond.second->matches(tag.second); }))
        result.push_back(*it_elem);
    }
    result.swap(it_idx->second);
  }
}


template< typename Id >
void Id_Constraint< Id >::restrict_to(const std::vector< Id >& rhs)
{
  if (invert)
  {
    std::vector< Id > new_ids(rhs.size());
    new_ids.erase(std::set_difference(
        rhs.begin(), rhs.end(), ids.begin(), ids.end(), new_ids.begin()), new_ids.end());
    ids.swap(new_ids);
    invert = false;
  }
  else
  {
    std::vector< Id > new_ids(std::min(ids.size(), rhs.size()));
    new_ids.erase(std::set_intersection(
        ids.begin(), ids.end(), rhs.begin(), rhs.end(), new_ids.begin()), new_ids.end());
    ids.swap(new_ids);
  }
}


template< class Id_Type >
void Query_Statement::collect_nodes(
    const Id_Constraint< Id_Type >& ids, Answer_State& answer_state, Set& into, Resource_Manager& rman)
{
  for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end() && answer_state < data_collected; ++it)
  {
    if ((*it)->collect_nodes(rman, into, ids.ids, ids.invert))
      answer_state = data_collected;
  }
}


template< typename Id_Type >
void Query_Statement::collect_elems(
    int type, const Id_Constraint< Id_Type >& ids, Answer_State& answer_state, Set& into, Resource_Manager& rman)
{
  for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end() && answer_state < data_collected; ++it)
  {
    if ((*it)->collect(rman, into, type, ids.ids, ids.invert))
      answer_state = data_collected;
  }
}


void Query_Statement::collect_elems(Answer_State& answer_state, Set& into, Resource_Manager& rman)
{
  for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end() && answer_state < data_collected; ++it)
  {
    if ((*it)->collect(rman, into))
      answer_state = data_collected;
  }
}


template< typename Index >
Ranges< Index > intersect_ranges(const Ranges< Index >& range_a, std::vector< Index >& range_vec)
{
  Ranges< Index > result;

  unsigned long long sum = 0;
  for (auto it = range_a.begin(); it != range_a.end(); ++it)
    sum += difference(it.lower_bound(), it.upper_bound());

  if (sum/256 < range_vec.size())
    return range_a;

  std::sort(range_vec.begin(), range_vec.end());

  auto it_a = range_a.begin();
  auto it_vec = range_vec.begin();

  while (it_a != range_a.end() && it_vec != range_vec.end())
  {
    if (!(it_a.lower_bound() < Index(it_vec->val() + 0x100)))
      ++it_vec;
    else if (!(*it_vec < it_a.upper_bound()))
      ++it_a;
    else if (Index(it_vec->val() + 0x100) < it_a.upper_bound())
    {
      result.push_back(std::max(it_a.lower_bound(), *it_vec), Index(it_vec->val() + 0x100));
      ++it_vec;
    }
    else
    {
      result.push_back(std::max(it_a.lower_bound(), *it_vec), it_a.upper_bound());
      ++it_a;
    }
  }

  return result;
}


void Query_Statement::apply_all_filters(
    Resource_Manager& rman, uint64 timestamp, Query_Filter_Strategy check_keys_late, Set& into)
{
  set_progress(5);
  rman.health_check(*this);

  for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(rman, into);

  set_progress(6);
  rman.health_check(*this);

  Request_Context context(this, rman);
  filter_attic_elements(context, timestamp, into.nodes, into.attic_nodes);
  filter_attic_elements(context, timestamp, into.ways, into.attic_ways);
  filter_attic_elements(context, timestamp, into.relations, into.attic_relations);

  set_progress(7);
  rman.health_check(*this);

  if (check_keys_late == prefer_ranges)
  {
    filter_by_tags(into.nodes, &into.attic_nodes, timestamp,
                   *osm_base_settings().NODE_TAGS_LOCAL, attic_settings().NODE_TAGS_LOCAL,
		   rman, *rman.get_transaction());
    filter_by_tags(into.ways, &into.attic_ways, timestamp,
                   *osm_base_settings().WAY_TAGS_LOCAL, attic_settings().WAY_TAGS_LOCAL,
		   rman, *rman.get_transaction());
    filter_by_tags(into.relations, &into.attic_relations, timestamp,
                   *osm_base_settings().RELATION_TAGS_LOCAL, attic_settings().RELATION_TAGS_LOCAL,
		   rman, *rman.get_transaction());
    if (rman.get_area_transaction())
      filter_by_tags(into.areas,
                     *area_settings().AREA_TAGS_LOCAL,
		     rman, *rman.get_transaction());
  }

  set_progress(8);
  rman.health_check(*this);

  for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(*this, rman, into);
}


void filter_elems_for_closed_ways(Set& arg)
{
  filter_elems_for_closed_ways(arg.ways);
  filter_elems_for_closed_ways(arg.attic_ways);
}


void Query_Statement::execute(Resource_Manager& rman)
{
  Cpu_Timer cpu(rman, 1);

  Answer_State node_answer_state = nothing;
  Answer_State way_answer_state = nothing;
  Answer_State relation_answer_state = nothing;
  Answer_State area_answer_state = nothing;
  Answer_State derived_answer_state = nothing;
  Set into;
  Set filtered;
  uint64 timestamp = rman.get_desired_timestamp();
  if (timestamp == 0)
    timestamp = NOW;

  set_progress(1);
  rman.health_check(*this);

  Query_Filter_Strategy check_keys_late = ids_required;
  for (std::vector< Query_Constraint* >::iterator it = constraints.begin(); it != constraints.end(); ++it)
    check_keys_late = std::max(check_keys_late, (*it)->delivers_data(rman));

  {
    Id_Constraint< Node::Id_Type > node_ids;
    Id_Constraint< Way::Id_Type > way_ids;
    Id_Constraint< Relation::Id_Type > relation_ids;
    Id_Constraint< Area_Skeleton::Id_Type > area_ids;

    std::vector< Uint32_Index > range_vec_32;
    std::vector< Uint31_Index > way_range_vec_31;
    std::vector< Uint31_Index > relation_range_vec_31;

    if (type & QUERY_NODE)
    {
      progress_1< Node_Skeleton, Node::Id_Type, Uint32_Index >(
          keys, key_values, key_regexes, regkey_regexes, key_nvalues, key_nregexes, regkey_nregexes,
          node_ids, range_vec_32, timestamp, check_keys_late, rman, *this);
      if (node_ids.empty())
        node_answer_state = data_collected;
      collect_nodes(node_ids, node_answer_state, into, rman);
    }
    if (type & QUERY_WAY)
    {
      progress_1< Way_Skeleton, Way::Id_Type, Uint31_Index >(
	  keys, key_values, key_regexes, regkey_regexes, key_nvalues, key_nregexes, regkey_nregexes,
          way_ids, way_range_vec_31, timestamp, check_keys_late, rman, *this);
      if (way_ids.empty())
        way_answer_state = data_collected;
      collect_elems(QUERY_WAY, way_ids, way_answer_state, into, rman);
      if (type & QUERY_CLOSED_WAY)
        filter_elems_for_closed_ways(into);
    }
    if (type & QUERY_RELATION)
    {
      progress_1< Relation_Skeleton, Relation::Id_Type, Uint31_Index >(
	  keys, key_values, key_regexes, regkey_regexes, key_nvalues, key_nregexes, regkey_nregexes,
          relation_ids, relation_range_vec_31, timestamp, check_keys_late, rman, *this);
      if (relation_ids.empty())
        relation_answer_state = data_collected;
      collect_elems(QUERY_RELATION, relation_ids, relation_answer_state, into, rman);
    }
    if (type & QUERY_DERIVED)
    {
      collect_elems(derived_answer_state, into, rman);
      filter_by_tags(into.deriveds);
    }
    if (type & QUERY_AREA)
    {
      try
      {
        progress_1(
          keys, key_values, key_regexes, regkey_regexes, key_nvalues, key_nregexes, regkey_nregexes,
          area_ids, check_keys_late, rman, *this);
        if (area_ids.empty())
          area_answer_state = data_collected;
        collect_elems(QUERY_AREA, area_ids, area_answer_state, into, rman);
      }
      catch (const File_Error& e)
      {
        if (e.error_number != ENOENT)
          throw;
        area_answer_state = data_collected;
      }
    }

    set_progress(2);
    rman.health_check(*this);

    if ((type & QUERY_NODE) && check_keys_late == prefer_ranges)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && node_answer_state < data_collected; ++it)
      {
        std::vector< Node::Id_Type > constraint_node_ids;
        if ((*it)->get_node_ids(rman, constraint_node_ids))
        {
          node_ids.restrict_to(constraint_node_ids);
          if (node_ids.empty())
            node_answer_state = data_collected;
        }
      }
    }
    if ((type & QUERY_WAY) && check_keys_late == prefer_ranges)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && way_answer_state < data_collected; ++it)
      {
        std::vector< Way::Id_Type > constraint_way_ids;
        if ((*it)->get_way_ids(rman, constraint_way_ids))
        {
          way_ids.restrict_to(constraint_way_ids);
          if (way_ids.empty())
            way_answer_state = data_collected;
        }
      }
    }
    if ((type & QUERY_RELATION) && check_keys_late == prefer_ranges)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && relation_answer_state < data_collected; ++it)
      {
        std::vector< Relation::Id_Type > constraint_relation_ids;
        if ((*it)->get_relation_ids(rman, constraint_relation_ids))
        {
          relation_ids.restrict_to(constraint_relation_ids);
          if (relation_ids.empty())
            relation_answer_state = data_collected;
        }
      }
    }
    if ((type & QUERY_AREA) && check_keys_late == prefer_ranges)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && area_answer_state < data_collected; ++it)
      {
        std::vector< Area_Skeleton::Id_Type > constraint_area_ids;
        if ((*it)->get_area_ids(rman, constraint_area_ids))
        {
          area_ids.restrict_to(constraint_area_ids);
          if (area_ids.empty())
            area_answer_state = data_collected;
        }
      }
    }

    Ranges< Uint32_Index > node_ranges = Ranges< Uint32_Index >::global();
    Ranges< Uint31_Index > way_ranges = Ranges< Uint31_Index >::global();
    Ranges< Uint31_Index > rel_ranges = Ranges< Uint31_Index >::global();

    if ((type & QUERY_NODE) && node_answer_state < data_collected)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end(); ++it)
        node_ranges.intersect((*it)->get_node_ranges(rman)).swap(node_ranges);

      if (!range_vec_32.empty())
        intersect_ranges(node_ranges, range_vec_32).swap(node_ranges);
    }
    if ((type & QUERY_WAY) && way_answer_state < data_collected)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end(); ++it)
        way_ranges.intersect((*it)->get_way_ranges(rman)).swap(way_ranges);

      if (!way_range_vec_31.empty())
        intersect_ranges(way_ranges, way_range_vec_31).swap(way_ranges);
    }
    if ((type & QUERY_RELATION) && relation_answer_state < data_collected)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end(); ++it)
        rel_ranges.intersect((*it)->get_relation_ranges(rman)).swap(rel_ranges);

      if (!relation_range_vec_31.empty())
        intersect_ranges(rel_ranges, relation_range_vec_31).swap(rel_ranges);
    }

    set_progress(3);
    rman.health_check(*this);

    if (type & QUERY_NODE)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && node_answer_state < data_collected; ++it)
      {
	if ((*it)->get_data(*this, rman, into, node_ranges, node_ids.ids, node_ids.invert))
	  node_answer_state = data_collected;
      }
    }
    if (type & QUERY_WAY)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && way_answer_state < data_collected; ++it)
      {
	if ((*it)->get_data(*this, rman, into, way_ranges, type & QUERY_WAY, way_ids.ids, way_ids.invert))
        {
          if (type & QUERY_CLOSED_WAY)
            filter_elems_for_closed_ways(into);
	  way_answer_state = data_collected;
        }
      }
    }
    if (type & QUERY_RELATION)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && relation_answer_state < data_collected; ++it)
      {
	if ((*it)->get_data(*this, rman, into, rel_ranges, type & QUERY_RELATION,
            relation_ids.ids, relation_ids.invert))
	  relation_answer_state = data_collected;
      }
    }
    if (type & QUERY_AREA)
    {
      for (std::vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && area_answer_state < data_collected; ++it)
      {
	if ((*it)->get_data(*this, rman, into, {}, type & QUERY_AREA, area_ids.ids, area_ids.invert))
	  area_answer_state = data_collected;
      }
    }

    set_progress(4);
    rman.health_check(*this);
    Request_Context context(this, rman);

    if (type & QUERY_NODE)
    {
      if (node_answer_state < data_collected)
      {
        if (node_ranges.is_global())
        {
          if (node_ids.invert)
          {
            if (node_ids.ids.empty())
              runtime_error("Filters too weak in query statement: specify in addition a bbox, a tag filter, or similar.");
          }
          else
          {
            node_ranges = Ranges< Uint32_Index >();
            std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(node_ids.ids, context);
            for (std::vector< Uint32_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
              node_ranges.push_back(*it, ++Uint32_Index(*it));
            node_ranges.sort();
          }
        }

        Collect_Items< Uint32_Index, Node_Skeleton > db_reader(
            node_ids.ids, node_ids.invert, node_ranges, *this, rman);
        while (db_reader.get_chunk(into.nodes, into.attic_nodes))
        {
          Set to_filter;
          to_filter.nodes.swap(into.nodes);
          to_filter.attic_nodes.swap(into.attic_nodes);
          apply_all_filters(rman, timestamp, check_keys_late, to_filter);
          indexed_set_union(filtered.nodes, to_filter.nodes);
          indexed_set_union(filtered.attic_nodes, to_filter.attic_nodes);
        }
      }
    }
    if (type & QUERY_WAY)
    {
      if (way_answer_state < data_collected)
      {
        if (way_ranges.is_global())
        {
          if (way_ids.invert)
          {
            if (way_ids.ids.empty())
              runtime_error("Filters too weak in query statement: specify in addition a bbox, a tag filter, or similar.");
          }
          else
          {
            way_ranges = Ranges< Uint31_Index >();
            std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(way_ids.ids, context);
            for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
              way_ranges.push_back(*it, inc(*it));
            way_ranges.sort();
          }
        }

        Collect_Items< Uint31_Index, Way_Skeleton > db_reader(
            way_ids.ids, way_ids.invert, way_ranges, *this, rman);
        while (db_reader.get_chunk(into.ways, into.attic_ways))
        {
          Set to_filter;
          to_filter.ways.swap(into.ways);
          to_filter.attic_ways.swap(into.attic_ways);
          if (type & QUERY_CLOSED_WAY)
          {
            filter_elems_for_closed_ways(to_filter.ways);
            filter_elems_for_closed_ways(to_filter.attic_ways);
          }
          apply_all_filters(rman, timestamp, check_keys_late, to_filter);
          indexed_set_union(filtered.ways, to_filter.ways);
          indexed_set_union(filtered.attic_ways, to_filter.attic_ways);
        }
      }
    }
    if (type & QUERY_RELATION)
    {
      if (relation_answer_state < data_collected)
      {
        if (rel_ranges.is_global())
        {
          if (!relation_ids.invert)
          {
            rel_ranges = Ranges< Uint31_Index >();
            std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Relation_Skeleton >(relation_ids.ids, context);
            for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
              rel_ranges.push_back(*it, inc(*it));
            rel_ranges.sort();
          }
        }

        Collect_Items< Uint31_Index, Relation_Skeleton > db_reader(
            relation_ids.ids, relation_ids.invert, rel_ranges, *this, rman);
        while (db_reader.get_chunk(into.relations, into.attic_relations))
        {
          Set to_filter;
          to_filter.relations.swap(into.relations);
          to_filter.attic_relations.swap(into.attic_relations);
          apply_all_filters(rman, timestamp, check_keys_late, to_filter);
          indexed_set_union(filtered.relations, to_filter.relations);
          indexed_set_union(filtered.attic_relations, to_filter.attic_relations);
        }
      }
    }
    if (type & QUERY_AREA)
    {
      if (area_answer_state == nothing && area_ids.empty())
        runtime_error("Filters too weak in query statement: specify in addition a bbox, a tag filter, or similar.");
      try
      {
        if (area_answer_state < data_collected)
          get_elements_by_id_from_db(into.areas, area_ids.ids, area_ids.invert, rman, *area_settings().AREAS);
      }
      catch (const File_Error& e)
      {
        if (e.error_number != ENOENT)
          throw;
      }
    }
  }

  if (type & QUERY_CLOSED_WAY)
    filter_elems_for_closed_ways(into);
  apply_all_filters(rman, timestamp, check_keys_late, into);
  indexed_set_union(into.nodes, filtered.nodes);
  indexed_set_union(into.attic_nodes, filtered.attic_nodes);
  indexed_set_union(into.ways, filtered.ways);
  indexed_set_union(into.attic_ways, filtered.attic_ways);
  indexed_set_union(into.relations, filtered.relations);
  indexed_set_union(into.attic_relations, filtered.attic_relations);
  indexed_set_union(into.areas, filtered.areas);
  indexed_set_union(into.deriveds, filtered.deriveds);

  set_progress(9);
  rman.health_check(*this);

  clear_empty_indices(into.nodes);
  clear_empty_indices(into.attic_nodes);
  clear_empty_indices(into.ways);
  clear_empty_indices(into.attic_ways);
  clear_empty_indices(into.relations);
  clear_empty_indices(into.deriveds);
  clear_empty_indices(into.areas);

  transfer_output(rman, into);
  rman.health_check(*this);
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Has_Kv_Statement > Has_Kv_Statement::statement_maker("has-kv");

Has_Kv_Statement::Has_Kv_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), regex(0), key_regex(0), straight(true), case_sensitive(false)
{
  std::map< std::string, std::string > attributes;

  attributes["k"] = "";
  attributes["regk"] = "";
  attributes["v"] = "";
  attributes["regv"] = "";
  attributes["modv"] = "";
  attributes["case"] = "sensitive";

  eval_attributes_array(get_name(), attributes, input_attributes);

  key = attributes["k"];
  value = attributes["v"];

  if (key == "" && attributes["regk"] == "")
  {
    std::ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }

  if (attributes["case"] != "ignore")
  {
    if (attributes["case"] != "sensitive")
      add_static_error("For the attribute \"case\" of the element \"has-kv\""
	  " the only allowed values are \"sensitive\" or \"ignore\".");
    case_sensitive = true;
  }

  if (attributes["regk"] != "")
  {
    if (key != "")
    {
      std::ostringstream temp("");
      temp<<"In the element \"has-kv\" only one of the attributes \"k\" and \"regk\""
            " can be nonempty.";
      add_static_error(temp.str());
    }
    if (value != "")
    {
      std::ostringstream temp("");
      temp<<"In the element \"has-kv\" the attribute \"regk\" must be combined with \"regv\".";
      add_static_error(temp.str());
    }

    try
    {
      key_regex = new Regular_Expression(attributes["regk"], case_sensitive);
      key = attributes["regk"];
    }
    catch (Regular_Expression_Error e)
    {
      add_static_error("Invalid regular expression: \"" + attributes["regk"] + "\"");
    }
  }

  if (attributes["regv"] != "")
  {
    if (value != "")
    {
      std::ostringstream temp("");
      temp<<"In the element \"has-kv\" only one of the attributes \"v\" and \"regv\""
            " can be nonempty.";
      add_static_error(temp.str());
    }

    try
    {
      regex = new Regular_Expression(attributes["regv"], case_sensitive);
      value = attributes["regv"];
    }
    catch (Regular_Expression_Error e)
    {
      add_static_error("Invalid regular expression: \"" + attributes["regv"] + "\"");
    }
  }

  if (attributes["modv"] == "" || attributes["modv"] == "not")
  {
    if (attributes["modv"] == "not")
    {
      if (attributes["regk"] == "")
        straight = false;
      else
	add_static_error("In the element \"has-kv\" regular expressions on keys cannot be combined"
	  " with negation.");
    }
  }
  else
  {
    std::ostringstream temp("");
    temp<<"In the element \"has-kv\" the attribute \"modv\" can only be empty or std::set to \"not\".";
    add_static_error(temp.str());
  }
}


Has_Kv_Statement::~Has_Kv_Statement()
{
  delete key_regex;
  delete regex;
}
