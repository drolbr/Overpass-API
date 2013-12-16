/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#include "basic_updater.h"


void Key_Storage::flush_keys(Transaction& transaction)
{
  map< Uint32_Index, set< String_Object > > db_to_delete;
  map< Uint32_Index, set< String_Object > > db_to_insert;
  
  for (std::map< std::string, uint32 >::const_iterator it = key_ids.begin(); it != key_ids.end(); ++it)
  {
    if (it->second >= max_written_key_id)
      db_to_insert[Uint32_Index(it->second)].insert(String_Object(it->first));
    if (it->second >= max_key_id)
      max_key_id = it->second + 1;
  }
  
  Block_Backend< Uint32_Index, String_Object > keys_db(transaction.data_index(file_properties));
  keys_db.update(db_to_delete, db_to_insert);
  max_written_key_id = max_key_id;
}


void Key_Storage::load_keys(Transaction& transaction)
{
  Block_Backend< Uint32_Index, String_Object > keys_db(transaction.data_index(file_properties));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
      it = keys_db.flat_begin(); !(it == keys_db.flat_end()); ++it)
  {
    key_ids[it.object().val()] = it.index().val();
    if (max_key_id <= it.index().val())
      max_key_id = it.index().val()+1;
  }
  max_written_key_id = max_key_id;
}


void Key_Storage::register_key(const string& s)
{
  map< string, uint32 >::const_iterator it(key_ids.find(s));
  if (it != key_ids.end())
    return;
  key_ids[s] = max_key_id;
  ++max_key_id;
}
