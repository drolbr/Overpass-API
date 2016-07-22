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

#include "clone_database.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../../template_db/block_backend.h"
#include "../../template_db/file_blocks.h"
#include "../../template_db/random_file.h"


void zero_out_tails(void* buf, uint32 block_size)
{
  uint32 net_size = *(uint32*)buf;
  if (block_size - net_size >= 4)
    *(uint32*)(((uint8*)buf) + net_size) = 0;
  for (uint i = (net_size+3)/4*4; i < block_size; i += 4)
    *(uint32*)(((uint8*)buf) + i) = 0;
}


template< class TIndex >
void clone_bin_file(const File_Properties& src_file_prop, const File_Properties& dest_file_prop,
		    Transaction& transaction, string dest_db_dir)
{
  try
  {
    if (src_file_prop.get_block_size() * src_file_prop.get_max_size()
        != dest_file_prop.get_block_size() * dest_file_prop.get_max_size())
    {
      std::cout<<"Block sizes of source and destination format are incompatible.\n";
      return;
    }
    
    File_Blocks_Index< TIndex >& src_idx =
        *dynamic_cast< File_Blocks_Index< TIndex >* >(transaction.data_index(&src_file_prop));
    File_Blocks< TIndex, typename set< TIndex >::const_iterator, Default_Range_Iterator< TIndex > >
	src_file(&src_idx);
    
    File_Blocks_Index< TIndex > dest_idx(dest_file_prop, true, false, dest_db_dir, "");
    File_Blocks< TIndex, typename set< TIndex >::const_iterator, Default_Range_Iterator< TIndex > >
	dest_file(&dest_idx);
    
    typename File_Blocks< TIndex, typename set< TIndex >::const_iterator,
        Default_Range_Iterator< TIndex > >::Flat_Iterator
	src_it = src_file.flat_begin();
    
    typename File_Blocks< TIndex, typename set< TIndex >::const_iterator,
        Default_Range_Iterator< TIndex > >::Discrete_Iterator
	dest_it = dest_file.discrete_end();
    
    while (!(src_it == src_file.flat_end()))
    {
      void* buf = src_file.read_block(src_it);
      zero_out_tails(buf, src_file_prop.get_block_size());
      dest_file.insert_block(dest_it, buf, src_it.block_it->max_keysize);
      ++src_it;
      dest_it = dest_file.discrete_end();
    }    
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<'\n';
  }
}


template< typename Key, typename TIndex >
void clone_map_file(const File_Properties& file_prop, Transaction& transaction, string dest_db_dir)
{
  try
  {
    Random_File_Index& src_idx = *transaction.random_index(&file_prop);
    Random_File< Key, TIndex > src_file(&src_idx);

    Random_File_Index dest_idx(file_prop, true, false, dest_db_dir, "");
    Random_File< Key, TIndex > dest_file(&dest_idx);
    
    for (vector< uint32 >::size_type i = 0; i < src_idx.blocks.size(); ++i)
    {
      if (src_idx.blocks[i].pos != src_idx.npos)
      {
	for (uint32 j = 0; j < src_idx.get_block_size()/TIndex::max_size_of(); ++j)
	{
	  TIndex val =
	      src_file.get(i*(src_idx.get_block_size()/TIndex::max_size_of()) + j);
	  if (!(val == TIndex(uint32(0))))
	    dest_file.put(i*(src_idx.get_block_size()/TIndex::max_size_of()) + j, val);
	}
      }
    }
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<'\n';
  }
}


