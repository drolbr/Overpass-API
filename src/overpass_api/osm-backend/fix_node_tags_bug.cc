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


struct By_Timestamp
{
public:
  bool operator()(const Attic< Uint31_Index >& lhs, const Attic< Uint31_Index >& rhs)
  {
    return lhs.timestamp < rhs.timestamp;
  }
};


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
    Nonsynced_Transaction transaction(true, false, db_dir, "");
    
    std::vector< Node_Skeleton::Id_Type > potentially_affected_nodes;
    {
      Node_Skeleton::Id_Type last_node = Node_Skeleton::Id_Type(0ull);
      
      Block_Backend< Node_Skeleton::Id_Type, Uint31_Index > db
          (transaction.data_index(attic_settings().NODE_IDX_LIST));
      for (Block_Backend< Node_Skeleton::Id_Type, Uint31_Index >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
	if (last_node == it.index() &&
	    (potentially_affected_nodes.empty()
	    || !(potentially_affected_nodes.back() == last_node)))
	  potentially_affected_nodes.push_back(it.index());
	last_node = it.index();
      }
    }
    
//     for (std::vector< Node_Skeleton::Id_Type >::const_iterator
//         it = potentially_affected_nodes.begin(); it != potentially_affected_nodes.end(); ++it)
//       std::cout<<it->val()<<'\n';


    std::map< Node_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > > idxs_by_id;
    int count = 0;
    {
      Block_Backend< Uint31_Index, Attic< Node_Skeleton > > db
          (transaction.data_index(attic_settings().NODES));
      for (Block_Backend< Uint31_Index, Attic< Node_Skeleton > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
	if (++count % (256*1024) == 0)
	  std::cerr<<'.';
	if (std::binary_search(potentially_affected_nodes.begin(), potentially_affected_nodes.end(), it.object().id))
	  idxs_by_id[it.object().id].push_back(
	    Attic< Uint31_Index >(Uint31_Index(it.index().val() & 0x7fffff00), it.object().timestamp));
      }
    }
    std::cerr<<'\n';
    
//     for (std::map< Node_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > >::iterator
//         it = idxs_by_id.begin(); it != idxs_by_id.end(); ++it)
//     {
//       std::sort(it->second.begin(), it->second.end());
//       std::cout<<it->first.val();
//       for (std::vector< Attic< Uint31_Index > >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
// 	   ++it2)
// 	std::cout<<'\t'<<hex<<it2->val()<<' '<<dec<<it2->timestamp;
//       std::cout<<'\n';
//     }

    std::vector< Attic< Uint31_Index > > last_idx_by_id(potentially_affected_nodes.size(),
        Attic< Uint31_Index >(Uint31_Index(0u), 0));
    std::map< Uint31_Index, std::vector< Attic< Node_Skeleton::Id_Type > > > ids_by_idx;
    for (std::map< Node_Skeleton::Id_Type, std::vector< Attic< Uint31_Index > > >::iterator
        it = idxs_by_id.begin(); it != idxs_by_id.end(); ++it)
    {
      std::sort(it->second.begin(), it->second.end(), By_Timestamp());
      Uint31_Index last_idx = 0xff;
      uint64 last_timestamp = 0;
      for (std::vector< Attic< Uint31_Index > >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
	   ++it2)
      {
	if (!(last_idx == *it2) && (last_idx.val() != 0xff))
	  ids_by_idx[*it2].push_back(Attic< Node_Skeleton::Id_Type >(it->first, last_timestamp));
	last_idx = *it2;
	last_timestamp = it2->timestamp;
      }

      std::vector< Node_Skeleton::Id_Type >::iterator it2
          = std::lower_bound(potentially_affected_nodes.begin(), potentially_affected_nodes.end(), it->first);
      if (it2 == potentially_affected_nodes.end())
	std::cerr<<"Node "<<dec<<it->first.val()<<" not found in potentially_affected_nodes.\n";
      else
        last_idx_by_id[std::distance(potentially_affected_nodes.begin(), it2)]
            = Attic< Uint31_Index >(last_idx, last_timestamp);
    }
    
    
    {
      Block_Backend< Uint31_Index, Node_Skeleton > db
          (transaction.data_index(osm_base_settings().NODES));
      for (Block_Backend< Uint31_Index, Node_Skeleton >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
	if (++count % (256*1024) == 0)
	  std::cerr<<'-';
        std::vector< Node_Skeleton::Id_Type >::iterator it2
            = std::lower_bound(potentially_affected_nodes.begin(), potentially_affected_nodes.end(), it.object().id);
	if (it2 != potentially_affected_nodes.end() && *it2 == it.object().id)
	{
	  Attic< Uint31_Index >& attic = last_idx_by_id[std::distance(potentially_affected_nodes.begin(), it2)];
	  if (attic.val() != 0u && !(attic.val() == (it.index().val() & 0x7fffff00)))
	    ids_by_idx[it.index().val() & 0x7fffff00]
	        .push_back(Attic< Node_Skeleton::Id_Type >(it.object().id, attic.timestamp));
	}
      }
    }
    std::cerr<<'\n';
    
