
#include "dispatcher_logger.h"
#include "global_reading_state.h"
#include "types.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

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


void trigger_new_requests(
    std::unordered_map< int, Socket_To_Client >& clients,
    std::vector< int >& new_connections,
    int& next_fd, std::vector< int >& available_fd, int& next_pid,
    uint32_t& client_token, uint32_t& client_token_large,
    time_t tsec, uint32_t j)
{
  if (j % 5 == 3) // One-time only clients
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.arguments[0] = ++client_token;
    socket.req_runtime = (client_token % 3) + 10*std::max(5*client_token % 61, (uint32_t)40) - 397;
    new_connections.push_back(fd);
  }
  
  if (j % 10 == 3) // One-time only clients with large requests
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.arguments[0] = ++client_token_large;
    socket.arguments[3] = 1024*1024*1024;
    socket.req_runtime = (client_token % 3) + 10*std::max(5*client_token % 61, (uint32_t)40) - 397;
    new_connections.push_back(fd);
  }
  
  if (j % 5 == 2 && j + 1080000 < tsec) // Once per minute clients
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.arguments[0] = 144u*16777216u + j/5 + 100*(tsec%60);
    socket.req_runtime = 140 + (2*j)%13;
    new_connections.push_back(fd);
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
    socket.req_runtime = 160 + (3*j)%17;
    new_connections.push_back(fd);
  }

  if (j % 5 == 2) // Once per second clients
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.arguments[0] = 176u*16777216u + j/5;
    socket.req_runtime = 200 + (7*j)%11;
    new_connections.push_back(fd);
  }
  
  if (j % 100 == 41 && tsec % 30 == 11) // Long runtime heavy load client
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.arguments[0] = 192u*16777216u + j/5;
    socket.req_runtime = 1500 + 500*(tsec % 60 / 30);
    socket.client_pid = tsec / 30 + 1920000000;
    new_connections.push_back(fd);
  }

  if (j % 100 == 31 && tsec % 30 == 26)
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.commands_to_send = { QUERY_BY_TOKEN };
    socket.arguments[0] = 192u*16777216u + j/5 + 2;
    socket.client_pid = 1920000000;
    new_connections.push_back(fd);
  }

  if (tsec % 3600 == 471) // Binge client
  {
    for (uint32_t k = 0; k < 5; ++k)
    {
      int fd = dispense_fd(next_fd, available_fd);
      Socket_To_Client& socket = clients[fd];
      socket.arguments[0] = 224u*16777216u;
      socket.req_runtime = 500 + k;
      new_connections.push_back(fd);
    }
  }

  if (j == 0 && tsec == 1166400)
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.client_pid = next_pid++;
    socket.commands_to_send = { TERMINATE };
    new_connections.push_back(fd);
  }
  
  if (tsec % 18000 == 305 && j == 0) // Area recreation loop
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.arguments[0] = 0;
    socket.arguments[2] = 86400;
    socket.arguments[3] = 0;
    socket.arguments[4] = 1;
    socket.req_runtime = 399501;
    new_connections.push_back(fd);
  }
  
  if (tsec % 61 == 2 && j == 50) // Write loop
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.client_pid = next_pid++;
    if (tsec % (61*1440) == 2 + 61*720)
    {
      socket.commands_to_send = { MIGRATE_COMMIT, MIGRATE_START };
      socket.req_runtime = 30000;
      std::cout<<"Triggered migrate: will replace one and displace four write cycles.\n";
    }
    else
    {
      socket.commands_to_send = { WRITE_COMMIT, WRITE_START };
      socket.req_runtime = 150;
    }
    new_connections.push_back(fd);
  }

  if (tsec % (61*720) == 32 + 360*61 && j == 50) // Rollback test
  {
    int fd = dispense_fd(next_fd, available_fd);
    Socket_To_Client& socket = clients[fd];
    socket.client_pid = next_pid++;
    if (tsec % (61*1440) == 32 + 360*61)
    {
      socket.commands_to_send = { WRITE_ROLLBACK, WRITE_START };
      socket.req_runtime = 50;
    }
    else
    {
      socket.commands_to_send = { MIGRATE_ROLLBACK, MIGRATE_START };
      socket.req_runtime = 50;
    }
    new_connections.push_back(fd);
  }
}


