
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
    if (commands_to_send.back() == READ_FINISHED && read_runtime > 0)
    {
      --read_runtime;
      return 0;
    }
    uint32_t result = commands_to_send.back();
    commands_to_send.pop_back();
    return result;
  }
  std::vector< uint32_t > get_arguments(uint32_t num) { return arguments; }
  void send(uint32_t arg)
  {
    if (arg == REQUEST_READ_AND_IDX)
    {
      commands_to_send = { READ_IDX_FINISHED, 0 };
      global_runtime += read_runtime;
    }
    else if (arg == READ_IDX_FINISHED)
      commands_to_send = { READ_FINISHED };
    last_answer = arg;
  }
  void send_and_close(uint32_t arg)
  {
    last_answer = arg;
    is_open = false;
  }

  std::vector< uint32_t > arguments = { 16777216, 0, 180, 512*1024*1024, 0 };
  std::vector< uint32_t > commands_to_send = { REQUEST_READ_AND_IDX };
  uint32_t read_runtime = 3;
  uint32_t last_answer = 0;
  bool is_open = true;
  static uint64_t global_runtime;
};


// started_connections: accept, then wait for pid, one per round

struct Global_Reading_State
{
  struct Statistics
  {
    Statistics() : shedded_per_second(60, 0), average_used_time_(15, 0), average_used_size_(15, 0) {}
    
    // Assert: shedded_per_minute is always the sum of all shedded_per_second
    time_t last_updated = 0;
    std::vector< uint32_t > shedded_per_second;
    uint32_t shedded_per_minute = 0;
    
    uint64_t sum_used_time = 0;
    uint64_t sum_used_size = 0;
    uint64_t num_average_samples = 0;
    std::vector< uint64_t > average_used_time_;
    std::vector< uint64_t > average_used_size_;
    
    void measure(uint32_t num_shedded, uint64_t maxtime_used, uint64_t maxsize_used, time_t now)
    {
      shedded_per_second[now % 60] += num_shedded;
      shedded_per_minute += num_shedded;
      
      sum_used_time += maxtime_used;
      sum_used_size += maxsize_used;
      ++num_average_samples;
    }
    
    void calc_data_per_second(time_t now)
    {
      if (last_updated == 0)
        last_updated = now;
      else if (last_updated < now)
      {
        while (last_updated < now)
        {
          ++last_updated;

          shedded_per_minute -= shedded_per_second[last_updated % 60];
          shedded_per_second[last_updated % 60] = 0;

          average_used_time_[last_updated % 15] = 0;
          average_used_size_[last_updated % 15] = 0;
        }
        
        if (num_average_samples > 0)
        {
          average_used_time_[now % 15] = sum_used_time/num_average_samples;
          average_used_size_[now % 15] = sum_used_size/num_average_samples;
          sum_used_size = 0;
          sum_used_time = 0;
          num_average_samples = 0;
        }
        else
        {
          average_used_time_[now % 15] = 0;
          average_used_size_[now % 15] = 0;
        }
      }
    }
    
    uint64_t average_used_time() const
    {
      uint64_t result = 0;
      for (auto i : average_used_time_)
        result += i;
      return result / average_used_time_.size();
    }
    
    uint64_t average_used_size() const
    {
      uint64_t result = 0;
      for (auto i : average_used_size_)
        result += i;
      return result / average_used_size_.size();
    }
  };

  
  Global_Reading_State(Resource_State global_state_)
    : global_state(global_state_), request_queue(client_register) {}

  bool poll_reading_requests(std::unordered_map< int, Socket_To_Client >& clients, time_t now)
  {
    bool some_state_changed = false;
    
    auto it = reading.begin();
    while (it != reading.end())
    {
      Socket_To_Client& socket = clients[it->first];

      uint32_t command = socket.get_command();
      if (command == READ_IDX_FINISHED)
      {
        socket.send(command);
        reading_idx.erase(it->first);
        some_state_changed = true;
      }
      else if (command == READ_FINISHED)
      {
        finish_request(*it, now);
        socket.send_and_close(command);
        reading_idx.erase(it->first);
        it = reading.erase(it);
        some_state_changed = true;
        continue;
      }
      else if (command == HANGUP)
      {
        finish_request(*it, now);
        reading_idx.erase(it->first);
        it = reading.erase(it);
        some_state_changed = true;
        continue;
      }
      ++it;
    }
    
    return some_state_changed;
  }

  void request_read_and_idx(int fd, time_t now, Socket_To_Client& socket)
  {
    std::vector< uint32_t > args = socket.get_arguments(5);
    if (args.size() < 5)
      socket.send_and_close(PROTOCOL_INVALID);

    Request_State request_state{
        ((uint64_t)args[0] | ((uint64_t)args[1]<<32)), args[2], ((uint64_t)args[3] | ((uint64_t)args[4]<<32)) };
    if (request_queue.accept(fd, request_state, global_state.rate_limit, now))
      queued.insert({ fd, request_state });
    else
      socket.send_and_close(RATE_LIMITED);
  }

  void grant_and_purge(std::unordered_map< int, Socket_To_Client >& clients, time_t now)
  {
    std::vector< int > granted = request_queue.grant_idx_and_read(global_state, now);
    for (int fd : granted)
    {
      auto queued_it = queued.find(fd);
      if (queued_it == queued.end())
        continue;
      auto reading_it = reading.insert(*queued_it).first;
      queued.erase(fd);
      reading_it->second.start_time = now;

      reading_idx.insert(fd);
      clients[fd].send(REQUEST_READ_AND_IDX);
    }

    statistics.calc_data_per_second(now);

    std::pair< std::vector< int >, std::vector< int > > purged = request_queue.purge(global_state, now);
    for (int fd : purged.first)
    {
      queued.erase(fd);
      clients[fd].send_and_close(QUERY_REJECTED);
    }
    for (int fd : purged.second)
    {
      queued.erase(fd);
      clients[fd].send_and_close(RATE_LIMITED);
    }
    statistics.measure(purged.first.size(), global_state.maxtime_used, global_state.maxsize_used, now);    
  }

  const Client_State* get_client_state(Client_Token t, time_t now)
  { return client_register.get_client_state(t, now); }
  
  const Statistics& get_statistics() const { return statistics; }

private:
  Resource_State global_state;
  std::unordered_map< int, Request_State > queued;
  std::unordered_map< int, Request_State > reading;
  std::unordered_set< int > reading_idx;
  Client_Register client_register;
  Request_Queue request_queue;
  Statistics statistics;

  void finish_request(const std::pair< const int, Request_State >& arg, time_t now)
  {
    time_t cooldown_time = now + 90 +
        ((uint64_t)now - arg.second.start_time + 1)
        * std::max(10u, std::min(100u, statistics.shedded_per_minute)) / 3;
    //std::cout<<"DEBUG "<<(cooldown_time - now)<<'\n';
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


uint64_t Socket_To_Client::global_runtime = 0;

int main(int argc, char* args[])
{
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
