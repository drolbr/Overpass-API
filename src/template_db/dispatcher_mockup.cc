
#include "global_reading_state.h"

#include <cstdint>
#include <iostream>
#include <unordered_map>

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
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" rate_limit\n";
    return 0;
  }
  
  std::unordered_map< int, Socket_To_Client > clients;
  std::vector< int > available_fd;
  int next_fd = 3;
  uint32_t client_token = 64u*16777216u;
  uint32_t client_token_large = 48u*16777216u;
  std::vector< uint32_t > http_429(16, 0);
  std::vector< uint32_t > http_504(16, 0);
  std::vector< uint32_t > http_200(16, 0);
  Global_Reading_State global_reading_state({ 10, atoi(args[1]), 3*86400, (uint64_t)16*1024*1024*1024 });

  for (time_t tsec = 1080000; tsec < 1166400; ++tsec)
  {
    if (tsec % 3600 == 0)
      std::cout<<"Nominal time: "<<tsec/3600<<
          ", Avg_Time "<<global_reading_state.get_statistics().average_used_time()<<
          ", Avg_Size "<<global_reading_state.get_statistics().average_used_size()<<
          ", Total_Runtime "<<Socket_To_Client::global_runtime<<'\n';
    
    for (uint32_t j = 0; j < 100; ++j)
    {
      global_reading_state.poll_reading_requests(clients, tsec);

      if (j % 5 == 3) // One-time only clients
      {
        int fd = dispense_fd(next_fd, available_fd);
        Socket_To_Client& socket = clients[fd];
        socket.arguments[0] = ++client_token;
        socket.read_runtime = (client_token % 3) + 10*std::max(5*client_token % 61, (uint32_t)40) - 397;
        global_reading_state.request_read_and_idx(fd, tsec, socket);
      }
      
      if (j % 10 == 3) // One-time only clients with large requests
      {
        int fd = dispense_fd(next_fd, available_fd);
        Socket_To_Client& socket = clients[fd];
        socket.arguments[0] = ++client_token_large;
        socket.arguments[3] = 1024*1024*1024;
        socket.read_runtime = (client_token % 3) + 10*std::max(5*client_token % 61, (uint32_t)40) - 397;
        global_reading_state.request_read_and_idx(fd, tsec, socket);
      }
      
      if (j % 5 == 2 && j + 1080000 < tsec) // Once per minute clients
      {
        int fd = dispense_fd(next_fd, available_fd);
        Socket_To_Client& socket = clients[fd];
        socket.arguments[0] = 144u*16777216u + j/5 + 100*(tsec%60);
        socket.read_runtime = 140 + (2*j)%13;
        global_reading_state.request_read_and_idx(fd, tsec, socket);
      }
      
      if (j % 5 == 2) // Once every five seconds clients
      {
        /*const auto* state = global_reading_state.get_client_state(160u*16777216u + j/5 + 100*(tsec%5), tsec);
        if (state)
        {
          std::cout<<"DEBUG Client_State: enqueued "<<state->enqueued<<", reading";
          for (auto fd : state->reading)
            std::cout<<' '<<fd;
          std::cout<<", cooldown";
          for (auto fd : state->cooldown)
            std::cout<<' '<<fd;
          std::cout<<'\n';
        }*/
        
        int fd = dispense_fd(next_fd, available_fd);
        Socket_To_Client& socket = clients[fd];
        socket.arguments[0] = 160u*16777216u + j/5 + 100*(tsec%5);
        socket.read_runtime = 160 + (3*j)%17;
        global_reading_state.request_read_and_idx(fd, tsec, socket);
      }

      if (j % 5 == 2) // Once per second clients
      {
        int fd = dispense_fd(next_fd, available_fd);
        Socket_To_Client& socket = clients[fd];
        socket.arguments[0] = 176u*16777216u + j/5;
        socket.read_runtime = 200 + (7*j)%11;
        global_reading_state.request_read_and_idx(fd, tsec, socket);
      }
      
      if (j % 100 == 41 && tsec % 30 == 11) // Long runtime heavy load client
      {
        int fd = dispense_fd(next_fd, available_fd);
        Socket_To_Client& socket = clients[fd];
        socket.arguments[0] = 192u*16777216u + j/5;
        socket.read_runtime = 1500 + 500*(tsec % 60 / 30);
        global_reading_state.request_read_and_idx(fd, tsec, socket);
      }
      
      if (tsec % 3600 == 471) // Binge client
      {
        for (uint32_t k = 0; k < 5; ++k)
        {
          int fd = dispense_fd(next_fd, available_fd);
          Socket_To_Client& socket = clients[fd];
          socket.arguments[0] = 224u*16777216u;
          socket.read_runtime = 500 + k;
          global_reading_state.request_read_and_idx(fd, tsec, socket);
        }
      }

      global_reading_state.grant_and_purge(clients, tsec);
    }

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
          ++http_429[it->second.arguments[0]>>28];
        else if (it->second.last_answer == QUERY_REJECTED)
          ++http_504[it->second.arguments[0]>>28];
        else if (it->second.last_answer == READ_FINISHED)
          ++http_200[it->second.arguments[0]>>28];
        available_fd.push_back(it->first);
        it = clients.erase(it);
      }
    }
  }

  {
    uint32_t sum = 0;
    std::cout<<" Rate limited:";
    for (auto i : http_429)
    {
      sum += i;
      std::cout<<'\t'<<i;
    }
    std::cout<<'\t'<<sum<<'\n';
  }
  {
    uint32_t sum = 0;
    std::cout<<"      Shedded:";
    for (auto i : http_504)
    {
      sum += i;
      std::cout<<'\t'<<i;
    }
    std::cout<<'\t'<<sum<<'\n';
  }
  {
    uint32_t sum = 0;
    std::cout<<"Read finished:";
    for (auto i : http_200)
    {
      sum += i;
      std::cout<<'\t'<<i;
    }
    std::cout<<'\t'<<sum<<'\n';
  }

  return 0;
}
