#ifndef VIGILANCE_CONTROL
#define VIGILANCE_CONTROL

#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "user_interface.h"

using namespace std;

// register the process for a penalty brake on a given timeout
// It starts the vigilance daemon if necesscary.
int add_timeout(int pid, int timeout);

// unregister the process' timeout
int remove_timeout();

// unregister the process' timeout
bool is_timed_out();

#endif
