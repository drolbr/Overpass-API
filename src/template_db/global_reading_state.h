#ifndef DE__OSM3S___TEMPLATE_DB__GLOBAL_READING_STATE
#define DE__OSM3S___TEMPLATE_DB__GLOBAL_READING_STATE

#include "client_register.h"
#include "request_queue.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>


static const uint32_t TERMINATE = 0x100;
static const uint32_t HANGUP = 0x300;
static const uint32_t QUERY_BY_TOKEN = 0x1601;

static const uint32_t REQUEST_READ_AND_IDX = 0x20106;
static const uint32_t READ_IDX_FINISHED = 0x20200;
static const uint32_t READ_FINISHED = 0x20300;

static const uint32_t WRITE_START = 0x10100;
static const uint32_t WRITE_ROLLBACK = 0x10200;
static const uint32_t WRITE_COMMIT = 0x10300;
static const uint32_t MIGRATE_START = 0x11100;
static const uint32_t MIGRATE_ROLLBACK = 0x11200;
static const uint32_t MIGRATE_COMMIT = 0x11300;

static const uint32_t PROTOCOL_INVALID = 0x1f100;
static const uint32_t RATE_LIMITED = 0x1f200;
static const uint32_t QUERY_REJECTED = 0x1f800;


#include <fstream>
struct Socket_To_Client
{
  uint32_t get_command()
  {
    if (commands_to_send.empty())
      return 0;
    if (commands_to_send.back() == READ_FINISHED && req_runtime > 0)
    {
      --req_runtime;
      return 0;
    }
    else if (commands_to_send.back() == WRITE_COMMIT || commands_to_send.back() == MIGRATE_COMMIT)
    {
      if (!has_write_semaphore)
      {
        std::ifstream lock("shadow.lock");
        if (lock.is_open())
        {
          pid_t locked_pid = 0;
          lock>>locked_pid;
          if (locked_pid == client_pid)
            has_write_semaphore = true;
        }
        if (!has_write_semaphore)
          return commands_to_send.back() == WRITE_COMMIT ? WRITE_START : MIGRATE_START;
      }
      if (req_runtime > 0)
      {
        --req_runtime;
        return 0;
      }
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
      commands_to_send = { READ_IDX_FINISHED, 0, 0, 0 };
      global_runtime += req_runtime;
    }
    else if (arg == READ_IDX_FINISHED)
      commands_to_send = { READ_FINISHED };
    last_answer = arg;
    is_answered = true;
  }
  void send_and_close(uint32_t arg)
  {
    last_answer = arg;
    is_answered = true;
    is_open = false;
  }

  std::vector< uint32_t > arguments = { 16777216, 0, 180, 512*1024*1024, 0 };
  std::vector< uint32_t > commands_to_send = { REQUEST_READ_AND_IDX };
  uint32_t req_runtime = 3;
  uint32_t last_answer = 0;
  pid_t client_pid = 0;
  bool is_open = true;
  bool is_answered = false;
  bool has_write_semaphore = false;
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
    
    void measure(uint32_t num_shedded, uint64_t maxtime_used, uint64_t maxsize_used, time_t now);
    void calc_data_per_second(time_t now);
    uint64_t average_used_time() const;
    uint64_t average_used_size() const;
  };

  
  Global_Reading_State(Resource_State global_state_)
    : global_state(global_state_), request_queue(client_register) {}

  bool poll_reading_requests(std::unordered_map< int, Socket_To_Client >& clients, time_t now);
  void request_read_and_idx(int fd, time_t now, Socket_To_Client& socket);
  void grant_and_purge(std::unordered_map< int, Socket_To_Client >& clients, bool pending_commit, time_t now);

  const Client_State* get_client_state(Client_Token t, time_t now)
  { return client_register.get_client_state(t, now); }
  
  const Statistics& get_statistics() const { return statistics; }
  bool is_reading_idx() const { return !reading_idx.empty(); }

private:
  Resource_State global_state;
  std::unordered_map< int, Request_State > queued;
  std::unordered_map< int, Request_State > reading;
  std::unordered_set< int > reading_idx;
  Client_Register client_register;
  Request_Queue request_queue;
  Statistics statistics;

  void finish_request(const std::pair< const int, Request_State >& arg, time_t now);
};


#endif
