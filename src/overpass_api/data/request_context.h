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

#ifndef DE__OSM3S___OVERPASS_API__DATA__REQUEST_CONTEXT_H
#define DE__OSM3S___OVERPASS_API__DATA__REQUEST_CONTEXT_H

#include "../dispatch/resource_manager.h"


class Statement;


class Health_Guard
{
public:
  Health_Guard(const Statement* stmt_, Resource_Manager& rman_) : stmt(stmt_), rman(rman_) {}

  void log_and_display_error(const std::string& message) const { rman.log_and_display_error(message); }
  bool check(uint32 extra_time = 0, uint64 extra_space = 0)
  { return (stmt) && rman.health_check(*stmt, extra_time, extra_space); }

private:
  const Statement* stmt;
  Resource_Manager& rman;
};


class Request_Context
{
public:
  Request_Context(const Statement* stmt_, Resource_Manager& rman_) : stmt(stmt_), rman(rman_) {}

  Health_Guard get_health_guard() { return Health_Guard(stmt, rman); }
  uint64 get_desired_timestamp() const { return rman.get_desired_timestamp(); }
  uint64 get_diff_from_timestamp() const { return rman.get_diff_from_timestamp(); }
  uint64 get_diff_to_timestamp() const { return rman.get_diff_to_timestamp(); }
  File_Blocks_Index_Base* data_index(const File_Properties* file_properties)
  { return rman.get_transaction()->data_index(file_properties); }
  Random_File_Index* random_index(const File_Properties* file_properties)
  { return rman.get_transaction()->random_index(file_properties); }

private:
  const Statement* stmt;
  Resource_Manager& rman;
};


#endif
