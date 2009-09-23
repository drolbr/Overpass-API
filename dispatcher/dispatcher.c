#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

#include <mysql.h>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

const char* DISPATCH_FIFO = "/tmp/dispatcher.pipe";
const char* DB1_FIFO = "/tmp/database_1.pipe";
const char* DB2_FIFO = "/tmp/database_2.pipe";
const char* SMALL_STATUS_FILE = "/tmp/small_status";
const char* BIG_STATUS_FILE = "/tmp/big_status";
const char* RESCUE_STATUS_FILE = "/opt/osm_why_api/rescue_status";
const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";

const uint TOTAL_MEMORY = 40*1000*1000;

uint ignore_whitespace(const string& input, uint pos)
{
  while ((pos < input.size()) && (isspace(input[pos])))
    ++pos;
  return pos;
}

uint read_uint64(const string& input, uint pos, uint64& result)
{
  result = 0;
  while ((pos < input.size()) && (isdigit(input[pos])))
    result = 10*result + (uint8)(input[pos++] - '0');
  return pos;
}

MYSQL_RES* mysql_query_use_wrapper(MYSQL* mysql, string query)
{
  mysql_query(mysql, query.c_str());
  MYSQL_RES* result(mysql_use_result(mysql));
  return result;
}

uint uint_query(MYSQL* mysql, string query)
{
  uint result_val(0);
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return 0;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  if ((row) && (row[0]))
    result_val = atoll(row[0]);
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_val;
}

void log_event(const string& message)
{
  ofstream log(LOGFILE, ios_base::app);
  log<<"dispatcher@"<<(uintmax_t)time(NULL)<<": "<<message<<'\n';
  log.close();
}

struct Database_State
{
  static const uint ACTIVE;
  static const uint PENDING;
  static const uint DATA_UPDATE;
  static const uint RULE_UPDATE;
  
  uint state;
  uint last_changefile;
  uint last_rule;
};

struct Process_State
{
  Process_State(uint process_id_)
  : process_id(process_id_), database_id(0), timeout_time(0), reserved_memory(0) {}
  
  uint process_id;
  uint database_id;
  uint64 timeout_time;
  uint64 reserved_memory;
};

struct State
{
  State() : last_changefile(0), db1(), db2(), processes() {}
  
  uint last_changefile;
  Database_State db1;
  Database_State db2;
  list< Process_State > processes;
};

void write_status_files(const State& state)
{
  uint available_memory(TOTAL_MEMORY);
  
  for (list< Process_State >::const_iterator it(state.processes.begin());
       it != state.processes.end(); ++it)
    available_memory -= it->reserved_memory;
  
  ofstream small_status(SMALL_STATUS_FILE);
  if (state.db1.state == Database_State::ACTIVE)
    small_status<<1;
  else if (state.db2.state == Database_State::ACTIVE)
    small_status<<2;
  small_status<<' '<<available_memory<<'\n';
  small_status.close();
  
  ofstream big_status(BIG_STATUS_FILE);
  big_status<<state.last_changefile<<'\n';
  big_status<<state.db1.state<<' '<<state.db1.last_changefile<<' '<<state.db1.last_rule<<'\n'
      <<state.db2.state<<' '<<state.db2.last_changefile<<' '<<state.db2.last_rule<<'\n';
  for (list< Process_State >::const_iterator it(state.processes.begin());
       it != state.processes.end(); ++it)
    big_status<<it->database_id<<' '<<it->timeout_time<<' '<<it->reserved_memory<<'\n';
  big_status.close();

  ofstream rescue_status(RESCUE_STATUS_FILE);
  if (state.db1.state == Database_State::DATA_UPDATE)
    rescue_status<<"dirty ";
  else
    rescue_status<<state.db1.last_changefile<<' ';
  if (state.db2.state == Database_State::DATA_UPDATE)
    rescue_status<<"dirty\n";
  else
    rescue_status<<state.db2.last_changefile<<'\n';
  rescue_status.close();
}

