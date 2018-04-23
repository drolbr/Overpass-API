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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../data/abstract_processing.h"
#include "../dispatch/resource_manager.h"
#include "../frontend/console_output.h"


template< typename Id_Type >
struct Local_Key_Entry
{
  Local_Key_Entry(Id_Type id_, long timestamp_) : id(id_), timestamp(timestamp_) {}

  Id_Type id;
  long timestamp;

  bool operator<(const Local_Key_Entry& rhs) const
  {
    if (id < rhs.id)
      return true;
    if (rhs.id < id)
      return false;
    return timestamp < rhs.timestamp;
  }
};


template< typename Id_Type >
void eval_local_key_entries(
    std::vector< Local_Key_Entry< Id_Type > >& local_key_entries,
    const std::string& last_key)
{
  std::sort(local_key_entries.begin(), local_key_entries.end());
  for (typename std::vector< Local_Key_Entry< Id_Type > >::size_type i = 0; i+1 < local_key_entries.size(); ++i)
  {
    if (local_key_entries[i].timestamp == local_key_entries[i+1].timestamp
	&& local_key_entries[i].id == local_key_entries[i+1].id)
      std::cerr<<"Multiple entries for way "<<local_key_entries[i].id.val()<<" at timestamp "
	  <<Timestamp(local_key_entries[i].timestamp).str()<<" and key "<<last_key<<'\n';
  }
}


int main(int argc, char* args[])
{
  if (argc < 3)
  {
    std::cout<<"Usage: "<<args[0]<<" db_dir target [--index=INDEX]\n";
    return 0;
  }

  string db_dir(args[1]);

  uint32 index_int;
  bool index_used = false;
  if (argc >= 4)
  {
    index_used = true;
    std::string index_s(args[3]);
    if (index_s.size() >= 10 && index_s.substr(8, 2) == "0x")
    {
      std::istringstream in(&args[3][10]);
      in>>std::hex>>index_int;
    }
    else
    {
      std::istringstream in(&args[3][8]);
      in>>index_int;
    }
  }
  Uint31_Index index(index_int);

  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Console_Output error_output(Error_Output::ASSISTING);
    Resource_Manager rman(transaction, 0, &error_output);
    rman.set_desired_timestamp(0);

    for (unsigned int i = 0; i < 0x80000000u; i += 0x10000)
    {
      std::cerr<<"Checking way index "<<std::hex<<i<<" ...";
      std::set< std::pair< Uint31_Index, Uint31_Index > > range_req;
      range_req.insert(std::make_pair(Uint31_Index(i), Uint31_Index(i + 0x10000)));

      std::map< Uint31_Index, std::vector< Way_Skeleton > > elements;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_elements;

      collect_items_range_by_timestamp(0, rman, range_req,
	  Trivial_Predicate< Way_Skeleton >(), elements, attic_elements);

      std::cerr<<' '<<std::dec<<elements.size() + attic_elements.size()<<" indexes reconstructed.\n";
    }

    {
      Uint31_Index last_index(0xffffffffu);
      std::string last_key = " ";
      last_key[1] = 0xff;
      std::vector< Local_Key_Entry< Way_Skeleton::Id_Type > > local_key_entries;

      Block_Backend< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().WAY_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >::Flat_Iterator
          it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        if (!(last_index == it.index().index) || last_key != it.index().key)
        {
	  eval_local_key_entries(local_key_entries, last_key);
	  local_key_entries.clear();

	  last_index = it.index().index;
	  last_key = it.index().key;
        }

        local_key_entries.push_back(Local_Key_Entry< Way_Skeleton::Id_Type >(it.object(), it.object().timestamp));
      }

      eval_local_key_entries(local_key_entries, last_key);
    }

    for (unsigned int i = 0; i < 0x80000000u; i += 0x10000)
    {
      std::cerr<<"Checking relation index "<<std::hex<<i<<" ...";
      std::set< std::pair< Uint31_Index, Uint31_Index > > range_req;
      range_req.insert(std::make_pair(Uint31_Index(i), Uint31_Index(i + 0x10000)));

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > elements;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_elements;

      collect_items_range_by_timestamp(0, rman, range_req,
	  Trivial_Predicate< Relation_Skeleton >(), elements, attic_elements);

      std::cerr<<' '<<std::dec<<elements.size() + attic_elements.size()<<" indexes reconstructed.\n";
    }

    {
      Uint31_Index last_index(0xffffffffu);
      std::string last_key = " ";
      last_key[1] = 0xff;
      std::vector< Local_Key_Entry< Relation_Skeleton::Id_Type > > local_key_entries;

      Block_Backend< Tag_Index_Local, Attic< Relation_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().RELATION_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Attic< Relation_Skeleton::Id_Type > >::Flat_Iterator
          it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        if (!(last_index == it.index().index) || last_key != it.index().key)
        {
	  eval_local_key_entries(local_key_entries, last_key);
	  local_key_entries.clear();

	  last_index = it.index().index;
	  last_key = it.index().key;
        }

        local_key_entries.push_back(Local_Key_Entry< Relation_Skeleton::Id_Type >(it.object(), it.object().timestamp));
      }

      eval_local_key_entries(local_key_entries, last_key);
    }
  }
  catch (File_Error e)
  {
    std::cerr<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }


  return 0;
}
