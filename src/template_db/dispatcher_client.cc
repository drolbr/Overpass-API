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

#include "dispatcher_client.h"
#include "dispatcher.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>


Dispatcher_Client::Dispatcher_Client(const std::string& dispatcher_share_name_)
    : dispatcher_share_name(dispatcher_share_name_), socket("")
{
  signal(SIGPIPE, SIG_IGN);

  char* db_dir_env = getenv("OVERPASS_DB_DIR");
  if (db_dir_env)
    db_dir = db_dir_env;

  if (db_dir.empty())
  {
    // open dispatcher_share
    int dispatcher_shm_fd = shm_open
        (dispatcher_share_name.c_str(), O_RDWR, S_666);
    if (dispatcher_shm_fd < 0)
      throw File_Error
          (errno, dispatcher_share_name, "Dispatcher_Client::1");
    struct stat stat_buf;
    fstat(dispatcher_shm_fd, &stat_buf);
    volatile uint8* dispatcher_shm_ptr = (uint8*)mmap(
        0, stat_buf.st_size, PROT_READ, MAP_SHARED, dispatcher_shm_fd, 0);

    if (dispatcher_shm_ptr)
    {
      // get db_dir and shadow_name
      db_dir = std::string((const char *)(dispatcher_shm_ptr + 4*sizeof(uint32)),
                      *(uint32*)(dispatcher_shm_ptr + 3*sizeof(uint32)));
      shadow_name = std::string((const char *)(dispatcher_shm_ptr + 5*sizeof(uint32)
          + db_dir.size()), *(uint32*)(dispatcher_shm_ptr + db_dir.size() + 4*sizeof(uint32)));

      munmap((void*)dispatcher_shm_ptr,
          Dispatcher::SHM_SIZE + db_dir.size() + shadow_name.size());
    }
    close(dispatcher_shm_fd);
  }
  else
    shadow_name = db_dir + (dispatcher_share_name.size() >= 8
        && dispatcher_share_name.substr(dispatcher_share_name.size()-8) == "osm_base"
        ? "/osm_base_shadow" : "/areas_shadow");
    
  if (!db_dir.empty() && db_dir.back() != '/')
    db_dir += '/';

  // initialize the socket for the client
  char* socket_dir_env = getenv("OVERPASS_SOCKET_DIR");
  socket.open((socket_dir_env ? std::string(socket_dir_env) : db_dir) + dispatcher_share_name_);
  std::string socket_name = db_dir + dispatcher_share_name_;

  pid_t pid = getpid();
  if (send(socket.descriptor(), &pid, sizeof(pid_t), 0) == -1)
    throw File_Error(errno, dispatcher_share_name, "Dispatcher_Client::4");
}


bool file_present(const std::string& full_path)
{
  struct stat stat_buf;
  int result = stat(full_path.c_str(), &stat_buf);
  return result == 0;
}


template< class TObject >
void Dispatcher_Client::send_message(TObject message, const std::string& source_pos)
{
  if (send(socket.descriptor(), &message, sizeof(TObject), 0) == -1)
    throw File_Error(errno, dispatcher_share_name, source_pos);
}


uint32 Dispatcher_Client::ack_arrived()
{
  uint32 answer = 0;
  int bytes_read = recv(socket.descriptor(), &answer, sizeof(uint32), 0);
  while (bytes_read == -1)
  {
    millisleep(50);
    bytes_read = recv(socket.descriptor(), &answer, sizeof(uint32), 0);
  }
  if (bytes_read == sizeof(uint32))
    return answer;

  return 0;
}


void Dispatcher_Client::write_start()
{
  pid_t pid = getpid();

  send_message(Dispatcher::WRITE_START, "Dispatcher_Client::write_start::socket");

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
          pid_t locked_pid = 0;
          std::ifstream lock((shadow_name + ".lock").c_str());
          lock>>locked_pid;
          if (locked_pid == pid)
            return;
        }
        catch (...) {}
      }
      send_message(Dispatcher::WRITE_START, "Dispatcher_Client::write_start::socket");
    }
    millisleep(500);
  }
}


