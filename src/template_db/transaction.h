#ifndef ORG__OVERPASS_API__TEMPLATE_DB__TRANSACTION
#define ORG__OVERPASS_API__TEMPLATE_DB__TRANSACTION

#include "random_file.h"

#include <map>
#include <vector>

using namespace std;

class Transaction
{
  public:
    virtual ~Transaction() {}
    virtual File_Blocks_Index_Base* data_index(const File_Properties*) = 0;
    virtual Random_File_Index* random_index(const File_Properties*) = 0;
};

class Nonsynced_Transaction : public Transaction
{
  public:
    Nonsynced_Transaction(bool writeable, bool use_shadow);
    virtual ~Nonsynced_Transaction();
    
    File_Blocks_Index_Base* data_index(const File_Properties*);
    Random_File_Index* random_index(const File_Properties*);
    
  private:
    map< const File_Properties*, File_Blocks_Index_Base* >
      data_files;
    map< const File_Properties*, Random_File_Index* >
      random_files;
    bool writeable, use_shadow;
};

inline Nonsynced_Transaction::Nonsynced_Transaction
    (bool writeable_, bool use_shadow_)
  : writeable(writeable_), use_shadow(use_shadow_) {}
  
inline Nonsynced_Transaction::~Nonsynced_Transaction()
{
  for (map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.begin(); it != data_files.end(); ++it)
    delete it->second;
  for (map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.begin(); it != random_files.end(); ++it)
    delete it->second;
}

inline File_Blocks_Index_Base* Nonsynced_Transaction::data_index
    (const File_Properties* fp)
{ 
  map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.find(fp);
  if (it != data_files.end())
    return it->second;
  
  Raw_File val_file(fp->get_file_base_name() + fp->get_data_suffix(),
                    O_RDONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
		    "Nonsynced_Transaction:1");
  uint64 block_count = lseek64(val_file.fd, 0, SEEK_END)
          /fp->get_block_size();
  File_Blocks_Index_Base* data_index = fp->new_data_index
    (fp->get_file_base_name() + fp->get_data_suffix() + fp->get_index_suffix()
     + (use_shadow ? fp->get_shadow_suffix() : ""),
     writeable ? fp->get_file_base_name() + fp->get_data_suffix()
     + fp->get_shadow_suffix() : "", "", block_count);
  if (data_index != 0)
    data_files[fp] = data_index;
  return data_index;
}

inline Random_File_Index* Nonsynced_Transaction::random_index(const File_Properties* fp)
{ 
  map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.find(fp);
  if (it != random_files.end())
    return it->second;
  
  Raw_File val_file(fp->get_file_base_name() + fp->get_id_suffix(),
                    O_RDONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
		    "Nonsynced_Transaction:2");
  uint64 block_count = lseek64(val_file.fd, 0, SEEK_END)
          /fp->get_map_block_size()/fp->id_max_size_of();
  random_files[fp] = new Random_File_Index
    (fp->get_file_base_name() + fp->get_id_suffix() + fp->get_index_suffix()
     + (use_shadow ? fp->get_shadow_suffix() : ""),
     writeable ? fp->get_file_base_name() + fp->get_id_suffix()
     + fp->get_shadow_suffix() : "", block_count);
  return random_files[fp];
}

#endif
