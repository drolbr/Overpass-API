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

#ifndef DE__OSM3S___TEMPLATE_DB__TRANSACTION_INSULATOR_H
#define DE__OSM3S___TEMPLATE_DB__TRANSACTION_INSULATOR_H

#include "file_tools.h"
#include "types.h"

#include <map>
#include <set>
#include <vector>


class Idx_Footprints
{
  public:
    typedef uint pid_t;

    void set_current_footprint(const std::vector< bool >& footprint);
    void register_pid(pid_t pid);
    void unregister_pid(pid_t pid);
    std::vector< pid_t > registered_processes() const;
    std::vector< bool > total_footprint() const;

  private:
    std::vector< bool > current_footprint;
    std::map< pid_t, std::vector< bool > > footprint_per_pid;
};


class Transaction_Insulator
{
public:
  Transaction_Insulator(const std::string& db_dir, const std::vector< File_Properties* >& controlled_files_);
  void request_read_and_idx(pid_t pid);
  void read_finished(pid_t pid);
  std::vector< ::pid_t > registered_pids() const;

  void copy_shadows_to_mains();
  void copy_mains_to_shadows();
  void remove_shadows();
  void remove_migrated();
  void set_current_footprints();
  void write_index_of_empty_blocks();
  void move_migrated_files_in_place();

  const std::string& db_dir() const { return db_dir_; }

private:
  std::string db_dir_;
  std::vector< File_Properties* > controlled_files;
  std::vector< Idx_Footprints > data_footprints;
  std::vector< Idx_Footprints > map_footprints;
};


#endif
