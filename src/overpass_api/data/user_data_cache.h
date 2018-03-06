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

#ifndef DE__OSM3S___OVERPASS_API__DATA__USER_DATA_CACHE_H
#define DE__OSM3S___OVERPASS_API__DATA__USER_DATA_CACHE_H


#include <map>
#include <string>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

struct User_Data_Cache
{
  User_Data_Cache() : loaded(false) {}
  const std::map< uint32, std::string >& users(Transaction& transaction);

private:
  std::map< uint32, std::string > users_;
  bool loaded;
};


inline const std::map< uint32, std::string >& User_Data_Cache::users(
    Transaction& transaction)
{
  if (!loaded)
  {
    Block_Backend< Uint32_Index, User_Data > user_db
        (transaction.data_index(meta_settings().USER_DATA));
    for (Block_Backend< Uint32_Index, User_Data >::Flat_Iterator it = user_db.flat_begin();
        !(it == user_db.flat_end()); ++it)
      users_[it.object().id] = it.object().name;

    loaded = true;
  }

  return users_;
}


#endif
