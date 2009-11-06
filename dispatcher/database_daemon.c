#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "../script_datatypes.h"
#include "../user_interface.h"
#include "process_rules.h"

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
string DB_FIFO("/tmp/database_%.pipe");
const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";

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

MYSQL_RES* mysql_query_simple_wrapper(MYSQL* mysql, string query)
{
  mysql_query(mysql, query.c_str());
  MYSQL_RES* result(mysql_use_result(mysql));
  return result;
}

uint uint_query(MYSQL* mysql, string query)
{
  uint result_val(0);
  MYSQL_RES* result(mysql_query_simple_wrapper(mysql, query));
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

void log_event(uint database_id, const string& message)
{
  ofstream log(LOGFILE, ios_base::app);
  log<<"database_daemon("<<database_id<<")@"<<(uintmax_t)time(NULL)<<": "<<message<<'\n';
  log.close();
}

string db_subdir;

void process_update
    (MYSQL* mysql, const string& database_name, uint database_id,
     uint from_version, uint to_version)
{
  log_event(database_id, "Updating OSM data.");
  //execute gunzip and apply_osc
  int pid = fork();
  if (pid == 0)
  {
    ostringstream from, to;
    from<<from_version;
    to<<to_version;
    execlp("./apply_gz_osc", "./apply_gz_osc",
	   database_name.c_str(), from.str().c_str(), to.str().c_str(), NULL);
  }
  else
  {
    int status;
    pid = wait(&status);
  }
  
  mysql_ping(mysql);
  mysql_query(mysql, "use osm");
  uint rule_version(uint_query(mysql, "select max(id) from rule_bodys"));
  //notify dispatcher
  ostringstream temp;
  temp<<" update_rules "<<database_id<<' '<<rule_version<<' ';
  log_event(database_id, (string)"sent: " + temp.str());
  int fd = open(DISPATCH_FIFO, O_WRONLY|O_NONBLOCK);
  write(fd, temp.str().data(), temp.str().size());
  close(fd);
  
  db_subdir = database_name + '/';
  try
  {
    mysql_ping(mysql);
    mysql_query(mysql, ((string)("use ") + database_name).c_str());
/*    process_rules(mysql, database_name, rule_version);*/
  }
  catch(File_Error e)
  {
    cerr<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
  }
  //sleep for a second - dummy
  struct timeval timeout;
  timeout.tv_sec = 60;
  timeout.tv_usec = 0;
  select (FD_SETSIZE, NULL, NULL, NULL, &timeout);

  //notify dispatcher
  temp.str("");
  temp<<" update_finished "<<database_id<<' ';
  log_event(database_id, (string)"sent: " + temp.str());
  fd = open(DISPATCH_FIFO, O_WRONLY|O_NONBLOCK);
  write(fd, temp.str().data(), temp.str().size());
  close(fd);
}

int main(int argc, char *argv[])
{
  set_output_cout();
  
  if (argc != 3)
  {
    cerr<<"Usage: "<<argv[0]<<" Database_Name Database_Id\n";
    
    return 0;
  }
  
  string database_name(argv[1]);
  uint database_id(atoi(argv[2]));
  string fifo_name(DB_FIFO);
  fifo_name.replace(fifo_name.find('%'), 1, argv[2]);

  // prepare connection to mysql
  MYSQL* mysql(NULL);
  mysql = mysql_init(NULL);
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    cerr<<"Connection to database failed.\n";
    remove(fifo_name.c_str());
    return 0;
  }
  {
    my_bool my_true(true);
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &my_true);
  }

  // create named pipe
  umask(0);
  mknod(fifo_name.c_str(), S_IFIFO|0666, 0);
      
  int fd = open(fifo_name.c_str(), O_RDONLY|O_NONBLOCK);
    
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
      
      if (input.substr(pos, 6) == "update")
      {
	uint64 from_version, to_version;
	pos += 6;
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, from_version);
	pos = ignore_whitespace(input, pos);
	pos = read_uint64(input, pos, to_version);
	process_update(mysql, database_name, database_id, from_version, to_version);
      }
      else if (input.substr(pos, 8) == "shutdown")
      {
	pos += 8;
	shutdown_req = true;
      }
      else
      {
	//Some garbage received
	while ((pos < input.size()) && (!isspace(input[pos])))
	  ++pos;
      }
    }
  }

  // delete named pipe
  close(fd);
  remove(fifo_name.c_str());
  
  return 0;
}
