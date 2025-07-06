#ifndef DE__OSM3S___TEMPLATE_DB__CLIENT_REGISTER
#define DE__OSM3S___TEMPLATE_DB__CLIENT_REGISTER

#include <cstdint>
#include <ctime>
#include <unordered_map>
#include <vector>


typedef uint64_t Client_Token;
const Client_Token LOCALHOST_BYPASS = 0;


struct Client_State
{
  uint32_t enqueued = 0;
  std::vector< int > reading;
  std::vector< time_t > cooldown;

  // Assert: cooldown is ordered
  void purge_cooldown(time_t now);
};


struct Client_Register
{
  // Assert: for all state != data[t] in data: state.reading.size() + state.cooldown.size() + state.enqueued is const
  // Assert: if ret==0 then data[t].reading.size() + data[t].cooldown.size() + data[t].enqueued is const
  uint32_t try_enqueue(Client_Token t, uint32_t rate_limit, time_t now);

  // Assert: for all state in data: state.reading.size() + state.cooldown.size() + state.enqueued is const
  void set_reading(Client_Token t, int fd);

  // Assert: for all state in data: state.reading.size() + state.cooldown.size() + state.enqueued is const
  void set_finished(Client_Token t, int fd, time_t cooldown_end);

  // Precondition: data[t].enqueued > 0
  void set_aborted(Client_Token t, int fd)
  { --data[t].enqueued; }

  // Assert: for all state in data: state.reading.size() + state.enqueued is const
  uint32_t num_active(Client_Token t, time_t now);

  // Assert: for all state in data: state.reading.size() + state.enqueued is const
  const Client_State* get_client_state(Client_Token t, time_t now);

  // Remove all clients that have zero activity
  void purge(time_t now);

private:
  std::unordered_map< Client_Token, Client_State > data;
  time_t last_purged = 0;
};


#endif
