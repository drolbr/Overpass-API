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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "node_updater.h"


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" db_dir\n";
    return 0;
  }

  string db_dir(args[1]);

  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Nonsynced_Transaction target_transaction(true, false, db_dir, "_compressed_");

    Block_Backend< Uint31_Index, Way_Skeleton > current_db
        (transaction.data_index(osm_base_settings().WAYS));
    Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator current_it(current_db.flat_begin());

    Block_Backend< Uint31_Index, Attic< Way_Skeleton > > attic_db
        (transaction.data_index(attic_settings().WAYS));
    Block_Backend< Uint31_Index, Attic< Way_Skeleton > >::Flat_Iterator attic_it(attic_db.flat_begin());

    std::map< Uint31_Index, std::set< Attic< Way_Delta > > > compressed_set;
    uint compressed_set_total_size = 0;

    while (!(attic_it == attic_db.flat_end()))
    {
      Uint31_Index current_idx = attic_it.index();

      std::cerr<<'.';

      while (!(current_it == current_db.flat_end()) && current_it.index() < current_idx)
        ++current_it;

      std::vector< Attic< Way_Skeleton > > ways;
      std::vector< Attic< Way_Delta > > ways_compressed;

      while (!(current_it == current_db.flat_end()) && current_it.index() == current_idx)
      {
        ways.push_back(Attic< Way_Skeleton >(current_it.object(), NOW));
        ++current_it;
      }
      while (!(attic_it == attic_db.flat_end()) && attic_it.index() == current_idx)
      {
        ways.push_back(attic_it.object());
        ++attic_it;
      }

      std::sort(ways.begin(), ways.end());

      std::vector< Attic< Way_Skeleton > >::const_iterator next = ways.begin();
      if (next != ways.end())
        ++next;
      for (std::vector< Attic< Way_Skeleton > >::const_iterator it = ways.begin(); it != ways.end(); ++it)
      {
        if (it->timestamp == NOW)
          ;
        else if (next == ways.end() || !(next->id == it->id))
          ways_compressed.push_back(Attic< Way_Delta >(
              Way_Delta(Attic< Way_Skeleton >(Way_Skeleton(), NOW), *it), it->timestamp));
        else
          ways_compressed.push_back(Attic< Way_Delta >(
              Way_Delta(*next, *it), it->timestamp));
        ++next;
      }

      compressed_set_total_size += ways_compressed.size();
      std::set< Attic< Way_Delta > >& compressed_ref = compressed_set[current_idx];

      for (std::vector< Attic< Way_Delta > >::const_iterator it = ways_compressed.begin();
           it != ways_compressed.end(); ++it)
        compressed_ref.insert(*it);

      if (compressed_set_total_size > 65*536)
      {
        Block_Backend< Uint31_Index, Attic< Way_Delta > >
            db(target_transaction.data_index(attic_settings().WAYS));
        db.update(std::map< Uint31_Index, std::set< Attic< Way_Delta > > >(), compressed_set);

        compressed_set.clear();
        compressed_set_total_size = 0;
      }
    }

    while (!(current_it == current_db.flat_end()))
      ++current_it;
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }

  return 0;
}
