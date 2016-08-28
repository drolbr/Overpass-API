/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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
* along with Template_DB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___TEMPLATE_DB__TRANSACTION_H
#define DE__OSM3S___TEMPLATE_DB__TRANSACTION_H

#include "random_file.h"

#include <map>
#include <vector>


class Transaction
{
  public:
    virtual ~Transaction() {}
    virtual File_Blocks_Index_Base* data_index(const File_Properties*) = 0;
    virtual Random_File_Index* random_index(const File_Properties*) = 0;
    virtual std::string get_db_dir() const = 0;
};


class Nonsynced_Transaction : public Transaction
{
  public:
    Nonsynced_Transaction
        (bool writeable, bool use_shadow,
	 const std::string& db_dir, const std::string& file_name_extension);
    virtual ~Nonsynced_Transaction();
    
    File_Blocks_Index_Base* data_index(const File_Properties*);
    Random_File_Index* random_index(const File_Properties*);
    
    void flush();
    std::string get_db_dir() const { return db_dir; }
    
  private:
    std::map< const File_Properties*, File_Blocks_Index_Base* >
      data_files;
    std::map< const File_Properties*, Random_File_Index* >
      random_files;
    bool writeable, use_shadow;
    std::string file_name_extension, db_dir;
};


inline Nonsynced_Transaction::Nonsynced_Transaction
    (bool writeable_, bool use_shadow_,
     const std::string& db_dir_, const std::string& file_name_extension_)
  : writeable(writeable_), use_shadow(use_shadow_),
    file_name_extension(file_name_extension_), db_dir(db_dir_) {}

    
inline Nonsynced_Transaction::~Nonsynced_Transaction()
{
  flush();
}


inline void Nonsynced_Transaction::flush()
{
  for (std::map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.begin(); it != data_files.end(); ++it)
    delete it->second;
  data_files.clear();
  for (std::map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.begin(); it != random_files.end(); ++it)
    delete it->second;
  random_files.clear();
}


inline File_Blocks_Index_Base* Nonsynced_Transaction::data_index
    (const File_Properties* fp)
{ 
  std::map< const File_Properties*, File_Blocks_Index_Base* >::iterator
      it = data_files.find(fp);
  if (it != data_files.end())
    return it->second;

  File_Blocks_Index_Base* data_index = fp->new_data_index
      (writeable, use_shadow, db_dir, file_name_extension);
  if (data_index != 0)
    data_files[fp] = data_index;
  return data_index;
}


inline Random_File_Index* Nonsynced_Transaction::random_index(const File_Properties* fp)
{ 
  std::map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.find(fp);
  if (it != random_files.end())
    return it->second;
  
  random_files[fp] = new Random_File_Index(*fp, writeable, use_shadow, db_dir, file_name_extension);
  return random_files[fp];
}


#endif
