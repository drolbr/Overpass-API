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
  if (argc < 3)
  {
    std::cout<<"Usage: "<<args[0]<<" db_dir target\n";
    return 0;
  }
  
  string db_dir(args[1]);
  
  try
  {    
    Nonsynced_Transaction transaction(false, false, db_dir, "");

    if (std::string("--node-changelog") == args[2])
    {
      Block_Backend< Timestamp, Change_Entry< Node_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().NODE_CHANGELOG));
      for (Block_Backend< Timestamp, Change_Entry< Node_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<dec<<it.index().timestamp<<'\t'
            <<hex<<it.object().old_idx.val()<<'\t'<<it.object().new_idx.val()<<'\t'
            <<dec<<it.object().elem_id.val()<<'\t'<<int(it.object().status_flags)<<'\n';
      }
    }
    else if (std::string("--ways") == args[2])
    {
      Block_Backend< Uint31_Index, Way_Skeleton > db
          (transaction.data_index(osm_base_settings().WAYS));
      for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().id.val()<<'\n';
      }
    }
    else if (std::string("--ways-meta") == args[2])
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > db
          (transaction.data_index(meta_settings().WAYS_META));
      for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
               ::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().ref.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--way-tags-local") == args[2])
    {
      Block_Backend< Tag_Index_Local, Way_Skeleton::Id_Type > db
          (transaction.data_index(osm_base_settings().WAY_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Way_Skeleton::Id_Type >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().index<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\n';
      }
    }
    else if (std::string("--attic-ways") == args[2])
    {
      Block_Backend< Uint31_Index, Attic< Way_Skeleton > > db
          (transaction.data_index(attic_settings().WAYS));
      for (Block_Backend< Uint31_Index, Attic< Way_Skeleton > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().id.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-ways-meta") == args[2])
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().WAYS_META));
      for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
               ::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().ref.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-way-tags-local") == args[2])
    {
      Block_Backend< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().WAY_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().index<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--rels") == args[2])
    {
      Block_Backend< Uint31_Index, Relation_Skeleton > db
          (transaction.data_index(osm_base_settings().RELATIONS));
      for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().id.val()<<'\n';
      }
    }
    else if (std::string("--rels-meta") == args[2])
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > > db
          (transaction.data_index(meta_settings().RELATIONS_META));
      for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > >
               ::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().ref.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--rel-tags-local") == args[2])
    {
      Block_Backend< Tag_Index_Local, Relation_Skeleton::Id_Type > db
          (transaction.data_index(osm_base_settings().RELATION_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Relation_Skeleton::Id_Type >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().index<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\n';
      }
    }
    else if (std::string("--attic-rels") == args[2])
    {
      Block_Backend< Uint31_Index, Attic< Relation_Skeleton > > db
          (transaction.data_index(attic_settings().RELATIONS));
      for (Block_Backend< Uint31_Index, Attic< Relation_Skeleton > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().id.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-rels-meta") == args[2])
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().RELATIONS_META));
      for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > >
               ::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().ref.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-rel-tags-local") == args[2])
    {
      Block_Backend< Tag_Index_Local, Attic< Relation_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().RELATION_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Attic< Relation_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().index<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else
      std::cout<<"Unknown target.\n";
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }
  
  return 0;
}
