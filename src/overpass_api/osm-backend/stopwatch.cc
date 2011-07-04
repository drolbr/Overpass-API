#include <time.h>

#include "stopwatch.h"

using namespace std;

Error_Output* Stopwatch::error_output(0);

void Stopwatch::start()
{
  for (uint i(0); i < stopwatches.size(); ++i)
    stopwatches[i] = 0;
  for (uint i(0); i < read_counts.size(); ++i)
    read_counts[i] = 0;
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatch = ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0;
}

void Stopwatch::skip()
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatch = ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0;
}

void Stopwatch::stop(uint32 account)
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatches[account] += (ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0)
    - stopwatch;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatch = ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0;
}

void Stopwatch::report(string info) const
{
  if (error_output)
    error_output->display_statement_stopwatch
        (info, stopwatches, read_counts);
}

void Stopwatch::sum(const Stopwatch& s)
{
  for (uint i(0); i < stopwatches.size(); ++i)
  {
    stopwatches[i] += s.stopwatches[i];
    read_counts[i] += s.read_counts[i];
  }
}