//     for (std::map< Uint31_Index, std::vector< Attic< Node_Skeleton::Id_Type > > >::iterator it = ids_by_idx.begin();
// 	 it != ids_by_idx.end(); ++it)
//     {
//       std::sort(it->second.begin(), it->second.end());
//       for (std::vector< Attic< Node_Skeleton::Id_Type > >::const_iterator it2 = it->second.begin();
// 	   it2 != it->second.end(); ++it2)
// 	std::cout<<hex<<it->first.val()<<'\t'<<dec<<it2->val()<<'\t'<<it2->timestamp<<'\n';
//     }
    
    for (std::map< Uint31_Index, std::vector< Attic< Node_Skeleton::Id_Type > > >::iterator it = ids_by_idx.begin();
	 it != ids_by_idx.end(); ++it)
      std::sort(it->second.begin(), it->second.end());

    
    std::map< Tag_Index_Local, std::set< Attic< Node_Skeleton::Id_Type > > > tags_to_insert;
    
    std::map< Uint31_Index, std::vector< Attic< Node_Skeleton::Id_Type > > >::const_iterator
	iit = ids_by_idx.end();
    {
      Block_Backend< Tag_Index_Local, Node_Skeleton::Id_Type > db
          (transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Node_Skeleton::Id_Type >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
	if (++count % (256*1024) == 0)
	  std::cerr<<'.';
	
	if (iit == ids_by_idx.end() || !(iit->first == it.index().index))
	{
	  iit = ids_by_idx.find(it.index().index);
	  if (iit == ids_by_idx.end())
	    iit = ids_by_idx.insert(std::make_pair(it.index().index,
						   std::vector< Attic< Node_Skeleton::Id_Type > >())).first;
	}
	
	std::vector< Attic< Node_Skeleton::Id_Type > >::const_iterator it2
	    = std::lower_bound(iit->second.begin(), iit->second.end(),
			       Attic< Node_Skeleton::Id_Type >(it.object(), 0));
	if (it2 != iit->second.end() && Node_Skeleton::Id_Type(*it2) == it.object())
	  tags_to_insert[Tag_Index_Local(it.index().index, it.index().key, void_tag_value())].insert(*it2);
      }
    }
    std::cerr<<'\n';
    
    iit = ids_by_idx.end();
    {
      Block_Backend< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().NODE_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
	if (++count % (256*1024) == 0)
	  std::cerr<<'-';
	
	if (iit == ids_by_idx.end() || !(iit->first == it.index().index))
	{
	  iit = ids_by_idx.find(it.index().index);
	  if (iit == ids_by_idx.end())
	    iit = ids_by_idx.insert(std::make_pair(it.index().index,
						   std::vector< Attic< Node_Skeleton::Id_Type > >())).first;
	}
	
	std::vector< Attic< Node_Skeleton::Id_Type > >::const_iterator it2
	    = std::lower_bound(iit->second.begin(), iit->second.end(), it.object());
	if (it2 != iit->second.end() && Node_Skeleton::Id_Type(*it2) == it.object()
	    && it2->timestamp < it.object().timestamp)
	  tags_to_insert[Tag_Index_Local(it.index().index, it.index().key, void_tag_value())].insert(*it2);
      }
    }
    std::cerr<<'\n';
    
    for (std::map< Tag_Index_Local, std::set< Attic< Node_Skeleton::Id_Type > > >::const_iterator
        it = tags_to_insert.begin(); it != tags_to_insert.end(); ++it)
    {
      for (std::set< Attic< Node_Skeleton::Id_Type > >::const_iterator it2 = it->second.begin();
	   it2 != it->second.end(); ++it2)
	std::cout<<hex<<it->first.index<<'\t'
	    <<dec<<it2->val()<<'\t'<<it2->timestamp<<'\t'<<it->first.key<<'\n';
    }
    
    
    update_elements(std::map< Tag_Index_Local, std::set< Attic < Node_Skeleton::Id_Type > > >(),
        tags_to_insert, transaction, *attic_settings().NODE_TAGS_LOCAL);

