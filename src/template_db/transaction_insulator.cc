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

#include "dispatcher.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>


void Idx_Footprints::set_current_footprint(const std::vector< bool >& footprint)
{
  current_footprint = footprint;
}


void Idx_Footprints::register_pid(pid_t pid)
{
  footprint_per_pid[pid] = current_footprint;
}


void Idx_Footprints::unregister_pid(pid_t pid)
{
  footprint_per_pid.erase(pid);
}


std::vector< Idx_Footprints::pid_t > Idx_Footprints::registered_processes() const
{
  std::vector< pid_t > result;
  for (std::map< pid_t, std::vector< bool > >::const_iterator
      it(footprint_per_pid.begin()); it != footprint_per_pid.end(); ++it)
    result.push_back(it->first);
  return result;
}


std::vector< bool > Idx_Footprints::total_footprint() const
{
  std::vector< bool > result = current_footprint;
  for (std::map< pid_t, std::vector< bool > >::const_iterator
      it(footprint_per_pid.begin()); it != footprint_per_pid.end(); ++it)
  {
    // By construction, it->second.size() <= result.size()
    for (std::vector< bool >::size_type i = 0; i < it->second.size(); ++i)
      result[i] = result[i] | (it->second)[i];
  }
  return result;
}


Transaction_Insulator::Transaction_Insulator(
    const std::string& db_dir, const std::vector< File_Properties* >& controlled_files_)
    : db_dir_(db_dir), controlled_files(controlled_files_),
    data_footprints(controlled_files_.size()), map_footprints(controlled_files_.size())
{
  // get the absolute pathname of the current directory
  if (db_dir.substr(0, 1) != "/")
    db_dir_ = getcwd() + db_dir;
}


void Transaction_Insulator::request_read_and_idx(pid_t pid)
{ 
  for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->register_pid(pid);
  for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->register_pid(pid);
}


void Transaction_Insulator::read_finished(pid_t pid)
{
  for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->unregister_pid(pid);
  for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->unregister_pid(pid);
}


void Transaction_Insulator::copy_shadows_to_mains()
{
  for (std::vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
      copy_file(db_dir() + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
                + (*it)->get_index_suffix() + (*it)->get_shadow_suffix(),
		db_dir() + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
		+ (*it)->get_index_suffix());
      copy_file(db_dir() + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
                + (*it)->get_index_suffix() + (*it)->get_shadow_suffix(),
		db_dir() + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
		+ (*it)->get_index_suffix());
  }
}


void Transaction_Insulator::copy_mains_to_shadows()
{
  for (std::vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
      copy_file(db_dir() + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
                + (*it)->get_index_suffix(),
		db_dir() + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
		+ (*it)->get_index_suffix() + (*it)->get_shadow_suffix());
      copy_file(db_dir() + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
                + (*it)->get_index_suffix(),
		db_dir() + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
		+ (*it)->get_index_suffix() + (*it)->get_shadow_suffix());
  }
}


void Transaction_Insulator::remove_shadows()
{
  for (std::vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
    remove((db_dir() + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
            + (*it)->get_index_suffix() + (*it)->get_shadow_suffix()).c_str());
    remove((db_dir() + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
            + (*it)->get_index_suffix() + (*it)->get_shadow_suffix()).c_str());
    remove((db_dir() + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
            + (*it)->get_shadow_suffix()).c_str());
    remove((db_dir() + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
            + (*it)->get_shadow_suffix()).c_str());
  }
}


void Transaction_Insulator::set_current_footprints()
{
  for (std::vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    try
    {
      data_footprints[i].set_current_footprint
          (controlled_files[i]->get_data_footprint(db_dir()));
    }
    catch (File_Error e)
    {
      std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    catch (...) {}
    
    try
    {
      map_footprints[i].set_current_footprint
          (controlled_files[i]->get_map_footprint(db_dir()));
    }
    catch (File_Error e)
    {
      std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    catch (...) {}
  }
}


std::set< pid_t > Transaction_Insulator::registered_pids() const
{
  std::set< pid_t > registered;
  
  for (std::vector< Idx_Footprints >::const_iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
  {
    std::vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (std::vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      registered.insert(*it);
  }
  for (std::vector< Idx_Footprints >::const_iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
  {
    std::vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (std::vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      registered.insert(*it);
  }
  
  return registered;
}


void write_to_index_empty_file_data(const std::vector< bool >& footprint, const std::string& filename)
{
  Void_Pointer< std::pair< uint32, uint32 > > buffer(footprint.size() * 8);  
  std::pair< uint32, uint32 >* pos = buffer.ptr;
  uint32 last_start = 0;
  for (uint32 i = 0; i < footprint.size(); ++i)
  {
    if (footprint[i])
    {
      if (last_start < i)
      {
	*pos = std::make_pair(i - last_start, last_start);
	++pos;
      }
      last_start = i+1;
    }
  }
  if (last_start < footprint.size())
  {
    *pos = std::make_pair(footprint.size() - last_start, last_start);
    ++pos;
  }
  
  Raw_File file(filename, O_RDWR|O_CREAT|O_TRUNC,
		S_666, "write_to_index_empty_file_data:1");
  file.write((uint8*)buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr), "Dispatcher:26");
}


void write_to_index_empty_file_ids(const std::vector< bool >& footprint, const std::string& filename)
{
  Void_Pointer< std::pair< uint32, uint32 > > buffer(footprint.size() * 8);
  std::pair< uint32, uint32 >* pos = buffer.ptr;
  uint32 last_start = 0;
  for (uint32 i = 0; i < footprint.size(); ++i)
  {
    if (footprint[i])
    {
      if (last_start < i)
      {
	*pos = std::make_pair(i - last_start, last_start);
	++pos;
      }
      last_start = i+1;
    }
  }
  if (last_start < footprint.size())
  {
    *pos = std::make_pair(footprint.size() - last_start, last_start);
    ++pos;
  }
  
  Raw_File file(filename, O_RDWR|O_CREAT|O_TRUNC,
		S_666, "write_to_index_empty_file_ids:1");
  file.write((uint8*)buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr), "Dispatcher:36");
}


void Transaction_Insulator::write_index_of_empty_blocks()
{
  for (std::vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    if (file_exists(db_dir() + controlled_files[i]->get_file_name_trunk()
        + controlled_files[i]->get_data_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file_data
          (data_footprints[i].total_footprint(),
	   db_dir() + controlled_files[i]->get_file_name_trunk()
	   + controlled_files[i]->get_data_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
    if (file_exists(db_dir() + controlled_files[i]->get_file_name_trunk()
        + controlled_files[i]->get_id_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file_ids
          (map_footprints[i].total_footprint(),
	   db_dir() + controlled_files[i]->get_file_name_trunk()
	   + controlled_files[i]->get_id_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
  }
}
