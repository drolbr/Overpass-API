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
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"

using namespace std;


void process_user_data(Transaction& transaction, map< uint32, string >& user_by_id,
		       map< uint32, vector< uint32 > >& idxs_by_user_id)
{
  {
    map< Uint32_Index, set< User_Data > > db_to_delete;
    map< Uint32_Index, set< User_Data > > db_to_insert;
  
    for (map< uint32, string >::const_iterator it = user_by_id.begin();
        it != user_by_id.end(); ++it)
    {
      User_Data user_data;
      user_data.id = it->first;
      db_to_delete[Uint32_Index(it->first & 0xffffff00)].insert(user_data);
    }
    for (map< uint32, string >::const_iterator it = user_by_id.begin();
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
    map< Uint32_Index, set< Uint31_Index > > db_to_delete;
    map< Uint32_Index, set< Uint31_Index > > db_to_insert;
  
    for (map< uint32, vector< uint32 > >::const_iterator it = idxs_by_user_id.begin();
        it != idxs_by_user_id.end(); ++it)
    {
      set< Uint31_Index >& ins = db_to_delete[it->first];
      for (vector< uint32 >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
	ins.insert(Uint31_Index(*it2));
    }
    for (map< uint32, vector< uint32 > >::const_iterator it = idxs_by_user_id.begin();
        it != idxs_by_user_id.end(); ++it)
    {
      set< Uint31_Index >& ins = db_to_insert[it->first];
      for (vector< uint32 >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
	ins.insert(Uint31_Index(*it2));
    }
  
    Block_Backend< Uint32_Index, Uint31_Index > user_db
        (transaction.data_index(meta_settings().USER_INDICES));
    user_db.update(db_to_delete, db_to_insert);
  }
}


Transaction_Collection::Transaction_Collection
    (bool writeable, bool use_shadow,
     const string& db_dir, const vector< string >& file_name_extensions_)
     : file_name_extensions(file_name_extensions_)
{
  for (vector< string >::const_iterator it = file_name_extensions.begin();
      it != file_name_extensions.end(); ++it)
    transactions.push_back(new Nonsynced_Transaction(writeable, use_shadow, db_dir, *it));
}

Transaction_Collection::~Transaction_Collection()
{
  for (vector< Transaction* >::const_iterator it = transactions.begin();
      it != transactions.end(); ++it)
    delete(*it);
}

void Transaction_Collection::remove_referred_files(const File_Properties& file_prop)
{
  for (vector< string >::const_iterator it = file_name_extensions.begin();
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

void rename_referred_file(const string& db_dir, const string& from, const string& to,
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
