/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../template_db/block_backend.h"
#include "../template_db/file_blocks.h"
#include "../template_db/random_file.h"
#include "clone_database.h"

using namespace std;

void zero_out_tails(void* buf, uint32 block_size)
{
  uint32 net_size = *(uint32*)buf;
  if (block_size - net_size >= 4)
    *(uint32*)(((uint8*)buf) + net_size) = 0;
  for (uint i = (net_size+3)/4*4; i < block_size; i += 4)
    *(uint32*)(((uint8*)buf) + i) = 0;
}

template< class TIndex >
void clone_bin_file(const File_Properties& file_prop, Transaction& transaction, string dest_db_dir)
{
  try
  {
    File_Blocks_Index< TIndex >& src_idx =
        *dynamic_cast< File_Blocks_Index< TIndex >* >(transaction.data_index(&file_prop));
    File_Blocks< TIndex, typename set< TIndex >::const_iterator, Default_Range_Iterator< TIndex > >
	src_file(&src_idx);
    
    File_Blocks_Index< TIndex > dest_idx(file_prop, true, false, dest_db_dir, "");
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
      zero_out_tails(buf, file_prop.get_block_size());
      dest_file.insert_block(dest_it, buf, src_it.block_it->max_keysize);
      ++src_it;
      dest_it = dest_file.discrete_end();
    }    
  }
  catch (File_Error e)
  {
    cout<<e.origin<<' '<<e.error_number<<' '<<e.filename<<'\n';
  }
}

template< class TIndex >
void clone_map_file(const File_Properties& file_prop, Transaction& transaction, string dest_db_dir)
{
  try
  {
    Random_File_Index& src_idx = *transaction.random_index(&file_prop);
    Random_File< TIndex > src_file(&src_idx);

    Random_File_Index dest_idx(file_prop, true, false, dest_db_dir);
    Random_File< TIndex > dest_file(&dest_idx);
    
    for (vector< uint32 >::size_type i = 0; i < src_idx.blocks.size(); ++i)
    {
      if (src_idx.blocks[i] != src_idx.npos)
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
    cout<<e.origin<<' '<<e.error_number<<' '<<e.filename<<'\n';
  }
}

void clone_database(Transaction& transaction, string dest_db_dir)
{
  clone_bin_file< Uint32_Index >(*osm_base_settings().NODES, transaction, dest_db_dir);
  clone_map_file< Uint32_Index >(*osm_base_settings().NODES, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*osm_base_settings().NODE_TAGS_LOCAL, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*osm_base_settings().NODE_TAGS_GLOBAL, transaction,
				     dest_db_dir);
  clone_bin_file< Uint31_Index >(*osm_base_settings().WAYS, transaction, dest_db_dir);
  clone_map_file< Uint31_Index >(*osm_base_settings().WAYS, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*osm_base_settings().WAY_TAGS_LOCAL, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*osm_base_settings().WAY_TAGS_GLOBAL, transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*osm_base_settings().RELATIONS, transaction, dest_db_dir);
  clone_map_file< Uint31_Index >(*osm_base_settings().RELATIONS, transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*osm_base_settings().RELATION_ROLES, transaction, dest_db_dir);
  clone_bin_file< Tag_Index_Local >(*osm_base_settings().RELATION_TAGS_LOCAL, transaction,
				    dest_db_dir);
  clone_bin_file< Tag_Index_Global >(*osm_base_settings().RELATION_TAGS_GLOBAL, transaction,
				     dest_db_dir);
  
  clone_bin_file< Uint31_Index >(*meta_settings().NODES_META, transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*meta_settings().WAYS_META, transaction, dest_db_dir);
  clone_bin_file< Uint31_Index >(*meta_settings().RELATIONS_META, transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*meta_settings().USER_DATA, transaction, dest_db_dir);
  clone_bin_file< Uint32_Index >(*meta_settings().USER_INDICES, transaction, dest_db_dir);
}