void check_pending_database(State& state, MYSQL* mysql)
{
  if (state.db1.state == Database_State::PENDING)
  {
    bool outdated(false), unused(true);
    outdated |= (state.db1.last_changefile < state.last_changefile);
    outdated |= (state.db1.last_rule < uint_query(mysql, "select max(id) from rule_bodys"));
    
    list< Process_State >::iterator it(state.processes.begin());
    while (it != state.processes.end())
    {
      if (it->database_id == 1)
      {
	unused = false;
	break;
      }
      ++it;
    }
    
    if (!(outdated && unused))
      return;
    
    uint new_version(state.last_changefile);
    if (state.db1.last_changefile > new_version)
      new_version = state.db1.last_changefile;
    ostringstream temp;
    temp<<" update "<<state.db1.last_changefile<<' '<<state.last_changefile<<' ';
    log_event((string)"sent: " + temp.str());
    int fd = open(DB1_FIFO, O_WRONLY|O_NONBLOCK);
    write(fd, temp.str().data(), temp.str().size());
    close(fd);
    
    state.db1.state = Database_State::DATA_UPDATE;
    state.db1.last_changefile = state.last_changefile;
  }
  else if (state.db2.state == Database_State::PENDING)
  {
    bool outdated(false), unused(true);
    outdated |= (state.db2.last_changefile < state.last_changefile);
    outdated |= (state.db2.last_rule < uint_query(mysql, "select max(id) from rule_bodys"));
    
    list< Process_State >::iterator it(state.processes.begin());
    while (it != state.processes.end())
    {
      if (it->database_id == 1)
      {
	unused = false;
	break;
      }
      ++it;
    }
    
    if (!(outdated && unused))
      return;
    
    uint new_version(state.last_changefile);
    if (state.db2.last_changefile > new_version)
      new_version = state.db2.last_changefile;
    ostringstream temp;
    temp<<" update "<<state.db2.last_changefile<<' '<<state.last_changefile<<' ';
    log_event((string)"sent: " + temp.str());
    int fd = open(DB2_FIFO, O_WRONLY|O_NONBLOCK);
    write(fd, temp.str().data(), temp.str().size());
    close(fd);
    
    state.db2.state = Database_State::DATA_UPDATE;
    state.db2.last_changefile = state.last_changefile;
  }
}

void check_process_time(State& state, MYSQL* mysql)
{
  uint64 current_time((uintmax_t)time(NULL));
  bool dirty(false);
  list< Process_State >::iterator it(state.processes.begin());
  while (it != state.processes.end())
  {
    if (it->timeout_time < current_time)
    {
      // kill runaway osm script
      mysql_ping(mysql);
      ostringstream temp;
      temp<<"kill "<<it->process_id;
      log_event(temp.str());
      mysql_query(mysql, temp.str().c_str());
    
      it = state.processes.erase(it);
      dirty = true;
    }
    else
      ++it;
  }

  if (dirty)
  {
    check_pending_database(state, mysql);
    write_status_files(state);
  }
}

void new_changefile(State& state, MYSQL* mysql, uint changefile_version)
{
  state.last_changefile = changefile_version;

  check_pending_database(state, mysql);
  write_status_files(state);
}

void update_rules(State& state, uint database_id, uint rule_version)
{
  if (database_id == 1)
  {
    state.db1.state = Database_State::RULE_UPDATE;
    state.db1.last_rule = rule_version;
  }
  else if (database_id == 2)
  {
    state.db2.state = Database_State::RULE_UPDATE;
    state.db2.last_rule = rule_version;
  }
  
  write_status_files(state);
}

void update_finished(State& state, MYSQL* mysql, uint database_id)
{
  if (database_id == 1)
  {
    state.db1.state = Database_State::ACTIVE;
    state.db2.state = Database_State::PENDING;
  }
  else if (database_id == 2)
  {
    state.db2.state = Database_State::ACTIVE;
    state.db1.state = Database_State::PENDING;
  }
  check_pending_database(state, mysql);
  write_status_files(state);
}

void query_start(State& state, uint database_id, uint process_id, uint timeout, uint size)
{
  Process_State new_process(process_id);
  new_process.database_id = database_id;
  new_process.timeout_time = (uintmax_t)time(NULL) + timeout;
  if (timeout == 0)
    new_process.timeout_time += 540;
  new_process.reserved_memory = size;
  state.processes.push_back(new_process);

  write_status_files(state);
}

void query_end(State& state, MYSQL* mysql, uint process_id)
{
  list< Process_State >::iterator it(state.processes.begin());
  while (it != state.processes.end())
  {
    if (it->process_id == process_id)
    {
      state.processes.erase(it);
      break;
    }
    ++it;
  }
  check_pending_database(state, mysql);
  write_status_files(state);
}

