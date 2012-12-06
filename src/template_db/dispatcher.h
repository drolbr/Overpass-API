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

#include "types.h"

#include <map>
#include <set>
#include <vector>

using namespace std;


class Idx_Footprints
{
  public:
    typedef uint pid_t;
    
    void set_current_footprint(const vector< bool >& footprint);
    void register_pid(pid_t pid); 
    void unregister_pid(pid_t pid); 
    vector< pid_t > registered_processes() const;
    vector< bool > total_footprint() const;
    
  private:
    vector< bool > current_footprint;
    map< pid_t, vector< bool > > footprint_per_pid;
};


/** Is called to log all operations of the dispatcher */
struct Dispatcher_Logger
{
  typedef uint pid_t;
  
  virtual void write_start(pid_t pid, const vector< pid_t >& registered) = 0;
  virtual void write_rollback(pid_t pid) = 0;
  virtual void write_commit(pid_t pid) = 0;
  virtual void request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space)
      = 0;
  virtual void read_idx_finished(pid_t pid) = 0;
  virtual void prolongate(pid_t pid) = 0;
  virtual void idle_counter(uint32 idle_count) = 0;
  virtual void read_finished(pid_t pid) = 0;
  virtual void purge(pid_t pid) = 0;
};


/* Represents the connection to a client that is blocking after it has send a command until
 * it gets an answer about the command execution state. This class ensures that the software cannot
 * fail due to a broken pipe. */
struct Blocking_Client_Socket
{
  Blocking_Client_Socket(int socket_descriptor_);
  uint32 get_command();
  vector< uint32 > get_arguments(int num_arguments);
  void clear_state();
  void send_result(uint32 result);
  ~Blocking_Client_Socket();
private:
  int socket_descriptor;
  enum { waiting, processing_command, disconnected } state;
  uint32 last_command;
};


class Connection_Per_Pid_Map
{
public:
  typedef uint pid_t;
    
  Blocking_Client_Socket* get(pid_t pid)
  {
    map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.find(pid);    
    if (it != connection_per_pid.end())
      return it->second;
    else
      return 0;
  }
  
  void set(pid_t pid, Blocking_Client_Socket* socket)
  {
    map< pid_t, Blocking_Client_Socket* >::iterator it = connection_per_pid.find(pid);
    if (it != connection_per_pid.end())
      delete it->second;
    if (socket != 0)
      connection_per_pid[pid] = socket;
    else
      connection_per_pid.erase(pid);
  }
  
  const map< pid_t, Blocking_Client_Socket* >& base_map() const { return connection_per_pid; }
  
private:
  map< pid_t, Blocking_Client_Socket* > connection_per_pid;
};


struct Reader_Entry
{
  Reader_Entry(uint32 ping_time_, uint64 max_space_, uint32 max_time_, uint32 client_token_)
    : ping_time(ping_time_), max_space(max_space_), max_time(max_time_), client_token(client_token_)
  {
    ++active_client_tokens[client_token];
  }
  
  Reader_Entry() : ping_time(0), max_space(0), max_time(0), client_token(0) {}
  
  Reader_Entry(const Reader_Entry& e)
    : ping_time(e.ping_time), max_space(e.max_space), max_time(e.max_time), client_token(e.client_token)
  {
    ++active_client_tokens[client_token];
  }
  
  Reader_Entry& operator=(const Reader_Entry& e)
  {
    --active_client_tokens[client_token];
    
    ping_time = e.ping_time;
    max_space = e.max_space;
    max_time = e.max_time;
    client_token = e.client_token;
    
    ++active_client_tokens[client_token];
    
    return *this;
  }
  
  ~Reader_Entry()
  {
    --active_client_tokens[client_token];
  }
  
  uint32 ping_time;
  uint64 max_space;
  uint32 max_time;
  uint32 client_token;

  static map< uint32, uint > active_client_tokens;
};


class Dispatcher
{
  public:
    typedef uint pid_t;
    
    static const int SHM_SIZE = 3*sizeof(uint32) + 2*sizeof(uint32);//20+12+2*(256+4);
    static const int OFFSET_BACK = 20;
    static const int OFFSET_DB_1 = OFFSET_BACK+12;
    static const int OFFSET_DB_2 = OFFSET_DB_1+(256+4);
    
    static const uint32 REGISTER_PID = 16;
    static const uint32 SET_LIMITS = 17;
    static const uint32 PING = 18;
    static const uint32 UNREGISTER_PID = 19;
    static const uint32 QUERY_REJECTED = 32;
    
