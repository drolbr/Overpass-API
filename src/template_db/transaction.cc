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

#include "block_backend_cache.h"
#include "transaction.h"

#include <map>
#include <string>
#include <vector>


Nonsynced_Transaction::Nonsynced_Transaction
    (bool writeable_, bool use_shadow_,
     const std::string& db_dir_, const std::string& file_name_extension_, uint64 max_cache_size_)
  : writeable(writeable_), use_shadow(use_shadow_),
    file_name_extension(file_name_extension_), db_dir(db_dir_),
    max_cache_size(max_cache_size_) {}

    
Nonsynced_Transaction::~Nonsynced_Transaction()
{
  flush();
}


void Nonsynced_Transaction::flush()
{
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >::iterator
      it = data_files.begin(); it != data_files.end(); ++it)
  {
    delete it->second.first;
    delete it->second.second;
  }
  data_files.clear();
  for (std::map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.begin(); it != random_files.end(); ++it)
    delete it->second;
  random_files.clear();
}


File_Blocks_Index_Base* Nonsynced_Transaction::data_index(const File_Properties& fp)
{ 
  std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >::iterator
      it = data_files.find(&fp);
  if (it != data_files.end())
    return it->second.first;

  File_Blocks_Index_Base* data_index = fp.new_data_index
      (writeable, use_shadow, db_dir, file_name_extension);
  if (data_index != 0)
    data_files[&fp] = std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* >
        (data_index, fp.new_cache(*data_index, *this));
  return data_index;
}


Block_Backend_Cache_Base& Nonsynced_Transaction::get_cache(const File_Properties& fp)
{
  std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >::iterator
      it = data_files.find(&fp);
  if (it != data_files.end())
    return *it->second.second;

  File_Blocks_Index_Base* data_index = fp.new_data_index
      (writeable, use_shadow, db_dir, file_name_extension);
  if (data_index != 0)
    data_files[&fp] = std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* >
      (data_index, fp.new_cache(*data_index, *this));
  return *data_files[&fp].second;
}


Random_File_Index* Nonsynced_Transaction::random_index(const File_Properties* fp)
{ 
  std::map< const File_Properties*, Random_File_Index* >::iterator
      it = random_files.find(fp);
  if (it != random_files.end())
    return it->second;
  
  random_files[fp] = new Random_File_Index(*fp, writeable, use_shadow, db_dir);
  return random_files[fp];
}


void Nonsynced_Transaction::trim_cache() const
{
  if (max_cache_size == 0)
    return;
  
  uint64 total_size = size_cached();
  if (total_size > max_cache_size)
  {
    for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
        ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    {
      if (it->second.second)
	it->second.second->trim_non_reserved();
    }
  }
  
  // size has changed because we have dropped part of the cache
  total_size = size_cached();
  if (total_size > max_cache_size)
  {
    for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
        ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    {
      if (it->second.second)
	it->second.second->trim_reserved();
    }
  }
}

  
uint64 Nonsynced_Transaction::size_cached() const
{
  uint64 result = 0;
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    result += it->second.second ? it->second.second->size_cached() : 0;
  return result;
}


uint64 Nonsynced_Transaction::size_total_requested() const
{
  uint64 result = 0;
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    result += it->second.second ? it->second.second->size_total_requested() : 0;
  return result;
}


uint64 Nonsynced_Transaction::size_read_from_disk() const
{
  uint64 result = 0;
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    result += it->second.second ? it->second.second->size_read_from_disk() : 0;
  return result;
}


uint32 Nonsynced_Transaction::num_cached() const
{
  uint64 result = 0;
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    result += it->second.second ? it->second.second->num_cached() : 0;
  return result;
}


uint32 Nonsynced_Transaction::num_total_requested() const
{
  uint64 result = 0;
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    result += it->second.second ? it->second.second->num_total_requested() : 0;
  return result;
}


uint32 Nonsynced_Transaction::num_read_from_disk() const
{
  uint64 result = 0;
  for (std::map< const File_Properties*, std::pair< File_Blocks_Index_Base*, Block_Backend_Cache_Base* > >
      ::const_iterator it = data_files.begin(); it != data_files.end(); ++it)
    result += it->second.second ? it->second.second->num_read_from_disk() : 0;
  return result;
}
