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
    Nonsynced_Transaction
        (bool writeable, bool use_shadow, const string& file_name_extension);
    virtual ~Nonsynced_Transaction();
    
    File_Blocks_Index_Base* data_index(const File_Properties*);
    Random_File_Index* random_index(const File_Properties*);
    
    void flush();
    string get_db_dir() const { return ""; }
    
  private:
    map< const File_Properties*, File_Blocks_Index_Base* >
      data_files;
    map< const File_Properties*, Random_File_Index* >
      random_files;
    bool writeable, use_shadow;
    string file_name_extension;
};

inline Nonsynced_Transaction::Nonsynced_Transaction
    (bool writeable_, bool use_shadow_, const string& file_name_extension_)
  : writeable(writeable_), use_shadow(use_shadow_),
    file_name_extension(file_name_extension_) {}
  
inline Nonsynced_Transaction::~Nonsynced_Transaction()
{
  flush();
}

inline void Nonsynced_Transaction::flush()
{
  for (map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.begin(); it != data_files.end(); ++it)
    delete it->second;
  data_files.clear();
  for (map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.begin(); it != random_files.end(); ++it)
    delete it->second;
  random_files.clear();
}

inline File_Blocks_Index_Base* Nonsynced_Transaction::data_index
    (const File_Properties* fp)
{ 
  map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.find(fp);
  if (it != data_files.end())
    return it->second;

  File_Blocks_Index_Base* data_index = fp->new_data_index
      (writeable, use_shadow, fp->get_basedir(), file_name_extension);
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
  
  random_files[fp] = new Random_File_Index
      (*fp, writeable, use_shadow, fp->get_basedir());
  return random_files[fp];
}

#endif