/*    if (std::string("--nodes") == args[2])
    {
      Block_Backend< Uint31_Index, Node_Skeleton > db
          (transaction.data_index(osm_base_settings().NODES));
      for (Block_Backend< Uint31_Index, Node_Skeleton >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().id.val()<<'\n';
      }
    }
    else if (std::string("--nodes-meta") == args[2])
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > db
          (transaction.data_index(meta_settings().NODES_META));
      for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
               ::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().ref.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--node-tags-local") == args[2])
    {
      Block_Backend< Tag_Index_Local, Node_Skeleton::Id_Type > db
          (transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Node_Skeleton::Id_Type >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().index<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\n';
      }
    }
    else if (std::string("--node-tags-global") == args[2])
    {
      Block_Backend< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > > db
          (transaction.data_index(osm_base_settings().NODE_TAGS_GLOBAL));
      for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.object().idx.val()<<'\t'
            <<dec<<it.object().id.val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\n';
      }
    }
    else if (std::string("--node-keys") == args[2])
    {
      Block_Backend< Uint32_Index, String_Object > db
          (transaction.data_index(osm_base_settings().NODE_KEYS));
      for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
        cout<<dec<<it.index().val()<<'\t'<<it.object().val()<<'\n';
    }
    else if (std::string("--attic-node-idxs") == args[2])
    {
      Block_Backend< Node_Skeleton::Id_Type, Uint31_Index > db
          (transaction.data_index(attic_settings().NODE_IDX_LIST));
      for (Block_Backend< Node_Skeleton::Id_Type, Uint31_Index >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
        cout<<dec<<it.index().val()<<'\t'
            <<hex<<it.object().val()<<'\n';
    }
    else if (std::string("--attic-nodes") == args[2])
    {
      Block_Backend< Uint31_Index, Attic< Node_Skeleton > > db
          (transaction.data_index(attic_settings().NODES));
      for (Block_Backend< Uint31_Index, Attic< Node_Skeleton > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().id.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-nodes-meta") == args[2])
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().NODES_META));
      for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
               ::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().ref.val()<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-node-tags-local") == args[2])
    {
      Block_Backend< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().NODE_TAGS_LOCAL));
      for (Block_Backend< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().index<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--attic-node-tags-global") == args[2])
    {
      Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > > db
          (transaction.data_index(attic_settings().NODE_TAGS_GLOBAL));
      for (Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > >
               ::Flat_Iterator it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.object().idx.val()<<'\t'
            <<dec<<it.object().id.val()<<'\t'
            <<it.index().key<<'\t'<<it.index().value<<'\t'
            <<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--nodes-undelete") == args[2])
    {
      Block_Backend< Uint32_Index, Attic< Node_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().NODES_UNDELETED));
      for (Block_Backend< Uint32_Index, Attic< Node_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<hex<<it.index().val()<<'\t'
            <<dec<<it.object().val()<<'\t'
            <<dec<<it.object().timestamp<<'\n';
      }
    }
    else if (std::string("--node-changelog") == args[2])
    {
      Block_Backend< Timestamp, Change_Entry< Node_Skeleton::Id_Type > > db
          (transaction.data_index(attic_settings().NODE_CHANGELOG));
      for (Block_Backend< Timestamp, Change_Entry< Node_Skeleton::Id_Type > >::Flat_Iterator
           it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        cout<<dec<<it.index().timestamp<<'\t'
            <<hex<<it.object().old_idx.val()<<'\t'<<it.object().new_idx.val()<<'\t'
            <<dec<<it.object().elem_id.val()<<'\n';
      }
    }
    else
      std::cout<<"Unknown target.\n";*/
  }
  catch (File_Error e)
  {
    std::cerr<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }
  
  return 0;
}
