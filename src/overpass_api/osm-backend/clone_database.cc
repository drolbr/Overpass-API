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

      Block_Backend< TIndex, TObject > from_db
          (transaction.data_index(&src_file_prop));      
      typename Block_Backend< TIndex, TObject >::Flat_Iterator it = from_db.flat_begin();
      typename std::map< TIndex, std::set< TObject > >::iterator dit = db_to_insert.begin();
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
        
        if (count >= 512*1024*1024)
        {
          Block_Backend< TIndex, TObject > into_db
              (into_transaction.data_index(&dest_file_prop));
          into_db.update(std::map< TIndex, std::set< TObject > >(), db_to_insert);
          db_to_insert.clear();
          count = 0;
        }
        
        ++it;
      }
      
      Block_Backend< TIndex, TObject > into_db
          (into_transaction.data_index(&dest_file_prop));
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


void clone_database(Transaction& transaction, const std::string& dest_db_dir, const Clone_Settings& clone_settings)
{
  clone_bin_file< Uint32_Index, Node_Skeleton >(*osm_base_settings().NODES, *osm_base_settings().NODES,
				 transaction, dest_db_dir, clone_settings);
  clone_map_file< Node_Skeleton::Id_Type, Uint32_Index >(*osm_base_settings().NODES, transaction, dest_db_dir, clone_settings);
  clone_bin_file< Tag_Index_Local, Node_Skeleton::Id_Type >(*osm_base_settings().NODE_TAGS_LOCAL, *osm_base_settings().NODE_TAGS_LOCAL,
				    transaction, dest_db_dir, clone_settings);
  clone_bin_file< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > >(*osm_base_settings().NODE_TAGS_GLOBAL, *osm_base_settings().NODE_TAGS_GLOBAL,
				     transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint32_Index, String_Object >(*osm_base_settings().NODE_KEYS, *osm_base_settings().NODE_KEYS,
				 transaction, dest_db_dir, clone_settings);

  clone_bin_file< Uint31_Index, Way_Skeleton >(*osm_base_settings().WAYS, *osm_base_settings().WAYS,
				 transaction, dest_db_dir, clone_settings);
  clone_map_file< Way_Skeleton::Id_Type, Uint31_Index >(*osm_base_settings().WAYS, transaction, dest_db_dir, clone_settings);
  clone_bin_file< Tag_Index_Local, Way_Skeleton::Id_Type >(*osm_base_settings().WAY_TAGS_LOCAL, *osm_base_settings().WAY_TAGS_LOCAL,
				    transaction, dest_db_dir, clone_settings);
  clone_bin_file< Tag_Index_Global, Tag_Object_Global< Way_Skeleton::Id_Type > >(*osm_base_settings().WAY_TAGS_GLOBAL, *osm_base_settings().WAY_TAGS_GLOBAL,
				     transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint32_Index, String_Object >(*osm_base_settings().WAY_KEYS, *osm_base_settings().WAY_KEYS,
				 transaction, dest_db_dir, clone_settings);

  clone_bin_file< Uint31_Index, Relation_Skeleton >(*osm_base_settings().RELATIONS, *osm_base_settings().RELATIONS,
				 transaction, dest_db_dir, clone_settings);
  clone_map_file< Relation_Skeleton::Id_Type, Uint31_Index >(
      *osm_base_settings().RELATIONS, transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint32_Index, String_Object >(*osm_base_settings().RELATION_ROLES, *osm_base_settings().RELATION_ROLES,
				 transaction, dest_db_dir, clone_settings);
  clone_bin_file< Tag_Index_Local, Relation_Skeleton::Id_Type >(
      *osm_base_settings().RELATION_TAGS_LOCAL, *osm_base_settings().RELATION_TAGS_LOCAL,
      transaction, dest_db_dir, clone_settings);
  clone_bin_file< Tag_Index_Global, Tag_Object_Global< Relation_Skeleton::Id_Type > >(
      *osm_base_settings().RELATION_TAGS_GLOBAL, *osm_base_settings().RELATION_TAGS_GLOBAL,
      transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint32_Index, String_Object >(*osm_base_settings().RELATION_KEYS, *osm_base_settings().RELATION_KEYS,
				 transaction, dest_db_dir, clone_settings);

  clone_bin_file< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(*meta_settings().NODES_META, *meta_settings().NODES_META,
				 transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(*meta_settings().WAYS_META, *meta_settings().WAYS_META,
				 transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > >(*meta_settings().RELATIONS_META, *meta_settings().RELATIONS_META,
				 transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint32_Index, User_Data >(*meta_settings().USER_DATA, *meta_settings().USER_DATA,
				 transaction, dest_db_dir, clone_settings);
  clone_bin_file< Uint32_Index, Uint31_Index >(*meta_settings().USER_INDICES, *meta_settings().USER_INDICES,
				 transaction, dest_db_dir, clone_settings);

  {
    clone_bin_file< Uint31_Index, Attic< Node_Skeleton > >(*attic_settings().NODES, *attic_settings().NODES,
                                   transaction, dest_db_dir, clone_settings);
    clone_map_file< Node_Skeleton::Id_Type, Uint31_Index >(*attic_settings().NODES, transaction, dest_db_dir, clone_settings);
    clone_bin_file< Uint31_Index, Attic< Node_Skeleton::Id_Type > >(*attic_settings().NODES_UNDELETED, *attic_settings().NODES_UNDELETED,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Node::Id_Type, Uint31_Index >(*attic_settings().NODE_IDX_LIST, *attic_settings().NODE_IDX_LIST,
                                    transaction, dest_db_dir, clone_settings);
    clone_bin_file< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > >(*attic_settings().NODE_TAGS_LOCAL, *attic_settings().NODE_TAGS_LOCAL,
                                      transaction, dest_db_dir, clone_settings);
    clone_bin_file< Tag_Index_Global, Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > >(*attic_settings().NODE_TAGS_GLOBAL, *attic_settings().NODE_TAGS_GLOBAL,
                                       transaction, dest_db_dir, clone_settings);
    clone_bin_file< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(*attic_settings().NODES_META, *attic_settings().NODES_META,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Timestamp, Change_Entry< Node_Skeleton::Id_Type > >(*attic_settings().NODE_CHANGELOG, *attic_settings().NODE_CHANGELOG,
                                transaction, dest_db_dir, clone_settings);

    clone_bin_file< Uint31_Index, Attic< Way_Skeleton > >(*attic_settings().WAYS, *attic_settings().WAYS,
                                   transaction, dest_db_dir, clone_settings);
    clone_map_file< Way_Skeleton::Id_Type, Uint31_Index >(*attic_settings().WAYS, transaction, dest_db_dir, clone_settings);
    clone_bin_file< Uint31_Index, Attic< Way_Skeleton::Id_Type > >(*attic_settings().WAYS_UNDELETED, *attic_settings().WAYS_UNDELETED,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Way::Id_Type, Uint31_Index >(*attic_settings().WAY_IDX_LIST, *attic_settings().WAY_IDX_LIST,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >(*attic_settings().WAY_TAGS_LOCAL, *attic_settings().WAY_TAGS_LOCAL,
                                      transaction, dest_db_dir, clone_settings);
    clone_bin_file< Tag_Index_Global, Attic< Tag_Object_Global< Way_Skeleton::Id_Type > > >(*attic_settings().WAY_TAGS_GLOBAL, *attic_settings().WAY_TAGS_GLOBAL,
                                       transaction, dest_db_dir, clone_settings);
    clone_bin_file< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(*attic_settings().WAYS_META, *attic_settings().WAYS_META,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Timestamp, Change_Entry< Way_Skeleton::Id_Type > >(*attic_settings().WAY_CHANGELOG, *attic_settings().WAY_CHANGELOG,
                                transaction, dest_db_dir, clone_settings);

    clone_bin_file< Uint31_Index, Attic< Relation_Skeleton > >(*attic_settings().RELATIONS, *attic_settings().RELATIONS,
                                   transaction, dest_db_dir, clone_settings);
    clone_map_file< Relation_Skeleton::Id_Type, Uint31_Index >(*attic_settings().RELATIONS, transaction, dest_db_dir, clone_settings);
    clone_bin_file< Uint31_Index, Attic< Relation_Skeleton::Id_Type > >(*attic_settings().RELATIONS_UNDELETED, *attic_settings().RELATIONS_UNDELETED,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Relation::Id_Type, Uint31_Index >(*attic_settings().RELATION_IDX_LIST, *attic_settings().RELATION_IDX_LIST,
                                        transaction, dest_db_dir, clone_settings);
    clone_bin_file< Tag_Index_Local, Attic< Relation_Skeleton::Id_Type > >(
        *attic_settings().RELATION_TAGS_LOCAL, *attic_settings().RELATION_TAGS_LOCAL,
        transaction, dest_db_dir, clone_settings);
    clone_bin_file< Tag_Index_Global, Attic< Tag_Object_Global< Relation_Skeleton::Id_Type > > >(
        *attic_settings().RELATION_TAGS_GLOBAL, *attic_settings().RELATION_TAGS_GLOBAL,
        transaction, dest_db_dir, clone_settings);
    clone_bin_file< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > >(*attic_settings().RELATIONS_META, *attic_settings().RELATIONS_META,
                                   transaction, dest_db_dir, clone_settings);
    clone_bin_file< Timestamp, Change_Entry< Relation_Skeleton::Id_Type > >(*attic_settings().RELATION_CHANGELOG, *attic_settings().RELATION_CHANGELOG,
                                transaction, dest_db_dir, clone_settings);
  }
}