struct Writing_State
{
  void poll_writing_process(std::unordered_map< int, Socket_To_Client >& clients, bool is_reading_idx)
  {
    if (writing_fd == 0)
      return;
    Socket_To_Client& socket = clients[writing_fd];
    auto command = socket.get_command();
    if (command == WRITE_COMMIT || command == MIGRATE_COMMIT)
      pending_commit = true;
    else if (command == WRITE_ROLLBACK || command == MIGRATE_ROLLBACK || command == HANGUP)
    {
      if (is_migrate)
         migrate_rollback(command == HANGUP ? nullptr : &socket);
      else
        write_rollback(command == HANGUP ? nullptr : &socket);
    }

    if (pending_commit && !is_reading_idx)
    {
      if (is_migrate)
        try_migrate_commit(socket);
      else
        try_write_commit(socket);
      pending_commit = false;
    }
//     else
//       std::cout<<"Waiting for reading_idx:\t"<<std::dec<<socket.client_pid<<'\t'<<writing_fd<<'\n';
  }

  void try_write_start(int fd, Socket_To_Client& socket);
  void try_migrate_start(int fd, Socket_To_Client& socket);
  
  bool has_pending_commit() const { return pending_commit; }
  
private:
  int writing_fd = 0;
  bool is_migrate = false;
  bool pending_commit = false;
  
  void try_write_commit(Socket_To_Client& socket);
  void try_migrate_commit(Socket_To_Client& socket);
  void write_rollback(Socket_To_Client* socket);
  void migrate_rollback(Socket_To_Client* socket);

  void try_write_pid_to_lockfile(pid_t pid)
  {
    try
    {
      std::ofstream lock("shadow.lock");
      lock<<pid;
    }
    catch (...) {}
  }


