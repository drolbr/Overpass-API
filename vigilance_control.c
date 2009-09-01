#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "user_interface.h"
#include "vigilance_control.h"

using namespace std;

const char* FIFO_FILE = "/tmp/dispatcher.pipe";

static uint64 timeout_(0);

// register the process for a penalty brake on a given timeout
// It starts the vigilance daemon if necesscary.
int register_process(uint mysql_id, uint database_id, uint32 timeout, uint64 max_element_count)
{
  // store current time.
  timeout_ = (uintmax_t)time(NULL) + timeout;
  if (timeout == 0)
    timeout_ += 540;
  
  ostringstream temp;
  temp<<" query_start "<<mysql_id<<' '<<database_id<<' '<<timeout<<' '<<max_element_count<<' ';
  
  int fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
  
  if (fd == -1)
  {
    runtime_error("Something's wrong with vigilance control - can't open timeout pipe.");
    return -1;
  }
  
  write(fd, temp.str().c_str(), temp.str().size());
  close(fd);
  
  return 0;
}

// unregister the process' timeout
int unregister_process(uint mysql_id)
{
  ostringstream temp;
  temp<<" query_end "<<mysql_id<<' ';
  
  int fd = open(FIFO_FILE, O_WRONLY|O_NONBLOCK);
  
  if (fd == -1)
  {
    runtime_error("Something's wrong with vigilance control - can't open timeout pipe for unregistering.");
    return -1;
  }
  
  write(fd, temp.str().c_str(), temp.str().size());
  close(fd);
  
  return 0;
}

bool is_timed_out()
{
  return ((timeout_ > 0) && ((uintmax_t)time(NULL) >= timeout_));
}

// int main(int argc, char *argv[])
// {
//   int pid, timeout;
//   get_output_stream()<<"Id: ";
//   cin>>pid;
//   get_output_stream()<<" timeout: ";
//   cin>>timeout;
//   
//   if (timeout > 0)
//     add_timeout(pid, timeout);
//   else
//     remove_timeout(pid);
//   
//   return 0;
// }
