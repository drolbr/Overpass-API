/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Template_DB.
*
* Template_DB is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Template_DB is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Template_DB.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "types.h"


int& global_read_counter()
{
  static int counter = 0;
  return counter;
}


void millisleep(uint32 milliseconds)
{
  struct timeval timeout_;
  timeout_.tv_sec = milliseconds/1000;
  timeout_.tv_usec = milliseconds*1000;
  select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
}