  void confirm_lockfile_or_show_error(const File_Error& e, pid_t pid)
  {
    if ((e.error_number == EEXIST) && (e.filename == ("shadow.lock")))
    {
      pid_t locked_pid;
      std::ifstream lock("shadow.lock");
      lock>>locked_pid;
      if (locked_pid == pid)
        return;
    }
    std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
};


void Writing_State::try_write_start(int fd, Socket_To_Client& socket)
{
  try
  {
    Raw_File shadow_file("shadow.lock", O_RDWR|O_CREAT|O_EXCL, S_666, "write_start::1");

//     transaction_insulator.copy_mains_to_shadows();
//     transaction_insulator.write_index_of_empty_blocks();
//     if (logger)
//       logger->write_start(pid, transaction_insulator.registered_pids());
  }
  catch (File_Error e)
  {
    confirm_lockfile_or_show_error(e, socket.client_pid);
    socket.send_and_close(WRITE_START);
    return;
  }
  writing_fd = fd;
  is_migrate = false;
  try_write_pid_to_lockfile(socket.client_pid);
  socket.send(WRITE_START);
}


void Writing_State::try_migrate_start(int fd, Socket_To_Client& socket)
{
  try
  {
    Raw_File shadow_file("shadow.lock", O_RDWR|O_CREAT|O_EXCL, S_666, "migrate_start::1");

//     if (logger)
//       logger->migrate_start(pid, transaction_insulator.registered_pids());
  }
  catch (File_Error e)
  {
    confirm_lockfile_or_show_error(e, socket.client_pid);
    socket.send_and_close(MIGRATE_START);
    return;
  }
  writing_fd = fd;
  is_migrate = true;
  try_write_pid_to_lockfile(socket.client_pid);
  socket.send(MIGRATE_START);
}


void Writing_State::try_write_commit(Socket_To_Client& socket)
{
//   if (logger)
//     logger->write_commit(pid);
  try
  {
    Raw_File shadow_file("shadow", O_RDWR|O_CREAT|O_EXCL, S_666, "write_commit:1");
//     transaction_insulator.copy_shadows_to_mains();
  }
  catch (File_Error e)
  {
    std::cerr<<"File_Error "<<e.error_number<<' '<<std::strerror(e.error_number)
        <<' '<<e.filename<<' '<<e.origin<<'\n';
    socket.send_and_close(WRITE_COMMIT);
    return;
  }

  remove("shadow");
//   transaction_insulator.remove_shadows();
  remove("shadow.lock");
//   transaction_insulator.set_current_footprints();

  socket.send_and_close(WRITE_COMMIT);
  writing_fd = 0;
}


void Writing_State::try_migrate_commit(Socket_To_Client& socket)
{
//   if (logger)
//     logger->migrate_commit(pid);
  try
  {
    Raw_File shadow_file("shadow", O_RDWR|O_CREAT|O_EXCL, S_666, "migrate_commit:1");
//     transaction_insulator.move_migrated_files_in_place();
  }
  catch (File_Error e)
  {
    std::cerr<<"File_Error "<<e.error_number<<' '<<std::strerror(e.error_number)
        <<' '<<e.filename<<' '<<e.origin<<'\n';
    socket.send_and_close(WRITE_COMMIT);
    return;
  }

  remove("shadow");
//   transaction_insulator.remove_migrated();
  remove("shadow.lock");
//   transaction_insulator.set_current_footprints();

  socket.send_and_close(MIGRATE_COMMIT);
  writing_fd = 0;
}



void Writing_State::write_rollback(Socket_To_Client* socket)
{
  //   if (logger)
  //     logger->write_rollback(pid);

  //   transaction_insulator.remove_shadows();
  remove("shadow.lock");

  if (socket)
    socket->send_and_close(WRITE_ROLLBACK);
  writing_fd = 0;
}


void Writing_State::migrate_rollback(Socket_To_Client* socket)
{
  //   if (logger)
  //     logger->migrate_rollback(pid);

  //   transaction_insulator.remove_migrated();
  remove("shadow.lock");

  if (socket)
    socket->send_and_close(MIGRATE_ROLLBACK);
  writing_fd = 0;
}


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" rate_limit\n";
    return 0;
  }
  
  std::unordered_map< int, Socket_To_Client > clients;
  std::vector< int > new_connections;
  std::vector< int > available_fd;
  int next_fd = 3;
  int next_pid = 4096;
  uint32_t client_token = 64u*16777216u;
  uint32_t client_token_large = 48u*16777216u;
  std::vector< uint32_t > http_429(16, 0);
  std::vector< uint32_t > http_504(16, 0);
  std::vector< uint32_t > http_200(16, 0);
  uint32_t sock_write_commit = 0;
  uint32_t sock_migrate_commit = 0;
  uint32_t sock_write_rollback = 0;
  uint32_t sock_migrate_rollback = 0;
  uint32_t no_query_found_by_token = 0;
  Global_Reading_State global_reading_state({ 10, (uint32_t)atoi(args[1]), 3*86400, (uint64_t)16*1024*1024*1024 });
  Writing_State writing_state;
  bool terminate = false;

  remove("database.log");
  Logger logger(1080000, "./", "database.log");

  for (time_t tsec = 1080000; tsec <= 1166400; ++tsec)
  {
    logger.set_now(tsec);
    if (tsec % 3600 == 0)
      std::cout<<"Nominal time: "<<std::dec<<tsec/3600<<
          ", Avg_Time "<<global_reading_state.get_statistics().average_used_time()<<
          ", Avg_Size "<<global_reading_state.get_statistics().average_used_size()<<
          ", Total_Runtime "<<Socket_To_Client::global_runtime<<'\n';
    
    for (uint32_t j = 0; j < 100; ++j)
    {
      // Scaffolding
      trigger_new_requests(
          clients, new_connections, next_fd, available_fd, next_pid, client_token, client_token_large, tsec, j);
      // Everything else in the loop is actual dispatcher work

      global_reading_state.poll_reading_requests(clients, tsec,  Dispatcher_Reading_Logger(logger));
      writing_state.poll_writing_process(clients, global_reading_state.is_reading_idx());
      
      for (int fd : new_connections)
      {
        Socket_To_Client& socket = clients[fd];
        auto command = socket.get_command();
        if (command == REQUEST_READ_AND_IDX)
          global_reading_state.request_read_and_idx(fd, tsec, socket);
        else if (command == WRITE_START)
          writing_state.try_write_start(fd, socket);
        else if (command == MIGRATE_START)
          writing_state.try_migrate_start(fd, socket);
        else if (command == TERMINATE)
        {
          terminate = true;
          socket.send_and_close(command);
          break;
        }
        else if (command == QUERY_BY_TOKEN)
        {
          std::vector< uint32_t > args = socket.get_arguments(2);
          if (args.size() < 2)
            socket.send_and_close(0);
          const Client_State* client_state = global_reading_state.get_client_state(
              ((uint64_t)args[0] | ((uint64_t)args[1]<<32)), tsec);
          int target_fd = (client_state && !client_state->reading.empty()) ? client_state->reading.back() : 0;
          if (target_fd > 0)
            socket.send_and_close(clients[target_fd].client_pid);
          else
            socket.send_and_close(0);
        }
        else
          std::cout<<"Request with invalid command dropped: 0x"<<std::hex<<command<<'\n';
      }
      new_connections.clear();
      if (terminate)
        break;

      global_reading_state.grant_and_purge(clients, writing_state.has_pending_commit(), tsec);
    }

    auto it = clients.begin();
    while (it != clients.end())
    {
      if (it->second.is_open)
        ++it;
      else
      {
        if (!it->second.is_answered)
          std::cout<<"Answer lost for fd "<<it->first<<'\n';
        else if (it->second.last_answer == RATE_LIMITED)
          ++http_429[it->second.arguments[0]>>28];
        else if (it->second.last_answer == QUERY_REJECTED)
          ++http_504[it->second.arguments[0]>>28];
        else if (it->second.last_answer == READ_FINISHED)
          ++http_200[it->second.arguments[0]>>28];
        else if (it->second.last_answer == WRITE_COMMIT)
          ++sock_write_commit;
        else if (it->second.last_answer == MIGRATE_COMMIT)
          ++sock_migrate_commit;
        else if (it->second.last_answer == WRITE_ROLLBACK)
          ++sock_write_rollback;
        else if (it->second.last_answer == MIGRATE_ROLLBACK)
          ++sock_migrate_rollback;
        else if (it->second.last_answer == WRITE_START)
          std::cout<<"Last answer for fd "<<std::dec<<it->first<<" was WRITE_START"<<'\n';
        else if (it->second.last_answer == MIGRATE_START)
          std::cout<<"Last answer for fd "<<std::dec<<it->first<<" was MIGRATE_START"<<'\n';
        else if (it->second.client_pid == 1920000000)
        {
          if (it->second.last_answer)
            std::cout<<"Found by token: pid "<<std::dec<<it->second.last_answer<<'\n';
          else
            ++no_query_found_by_token;
        }
        else
          std::cout<<"Last answer for fd "<<std::dec<<it->first<<" was 0x"<<std::hex<<it->second.last_answer<<'\n';
        available_fd.push_back(it->first);
        it = clients.erase(it);
      }
    }
    if (terminate)
      break;
  }

  {
    uint32_t sum = 0;
    std::cout<<" Rate limited:";
    for (auto i : http_429)
    {
      sum += i;
      std::cout<<'\t'<<std::dec<<i;
    }
    std::cout<<'\t'<<std::dec<<sum<<'\n';
  }
  {
    uint32_t sum = 0;
    std::cout<<"      Shedded:";
    for (auto i : http_504)
    {
      sum += i;
      std::cout<<'\t'<<std::dec<<i;
    }
    std::cout<<'\t'<<std::dec<<sum<<'\n';
  }
  {
    uint32_t sum = 0;
    std::cout<<"Read finished:";
    for (auto i : http_200)
    {
      sum += i;
      std::cout<<'\t'<<std::dec<<i;
    }
    std::cout<<'\t'<<std::dec<<sum<<'\n';
  }
  std::cout<<"Write commit:\t"<<std::dec<<sock_write_commit<<'\n';
  std::cout<<"Migrate commit:\t"<<std::dec<<sock_migrate_commit<<'\n';
  std::cout<<"Write rollback:\t"<<std::dec<<sock_write_rollback<<'\n';
  std::cout<<"Migrate rollback:\t"<<std::dec<<sock_migrate_rollback<<'\n';
  std::cout<<"No query found by token:\t"<<std::dec<<no_query_found_by_token<<'\n';

  return 0;
}
