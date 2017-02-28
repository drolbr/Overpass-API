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
#include "quota.h"


uint32 Client::probe(uint32 maxtime, uint64 maxspace, uint32 first_seen)
{
  if (global->rate_limit() > 0 && num_pending >= 2*global->rate_limit())
    return Dispatcher::RATE_LIMITED;
  
  uint current_time = time(0);
  for (std::vector< uint32 >::iterator it = quota_times.begin(); it != quota_times.end(); )
  {
    if (*it <= current_time)
    {
      *it = quota_times.back();
      quota_times.pop_back();
    }
    else
      ++it;
  }
  
  if (global->rate_limit() > 0 && num_reading_idx + num_running + quota_times.size() >= global->rate_limit())
  {
    if (current_time - first_seen < 15)
      return 0;
    else
    {
      if (num_pending > 0)
        --num_pending;
      return Dispatcher::RATE_LIMITED;
    }
  }
  
  uint32 result = global->probe(maxtime, maxspace);
  
  if (result == Dispatcher::REQUEST_READ_AND_IDX)
  {
    sum_maxtime += maxtime;
    sum_maxspace += maxspace;
    if (num_pending > 0)
      --num_pending;
    ++num_reading_idx;
  }
  else if (result == 0)
  {
    if (current_time - first_seen < 15)
      return 0;
    else
    {
      if (num_pending > 0)
        --num_pending;
      return Dispatcher::RATE_LIMITED;
    }
  }
  
  return result;
}


void Client::change_state(Connection_State::_ old_state, Connection_State::_ new_state, uint32 maxtime, uint64 maxspace)
{
  if ((old_state == Connection_State::reading_idx || old_state == Connection_State::running)
    && (new_state == Connection_State::finished || new_state == Connection_State::zombie))
  {
    sum_maxtime -= maxtime;
    sum_maxspace -= maxspace;
  }
  
  if (new_state == Connection_State::pending || new_state == Connection_State::reading_idx)
    ; // TODO: log as error
  else if (new_state == Connection_State::running)
    ++num_running;
  
  if (old_state == Connection_State::pending)
    --num_pending;
  else if (old_state == Connection_State::reading_idx)
    --num_reading_idx;
  else if (old_state == Connection_State::running)
    --num_running;
  
  uint32 quota_time = global->change_state(old_state, new_state, maxtime, maxspace);
  if (quota_time)
    quota_times.push_back(quota_time);
}


uint32 Connection::poll(Global_Status& global)
{
  if (!socket)
    return Dispatcher::HANGUP;
  
  uint32 command = socket->get_command();
  if (command == 0)
    return 0;
  
  if (command == Dispatcher::HANGUP)
  {    
    if (client)
      client->change_state(state_, Connection_State::zombie, maxtime, maxspace);
    
    state_ = Connection_State::zombie;
    delete socket;
    socket = 0;
    client = 0;
  }
  else if (command == Dispatcher::REQUEST_READ_AND_IDX)
  {
    std::vector< uint32 > arguments = socket->get_arguments(4);
    if (arguments.size() < 4)
    {
      socket->send_result(0);
      return 0;
    }
    maxtime = arguments[0];
    maxspace = (((uint64)arguments[2])<<32 | arguments[1]);
    if (!client)
      client = global.get_client(arguments[3]);

    uint32 result = client ? client->probe(maxtime, maxspace, first_seen) : 0;
    
    socket->send_result(result);
    
    if (result == Dispatcher::REQUEST_READ_AND_IDX)
      state_ = Connection_State::reading_idx;
    else if (result == Dispatcher::RATE_LIMITED || result == Dispatcher::QUERY_REJECTED)
    {
      state_ = Connection_State::zombie;
      delete socket;
      socket = 0;
      client = 0;
      
      return result;
    }
  }
  else if (command == Dispatcher::READ_IDX_FINISHED)
  {
    if (client)
      client->change_state(state_, Connection_State::running, maxtime, maxspace);

    socket->send_result(command);
    
    state_ = Connection_State::running;
  }
  else if (command == Dispatcher::READ_FINISHED)
  {
    if (client)
      client->change_state(state_, Connection_State::finished, maxtime, maxspace);
    
    socket->send_result(command);
    
    state_ = Connection_State::finished;
    delete socket;
    socket = 0;
    client = 0;
  }
  
  return command;
}


Connection_Container::~Connection_Container()
{
  for (std::vector< Connection* >::iterator it = connections.begin(); it != connections.end(); ++it)
    delete *it;
}


Connection* Connection_Container::poll_round_robin(Global_Status& global, uint32& command, uint32& client_pid)
{
  uint32 i = last_polled_pos;
  command = 0;
  while (i < connections.size() && !command)
  {
    command = connections[i]->poll(global);
    ++i;
  }
  
  if (!command)
  {
    i = 0;
    while (i < last_polled_pos && !command)
    {
      command = connections[i]->poll(global);
      ++i;
    }
  }
  
  last_polled_pos = i;
  
  if (command)
  {    
    --i;
    client_pid = connections[i]->pid_();
    
    if (!connections[i]->is_active())
    {
      delete connections[i];
      connections[i] = connections.back();
      connections.pop_back();
      
      last_polled_pos = i;
      return 0;
    }    
    return connections[i];
  }
  return 0;
}


bool Connection_Container::any_reading_idx() const
{
  for (std::vector< Connection* >::const_iterator it = connections.begin(); it != connections.end(); ++it)
  {
    if ((*it)->state() == Connection_State::reading_idx)
      return true;
  }
  return false;
}


Client_Container::~Client_Container()
{
  for (std::vector< Client* >::iterator it = clients.begin(); it != clients.end(); ++it)
    delete *it;
}


Client* Client_Container::get_client(Global_Status& global, uint32 client_token)
{
  Client* result = 0;
  for (std::vector< Client* >::iterator it = clients.begin(); it != clients.end(); ++it)
  {
    if ((*it)->client_token() == client_token)
    {
      result = *it;
      break;
    }
  }
  
  if (!result)
  {
    for (std::vector< Client* >::iterator it = clients.begin(); it != clients.end(); )
    {
      if ((*it)->is_idle())
      {
        delete *it;
        *it = clients.back();
        clients.pop_back();
      }
      else
        ++it;
    }
    
    clients.push_back(new Client(global, client_token));
    result = clients.back();
  }
  
  result->add_pending();
  return result;
}


uint32 Global_Status::probe(uint32 maxtime, uint64 maxspace)
{
  if (idx_lock)
    return 0;
  
  if (available_time_ < sum_maxtime ||
      maxtime > (available_time_ - sum_maxtime)/2 ||
      available_space_ < sum_maxspace ||
      maxspace > (available_space_ - sum_maxspace)/2)
    return 0;
  
  sum_maxtime += maxtime;
  sum_maxspace += maxspace;
  
  return Dispatcher::REQUEST_READ_AND_IDX;
}


uint32 Global_Status::change_state(Connection_State::_ old_state, Connection_State::_ new_state,
    uint32 maxtime, uint64 maxspace)
{
  if ((old_state == Connection_State::reading_idx || old_state == Connection_State::running)
    && (new_state == Connection_State::finished || new_state == Connection_State::zombie))
  {
    sum_maxtime -= maxtime;
    sum_maxspace -= maxspace;
  }
  
  return 0; //TODO: quota time
}


bool Global_Status::obtain_idx_lock()
{
  idx_lock = true;
  return !connections.any_reading_idx();
}
