
#include "client_register.h"
#include "request_queue.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>


static const uint32_t HANGUP = 0x300;

static const uint32_t REQUEST_READ_AND_IDX = 0x20106;
static const uint32_t READ_IDX_FINISHED = 0x20200;
static const uint32_t READ_FINISHED = 0x20300;

static const uint32_t PROTOCOL_INVALID = 0x1f100;
static const uint32_t RATE_LIMITED = 0x1f200;
static const uint32_t QUERY_REJECTED = 0x1f800;


struct Socket_To_Client
{
  uint32_t get_command()
  {
    if (commands_to_send.empty())
      return 0;
    commands_to_send.pop_back();
    return commands_to_send.back();
  }
  std::vector< uint32_t > get_arguments(uint32_t num) { return arguments; }
  void send(uint32_t arg)
  {
    if (arg == REQUEST_READ_AND_IDX)
      commands_to_send = { READ_IDX_FINISHED, 0, 0 };
    else if (arg == READ_IDX_FINISHED)
      commands_to_send = { READ_FINISHED, 0, 0, 0, 0 };
    last_answer = arg;
  }
  void send_and_close(uint32_t arg)
  { 
    last_answer = arg;
    is_open = false;
  }

  std::vector< uint32_t > arguments = { 16777216, 0, 180, 512*1024*1024, 0 };
  std::vector< uint32_t > commands_to_send = { REQUEST_READ_AND_IDX, 0 };
  uint32_t last_answer = 0;
  bool is_open = true;
};


struct Global_Reading_State
{
  Global_Reading_State(Resource_State global_state_)
    : global_state(global_state_), request_queue(client_register) {}
  
  void poll_reading_requests(std::unordered_map< int, Socket_To_Client >& clients, time_t now)
  {
    auto it = reading.begin();
    while (it != reading.end())
    {
      Socket_To_Client& socket = clients[it->first];

      uint32_t command = socket.get_command();
      if (command == READ_IDX_FINISHED)
      {
        socket.send(command);
        reading_idx.erase(it->first);
      }
      else if (command == READ_FINISHED)
      {
        finish_request(*it, now);
        socket.send_and_close(command);
        reading_idx.erase(it->first);
        it = reading.erase(it);
      }
      else if (command == HANGUP)
      {
        finish_request(*it, now);
        reading_idx.erase(it->first);
        it = reading.erase(it);
      }
      else
        ++it;
    }
  }

  void request_read_and_idx(int fd, time_t now, Socket_To_Client& socket)
  {
    std::vector< uint32_t > args = socket.get_arguments(5);
    if (args.size() < 5)
      socket.send_and_close(PROTOCOL_INVALID);

    Request_State request_state{
        ((uint64_t)args[0] | ((uint64_t)args[1]<<32)), args[2], ((uint64_t)args[3] | ((uint64_t)args[4]<<32)) };
    if (request_queue.accept(fd, request_state, global_state.rate_limit, now))
      reading.insert({ fd, request_state });
    else
      socket.send_and_close(RATE_LIMITED);
  }

  void grant_and_purge(std::unordered_map< int, Socket_To_Client >& clients, time_t now)
  {
    std::vector< int > granted = request_queue.grant_idx_and_read(global_state, now);
    for (int fd : granted)
    {
      reading_idx.insert(fd);
      reading[fd].start_time = now;
      clients[fd].send(REQUEST_READ_AND_IDX);
    }
    
    std::pair< std::vector< int >, std::vector< int > > purged = request_queue.purge(global_state, now);
    for (int fd : purged.first)
    {
      reading.erase(fd);
      clients[fd].send_and_close(QUERY_REJECTED);
    }
    for (int fd : purged.second)
    {
      reading.erase(fd);
      clients[fd].send_and_close(RATE_LIMITED);
    }
  }

private:
  Resource_State global_state;
  std::unordered_map< int, Request_State > reading;
  std::unordered_set< int > reading_idx;
  Client_Register client_register;
  Request_Queue request_queue;

  void finish_request(const std::pair< const int, Request_State >& arg, time_t now)
  {
    time_t cooldown_time = now +
        ((uint64_t)now - arg.second.start_time) * global_state.maxsize_used / global_state.maxsize_limit;
    global_state.maxtime_used -= arg.second.maxtime;
    global_state.maxsize_used -= arg.second.maxsize;
    client_register.set_finished(arg.second.t, arg.first, cooldown_time);
  }
};


#include <iostream>

/*void millisleep(uint32_t milliseconds)
{
  struct timeval timeout_;
  timeout_.tv_sec = milliseconds/1000;
  timeout_.tv_usec = milliseconds*1000;
  select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
}*/


int dispense_fd(int& next_fd, std::vector< int >& available_fd)
{
  int fd = next_fd;
  if (available_fd.empty())
    ++next_fd;
  else
  {
    fd = available_fd.back();
    available_fd.pop_back();
  }
  return fd;
}


int main(int argc, char* args[])
{
  std::unordered_map< int, Socket_To_Client > clients;
  std::vector< int > available_fd;
  int next_fd = 3;
  uint32_t client_token = 16777216;
  uint32_t http_429 = 0;
  uint32_t http_504 = 0;
  uint32_t http_200 = 0;
  Global_Reading_State global_reading_state({ 15, 4, 3*86400, (uint64_t)4*1024*1024*1024 });

  for (time_t tsec = 1000000; tsec < 1086400; ++tsec)
  {
    global_reading_state.poll_reading_requests(clients, tsec);
    
    {
      int fd = dispense_fd(next_fd, available_fd);
      Socket_To_Client& socket = clients[fd];
      socket.arguments[0] = ++client_token;
      global_reading_state.request_read_and_idx(fd, tsec, clients[fd]);
    }

    global_reading_state.grant_and_purge(clients, tsec);
    
    auto it = clients.begin();
    while (it != clients.end())
    {
      if (it->second.is_open)
        ++it;
      else
      {
        if (!it->second.last_answer)
          std::cout<<"Answer lost for fd "<<it->first<<'\n';
        else if (it->second.last_answer == RATE_LIMITED)
          ++http_429;
        else if (it->second.last_answer == QUERY_REJECTED)
          ++http_504;
        else if (it->second.last_answer == READ_FINISHED)
          ++http_200;
        available_fd.push_back(it->first);
        it = clients.erase(it);
      }
    }
  }
  std::cout<<"Rate limited: "<<http_429<<'\n';
  std::cout<<"Shedded: "<<http_504<<'\n';
  std::cout<<"Read finished: "<<http_200<<'\n';

  return 0;
}
