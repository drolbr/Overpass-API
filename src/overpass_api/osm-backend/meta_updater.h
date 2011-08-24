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

void rename_referred_file(const string& db_dir, const string& from, const string& to,
			  const File_Properties& file_prop);
			   
class Transaction_Collection
{
  public:
    Transaction_Collection(bool writeable, bool use_shadow,
			   const string& db_dir, const vector< string >& file_name_extensions);
    ~Transaction_Collection();
    
    void remove_referred_files(const File_Properties& file_prop);
    
    vector< string > file_name_extensions;
    vector< Transaction* > transactions;
};

template < typename TIndex, typename TObject >
class Block_Backend_Collection
{
  public:
    Block_Backend_Collection
        (Transaction_Collection& transactions, const File_Properties& file_prop);
    ~Block_Backend_Collection();
    
    vector< Block_Backend< TIndex, TObject >* > dbs;
};

template < typename TIndex, typename TObject >
Block_Backend_Collection< TIndex, TObject >::Block_Backend_Collection
    (Transaction_Collection& transactions, const File_Properties& file_prop)
{
  for (vector< Transaction* >::const_iterator it = transactions.transactions.begin();
      it != transactions.transactions.end(); ++it)
    dbs.push_back(new Block_Backend< TIndex, TObject >((*it)->data_index(&file_prop)));  
}

template < typename TIndex, typename TObject >
Block_Backend_Collection< TIndex, TObject >::~Block_Backend_Collection()
{
  for (typename vector< Block_Backend< TIndex, TObject >* >::const_iterator
      it = dbs.begin(); it != dbs.end(); ++it)
    delete(*it);
}

template < typename TIndex, typename TObject >
void merge_files
    (Transaction_Collection& from_transaction, Transaction& into_transaction,
     const File_Properties& file_prop)
{
  {
    map< TIndex, set< TObject > > db_to_delete;
    map< TIndex, set< TObject > > db_to_insert;
    
    uint32 item_count = 0;
    Block_Backend_Collection< TIndex, TObject > from_dbs(from_transaction, file_prop);
    vector< pair< typename Block_Backend< TIndex, TObject >::Flat_Iterator,
        typename Block_Backend< TIndex, TObject >::Flat_Iterator > > from_its;
    set< TIndex > current_idxs;
    for (typename vector< Block_Backend< TIndex, TObject >* >::const_iterator
        it = from_dbs.dbs.begin(); it != from_dbs.dbs.end(); ++it)
    {
      from_its.push_back(make_pair((*it)->flat_begin(), (*it)->flat_end()));
      if (!(from_its.back().first == from_its.back().second))
        current_idxs.insert(from_its.back().first.index());
    }
    while (!current_idxs.empty())
    {
      TIndex current_idx = *current_idxs.begin();
      current_idxs.erase(current_idxs.begin());
      for (typename vector< pair< typename Block_Backend< TIndex, TObject >::Flat_Iterator,
	      typename Block_Backend< TIndex, TObject >::Flat_Iterator > >::iterator
	  it = from_its.begin(); it != from_its.end(); ++it)
      {
	while (!(it->first == it->second) && (it->first.index() == current_idx))
	{
	  db_to_insert[it->first.index()].insert(it->first.object());
	  ++(it->first);

	  if (++item_count > 4*1024*1024)
	  {
	    Block_Backend< TIndex, TObject > into_db
	        (into_transaction.data_index(&file_prop));
	    into_db.update(db_to_delete, db_to_insert);
	    db_to_insert.clear();
	    item_count = 0;
	  }
	}
	if (!(it->first == it->second))
	  current_idxs.insert(it->first.index());
      }
    }
    
    Block_Backend< TIndex, TObject > into_db
        (into_transaction.data_index(&file_prop));
    into_db.update(db_to_delete, db_to_insert);
  }
  from_transaction.remove_referred_files(file_prop);
}

#endif
