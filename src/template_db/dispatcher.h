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

#ifndef DE__OSM3S___TEMPLATE_DB__DISPATCHER_H
#define DE__OSM3S___TEMPLATE_DB__DISPATCHER_H

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


/** Is called to log all operations of the dispatcher */
struct Dispatcher_Logger
{
  typedef uint pid_t;
  
  virtual void write_start(pid_t pid, const std::vector< pid_t >& registered) = 0;
  virtual void write_rollback(pid_t pid) = 0;
  virtual void write_commit(pid_t pid) = 0;
  virtual void request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space)
      = 0;
  virtual void read_idx_finished(pid_t pid) = 0;
  virtual void prolongate(pid_t pid) = 0;
  virtual void idle_counter(uint32 idle_count) = 0;
  virtual void read_finished(pid_t pid) = 0;
  virtual void read_aborted(pid_t pid) = 0;
  virtual void purge(pid_t pid) = 0;
};


struct Reader_Entry
{
  Reader_Entry(uint32 client_pid_, uint64 max_space_, uint32 max_time_, uint32 client_token_, uint32 start_time_)
    : client_pid(client_pid_), max_space(max_space_), max_time(max_time_), start_time(start_time_),
      client_token(client_token_) {}
  
  uint32 client_pid;
  uint64 max_space;
  uint32 max_time;
  uint32 start_time;
  uint32 client_token;
};


struct Quota_Entry
{
  Quota_Entry(uint32 client_token_, uint32 expiration_time_)
    : client_token(client_token_), expiration_time(expiration_time_) {}
  
  uint32 client_token;
  uint32 expiration_time;
};


class Global_Resource_Planner
{
public:
  Global_Resource_Planner(uint32 global_available_time_, uint64 global_available_space_, uint32 rate_limit_)
      : global_used_time(0), global_available_time(global_available_time_),
        global_used_space(0), global_available_space(global_available_space_),
        rate_limit(rate_limit_), recent_average_used_time(15), recent_average_used_space(15),
        last_update_time(0), last_used_time(0), last_used_space(0), last_counted(0),
        average_used_time(0), average_used_space(0) {}
  
  // Returns true if the process is acceptable in terms of server load and quotas
  // In this case it is registered as running
  int probe(uint32 pid, uint32 client_token, uint32 time_units, uint64 max_space);

  // Unregisters the process
  void remove(uint32 pid);

  // Unregister all processes that don't have a connection anymore
  void purge(Connection_Per_Pid_Map& connection_per_pid);
    
  void set_total_available_time(uint32 global_available_time_) { global_available_time = global_available_time_; }
  void set_total_available_space(uint64 global_available_space_) { global_available_space = global_available_space_; }
  void set_rate_limit(uint rate_limit_) { rate_limit = rate_limit_; }

  const std::vector< Reader_Entry >& get_active() const { return active; }
  const std::vector< Quota_Entry >& get_afterwards() const { return afterwards; }  
  uint32 get_total_claimed_time() const { return global_used_time; }
  uint32 get_total_available_time() const { return global_available_time; }
  uint64 get_total_claimed_space() const { return global_used_space; }
  uint64 get_total_available_space() const { return global_available_space; }
  uint32 get_rate_limit() const { return rate_limit; }
  uint32 get_average_claimed_time() const { return average_used_time; }
  uint64 get_average_claimed_space() const { return average_used_space; }

private:
  std::vector< Reader_Entry > active;
  std::vector< Quota_Entry > afterwards;
  uint32 global_used_time;
  uint32 global_available_time;
  uint64 global_used_space;
  uint64 global_available_space;
  uint32 rate_limit;
  
  std::vector< uint32 > recent_average_used_time;
  std::vector< uint64 > recent_average_used_space;
  uint32 last_update_time;
  uint64 last_used_time;
  uint64 last_used_space;
  uint32 last_counted;
  uint32 average_used_time;
  uint64 average_used_space;
  
  void remove_entry(std::vector< Reader_Entry >::iterator& it);
};


// Cares for the communication between server and client
class Dispatcher_Socket
{
public:
  Dispatcher_Socket(const std::string& dispatcher_share_name,
		    const std::string& shadow_name_,
		    const std::string& db_dir_,
		    uint max_num_reading_processes_);
  ~Dispatcher_Socket();
  
  void look_for_a_new_connection(Connection_Per_Pid_Map& connection_per_pid);
  std::vector< int >::size_type num_started_connections() { return started_connections.size(); }
    
private:
  int socket_descriptor;
  std::string socket_name;
  std::vector< int > started_connections;  
};


