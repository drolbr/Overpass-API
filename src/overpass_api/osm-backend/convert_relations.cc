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

    Block_Backend< Uint31_Index, Relation_Skeleton > current_db
        (transaction.data_index(osm_base_settings().RELATIONS));
    Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator current_it(current_db.flat_begin());
        
    Block_Backend< Uint31_Index, Attic< Relation_Skeleton > > attic_db
        (transaction.data_index(attic_settings().RELATIONS));
    Block_Backend< Uint31_Index, Attic< Relation_Skeleton > >::Flat_Iterator attic_it(attic_db.flat_begin());
    
    std::map< Uint31_Index, std::set< Attic< Relation_Delta > > > compressed_set;
    uint compressed_set_total_size = 0;
    
    while (!(attic_it == attic_db.flat_end()))
    {
      Uint31_Index current_idx = attic_it.index();
      
      std::cerr<<'.';
      
      while (!(current_it == current_db.flat_end()) && current_it.index() < current_idx)
      {
//         cout<<hex<<current_it.index().val()<<'\t'
//             <<dec<<current_it.object().id.val()
//             <<'\t'<<current_it.object().members.size()
//             <<'\t'<<current_it.object().node_idxs.size()
//             <<'\t'<<current_it.object().way_idxs.size()<<'\n';
        ++current_it;
      }
      
//       std::cout<<'\n';
      std::vector< Attic< Relation_Skeleton > > rels;
      std::vector< Attic< Relation_Delta > > rels_compressed;
      
      while (!(current_it == current_db.flat_end()) && current_it.index() == current_idx)
      {
        rels.push_back(Attic< Relation_Skeleton >(current_it.object(), NOW));
//         cout<<hex<<current_it.index().val()<<'\t'
//             <<dec<<current_it.object().id.val()
//             <<'\t'<<current_it.object().members.size()
//             <<'\t'<<current_it.object().node_idxs.size()
//             <<'\t'<<current_it.object().way_idxs.size()<<'\n';
        ++current_it;
      }
      while (!(attic_it == attic_db.flat_end()) && attic_it.index() == current_idx)
      {
        rels.push_back(attic_it.object());
//         cout<<hex<<attic_it.index().val()<<'\t'
//             <<dec<<attic_it.object().id.val()
//             <<'\t'<<attic_it.object().members.size()
//             <<'\t'<<attic_it.object().node_idxs.size()
//             <<'\t'<<attic_it.object().way_idxs.size()<<'\t'
//             <<attic_it.object().timestamp<<'\n';
        ++attic_it;
      }
      
      std::sort(rels.begin(), rels.end());
      
      std::vector< Attic< Relation_Skeleton > >::const_iterator next = rels.begin();
      if (next != rels.end())
        ++next;
      for (std::vector< Attic< Relation_Skeleton > >::const_iterator it = rels.begin(); it != rels.end(); ++it)
      {
        if (it->timestamp == NOW)
          ;
        else if (next == rels.end() || !(next->id == it->id))
          rels_compressed.push_back(Attic< Relation_Delta >(
              Relation_Delta(Attic< Relation_Skeleton >(Relation_Skeleton(), NOW), *it), it->timestamp));
        else
          rels_compressed.push_back(Attic< Relation_Delta >(
              Relation_Delta(*next, *it), it->timestamp));
//         cout<<hex<<current_idx.val()<<'\t'<<dec<<it->id.val()<<'\t'<<it->timestamp<<'\n';
        ++next;
      }
      
      compressed_set_total_size += rels_compressed.size();
      std::set< Attic< Relation_Delta > >& compressed_ref = compressed_set[current_idx];
      
      for (std::vector< Attic< Relation_Delta > >::const_iterator it = rels_compressed.begin();
           it != rels_compressed.end(); ++it)
      {
        compressed_ref.insert(*it);
//         cout<<hex<<current_idx.val()<<'\t'<<dec<<it->id.val()<<'\t'<<it->full<<'\t'
//             <<it->members_removed.size()<<' '<<it->members_added.size()<<'\t'
//             <<it->node_idxs_removed.size()<<' '<<it->node_idxs_added.size()<<'\t'
//             <<it->way_idxs_removed.size()<<' '<<it->way_idxs_added.size()<<'\t'
//             <<it->timestamp<<'\t';
//         for (std::vector< uint >::const_iterator it2 = it->members_removed.begin();
//              it2 != it->members_removed.end(); ++it2)
//           cout<<*it2<<' ';
//         cout<<'\t';
//         for (std::vector< std::pair< uint, Relation_Entry > >::const_iterator it2 = it->members_added.begin();
//              it2 != it->members_added.end(); ++it2)
//           cout<<it2->first<<' '<<it2->second.ref.val()<<' '<<it2->second.role<<' ';
//         cout<<'\n';
      }
      
      if (compressed_set_total_size > 65*536)
      {
        Block_Backend< Uint31_Index, Attic< Relation_Delta > >
            db(target_transaction.data_index(attic_settings().RELATIONS));
        db.update(std::map< Uint31_Index, std::set< Attic< Relation_Delta > > >(), compressed_set);
        
        compressed_set.clear();
        compressed_set_total_size = 0;
      }
    }
    
//     std::cout<<'\n';
    
    while (!(current_it == current_db.flat_end()))
    {
//       cout<<hex<<current_it.index().val()<<'\t'
//           <<dec<<current_it.object().id.val()<<'\n';
      ++current_it;
    }
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }
  
  return 0;
}