void Dispatcher_Client::migrate_start()
{
  pid_t pid = getpid();

  send_message(Dispatcher::MIGRATE_START, "Dispatcher_Client::migrate_start::socket");

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
          pid_t locked_pid = 0;
          std::ifstream lock((shadow_name + ".lock").c_str());
          lock>>locked_pid;
          if (locked_pid == pid)
            return;
        }
        catch (...) {}
      }
      send_message(Dispatcher::MIGRATE_START, "Dispatcher_Client::migrate_start::socket");
    }
    millisleep(500);
  }
}


void Dispatcher_Client::write_rollback()
{
  pid_t pid = getpid();

  send_message(Dispatcher::WRITE_ROLLBACK, "Dispatcher_Client::write_rollback::socket");

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
          pid_t locked_pid = 0;
          std::ifstream lock((shadow_name + ".lock").c_str());
          lock>>locked_pid;
          if (locked_pid != pid)
            return;
        }
        catch (...) {}
      }
      else
        return;
      send_message(Dispatcher::WRITE_ROLLBACK, "Dispatcher_Client::write_rollback::socket");
    }

    millisleep(500);
  }
}


void Dispatcher_Client::migrate_rollback()
{
  pid_t pid = getpid();

  send_message(Dispatcher::MIGRATE_ROLLBACK, "Dispatcher_Client::migrate_rollback::socket");

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
          pid_t locked_pid = 0;
          std::ifstream lock((shadow_name + ".lock").c_str());
          lock>>locked_pid;
          if (locked_pid != pid)
            return;
        }
        catch (...) {}
      }
      else
        return;
      send_message(Dispatcher::MIGRATE_ROLLBACK, "Dispatcher_Client::migrate_rollback::socket");
    }

    millisleep(500);
  }
}


void Dispatcher_Client::write_commit()
{
  pid_t pid = getpid();

  send_message(Dispatcher::WRITE_COMMIT, "Dispatcher_Client::write_commit::socket");
  millisleep(200);

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
          pid_t locked_pid = 0;
          std::ifstream lock((shadow_name + ".lock").c_str());
          lock>>locked_pid;
          if (locked_pid != pid)
            return;
        }
        catch (...) {}
      }
      else
        return;
    }

    send_message(Dispatcher::WRITE_COMMIT, "Dispatcher_Client::write_commit::socket");
    millisleep(200);
  }
}


void Dispatcher_Client::migrate_commit()
{
  pid_t pid = getpid();

  send_message(Dispatcher::MIGRATE_COMMIT, "Dispatcher_Client::migrate_commit::socket");
  millisleep(200);

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
          pid_t locked_pid = 0;
          std::ifstream lock((shadow_name + ".lock").c_str());
          lock>>locked_pid;
          if (locked_pid != pid)
            return;
        }
        catch (...) {}
      }
      else
        return;
    }

    send_message(Dispatcher::MIGRATE_COMMIT, "Dispatcher_Client::migrate_commit::socket");
    millisleep(200);
  }
}


void Dispatcher_Client::request_read_and_idx(
    uint32 max_allowed_time, uint64 max_allowed_space, uint32 client_token, uint64 request_full_hash)
{
  uint counter = 0;
  uint32 ack = 0;
  while (ack == 0 && ++counter <= 100)
  {
    send_message(Dispatcher::REQUEST_READ_AND_IDX,
		 "Dispatcher_Client::request_read_and_idx::socket::1");
    send_message(max_allowed_time, "Dispatcher_Client::request_read_and_idx::socket::2");
    send_message(max_allowed_space, "Dispatcher_Client::request_read_and_idx::socket::3");
    send_message(client_token, "Dispatcher_Client::request_read_and_idx::socket::4");
    send_message(request_full_hash, "Dispatcher_Client::request_read_and_idx::socket::5");

    ack = ack_arrived();
    if (ack == Dispatcher::REQUEST_READ_AND_IDX)
      return;

    millisleep(300);
  }
  if (ack == Dispatcher::RATE_LIMITED)
    throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::request_read_and_idx::rate_limited");
  else if (ack == Dispatcher::QUERY_REJECTED)
    throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::request_read_and_idx::timeout");
  else if (ack == Dispatcher::DUPLICATE_QUERY)
    throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::request_read_and_idx::duplicate_query");
  else
    throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::request_read_and_idx::protocol_error");
}


