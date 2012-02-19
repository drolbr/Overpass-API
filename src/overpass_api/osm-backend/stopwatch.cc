/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

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
