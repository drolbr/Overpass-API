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

#ifndef DE__OSM3S___OVERPASS_API__DATA__FILTER_BY_TAGS_H
#define DE__OSM3S___OVERPASS_API__DATA__FILTER_BY_TAGS_H


#include "regular_expression.h"


Ranges< Tag_Index_Global > get_kv_req(const std::string& key, const std::string& value)
{
  return Ranges< Tag_Index_Global >(
      Tag_Index_Global{ key, value }, Tag_Index_Global{ key, value + (char)0 });
}


Ranges< Tag_Index_Global > get_k_req(const std::string& key)
{
  return Ranges< Tag_Index_Global >(
      Tag_Index_Global{ key, "" }, Tag_Index_Global{ key + (char)0, "" });
}


template< typename Skeleton >
Ranges< Tag_Index_Global > get_regk_req(Regular_Expression* key, Resource_Manager& rman, const Statement& stmt)
{
  Ranges< Tag_Index_Global > result;

  Block_Backend< Uint32_Index, String_Object > db
      (rman.get_transaction()->data_index(key_file_properties< Skeleton >()));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
       it(db.flat_begin()); !(it == db.flat_end()); ++it)
  {
    if (key->matches(it.object().val()))
      result.push_back({ it.object().val(), "" }, { it.object().val() + (char)0, "" });
  }
  result.sort();
  rman.health_check(stmt);

  return result;
}


#endif