class Dispatcher
{
  public:
    typedef uint pid_t;
    
    static const int SHM_SIZE = 3*sizeof(uint32) + 2*sizeof(uint32);//20+12+2*(256+4);
    static const int OFFSET_BACK = 20;
    static const int OFFSET_DB_1 = OFFSET_BACK+12;
    static const int OFFSET_DB_2 = OFFSET_DB_1+(256+4);
    
    static const uint32 TERMINATE = 1;
    static const uint32 OUTPUT_STATUS = 2;
    static const uint32 HANGUP = 3;
    static const uint32 PURGE = 4;
    static const uint32 SET_GLOBAL_LIMITS = 5;
    
    static const uint32 REGISTER_PID = 16;
    static const uint32 SET_LIMITS = 17;
    static const uint32 PING = 18;
    static const uint32 UNREGISTER_PID = 19;
    static const uint32 QUERY_BY_TOKEN = 20;
    
    static const uint32 RATE_LIMITED = 31;
    static const uint32 QUERY_REJECTED = 32;
    
    static const uint32 WRITE_START = 101;
    static const uint32 WRITE_ROLLBACK = 102;
    static const uint32 WRITE_COMMIT = 103;
    static const uint32 REQUEST_READ_AND_IDX = 201;
    static const uint32 READ_IDX_FINISHED = 202;
    static const uint32 READ_FINISHED = 203;
    static const uint32 READ_ABORTED = 204;
    
    /** Opens a shared memory for dispatcher communication. Furthermore,
      * detects whether idx or idy are valid, clears to idx if necessary,
      * and loads them into the shared memory idx_share_name. */
    Dispatcher(std::string dispatcher_share_name,
	       std::string index_share_name,
	       std::string shadow_name,
	       std::string db_dir,
	       uint max_num_reading_processes, uint purge_timeout,
	       uint64 total_available_space,
	       uint64 total_available_time_units,
	       const std::vector< File_Properties* >& controlled_files,
	       Dispatcher_Logger* logger = 0);
	       
    ~Dispatcher();

    /** Write operations: -------------------------------------------------- */
	       
    /** Allocates a write lock if possible. Returns without doing anything
        otherwise. */
    void write_start(pid_t pid);
    
    /** Removes the mutex for the write process without changing any
        index file. */
    void write_rollback(pid_t pid);
    
    /** Copies the shadow files onto the main index files. A lock prevents
        that incomplete copies after a crash may leave the database in an
	unstable state. Removes the mutex for the write process. */
    void write_commit(pid_t pid);
    
    /** Read operations: --------------------------------------------------- */

    /** Request the index for a read operation and registers the reading process.
        Reading the index files should be taking a quick copy, because if any process
	is in this state, write_commits are blocked. */
    void request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space,
			      uint32 client_token);
    
    /** Changes the registered state from reading the index to reading the
        database. Can be safely called multiple times for the same process. */
    void read_idx_finished(pid_t pid);
    
    /** Unregisters a reading process on its request. */
    void read_finished(pid_t pid);
    
    /** Unregisters a reading process for other reasons. */
    void read_aborted(pid_t pid);
    
    /** Other operations: -------------------------------------------------- */
    
    /** Waits for input for the given amount of time. If milliseconds if zero,
        it remains in standby forever. */
    void standby_loop(uint64 milliseconds);

    /** Outputs the status of the processes registered with the dispatcher
        into shadow_name.status. */
    void output_status();
    
    /** Set the limit of simultaneous queries from a single IP address. */
    void set_rate_limit(uint rate_limit) { global_resource_planner.set_rate_limit(rate_limit); }
    
  private:
    Dispatcher_Socket socket;
    Connection_Per_Pid_Map connection_per_pid;
    std::vector< File_Properties* > controlled_files;
    std::vector< Idx_Footprints > data_footprints;
    std::vector< Idx_Footprints > map_footprints;
    std::set< pid_t > processes_reading_idx;
    std::string shadow_name, db_dir;
    std::string dispatcher_share_name;
    int dispatcher_shm_fd;
    volatile uint8* dispatcher_shm_ptr;
    Dispatcher_Logger* logger;
    std::set< pid_t > disconnected;
    bool pending_commit;
    uint32 requests_started_counter;
    uint32 requests_finished_counter;
    Global_Resource_Planner global_resource_planner;
    
    void copy_shadows_to_mains();
    void copy_mains_to_shadows();
    void remove_shadows();
    void set_current_footprints();
    std::vector< pid_t > write_index_of_empty_blocks();
    void check_and_purge();
    uint64 total_claimed_space() const;
    uint64 total_claimed_time_units() const;
};


#endif
