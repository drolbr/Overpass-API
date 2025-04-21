
#include "client_register.h"
#include "request_queue.h"


bool Request_Queue::accept(int fd, const Request_State& req, uint32_t rate_limit, time_t now)
{
  uint32_t prio = client_register.try_enqueue(req.t, rate_limit, now);
  if (prio)
  {
    if (queue.size() <= prio-1)
      queue.resize(prio);
    if (queue[prio-1].size() <= req.maxsize / (256*1024*1024))
      queue[prio-1].resize(req.maxsize / (256*1024*1024) + 1);
    queue[prio-1][req.maxsize / (256*1024*1024)].push_back(
        { req.t, fd, req.maxtime, req.maxsize, now });
    return true;
  }
  return false;
}


std::vector< int > Request_Queue::grant_idx_and_read(Resource_State& res, time_t now)
{
  std::vector< int > result;
  if (res.maxsize_used >= res.maxsize_limit || res.maxtime_used >= res.maxtime_limit)
    return result;

  for (decltype(queue.size()) i = 0; i < queue.size(); ++i)
  {
    for (decltype(queue[i].size()) j = 0; j < queue[i].size(); ++j)
    {
      if (j*(2*256*1024*1024) > (res.maxsize_limit - res.maxsize_used))
        break;

      for (auto it = queue[i][j].begin(); it != queue[i][j].end(); )
      {
        if (2*it->maxsize <= res.maxsize_limit - res.maxsize_used
            && 2*it->maxtime <= res.maxtime_limit - res.maxtime_used
            && (res.rate_limit == 0 || client_register.num_active(it->t, now) <= res.rate_limit))
        {
          res.maxsize_used += it->maxsize;
          res.maxtime_used += it->maxtime;
          client_register.set_reading(it->t, it->fd);
          result.push_back(it->fd);
          it = queue[i][j].erase(it);
        }
        else
          ++it;

        if (j*(2*256*1024*1024) > (res.maxsize_limit - res.maxsize_used))
          break;
      }
    }
  }

  return result;
}


std::pair< std::vector< int >, std::vector< int > > Request_Queue::purge(const Resource_State& res, time_t now)
{
  std::pair< std::vector< int >, std::vector< int > > result;

  if (now <= last_purged)
    return result;
  last_purged = now;

  uint32_t rate_limit = (res.rate_limit > 0 ? res.rate_limit : queue.size());
  for (decltype(queue.size()) i = 0; i < queue.size() && i < rate_limit; ++i)
  {
    for (decltype(queue[i].size()) j = 0; j < queue[i].size(); ++j)
    {
      auto it = queue[i][j].begin();
      while (it != queue[i][j].end() && it->start_time + res.acceptable_wait_time < now)
      {
        result.first.push_back(it->fd);
        client_register.set_aborted(it->t, it->fd);
        ++it;
      }
      if (it != queue[i][j].begin())
        queue[i][j].erase(queue[i][j].begin(), it);
    }
  }
  for (decltype(queue.size()) i = rate_limit; i < queue.size(); ++i)
  {
    for (decltype(queue[i].size()) j = 0; j < queue[i].size(); ++j)
    {
      auto it = queue[i][j].begin();
      while (it != queue[i][j].end() && it->start_time + res.acceptable_wait_time < now)
      {
        result.second.push_back(it->fd);
        client_register.set_aborted(it->t, it->fd);
        ++it;
      }
      if (it != queue[i][j].begin())
        queue[i][j].erase(queue[i][j].begin(), it);
    }
  }

  client_register.purge(now);

  return result;
}


#include <iostream>

/*int main(int argc, char* args[])
{
  {
    std::cout<<"\nIsolated test of regular lifecycle:\n";

    Client_Register client_register;
    Request_Queue request_queue(client_register);
    Resource_State global_state = { 15, 2, 86400, 4ull*1024*1024*1024 };

    std::cout<<"Request_Queue::acept(): "
      <<request_queue.accept({ 1677216, 3, 180, 512*1024 }, global_state.rate_limit, 1000000)<<'\n';
    std::cout<<"Request_Queue::grant_idx_and_read():";
    auto granted = request_queue.grant_idx_and_read(global_state, 1000000);
    for (auto i : granted)
      std::cout<<' '<<i;
    std::cout<<'\n';

    request_queue.purge(global_state, 1000000);
  }

  {
    std::cout<<"\nToo simplistic load test:\n";

    Client_Register client_register;
    Request_Queue request_queue(client_register);
    Resource_State global_state = { 15, 2, 86400, 4ull*1024*1024*1024 };

    for (time_t i = 1000000; i < 2000000; ++i)
    {
      bool accepted = request_queue.accept(
          { 16777216 + i % 1024, i % 1024 + 4, 180, 512*1024*1024 }, global_state.rate_limit, i);
      if (!accepted)
      {
        std::cout<<"Rejected: "<<i<<' '<<global_state.maxsize_used<<' '<<global_state.maxtime_used<<'\n';
        break;
      }

      auto granted = request_queue.grant_idx_and_read(global_state, 1000000);
      if (granted.empty())
      {
        std::cout<<"No requests granted.\n";
        break;
      }

      request_queue.purge(global_state, 1000000);

      if (i % 4 == 3)
      {
        for (int j = i-3; j <= i; ++j)
        {
          auto client_state = client_register.get_client_state(16777216 + j % 1024, i);
          for (auto k : client_state.reading)
            client_register.set_finished(16777216 + j % 1024, k, i + 15);
        }
        global_state.maxsize_used = 0;
        global_state.maxtime_used = 0;
      }
    }
  }

  return 0;
}
*/
