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
#include <string>
#include <vector>


struct Block_Backend_Cache_Base
{
  virtual long long get_total_size() = 0;
  virtual void trim_non_reserved() = 0;
  virtual void trim_reserved() = 0;
  
  virtual ~Block_Backend_Cache_Base() {}
};


class Transaction
{
  public:
    virtual ~Transaction() {}
    virtual File_Blocks_Index_Base* data_index(const File_Properties*) = 0;
    virtual Random_File_Index* random_index(const File_Properties*) = 0;
    virtual std::string get_db_dir() const = 0;
    
    virtual Block_Backend_Cache_Base& get_cache(const File_Properties&) = 0;
    virtual void trim_cache() const = 0;
};


class Nonsynced_Transaction : public Transaction
{
  public:
    Nonsynced_Transaction
        (bool writeable, bool use_shadow,
	 const std::string& db_dir, const std::string& file_name_extension, long long max_cache_size_ = 0);
    virtual ~Nonsynced_Transaction();
    
    virtual File_Blocks_Index_Base* data_index(const File_Properties*);
    virtual Random_File_Index* random_index(const File_Properties*);
    
    void flush();
    virtual std::string get_db_dir() const { return db_dir; }
    
    virtual Block_Backend_Cache_Base& get_cache(const File_Properties&);
    virtual void trim_cache() const;
    
  private:
    std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      data_files;
    std::map< const File_Properties*, Random_File_Index* >
      random_files;
    bool writeable, use_shadow;
    std::string file_name_extension, db_dir;
    long long max_cache_size;
};


#endif
