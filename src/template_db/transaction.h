/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
 *
 * This file is part of Template_DB.
 *
 * Template_DB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Template_DB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DE__OSM3S___TEMPLATE_DB__TRANSACTION_H
#define DE__OSM3S___TEMPLATE_DB__TRANSACTION_H

#include "random_file.h"

#include <map>
#include <mutex>
#include <vector>


class Index_Cache
{

public:
  Index_Cache() : replicate_id("") {};

private:
  std::map< const File_Properties*, File_Blocks_Index_Base* >
    data_files;
  std::map< const File_Properties*, Random_File_Index* >
    random_files;

  std::string replicate_id;

  friend class Nonsynced_Transaction;
};



class Transaction
{
  public:
    virtual ~Transaction() {}
    virtual File_Blocks_Index_Base* data_index(const File_Properties*) = 0;
    virtual Random_File_Index* random_index(const File_Properties*) = 0;
    virtual std::string get_db_dir() const = 0;
    virtual std::string get_replicate_id() const = 0;
    virtual void set_replicate_id(std::string replicate_id) = 0;
};


class Nonsynced_Transaction : public Transaction
{
  public:
    Nonsynced_Transaction
        (bool writeable, bool use_shadow,
	 const std::string& db_dir, const std::string& file_name_extension);

    Nonsynced_Transaction
          (bool writeable, bool use_shadow,
           const std::string& db_dir, const std::string& file_name_extension,
           Index_Cache* ic);

    virtual ~Nonsynced_Transaction();
    
    File_Blocks_Index_Base* data_index(const File_Properties*);
    Random_File_Index* random_index(const File_Properties*);
    
    void flush();
    void flush_outdated_index_cache();
    std::string get_db_dir() const { return db_dir; }
    std::string get_replicate_id() const { return replicate_id; }
    void set_replicate_id(std::string replicate_id_) { replicate_id = replicate_id_; };
    
  private:
    std::map< const File_Properties*, File_Blocks_Index_Base* >
      data_files;
    std::map< const File_Properties*, Random_File_Index* >
      random_files;
    bool writeable, use_shadow;
    std::string file_name_extension, db_dir;
    std::mutex transaction_mutex;
    Index_Cache* ic;
    std::string replicate_id;
};


inline Nonsynced_Transaction::Nonsynced_Transaction
    (bool writeable_, bool use_shadow_,
     const std::string& db_dir_, const std::string& file_name_extension_)
   : Nonsynced_Transaction(writeable_, use_shadow_, db_dir, file_name_extension, nullptr) {}


inline Nonsynced_Transaction::Nonsynced_Transaction
    (bool writeable_, bool use_shadow_,
     const std::string& db_dir_, const std::string& file_name_extension_,
     Index_Cache* ic_)
  : writeable(writeable_), use_shadow(use_shadow_),
    file_name_extension(file_name_extension_), db_dir(db_dir_), ic(ic_), replicate_id("")
{
  if (!db_dir.empty() && db_dir[db_dir.size()-1] != '/')
    db_dir += "/";
}

    
inline Nonsynced_Transaction::~Nonsynced_Transaction()
{
  flush();
}


inline void Nonsynced_Transaction::flush()
{
  std::lock_guard<std::mutex> guard(transaction_mutex);

  for (std::map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.begin(); it != data_files.end(); ++it)
    delete it->second;
  data_files.clear();
  for (std::map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.begin(); it != random_files.end(); ++it)
    delete it->second;
  random_files.clear();
}



inline void Nonsynced_Transaction::flush_outdated_index_cache()
{
  std::lock_guard<std::mutex> guard(transaction_mutex);

  if (ic != nullptr && ic->replicate_id != get_replicate_id())
  {
    for (std::map< const File_Properties*, File_Blocks_Index_Base* >::iterator
        it = ic->data_files.begin(); it != ic->data_files.end(); ++it)
      delete it->second;
    ic->data_files.clear();
    for (std::map< const File_Properties*, Random_File_Index* >::iterator
        it = ic->random_files.begin(); it != ic->random_files.end(); ++it)
      delete it->second;
    ic->random_files.clear();
    ic->replicate_id = get_replicate_id();
  }
}


inline File_Blocks_Index_Base* Nonsynced_Transaction::data_index
    (const File_Properties* fp)
{ 
  std::lock_guard<std::mutex> guard(transaction_mutex);

  std::map< const File_Properties*, File_Blocks_Index_Base* > * df;

  df = (ic != nullptr) ? &ic->data_files : &data_files;

  std::map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = df->find(fp);
  if (it != df->end())
    return it->second;

  File_Blocks_Index_Base* data_index = fp->new_data_index
      (writeable, use_shadow, db_dir, file_name_extension);
  if (data_index != 0)
    (*df)[fp] = data_index;
  return data_index;
}


inline Random_File_Index* Nonsynced_Transaction::random_index(const File_Properties* fp)
{ 
  std::lock_guard<std::mutex> guard(transaction_mutex);

  std::map< const File_Properties*, Random_File_Index* > * rf;

  rf = (ic != nullptr) ? &ic->random_files : &random_files;

  std::map< const File_Properties*, Random_File_Index* >::iterator
      it = rf->find(fp);
  if (it != rf->end())
    return it->second;
  
  (*rf)[fp] = new Random_File_Index(*fp, writeable, use_shadow, db_dir, file_name_extension);
  return (*rf)[fp];
}

#endif