    /** Opens a shared memory for dispatcher communication. Furthermore,
      * detects whether idx or idy are valid, clears to idx if necessary,
      * and loads them into the shared memory idx_share_name. */
    Dispatcher(string dispatcher_share_name,
	       string index_share_name,
	       string shadow_name,
	       string db_dir,
	       uint max_num_reading_processes, uint purge_timeout,
	       uint64 total_available_space,
	       uint64 total_available_time_units,
	       const vector< File_Properties* >& controlled_files,
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
    
    /** Refreshes the timeout for a reading process. */
    void prolongate(pid_t pid);
    
    /** Unregisters a reading process. */
    void read_finished(pid_t pid);
    
    /** Other operations: -------------------------------------------------- */
    
    /** Waits for input for the given amount of time. If milliseconds if zero,
        it remains in standby forever. */
    void standby_loop(uint64 milliseconds);

    /** Outputs the status of the processes registered with the dispatcher
        into shadow_name.status. */
    void output_status();
    
    static const uint32 TERMINATE = 1;
    static const uint32 OUTPUT_STATUS = 2;
    static const uint32 HANGUP = 3;
    static const uint32 PURGE = 4;
    static const uint32 SET_GLOBAL_LIMITS = 5;
    static const uint32 RATE_LIMITED = 6;
    static const uint32 WRITE_START = 101;
    static const uint32 WRITE_ROLLBACK = 102;
    static const uint32 WRITE_COMMIT = 103;
    static const uint32 REQUEST_READ_AND_IDX = 201;
    static const uint32 READ_IDX_FINISHED = 202;
    static const uint32 READ_FINISHED = 203;
    static const uint32 READ_ABORTED = 204;
    
  private:
    vector< File_Properties* > controlled_files;
    vector< Idx_Footprints > data_footprints;
    vector< Idx_Footprints > map_footprints;
    set< pid_t > processes_reading_idx;
    map< pid_t, Reader_Entry > processes_reading;
    string shadow_name, db_dir;
    string dispatcher_share_name;
    int dispatcher_shm_fd;
    uint max_num_reading_processes;
    uint purge_timeout;
    uint64 total_available_space;
    uint64 total_available_time_units;
    volatile uint8* dispatcher_shm_ptr;
    Dispatcher_Logger* logger;
    int socket_descriptor;
    vector< int > started_connections;
    Connection_Per_Pid_Map connection_per_pid;
    set< pid_t > disconnected;
    bool pending_commit;
    
    void copy_shadows_to_mains();
    void copy_mains_to_shadows();
    void remove_shadows();
    void set_current_footprints();
    vector< pid_t > write_index_of_empty_blocks();
    void check_and_purge();
    uint64 total_claimed_space() const;
    uint64 total_claimed_time_units() const;
};


class Dispatcher_Client
{
  public:
    /** Opens a shared memory for dispatcher communication.*/
    Dispatcher_Client(string dispatcher_share_name);
    ~Dispatcher_Client();

    /** Write operations: -------------------------------------------------- */
	       
    /** Allocates a write lock. Waits if necessary. */
    void write_start();
    
    /** Aborts an active writing operation. Results are undefined if it is
        called outside a writing operation. */
    void write_rollback();
    
    /** Commits an active writing operation. Results are undefined if it is
        called outside a writing operation. */
    void write_commit();
    
    /** Read operations: --------------------------------------------------- */

    /** Request the index for a read operation and registers the reading process.
    Reading the index files should be taking a quick copy, because if any process
    is in this state, write_commits are blocked. */
    void request_read_and_idx(uint32 max_allowed_time, uint64 max_allowed_space,
			      uint32 client_token);
    
    /** Changes the registered state from reading the index to reading the
    database. Can be safely called multiple times for the same process. */
    void read_idx_finished();
    
    /** Unregisteres a reading process. */
    void read_finished();
    
    /** Other operations: -------------------------------------------------- */
    
    /** Terminate another instance running in the standby_loop. */
    void terminate();

    /** Let another instance running in the standby_loop output its status. */
    void output_status();
    
    /** Purge another instance. */
    void purge(uint32 pid);
    
    /** Purge another instance. */
    void set_global_limits(uint64 max_allowed_space, uint64 max_allowed_time_units);
    
    /** Called regularly to tell the dispatcher that this process is still alive */
    void ping();
    
    const string& get_db_dir() { return db_dir; }
    const string& get_shadow_name() { return shadow_name; }
    
  private:
    string dispatcher_share_name;
    int dispatcher_shm_fd;
    volatile uint8* dispatcher_shm_ptr;
    string db_dir, shadow_name;
    int socket_descriptor;
    
    uint32 ack_arrived();
    
    template< class TObject >
    void send_message(TObject message, string source_pos);
};


void millisleep(uint32 milliseconds);


#endif
