
#include "client_register.h"

#include <ctime>


struct Resource_State
{
  uint32_t acceptable_wait_time = 15;
  uint32_t rate_limit = 0;
  uint32_t maxtime_limit = 0;
  uint64_t maxsize_limit = 0;

  uint32_t maxtime_used = 0;
  uint64_t maxsize_used = 0;
};


struct Request_State
{
  Client_Token t;
  uint32_t maxtime = 0;
  uint64_t maxsize = 0;
  time_t start_time = 0;  
};


struct Request_Queue
{
  Request_Queue(Client_Register& client_register_)
      : client_register(client_register_) {}
  
  // Assert: sum(queue[..].size()) == sum(client_register.enqueued)
  // Assert: if ret true set{ req.fd } + set{ queue[i][j].fd } is const
  // Assert: if ret false then set{ queue[i][j].fd } is const
  bool accept(int fd, const Request_State& req, uint32_t rate_limit, time_t now);

  // Assert: sum(queue[..].size()) == sum(client_register.enqueued)
  // Assert: set{ result } + set{ queue[i][j].fd } is const
  std::vector< int > grant_idx_and_read(Resource_State& res, time_t now);
  
  // Assert: sum(queue[..].size()) == sum(client_register.enqueued)
  // Assert: set{ result } + set{ queue[i][j].fd } is const
  std::pair< std::vector< int >, std::vector< int > > purge(const Resource_State& res, time_t now);

private:
  struct Queue_Entry
  {
    Client_Token t;
    int fd;
    uint32_t maxtime;
    uint64_t maxsize;
    time_t start_time;
  };
  
  std::vector< std::vector< std::vector< Queue_Entry > > > queue;
  Client_Register& client_register;
};
