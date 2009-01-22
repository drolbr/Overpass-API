#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "user_interface.h"
#include "vigilance_control.h"

using namespace std;

const char* FIFO_FILE = "/tmp/osmy_watchdog.pipe";

static unsigned long timeout_time(0);
static unsigned long pid_(0);

// register the process for a penalty brake on a given timeout
// It starts the vigilance daemon if necesscary.
int add_timeout(int pid, int timeout)
{
  // automatic start of osmy_vigilance does somehow prevent the query from returning
  // it's by the way also a security issue
  /*  int fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
  
  if (fd == -1)
  {
    if (errno == ENOENT)
    {
      int child = fork();
      if (child)
      {
	if (child == -1)
	  runtime_error("Something's wrong with vigilance control - can't start vigilance process.", cout);
	
	// number of retries for opening the pipe
	// -- maybe the just started 
	int retries(10);
	fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
	while ((fd == -1) && (--retries >= 0))
	{
	  //sleep for a second
	  struct timeval timeout;
	  timeout.tv_sec = 1;
	  timeout.tv_usec = 0;
	  select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
    
	  fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
	}
	if (fd == -1)
	  runtime_error("Something's wrong with vigilance control - can't open timeout pipe.", cout);
      }
      else
	execl("../osmy_vigilance", "osmy_vigilance", (char*) NULL);
    }
    else
      runtime_error("Something's wrong with vigilance control - can't open timeout pipe.", cout);
  }*/
  
  // store current time.
  timeout_time = (uintmax_t)time(NULL) + timeout;
  pid_ = pid;
  
  ostringstream temp;
  temp<<' '<<pid<<' '<<timeout<<' ';
  
  int fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
  
  if (fd == -1)
  {
    runtime_error("Something's wrong with vigilance control - can't open timeout pipe.", cout);
    return -1;
  }
  
  write(fd, temp.str().c_str(), temp.str().size());
  close(fd);
  
  return 0;
}

// unregister the process' timeout
int remove_timeout()
{
  ostringstream temp;
  temp<<' '<<pid_<<" accomplished ";
  
  int fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
  
  if (fd == -1)
  {
    runtime_error("Something's wrong with vigilance control - can't open timeout pipe for unregistering.", cout);
    return -1;
  }
  
  write(fd, temp.str().c_str(), temp.str().size());
  close(fd);
  
  return 0;
}

// unregister the process' timeout
bool is_timed_out()
{
  return ((timeout_time > 0) && ((uintmax_t)time(NULL) >= timeout_time));
}

// int main(int argc, char *argv[])
// {
//   int pid, timeout;
//   cout<<"Id: ";
//   cin>>pid;
//   cout<<" timeout: ";
//   cin>>timeout;
//   
//   if (timeout > 0)
//     add_timeout(pid, timeout);
//   else
//     remove_timeout(pid);
//   
//   return 0;
// }