void Dispatcher_Client::read_idx_finished()
{
  uint counter = 0;
  while (++counter <= 300)
  {
    send_message(Dispatcher::READ_IDX_FINISHED, "Dispatcher_Client::read_idx_finished::socket");

    if (ack_arrived())
      return;
  }
  throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::read_idx_finished::timeout");
}


void Dispatcher_Client::read_finished()
{
  uint counter = 0;
  while (++counter <= 300)
  {
    send_message(Dispatcher::READ_FINISHED, "Dispatcher_Client::read_finished::socket");

    if (ack_arrived())
      return;
  }
  throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::read_finished::timeout");
}


void Dispatcher_Client::purge(uint32 pid)
{
  while (true)
  {
    send_message(Dispatcher::PURGE, "Dispatcher_Client::purge::socket::1");
    send_message(pid, "Dispatcher_Client::purge::socket::2");

    if (ack_arrived())
      return;
  }
}


pid_t Dispatcher_Client::query_by_token(uint32 token)
{
  send_message(Dispatcher::QUERY_BY_TOKEN, "Dispatcher_Client::query_by_token::socket::1");
  send_message(token, "Dispatcher_Client::query_by_token::socket::2");

  return ack_arrived();
}


Client_Status Dispatcher_Client::query_my_status(uint32 token)
{
  send_message(Dispatcher::QUERY_MY_STATUS, "Dispatcher_Client::query_my_status::socket::1");
  send_message(token, "Dispatcher_Client::query_my_status::socket::2");

  Client_Status result;
  result.rate_limit = ack_arrived();

  while (true)
  {
    Running_Query query;
    query.status = ack_arrived();
    if (query.status == 0)
      break;
    query.pid = ack_arrived();
    query.max_time = ack_arrived();
    query.max_space = ((uint64)ack_arrived() <<32) | ack_arrived();
    query.start_time = ack_arrived();
    result.queries.push_back(query);
  }

  while (true)
  {
    uint32 slot_start = ack_arrived();
    if (slot_start == 0)
      break;
    result.slot_starts.push_back(slot_start);
  }

  std::sort(result.slot_starts.begin(), result.slot_starts.end());

  return result;
}


void Dispatcher_Client::set_global_limits(uint64 max_allowed_space, uint64 max_allowed_time_units,
                                          int rate_limit)
{
  while (true)
  {
    send_message(Dispatcher::SET_GLOBAL_LIMITS, "Dispatcher_Client::set_global_limits::1");
    send_message(max_allowed_space, "Dispatcher_Client::set_global_limits::2");
    send_message(max_allowed_time_units, "Dispatcher_Client::set_global_limits::3");
    send_message(rate_limit, "Dispatcher_Client::set_global_limits::4");

    if (ack_arrived())
      return;
  }
}


void Dispatcher_Client::ping()
{
// Ping-Feature removed. The concept of unassured messages doesn't fit in the context of strict
// two-directional communication.
//   send_message(Dispatcher::PING, "Dispatcher_Client::ping::socket");
}


void Dispatcher_Client::terminate()
{
  while (true)
  {
    send_message(Dispatcher::TERMINATE, "Dispatcher_Client::terminate::socket");

    if (ack_arrived())
      return;
  }
}


void Dispatcher_Client::output_status()
{
  while (true)
  {
    send_message(Dispatcher::OUTPUT_STATUS, "Dispatcher_Client::output_status::socket");

    if (ack_arrived())
      break;
  }

  std::ifstream status((shadow_name + ".status").c_str());
  std::string buffer;
  std::getline(status, buffer);
  while (status.good())
  {
    std::cout<<buffer<<'\n';
    std::getline(status, buffer);
  }
}
