
#include "client_register.h"


template< typename Container >
void erase_from_unordered(Container& container, decltype(container[0]) elem)
{
  for (auto it = container.begin(); it != container.end(); ++it)
  {
    if (*it == elem)
    {
      *it = container.back();
      container.pop_back();
      return;
    }
  }
}


void Client_State::purge_cooldown(time_t now)
{
  auto it = cooldown.begin();
  while (it != cooldown.end() && *it < now)
    ++it;
  cooldown.erase(cooldown.begin(), it);
}


uint32_t Client_Register::try_enqueue(Client_Token t, uint32_t rate_limit, time_t now)
{
  Client_State& state = data[t];
  state.purge_cooldown(now);
  if (rate_limit > 0)
  {
    if (state.reading.size() + state.cooldown.size() + state.enqueued >= 2*rate_limit)
      return 0;
    ++state.enqueued;
  }
  else
    state.enqueued = 1;
  return state.reading.size() + state.cooldown.size() + state.enqueued;
}


void Client_Register::set_reading(Client_Token t, int fd)
{
  Client_State& state = data[t];
  if (state.enqueued > 0)
    --state.enqueued;
  state.reading.push_back(fd);
}


void Client_Register::set_finished(Client_Token t, int fd, time_t cooldown_end)
{
  Client_State& state = data[t];
  erase_from_unordered(state.reading, fd);
  state.cooldown.push_back(cooldown_end);
}


uint32_t Client_Register::num_active(Client_Token t, time_t now)
{
  Client_State& state = data[t];
  state.purge_cooldown(now);
  return state.reading.size() + state.cooldown.size();
}


const Client_State* Client_Register::get_client_state(Client_Token t, time_t now)
{
  auto it = data.find(t);
  if (it == data.end())
    return nullptr;
  it->second.purge_cooldown(now);
  return &it->second;
}


void Client_Register::purge(time_t now)
{
  if (now <= last_purged)
    return;
  last_purged = now;

  auto it = data.begin();
  while (it != data.end())
  {
    if (it->second.enqueued == 0 && it->second.reading.empty())
    {
      it->second.purge_cooldown(now);
      if (it->second.cooldown.empty())
        it = data.erase(it);
      else
        ++it;
    }
    else
      ++it;
  }
}


/*#include <iostream>

int main(int argc, char* args[])
{
  {
    std::cout<<"\nIsolated test of regular lifecycle:\n";

    Client_Register client_register;
    std::cout<<"num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    std::cout<<"try_enqueue: "<<client_register.try_enqueue(16777216, 4, 1000)<<", ";
    std::cout<<"num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    client_register.set_reading(16777216, 2222);
    std::cout<<"set_reading(1.0.0.0, 2222): num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    client_register.set_finished(16777216, 2222, 1010);
    std::cout<<"set_finished(1.0.0.0, 2222, 1010): num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    std::cout<<"num_active(1010): "<<client_register.num_active(16777216, 1010)<<'\n';
    std::cout<<"num_active(1011): "<<client_register.num_active(16777216, 1011)<<'\n';
  }
  {
    std::cout<<"\nIsolated test of inadmissible request lifecycle:\n";

    Client_Register client_register;
    std::cout<<"num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    std::cout<<"try_enqueue: "<<client_register.try_enqueue(16777216, 4, 1000)<<", ";
    std::cout<<"num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    client_register.set_aborted(16777216, 2222);
    std::cout<<"set_aborted(1.0.0.0, 2222): num_active "<<client_register.num_active(16777216, 1000)<<'\n';
    std::cout<<"num_active(1010): "<<client_register.num_active(16777216, 1010)<<'\n';
    std::cout<<"num_active(1011): "<<client_register.num_active(16777216, 1011)<<'\n';
  }

  {
    std::cout<<"\nFirst load test: 2^20 clients pose one request each ... ";

    Client_Register client_register;
    for (uint32_t i = 0; i < 20*1024*1024; ++i)
    {
      client_register.try_enqueue(16777216 + i, 4, 1024 + i/1024);
      client_register.set_reading(16777216 + i, 65536 + i);
      client_register.set_finished(16777216 + i, 65536 + i, 1025 + i/1024);
    }

    std::cout<<"done.\n";
  }

  return 0;
}
*/
