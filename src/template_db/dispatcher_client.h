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

#ifndef DE__OSM3S___TEMPLATE_DB__DISPATCHER_CLIENT_H
#define DE__OSM3S___TEMPLATE_DB__DISPATCHER_CLIENT_H

#include "types.h"

#include <vector>


struct Running_Query
{
  uint32 status;
  uint32 pid;
  uint32 max_time;
  uint64 max_space;
  time_t start_time;
};

struct Client_Status
{
  uint32 rate_limit;
  std::vector< Running_Query > queries;
  std::vector< time_t > slot_starts;
};


class Dispatcher_Client
{
  public:
    /** Opens a shared memory for dispatcher communication.*/
    Dispatcher_Client(const std::string& dispatcher_share_name);
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
			      const std::string& client_token);

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

    /** Query the pid of the instance with the given token. */
    pid_t query_by_token(const std::string& token);

    Client_Status query_my_status(const std::string& token);

    void set_global_limits(uint64 max_allowed_space, uint64 max_allowed_time_units, int rate_limit);

    /** Called regularly to tell the dispatcher that this process is still alive */
    void ping();

    const std::string& get_db_dir() { return db_dir; }
    const std::string& get_shadow_name() { return shadow_name; }

  private:
    std::string dispatcher_share_name;
    int dispatcher_shm_fd;
    volatile uint8* dispatcher_shm_ptr;
    std::string db_dir, shadow_name;
    Unix_Socket socket;

    uint32 ack_arrived();

    template< class TObject >
    void send_message(TObject message, const std::string& source_pos);

    void send_message(const std::string& message, const std::string& source_pos);
};


bool file_present(const std::string& full_path);


struct Context_Error
{
  Context_Error(const std::string message_) : message(message_) {}

  std::string message;
};


#endif