void clone_database(Transaction& transaction, string dest_db_dir)
{
  clone_bin_file< Uint32_Index >(*osm_base_settings().NODES, *osm_base_settings().NODES,
				 transaction, dest_db_dir);
  clone_map_file< Node_Skeleton::Id_Type, Uint32_Index >(*osm_base_settings().NODES, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*osm_base_settings().NODE_TAGS_LOCAL, *osm_base_settings().NODE_TAGS_LOCAL,
				    transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*osm_base_settings().NODE_TAGS_GLOBAL, *osm_base_settings().NODE_TAGS_GLOBAL,
				     transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*osm_base_settings().NODE_KEYS, *osm_base_settings().NODE_KEYS,
				 transaction, dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*osm_base_settings().WAYS, *osm_base_settings().WAYS,
				 transaction, dest_db_dir);
  clone_map_file< Way_Skeleton::Id_Type, Uint31_Index >(*osm_base_settings().WAYS, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*osm_base_settings().WAY_TAGS_LOCAL, *osm_base_settings().WAY_TAGS_LOCAL,
				    transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*osm_base_settings().WAY_TAGS_GLOBAL, *osm_base_settings().WAY_TAGS_GLOBAL,
				     transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*osm_base_settings().WAY_KEYS, *osm_base_settings().WAY_KEYS,
				 transaction, dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*osm_base_settings().RELATIONS, *osm_base_settings().RELATIONS,
				 transaction, dest_db_dir);
  clone_map_file< Relation_Skeleton::Id_Type, Uint31_Index >(
      *osm_base_settings().RELATIONS, transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*osm_base_settings().RELATION_ROLES, *osm_base_settings().RELATION_ROLES,
				 transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(
      *osm_base_settings().RELATION_TAGS_LOCAL, *osm_base_settings().RELATION_TAGS_LOCAL,
      transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(
      *osm_base_settings().RELATION_TAGS_GLOBAL, *osm_base_settings().RELATION_TAGS_GLOBAL,
      transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*osm_base_settings().RELATION_KEYS, *osm_base_settings().RELATION_KEYS,
				 transaction, dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*meta_settings().NODES_META, *meta_settings().NODES_META,
				 transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*meta_settings().WAYS_META, *meta_settings().WAYS_META,
				 transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*meta_settings().RELATIONS_META, *meta_settings().RELATIONS_META,
				 transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*meta_settings().USER_DATA, *meta_settings().USER_DATA,
				 transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*meta_settings().USER_INDICES, *meta_settings().USER_INDICES,
				 transaction, dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*attic_settings().NODES, *attic_settings().NODES,
				 transaction, dest_db_dir);
  clone_map_file< Node_Skeleton::Id_Type, Uint31_Index >(*attic_settings().NODES, transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*attic_settings().NODES_UNDELETED, *attic_settings().NODES_UNDELETED,
				 transaction, dest_db_dir);
  clone_bin_file< Node::Id_Type >(*attic_settings().NODE_IDX_LIST, *attic_settings().NODE_IDX_LIST,
				  transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*attic_settings().NODE_TAGS_LOCAL, *attic_settings().NODE_TAGS_LOCAL,
				    transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*attic_settings().NODE_TAGS_GLOBAL, *attic_settings().NODE_TAGS_GLOBAL,
				     transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*attic_settings().NODES_META, *attic_settings().NODES_META,
				 transaction, dest_db_dir);
  clone_bin_file< Timestamp >(*attic_settings().NODE_CHANGELOG, *attic_settings().NODE_CHANGELOG,
			      transaction, dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*attic_settings().WAYS, *attic_settings().WAYS,
				 transaction, dest_db_dir);
  clone_map_file< Way_Skeleton::Id_Type, Uint31_Index >(*attic_settings().WAYS, transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*attic_settings().WAYS_UNDELETED, *attic_settings().WAYS_UNDELETED,
				 transaction, dest_db_dir);
  clone_bin_file< Way::Id_Type >(*attic_settings().WAY_IDX_LIST, *attic_settings().WAY_IDX_LIST,
				 transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*attic_settings().WAY_TAGS_LOCAL, *attic_settings().WAY_TAGS_LOCAL,
				    transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*attic_settings().WAY_TAGS_GLOBAL, *attic_settings().WAY_TAGS_GLOBAL,
				     transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*attic_settings().WAYS_META, *attic_settings().WAYS_META,
				 transaction, dest_db_dir);
  clone_bin_file< Timestamp >(*attic_settings().WAY_CHANGELOG, *attic_settings().WAY_CHANGELOG,
			      transaction, dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*attic_settings().RELATIONS, *attic_settings().RELATIONS,
				 transaction, dest_db_dir);
  clone_map_file< Relation_Skeleton::Id_Type, Uint31_Index >(*attic_settings().RELATIONS, transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*attic_settings().RELATIONS_UNDELETED, *attic_settings().RELATIONS_UNDELETED,
				 transaction, dest_db_dir);
  clone_bin_file< Relation::Id_Type >(*attic_settings().RELATION_IDX_LIST, *attic_settings().RELATION_IDX_LIST,
				      transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(
      *attic_settings().RELATION_TAGS_LOCAL, *attic_settings().RELATION_TAGS_LOCAL,
      transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(
      *attic_settings().RELATION_TAGS_GLOBAL, *attic_settings().RELATION_TAGS_GLOBAL,
      transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*attic_settings().RELATIONS_META, *attic_settings().RELATIONS_META,
				 transaction, dest_db_dir);
  clone_bin_file< Timestamp >(*attic_settings().RELATION_CHANGELOG, *attic_settings().RELATION_CHANGELOG,
			      transaction, dest_db_dir);
}
