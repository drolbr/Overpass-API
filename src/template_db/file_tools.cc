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

#include <cstdint>
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


Blocking_Client_Socket::Blocking_Client_Socket(int socket_descriptor_)
    : socket_descriptor(socket_descriptor_), state(waiting), buffer(64), bytes_in_buffer(0), bytes_expected(0),
      counter(0) {}


uint32 Blocking_Client_Socket::get_command()
{
  if (state == disconnected)
    return Dispatcher::HANGUP;
  else if (state == processing_command && bytes_expected <= bytes_in_buffer)
    return buffer[0];

  int bytes_read = recv(
      socket_descriptor, ((uint8_t*)&buffer[0])+bytes_in_buffer, buffer.size()*sizeof(uint32) - bytes_in_buffer, 0);
//   std::cerr<<"get_command "<<bytes_in_buffer<<' '<<bytes_read<<' '<<std::hex<<buffer[0]<<' '<<std::dec<<counter<<'\n';
  if (bytes_read == -1)
  {
    if (state == processing_command && --counter <= 0)
      return buffer[0];
    return 0;
  }
  else if (bytes_read == 0)
  {
    state = disconnected;
    return Dispatcher::HANGUP;
  }

  bytes_expected = (buffer[0] & 0xff)*4 + 4;
  counter = 100;
  
  bytes_in_buffer += bytes_read;
  state = processing_command;
  if (bytes_expected <= bytes_in_buffer || --counter <= 0)
    return buffer[0];
  return 0;
}


std::vector< uint32 > Blocking_Client_Socket::get_arguments(int num_arguments)
{
//   std::cerr<<"get_arguments "<<bytes_in_buffer<<' '<<bytes_expected<<' '<<buffer[0]<<'\n';
  if (state == disconnected || state == waiting || bytes_in_buffer < bytes_expected
      || bytes_expected != 4*num_arguments + 4)
    return {};

  return { buffer.begin()+1, buffer.begin()+(num_arguments+1) };
}


void Blocking_Client_Socket::clear_state()
{
  if (state == disconnected || state == waiting)
    return;

  // remove any pending data. The connection should be clear at the end of the command.
  int bytes_read = recv(socket_descriptor, &buffer[0], buffer.size()*sizeof(uint32), 0);
//   std::cerr<<"clear_state A "<<bytes_in_buffer<<' '<<bytes_read<<' '<<buffer[0]<<'\n';
  while (bytes_read > 0)
    bytes_read = recv(socket_descriptor, &buffer[0], buffer.size()*sizeof(uint32), 0);
//   std::cerr<<"clear_state B "<<bytes_in_buffer<<' '<<bytes_read<<' '<<buffer[0]<<'\n';

  if (bytes_read == 0)
  {
    state = disconnected;
    return;
  }

  state = waiting;
  bytes_in_buffer = 0;
  bytes_expected = 0;
}


void Blocking_Client_Socket::send_data(uint32 result)
{
  send(socket_descriptor, &result, sizeof(uint32), 0);
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


Connection_Per_Pid_Map::~Connection_Per_Pid_Map()
{
  for (auto i : data)
    delete i.second;
}


Blocking_Client_Socket* Connection_Per_Pid_Map::get(pid_t pid)
{
  std::map< pid_t, Blocking_Client_Socket* >::const_iterator it = data.find(pid);
  if (it != data.end())
    return it->second;
  else
    return 0;
}


void Connection_Per_Pid_Map::insert(pid_t pid, int socket_fd)
{
  std::map< pid_t, Blocking_Client_Socket* >::iterator it = data.find(pid);
  if (it != data.end())
    delete it->second;
  data[pid] = new Blocking_Client_Socket(socket_fd);
}


void Connection_Per_Pid_Map::erase(pid_t pid)
{
  std::map< pid_t, Blocking_Client_Socket* >::iterator it = data.find(pid);
  if (it != data.end())
    delete it->second;
  data.erase(it);
}


void Connection_Per_Pid_Map::poll_command_round_robin(uint32& command, uint32& client_pid)
{
  // poll all open connections round robin
  for (auto it = data.upper_bound(last_pid); it != data.end(); ++it)
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
    for (auto it = data.begin(); it != data.upper_bound(last_pid); ++it)
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
