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

#include "dispatcher.h"
#include "file_tools.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>


std::string getcwd()
{
  int size = 256;
  char* buf;
  while (true)
  {
    buf = (char*)malloc(size);
    errno = 0;
    buf = getcwd(buf, size);
    if (errno != ERANGE)
      break;
    
    free(buf);
    size *= 2;
  }
  if (errno != 0)
  {
    free(buf);
    throw File_Error(errno, "wd", "Dispatcher::getcwd");
  }
  std::string result(buf);
  free(buf);
  if ((result != "") && (result[result.size()-1] != '/'))
    result += '/';
  return result;
}


Blocking_Client_Socket::Blocking_Client_Socket
  (int socket_descriptor_) : socket_descriptor(socket_descriptor_), state(waiting) {}


uint32 Blocking_Client_Socket::get_command()
{
  if (state == disconnected)
    return Dispatcher::HANGUP;
  else if (state == processing_command)
    return last_command;
  
  int bytes_read = recv(socket_descriptor, &last_command, sizeof(uint32), 0);
  if (bytes_read == -1)
    return 0;
  else if (bytes_read == 0)
  {
    state = disconnected;
    return Dispatcher::HANGUP;
  }
  else
  {
    state = processing_command;
    return last_command;
  }
}


std::vector< uint32 > Blocking_Client_Socket::get_arguments(int num_arguments)
{
  std::vector< uint32 > result;
  if (state == disconnected || state == waiting)
    return result;
  
  for (int i = 0; i < num_arguments; ++i)
  {
    // Wait for each argument up to 0.1 seconds
    result.push_back(0);
    int bytes_read = recv(socket_descriptor, &result.back(), sizeof(uint32), 0);
    uint counter = 0;
    while (bytes_read == -1 && counter <= 100)
    {
      bytes_read = recv(socket_descriptor, &result.back(), sizeof(uint32), 0);
      millisleep(1);
      ++counter;
    }
    if (bytes_read == 0)
    {
      state = disconnected;
      result.clear();
      break;
    }
    if (bytes_read == -1)
      break;
  }
  return result;
}


void Blocking_Client_Socket::clear_state()
{
  if (state == disconnected || state == waiting)
    return;
  
  // remove any pending data. The connection should be clear at the end of the command.
  uint32 dummy;
  int bytes_read = recv(socket_descriptor, &dummy, sizeof(uint32), 0);
  while (bytes_read > 0)
    bytes_read = recv(socket_descriptor, &dummy, sizeof(uint32), 0);
  
  if (bytes_read == 0)
  {
    state = disconnected;
    return;
  }
  state = waiting;
}


void Blocking_Client_Socket::send_result(uint32 result)
{
  if (state == disconnected || state == waiting)
    return;
  
  clear_state();

  if (state == disconnected)
    return;
  
  send(socket_descriptor, &result, sizeof(uint32), 0);
  state = waiting;
}


Blocking_Client_Socket::~Blocking_Client_Socket()
{
  close(socket_descriptor);
}

  
Blocking_Client_Socket* Connection_Per_Pid_Map::get(pid_t pid)
{
  std::map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.find(pid);    
  if (it != connection_per_pid.end())
    return it->second;
  else
    return 0;
}

  
void Connection_Per_Pid_Map::set(pid_t pid, Blocking_Client_Socket* socket)
{
  std::map< pid_t, Blocking_Client_Socket* >::iterator it = connection_per_pid.find(pid);
  if (it != connection_per_pid.end())
    delete it->second;
  if (socket != 0)
    connection_per_pid[pid] = socket;
  else
    connection_per_pid.erase(pid);
}

  
void Connection_Per_Pid_Map::poll_command_round_robin(uint32& command, uint32& client_pid)
{
  // poll all open connections round robin
  for (std::map< pid_t, Blocking_Client_Socket* >::const_iterator
      it = connection_per_pid.upper_bound(last_pid);
      it != connection_per_pid.end(); ++it)
  {
    command = it->second->get_command();
    if (command != 0)
    {
      client_pid = it->first;
      break;
    }
  }
  if (command == 0)
  {
    for (std::map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.begin();
        it != connection_per_pid.upper_bound(last_pid); ++it)
    {
      command = it->second->get_command();
      if (command != 0)
      {
	client_pid = it->first;
	break;
      }
    }
  }
  if (command != 0)
    last_pid = client_pid;
}
