
#include "global_reading_state.h"


void Global_Reading_State::Statistics::measure(
    uint32_t num_shedded, uint64_t maxtime_used, uint64_t maxsize_used, time_t now)
{
  shedded_per_second[now % 60] += num_shedded;
  shedded_per_minute += num_shedded;
  
  sum_used_time += maxtime_used;
  sum_used_size += maxsize_used;
  ++num_average_samples;
}

void Global_Reading_State::Statistics::calc_data_per_second(time_t now)
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


uint64_t Global_Reading_State::Statistics::average_used_time() const
{
  uint64_t result = 0;
  for (auto i : average_used_time_)
    result += i;
  return result / average_used_time_.size();
}


uint64_t Global_Reading_State::Statistics::average_used_size() const
{
  uint64_t result = 0;
  for (auto i : average_used_size_)
    result += i;
  return result / average_used_size_.size();
}


void Global_Reading_State::request_read_and_idx(int fd, time_t now, Socket_To_Client& socket)
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


bool Global_Reading_State::poll_reading_requests(std::unordered_map< int, Socket_To_Client >& clients, time_t now)
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


void Global_Reading_State::grant_and_purge(std::unordered_map< int, Socket_To_Client >& clients, time_t now)
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


void Global_Reading_State::finish_request(const std::pair< const int, Request_State >& arg, time_t now)
{
  time_t cooldown_time = now + 90 +
      ((uint64_t)now - arg.second.start_time + 1)
      * std::max(10u, std::min(100u, statistics.shedded_per_minute)) / 3;
  //std::cout<<"DEBUG "<<(cooldown_time - now)<<'\n';
  global_state.maxtime_used -= arg.second.maxtime;
  global_state.maxsize_used -= arg.second.maxsize;
  client_register.set_finished(arg.second.t, arg.first, cooldown_time);
}


uint64_t Socket_To_Client::global_runtime = 0;
