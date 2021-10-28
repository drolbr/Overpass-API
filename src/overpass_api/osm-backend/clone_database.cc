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

#include "clone_database.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../data/filenames.h"
#include "../../template_db/block_backend.h"
#include "../../template_db/file_blocks.h"
#include "../../template_db/random_file.h"


template< typename TIndex, typename TObject >
void clone_bin_file(const File_Properties& src_file_prop, const File_Properties& dest_file_prop,
		    Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)

{
  try
  {
    File_Blocks_Index< TIndex >& src_idx =
        *dynamic_cast< File_Blocks_Index< TIndex >* >(transaction.data_index(&src_file_prop));
    File_Blocks< TIndex, typename std::set< TIndex >::const_iterator, Default_Range_Iterator< TIndex > >
	src_file(&src_idx);
    uint32 block_size = src_idx.get_block_size() * src_idx.get_compression_factor();

    if (block_size == dest_file_prop.get_block_size() * dest_file_prop.get_compression_factor())
    {
      File_Blocks_Index< TIndex > dest_idx(dest_file_prop, true, false, dest_db_dir, "",
          clone_settings.compression_method);
      File_Blocks< TIndex, typename std::set< TIndex >::const_iterator, Default_Range_Iterator< TIndex > >
          dest_file(&dest_idx);

      typename File_Blocks< TIndex, typename std::set< TIndex >::const_iterator,
          Default_Range_Iterator< TIndex > >::Flat_Iterator
          src_it = src_file.flat_begin();

      uint32 excess_bytes = 0;
      while (!src_it.is_end())
      {
        if (excess_bytes > 0)
        {
          uint64* buf = src_file.read_block(src_it, false);
          dest_file.insert_block(
              dest_file.write_end(), buf, std::min(excess_bytes, block_size),
              src_it.block_it->max_keysize, src_it.block_it->index);
          excess_bytes = std::max(excess_bytes, block_size) - block_size;
        }
        else
        {
          uint64* buf = src_file.read_block(src_it);
          dest_file.insert_block(dest_file.write_end(), buf, src_it.block_it->max_keysize);
          if (((uint32*)buf)[1] > block_size)
            excess_bytes = ((uint32*)buf)[1] - block_size;
        }
        ++src_it;
      }
    }
    else
    {
      Nonsynced_Transaction into_transaction(true, false, dest_db_dir, "");
      std::map< TIndex, std::set< TObject > > db_to_insert;

      Block_Backend< TIndex, TObject > from_db(transaction.data_index(&src_file_prop));      
      typename Block_Backend< TIndex, TObject >::Flat_Iterator it = from_db.flat_begin();
      typename std::map< TIndex, std::set< TObject > >::iterator dit = db_to_insert.begin();

      File_Blocks_Index< TIndex > dest_idx(dest_file_prop, true, false, dest_db_dir, "",
          clone_settings.compression_method);
      Block_Backend< TIndex, TObject > into_db(&dest_idx);

      uint64 count = 0;
      while (!(it == from_db.flat_end()))
      {
        if (dit == db_to_insert.end() || !(dit->first == it.index()))
        {
          dit = db_to_insert.insert(std::make_pair(it.index(), std::set< TObject >())).first;
          count += it.index().size_of();
        }
        dit->second.insert(it.object());
        count += it.object().size_of();
        
        if (count >= 64*1024*1024)
        {
          into_db.update(std::map< TIndex, std::set< TObject > >(), db_to_insert);
          db_to_insert.clear();
          dit = db_to_insert.begin();
          count = 0;
        }
        
        ++it;
      }
      into_db.update(std::map< TIndex, std::set< TObject > >(), db_to_insert);
    }    
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<'\n';
  }
}


template< typename Key, typename TIndex >
void clone_map_file(const File_Properties& file_prop, Transaction& transaction, std::string dest_db_dir, Clone_Settings clone_settings)
{
  try
  {
    Random_File_Index& src_idx = *transaction.random_index(&file_prop);
    Random_File< Key, TIndex > src_file(&src_idx);

    Random_File_Index dest_idx(file_prop, true, false, dest_db_dir, "", clone_settings.map_compression_method);
    Random_File< Key, TIndex > dest_file(&dest_idx);

    for (std::vector< uint32 >::size_type i = 0; i < src_idx.get_blocks().size(); ++i)
    {
      if (src_idx.get_blocks()[i].pos != src_idx.npos)
      {
	for (uint32 j = 0; j < src_idx.get_block_size()*src_idx.get_compression_factor()/TIndex::max_size_of(); ++j)
	{
	  TIndex val =
	      src_file.get(i*(src_idx.get_block_size()*src_idx.get_compression_factor()/TIndex::max_size_of()) + j);
	  if (!(val == TIndex(uint32(0))))
	    dest_file.put(i*(src_idx.get_block_size()*src_idx.get_compression_factor()/TIndex::max_size_of()) + j, val);
	}
      }
    }
  }
  catch (File_Error e)
  {
    std::cout<<e.origin<<' '<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<'\n';
  }
}


