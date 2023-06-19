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

#ifndef DE__OSM3S___TEMPLATE_DB__DISPATCHER_H
#define DE__OSM3S___TEMPLATE_DB__DISPATCHER_H

#include "file_tools.h"
#include "types.h"
#include "transaction_insulator.h"

#include <map>
#include <set>
#include <vector>


/** Is called to log all operations of the dispatcher */
struct Dispatcher_Logger
{
  typedef uint pid_t;

  virtual void terminate_triggered(int32 countdown, pid_t writing_process) = 0;

  virtual void write_start(pid_t pid, const std::vector< ::pid_t >& registered) = 0;
  virtual void write_rollback(pid_t pid) = 0;
  virtual void write_pending(pid_t pid, const std::set< pid_t >& reading) = 0;
  virtual void write_commit(pid_t pid) = 0;
  virtual void migrate_start(pid_t pid, const std::vector< ::pid_t >& registered) = 0;
  virtual void migrate_rollback(pid_t pid) = 0;
  virtual void migrate_commit(pid_t pid) = 0;

  virtual void request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space)
      = 0;
  virtual void read_idx_finished(pid_t pid) = 0;
  virtual void prolongate(pid_t pid) = 0;
  virtual void idle_counter(uint32 idle_count) = 0;
  virtual void read_finished(pid_t pid) = 0;
  virtual void query_my_status(pid_t pid) = 0;
  virtual void read_aborted(pid_t pid) = 0;
  virtual void hangup(pid_t pid) = 0;
  virtual void purge(pid_t pid) = 0;
};


struct Reader_Entry
{
  Reader_Entry(uint32 client_pid_, uint64 max_space_, uint32 max_time_, uint32 client_token_, uint32 start_time_)
    : client_pid(client_pid_), max_space(max_space_), max_time(max_time_), start_time(start_time_),
      client_token(client_token_) {}

  pid_t client_pid;
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


struct Pending_Client
{
  Pending_Client(pid_t pid_, uint32 first_seen_) : pid(pid_), first_seen(first_seen_) {}

  pid_t pid;
  uint32 first_seen;
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
  int probe(pid_t pid, uint32 client_token, uint32 time_units, uint64 max_space);

  // Unregisters the process
  void remove(pid_t pid);

  // Unregister all processes that don't have a connection anymore
  void purge(Connection_Per_Pid_Map& connection_per_pid);

  void set_total_available_time(uint32 global_available_time_) { global_available_time = global_available_time_; }
  void set_total_available_space(uint64 global_available_space_) { global_available_space = global_available_space_; }
  void set_rate_limit(uint rate_limit_) { rate_limit = rate_limit_; }

  const std::vector< Reader_Entry >& get_active() const { return active; }
  bool is_active(pid_t client_pid) const;
  const std::vector< Quota_Entry >& get_afterwards() const { return afterwards; }
  uint32 get_total_claimed_time() const { return global_used_time; }
  uint32 get_total_available_time() const { return global_available_time; }
  uint64 get_total_claimed_space() const { return global_used_space; }
  uint64 get_total_available_space() const { return global_available_space; }
  uint32 get_rate_limit() const { return rate_limit; }
  uint32 get_average_claimed_time() const { return average_used_time; }
  uint64 get_average_claimed_space() const { return average_used_space; }

private:
  std::map< uint32, std::vector< Pending_Client > > pending;
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
  Dispatcher_Socket(
      const std::string& dispatcher_share_name, const std::string& db_dir_,
      uint max_num_reading_processes_, uint max_num_socket_clients);
  ~Dispatcher_Socket();

  void look_for_a_new_connection(Connection_Per_Pid_Map& connection_per_pid);
  std::vector< int >::size_type num_started_connections() { return started_connections.size(); }

private:
  Unix_Socket socket;
  std::string socket_name;
  std::vector< int > started_connections;
  uint open_socket_limit;
};


class Dispatcher
{
  public:
    typedef uint pid_t;

    static const int SHM_SIZE = 3*sizeof(uint32) + 2*sizeof(uint32);//20+12+2*(256+4);
    static const int OFFSET_BACK = 20;
    static const int OFFSET_DB_1 = OFFSET_BACK+12;
    static const int OFFSET_DB_2 = OFFSET_DB_1+(256+4);
    
