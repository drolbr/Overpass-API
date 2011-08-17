#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__META_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__META_UPDATER_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../../template_db/transaction.h"

using namespace std;

void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
   const vector< pair< uint32, bool > >& ids_to_modify,
   const map< uint32, vector< uint32 > >& to_delete);

void create_idxs_by_id
    (const vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
     map< uint32, vector< uint32 > >& idxs_by_user_id);
   
void process_user_data(Transaction& transaction, map< uint32, string >& user_by_id,
   map< uint32, vector< uint32 > >& idxs_by_user_id);

void collect_old_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   const map< uint32, vector< uint32 > >& to_delete,
   map< uint32, uint32 >& new_index_by_id,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert);

struct Meta_Comparator_By_Id {
  bool operator()
  (const pair< OSM_Element_Metadata_Skeleton, uint32 >& a,
   const pair< OSM_Element_Metadata_Skeleton, uint32 >& b)
   {
     return (a.first.ref < b.first.ref);
   }
};

struct Meta_Equal_Id {
  bool operator()
  (const pair< OSM_Element_Metadata_Skeleton, uint32 >& a,
   const pair< OSM_Element_Metadata_Skeleton, uint32 >& b)
   {
     return (a.first.ref == b.first.ref);
   }
};

template < typename TIndex, typename TObject >
void merge_file
    (Transaction& from_transaction, Transaction& into_transaction,
     string from, const File_Properties& file_prop)
{
  {
    map< TIndex, set< TObject > > db_to_delete;
    map< TIndex, set< TObject > > db_to_insert;
    
    uint32 item_count(0);
    Block_Backend< TIndex, TObject > from_db
        (from_transaction.data_index(&file_prop));
    for (typename Block_Backend< TIndex, TObject >::Flat_Iterator
      it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
    {
      db_to_insert[it.index()].insert(it.object());
      if (++item_count >= 4*1024*1024)
      {
	Block_Backend< TIndex, TObject > into_db
	    (into_transaction.data_index(&file_prop));
	into_db.update(db_to_delete, db_to_insert);
	db_to_insert.clear();
	item_count = 0;
      }
    }
    
    Block_Backend< TIndex, TObject > into_db
        (into_transaction.data_index(&file_prop));
    into_db.update(db_to_delete, db_to_insert);
  }
  remove((from_transaction.get_db_dir()
      + file_prop.get_file_name_trunk() + from 
      + file_prop.get_data_suffix()
      + file_prop.get_index_suffix()).c_str());
  remove((from_transaction.get_db_dir()
      + file_prop.get_file_name_trunk() + from 
      + file_prop.get_data_suffix()).c_str());
}

#endif