const uint Database_State::ACTIVE(1);
const uint Database_State::PENDING(2);
const uint Database_State::DATA_UPDATE(3);
const uint Database_State::RULE_UPDATE(4);

int main(int argc, char *argv[])
{
  if ((argc == 2) && (!strcmp(argv[1], "shutdown")))
  {
    //perform only shutdown, no server operation
    int fd = open(DISPATCH_FIFO, O_WRONLY|O_NONBLOCK);
    write(fd, " shutdown ", 10);
    
    return 0;
  }
  
  if (argc != 3)
  {
    cerr<<"Usage: "<<argv[0]<<" Last_Change_Db1 Last_Change_Db2\n";
    cerr<<"  or   "<<argv[0]<<" shutdown\n";
    
    return 0;
  }
  
  // prepare connection to mysql
  MYSQL* mysql(NULL);
  mysql = mysql_init(NULL);
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    remove(DISPATCH_FIFO);
    return 0;
  }
  {
    my_bool my_true(true);
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &my_true);
  }

  State state;
  state.db1.state = Database_State::ACTIVE;
  state.db1.last_changefile = atoi(argv[1]);
  state.db2.state = Database_State::PENDING;
  state.db2.last_changefile = atoi(argv[2]);
  state.last_changefile = state.db1.last_changefile;
  if (state.db2.last_changefile > state.db1.last_changefile)
    state.last_changefile = state.db2.last_changefile;
      
  check_pending_database(state, mysql);
  write_status_files(state);
  
  // create named pipe
  umask(0);
  mknod(DISPATCH_FIFO, S_IFIFO|0666, 0);
      
  int fd = open(DISPATCH_FIFO, O_RDONLY|O_NONBLOCK);
    
  bool shutdown_req(false);
  while (!shutdown_req)
  {
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
    
    //get data if any
    string input;
    char buf[80];
    int count(read(fd, buf, 79));
    while (count)
    {
      buf[count] = '\0';
      input += buf;
      count = read(fd, buf, 79);
    }
    
/*    uint64 current_time((uintmax_t)time(NULL));*/
    
    // process data
    uint pos(0);
    while (pos < input.size())
    {
      pos = ignore_whitespace(input, pos);
      
      if (input.substr(pos, 14) == "new_changefile")
      {
	uint64 changefile_version;
	pos += 14;
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, changefile_version);
	ostringstream temp;
	temp<<"new_changefile "<<changefile_version;
	log_event(temp.str());
	new_changefile(state, mysql, changefile_version);
      }
      else if (input.substr(pos, 12) == "update_rules")
      {
	uint64 database_id, rule_version;
	pos += 12;
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, database_id);
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, rule_version);
	ostringstream temp;
	temp<<"update_rules "<<database_id<<' '<<rule_version;
	log_event(temp.str());
	update_rules(state, database_id, rule_version);
      }
      else if (input.substr(pos, 15) == "update_finished")
      {
	uint64 database_id;
	pos += 15;
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, database_id);
	ostringstream temp;
	temp<<"update_finished "<<database_id;
	log_event(temp.str());
	update_finished(state, mysql, database_id);
      }
      else if (input.substr(pos, 11) == "query_start")
      {
	uint64 process_id, database_id, timeout, size;
	pos += 11;
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, process_id);
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, database_id);
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, timeout);
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, size);
	ostringstream temp;
	temp<<"query_start "<<process_id<<' '<<database_id<<' '<<timeout<<' '<<size;
	log_event(temp.str());
	query_start(state, database_id, process_id, timeout, size);
      }
      else if (input.substr(pos, 9) == "query_end")
      {
	uint64 process_id;
	pos += 9;
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, process_id);
	ostringstream temp;
	temp<<"query_end "<<process_id;
	log_event(temp.str());
	query_end(state, mysql, process_id);
      }
      else if (input.substr(pos, 8) == "shutdown")
      {
	pos += 8;
	shutdown_req = true;
	ostringstream temp;
	temp<<"shutdown";
	log_event(temp.str());
      }
      else
      {
	//Some garbage received
	while ((pos < input.size()) && (!isspace(input[pos])))
	  ++pos;
      }
    }
    
    check_process_time(state, mysql);
  }
  
  // delete named pipe
  close(fd);
  remove(DISPATCH_FIFO);
  
  return 0;
}
