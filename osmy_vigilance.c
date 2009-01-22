#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

#include <mysql.h>

using namespace std;

const char* FIFO_FILE = "/tmp/osmy_watchdog.pipe";

int main(int argc, char *argv[])
{
  if ((argc == 2) && (!strcmp(argv[1], "shutdown")))
  {
    //perform only shutdown, no server operation
    int fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
    write(fd, " shutdown ", 10);
    
    return 0;
  }
  
  // prepare connection to mysql
  MYSQL* mysql(NULL);
  mysql = mysql_init(NULL);
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", NULL, 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    remove(FIFO_FILE);
    return 0;
  }

  multimap< unsigned long long, unsigned long long > event_queue;
  
  // create named pipe
  umask(0);
  mknod(FIFO_FILE, S_IFIFO|0666, 0);
      
  int fd = open(FIFO_FILE, O_RDONLY|O_NONBLOCK);
    
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
    
    unsigned long long current_time((uintmax_t)time(NULL));
    
    // process data
    unsigned int pos(0);
    while (pos < input.size())
    {
      unsigned long long pid(0), timeout(0);
      while ((pos < input.size()) && (isspace(input[pos])))
	++pos;
      while ((pos < input.size()) && (isdigit(input[pos])))
	pid = 10*pid + (unsigned char)(input[pos++] - '0');
      while ((pos < input.size()) && (isspace(input[pos])))
	++pos;
      while ((pos < input.size()) && (isdigit(input[pos])))
	timeout = 10*timeout + (unsigned char)(input[pos++] - '0');
      //cerr<<'['<<input<<"]\t"<<pid<<' '<<timeout<<'\n';
      if ((pid > 0) && (timeout > 0))
      {
/*	cerr<<current_time
	    <<": setting timeout for "<<pid<<" to "<<timeout + current_time<<'\n';*/
	event_queue.insert(make_pair< unsigned long long, unsigned long long >
	     (timeout + current_time, pid));
      }
      else if ((pid > 0) && (input.substr(pos, 12) == "accomplished"))
      {
	pos += 12;
// 	cerr<<current_time<<": removing "<<pid<<" ... \n";
	map< unsigned long long, unsigned long long >::iterator it(event_queue.begin());
	while ((it != event_queue.end()) && (it->second != pid))
	  ++it;
	if (it != event_queue.end())
	  event_queue.erase(it);
      }
      else if (input.substr(pos, 8) == "shutdown")
      {
/*	cerr<<current_time<<": shutdown requested\n";*/
	pos += 8;
	shutdown_req = true;
      }
      else
      {
/*	cerr<<current_time<<": Warning: Some garbage received\n";*/
	while ((pos < input.size()) && (!isspace(input[pos])))
	  ++pos;
      }
      while ((pos < input.size()) && (isspace(input[pos])))
	++pos;
    }
    
    //test whether a timeout has occurred
    map< unsigned long long, unsigned long long >::iterator it(event_queue.begin());
    while ((it != event_queue.end()) && (it->first <= current_time))
    {
      // kill runaway osm script
      mysql_ping(mysql);
      ostringstream temp;
      temp<<"kill "<<it->second;
      mysql_query(mysql, temp.str().c_str());
      
      cerr<<current_time<<": '"<<temp.str()<<"' sent to mysql\n";
      
      ++it;
    }
    if (it != event_queue.begin())
      event_queue.erase(event_queue.begin(), it);
    
    // display that it is still alive
/*    if (pos == 0)
      cerr<<current_time<<'\n';*/
  }
  
  // delete named pipe
  close(fd);
  remove(FIFO_FILE);
  
  return 0;
}
