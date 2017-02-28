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

#ifndef DE__OSM3S___TEMPLATE_DB__QUOTA_H
#define DE__OSM3S___TEMPLATE_DB__QUOTA_H


#include "file_tools.h"

#include <vector>


class Global_Status;


struct Connection_State
{
  enum _ { pending, reading_idx, running, finished, zombie };
};


class Client
{
public:
  Client(Global_Status& global_, uint32 client_token)
      : client_token_(client_token), num_pending(0), num_reading_idx(0), num_running(0),
      sum_maxtime(0), sum_maxspace(0), global(&global_) {}
  
  void add_pending() { ++num_pending; }
  uint32 probe(uint32 maxtime, uint64 maxspace, uint32 first_seen);
  void change_state(Connection_State::_ old_state, Connection_State::_ new_state, uint32 maxtime, uint64 maxspace);
  uint32 client_token() const { return client_token_; }
  bool is_idle() const { return quota_times.empty() && num_pending + num_reading_idx + num_running == 0; }
  
private:
  Client(const Client&);
  Client& operator=(const Client&);
  
  uint32 client_token_;
  uint32 num_pending;
  uint32 num_reading_idx;
  uint32 num_running;
  uint32 sum_maxtime;
  uint64 sum_maxspace;
  std::vector< uint32 > quota_times;
  Global_Status* global;
};


// Takes ownership of the socket
class Connection
{
public:  
  Connection(uint socket_, pid_t pid_)
      : socket(new Blocking_Client_Socket(socket_)), pid(pid_),
      state_(Connection_State::pending), maxtime(0), maxspace(0), first_seen(time(0)), client(0) {}
  ~Connection()
  {
    delete socket;
    socket = 0;
    
    if (client)
      client->change_state(state_, Connection_State::zombie, maxtime, maxspace);
  }
  
  uint32 poll(Global_Status& global);
  bool is_active() const { return socket; }
  Connection_State::_ state() const { return state_; }
  Blocking_Client_Socket* socket_() { return socket; } //TODO: temp
  uint32 pid_() { return pid; } //TODO: temp
  uint32 maxtime_() { return maxtime; } //TODO: temp
  uint64 maxspace_() { return maxspace; } //TODO: temp
  
private:
  // class invariant: socket ^ (state_ == finished || state_ == zombie)
  // class invariant: !socket => !client
  // class invariant: socket && !client => state_ == pending
  
  Blocking_Client_Socket* socket;
  pid_t pid;
  
  Connection_State::_ state_;
  uint32 maxtime;
  uint64 maxspace;
  uint32 first_seen;
  Client* client;
  
  Connection(const Connection&);
  Connection& operator=(const Connection&);
};


// Takes ownership of the connections
class Connection_Container
{
public:
  Connection_Container() : last_polled_pos(0) {}
  ~Connection_Container();
  
  void insert(Connection* connection) { connections.push_back(connection); }
  
  Connection* poll_round_robin(Global_Status& global, uint32& command, uint32& client_pid);
  void output_connection_status() const;
  bool any_reading_idx() const;
  
private:
  Connection_Container(const Connection_Container&);
  Connection_Container& operator=(const Connection_Container&);
  
  std::vector< Connection* > connections;
  uint32 last_polled_pos;
};


class Client_Container
{
public:
  Client_Container() {}
  ~Client_Container();
  
  Client* get_client(Global_Status& global, uint32 client_token);
  
private:
  Client_Container(const Client_Container&);
  Client_Container& operator=(const Client_Container&);
  
  std::vector< Client* > clients;
};


class Global_Status
{
public:
  Global_Status(uint32 available_time, uint64 available_space, uint32 rate_limit)
      : idx_lock(false), sum_maxtime(0), sum_maxspace(0),
      available_time_(available_time), available_space_(available_space), rate_limit_(rate_limit) {}
  
  void add_connection(uint socket, pid_t pid) { connections.insert(new Connection(socket, pid)); }
  Client* get_client(uint32 client_token) { return clients.get_client(*this, client_token); }
  uint32 probe(uint32 maxtime, uint64 maxspace);
  Connection* poll_round_robin(uint32& command, uint32& client_pid)
  { return connections.poll_round_robin(*this, command, client_pid); }
  uint32 rate_limit() const { return rate_limit_; }
  uint32 change_state(Connection_State::_ old_state, Connection_State::_ new_state, uint32 maxtime, uint64 maxspace);
  bool obtain_idx_lock();
  void release_idx_lock() { idx_lock = false; }
  
private:
  Client_Container clients;
  Connection_Container connections;
  bool idx_lock;
  uint32 sum_maxtime;
  uint64 sum_maxspace;
  uint32 available_time_;
  uint64 available_space_;
  uint32 rate_limit_;
};


#endif