    static const int32 TERMINATE_COUNTDOWN_START = 500;

    static const uint32 TERMINATE = 0x100;
    static const uint32 OUTPUT_STATUS = 0x200;
    static const uint32 HANGUP = 0x300;
    static const uint32 PURGE = 0x401;
    static const uint32 SET_GLOBAL_LIMITS = 0x505;

    static const uint32 QUERY_MY_STATUS = 0x1101;
    static const uint32 REGISTER_PID = 0x1200;
    static const uint32 SET_LIMITS = 0x1300;
    static const uint32 PING = 0x1400;
    static const uint32 UNREGISTER_PID = 0x1500;
    static const uint32 QUERY_BY_TOKEN = 0x1601;

    static const uint32 PROTOCOL_INVALID = 0x1f100;
    static const uint32 RATE_LIMITED = 0x1f200;
    static const uint32 QUERY_REJECTED = 0x1f300;

    static const uint32 WRITE_START = 0x10100;
    static const uint32 WRITE_ROLLBACK = 0x10200;
    static const uint32 WRITE_COMMIT = 0x10300;
    static const uint32 MIGRATE_START = 0x11100;
    static const uint32 MIGRATE_ROLLBACK = 0x11200;
    static const uint32 MIGRATE_COMMIT = 0x11300;
    static const uint32 REQUEST_READ_AND_IDX = 0x20104;
    static const uint32 READ_IDX_FINISHED = 0x20200;
    static const uint32 READ_FINISHED = 0x20300;
    static const uint32 READ_ABORTED = 0x20400;

    /** Opens a shared memory for dispatcher communication. Furthermore,
      * detects whether idx or idy are valid, clears to idx if necessary,
      * and loads them into the shared memory idx_share_name. */
    Dispatcher(
      const std::string& dispatcher_share_name_,
      const std::string& index_share_name, const std::string& shadow_name_,
      const std::string& db_dir_, const std::string& socket_dir,
      uint max_num_reading_processes, uint max_num_socket_clients, uint purge_timeout,
      uint64 total_available_space, uint64 total_available_time_units,
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

    /** Allocates a write lock if possible. Returns without doing anything
        otherwise. */
    void migrate_start(pid_t pid);

    /** Removes the mutex for the write process and the migrated data and index files
        without changing any mainline index file. */
    void migrate_rollback(pid_t pid);

    /** Replaces the data and index files with the migrated files
        in all cases where those files exist. A lock prevents
        that incomplete copies after a crash may leave the database in an
        unstable state. Removes the mutex for the write process. */
    void migrate_commit(pid_t pid);

    /** Read operations: --------------------------------------------------- */

    /** Request the index for a read operation and registers the reading process.
        Reading the index files should be taking a quick copy, because if any process
        is in this state, write_commits are blocked. */
    void request_read_and_idx(
        pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space, uint32 client_token);

    /** Changes the registered state from reading the index to reading the
        database. Can be safely called multiple times for the same process. */
    void read_idx_finished(pid_t pid);

    /** Unregisters a reading process on its request. */
    void read_finished(pid_t pid);

    /** Unregisters a reading process for other reasons. */
    void read_aborted(pid_t pid);

    /** Unregisters a non-reading process. */
    void hangup(pid_t pid);

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
    Transaction_Insulator transaction_insulator;
    std::set< pid_t > processes_reading_idx;
    pid_t writing_process;
    std::string shadow_name;
    std::string dispatcher_share_name;
    int dispatcher_shm_fd;
    volatile uint8* dispatcher_shm_ptr;
    Dispatcher_Logger* logger;
    std::set< pid_t > disconnected;
    bool pending_commit;
    int32 terminate_countdown;
    uint32 requests_started_counter;
    uint32 requests_finished_counter;
    Global_Resource_Planner global_resource_planner;

    bool get_lock_for_idx_change(pid_t pid);
    uint64 total_claimed_space() const;
    uint64 total_claimed_time_units() const;
};


#endif
