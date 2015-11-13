/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#ifndef DE__OSM3S___OVERPASS_API__DISPATCH__RESOURCE_MANAGER_H
#define DE__OSM3S___OVERPASS_API__DISPATCH__RESOURCE_MANAGER_H

#include <ctime>
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../osm-backend/area_updater.h"
#include "../statements/user_data_cache.h"

using namespace std;

class Statement;

struct Watchdog_Callback
{
  virtual void ping() const = 0;
};

class Resource_Manager
{
public:
  Resource_Manager(Transaction& transaction_, Watchdog_Callback* watchdog_ = 0,
		   Error_Output* error_output_ = 0)
      : transaction(&transaction_), error_output(error_output_),
        area_transaction(0), area_updater_(0), user_data_cache_(0),
        watchdog(watchdog_),
	start_time(time(NULL)), last_ping_time(0), last_report_time(0),
	max_allowed_time(0), max_allowed_space(0),
	desired_timestamp(NOW), diff_from_timestamp(NOW), diff_to_timestamp(NOW) {}
  
  Resource_Manager(Transaction& transaction_, Error_Output* error_output_,
		   Transaction& area_transaction_, Watchdog_Callback* watchdog_,
		   Area_Usage_Listener* area_updater__)
      : transaction(&transaction_), error_output(error_output_),
        area_transaction(&area_transaction_),
        area_updater_(area_updater__),
        user_data_cache_(0),
	watchdog(watchdog_), start_time(time(NULL)), last_ping_time(0), last_report_time(0),
	max_allowed_time(0), max_allowed_space(0),
	desired_timestamp(NOW), diff_from_timestamp(NOW), diff_to_timestamp(NOW) {}
	
  ~Resource_Manager()
  {
    if (area_updater_)
      delete area_updater_;
    area_updater_ = 0;

    if (user_data_cache_)
      delete user_data_cache_;
    user_data_cache_ = 0;
  }
  
  map< string, Set >& sets()
  {
    return sets_;
  }
  
  Area_Usage_Listener* area_updater()
  {
    return area_updater_;
  }

  void push_reference(const Set& set_);
  void pop_reference();
  void count_loop();

  void log_and_display_error(std::string message);

  void health_check(const Statement& stmt, uint32 extra_time = 0, uint64 extra_space = 0);
  
  void set_limits(uint32 max_allowed_time_, uint64 max_allowed_space_)
  {
    max_allowed_time = max_allowed_time_;
    max_allowed_space = max_allowed_space_;
  }
  
  Transaction* get_transaction() { return transaction; }
  Transaction* get_area_transaction() { return area_transaction; }
  
  uint64 get_desired_timestamp() const { return desired_timestamp; }
  uint64 get_diff_from_timestamp() const { return diff_from_timestamp; }
  uint64 get_diff_to_timestamp() const { return diff_to_timestamp; }
  
  void set_desired_timestamp(uint64 timestamp) { desired_timestamp = timestamp; }
  void set_diff_from_timestamp(uint64 timestamp) { diff_from_timestamp = timestamp; }
  void set_diff_to_timestamp(uint64 timestamp) { diff_to_timestamp = timestamp; }
  
  void set_user_data_cache(User_Data_Cache & user_data_cache) { user_data_cache_ = &user_data_cache; }
  User_Data_Cache* get_user_data_cache() { return user_data_cache_; }
  bool has_user_data_cache() { return user_data_cache_ != 0; }

private:
  map< string, Set > sets_;
  vector< const Set* > set_stack;
  vector< pair< uint, uint > > stack_progress;
  vector< long long > set_stack_sizes;
  Transaction* transaction;
  Error_Output* error_output;
  Transaction* area_transaction;
  Area_Usage_Listener* area_updater_;
  Watchdog_Callback* watchdog;
  User_Data_Cache* user_data_cache_;
  int start_time;
  uint32 last_ping_time;
  uint32 last_report_time;
  uint32 max_allowed_time;
  uint64 max_allowed_space;
  
  uint64 desired_timestamp;
  uint64 diff_from_timestamp;
  uint64 diff_to_timestamp;
};


uint64 eval_map(const std::map< Uint32_Index, vector< Node_Skeleton > >& nodes);
uint64 eval_map(const std::map< Uint31_Index, vector< Way_Skeleton > >& ways);
uint64 eval_map(const std::map< Uint31_Index, vector< Relation_Skeleton > >& relations);

uint64 eval_map(const std::map< Uint32_Index, vector< Attic< Node_Skeleton > > >& nodes);
uint64 eval_map(const std::map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways);
uint64 eval_map(const std::map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& relations);

uint64 eval_map(const std::map< Uint31_Index, vector< Area_Skeleton > >& areas);


struct Resource_Error
{
  bool timed_out;
  string stmt_name;
  uint line_number;
  uint64 size;
  uint runtime;
};

#endif