template< typename Index, typename Object >
void clone_matching_bin_file(
    const File_Properties& file_prop,
    Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  if (clone_settings.single_file_name.empty()
      || file_prop.get_file_name_trunk() + ".bin" == clone_settings.single_file_name)
    clone_bin_file< Index, Object >(
        file_prop, file_prop, transaction, dest_db_dir, clone_settings);
}


template< typename Key, typename Index >
void clone_matching_map_file(
    const File_Properties& file_prop,
    Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  if (clone_settings.single_file_name.empty()
      || file_prop.get_file_name_trunk() + ".map" == clone_settings.single_file_name)
    clone_map_file< Key, Index >(
        file_prop, transaction, dest_db_dir, clone_settings);
}


template< typename Index, typename Skeleton >
void clone_skeleton_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Index, Skeleton >(
      *current_skeleton_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Index, typename Skeleton >
void clone_current_map_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_map_file< typename Skeleton::Id_Type, Index >(
      *current_skeleton_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Index, typename Skeleton >
void clone_meta_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >(
      *current_meta_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Index, typename Skeleton, typename Skel_or_Delta >
void clone_attic_skel_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Index, Attic< Skel_or_Delta > >(
      *attic_skeleton_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Index, typename Skeleton >
void clone_attic_map_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_map_file< typename Skeleton::Id_Type, Index >(
      *attic_skeleton_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_local_tags_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Tag_Index_Local, typename Skeleton::Id_Type >(
      *current_local_tags_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_global_tags_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Tag_Index_Global, Tag_Object_Global< typename Skeleton::Id_Type > >(
      *current_global_tags_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_keys_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Uint31_Index, String_Object >(
      *key_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_attic_idx_list_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< typename Skeleton::Id_Type, Uint31_Index >(
      *attic_idx_list_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_attic_undeleted_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Uint31_Index, Attic< typename Skeleton::Id_Type > >(
      *attic_undeleted_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Index, typename Skeleton >
void clone_attic_meta_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >(
      *attic_meta_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_attic_local_tags_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Tag_Index_Local, Attic< typename Skeleton::Id_Type > >(
      *attic_local_tags_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_attic_global_tags_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Tag_Index_Global, Attic< Tag_Object_Global< typename Skeleton::Id_Type > > >(
      *attic_global_tags_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}
template< typename Skeleton >
void clone_changelog_file(Transaction& transaction, std::string dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_matching_bin_file< Timestamp, Change_Entry< typename Skeleton::Id_Type > >(
      *changelog_file_properties< Skeleton >(), transaction, dest_db_dir, clone_settings);
}


void clone_database(Transaction& transaction, const std::string& dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_skeleton_file< Uint32_Index, Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_current_map_file< Uint32_Index, Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_local_tags_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_global_tags_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_keys_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);

  clone_skeleton_file< Uint31_Index, Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_current_map_file< Uint31_Index, Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_local_tags_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_global_tags_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_keys_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);

  clone_skeleton_file< Uint31_Index, Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_current_map_file< Uint31_Index, Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_local_tags_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_global_tags_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_keys_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_matching_bin_file< Uint32_Index, String_Object >(
      *osm_base_settings().RELATION_ROLES, transaction, dest_db_dir, clone_settings);

  clone_meta_file< Uint31_Index, Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_meta_file< Uint31_Index, Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_meta_file< Uint31_Index, Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_matching_bin_file< Uint32_Index, User_Data >(
      *meta_settings().USER_DATA, transaction, dest_db_dir, clone_settings);
  clone_matching_bin_file< Uint32_Index, Uint31_Index >(
      *meta_settings().USER_INDICES, transaction, dest_db_dir, clone_settings);

  clone_attic_skel_file< Uint31_Index, Node_Skeleton, Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_map_file< Uint31_Index, Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_undeleted_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_idx_list_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_local_tags_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_global_tags_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_meta_file< Uint31_Index, Node_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_changelog_file< Node_Skeleton >(transaction, dest_db_dir, clone_settings);

  clone_attic_skel_file< Uint31_Index, Way_Skeleton, Way_Delta >(transaction, dest_db_dir, clone_settings);
  clone_attic_map_file< Uint31_Index, Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_undeleted_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_idx_list_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_local_tags_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_global_tags_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_meta_file< Uint31_Index, Way_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_changelog_file< Way_Skeleton >(transaction, dest_db_dir, clone_settings);

  clone_attic_skel_file< Uint31_Index, Relation_Skeleton, Relation_Delta >(transaction, dest_db_dir, clone_settings);
  clone_attic_map_file< Uint31_Index, Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_undeleted_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_idx_list_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_local_tags_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_global_tags_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_attic_meta_file< Uint31_Index, Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
  clone_changelog_file< Relation_Skeleton >(transaction, dest_db_dir, clone_settings);
}
