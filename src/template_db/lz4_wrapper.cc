/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht, 2015 mmd
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

#include "lz4_wrapper.h"

#include <cstring>
#include <iostream>
#include <sstream>


LZ4_Deflate::LZ4_Deflate() { }

LZ4_Deflate::~LZ4_Deflate() { }

int LZ4_Deflate::compress(const void* in, int in_size, void* out, int out_buffer_size)
{
   int ret = LZ4_compress_limitedOutput((const char*) in, (char *) out + 4, in_size, out_buffer_size - 4);

   if (ret == 0)
   { // compression failed
     if (in_size > out_buffer_size - 4)
       throw Error (-5);

     *(int*)out = in_size * -1;
     std::memcpy ((char *) out + 4, (const char*)in, in_size);
     ret = in_size;
   }
   else
     *(int*)out = ret;

   return ret + 4;
}

LZ4_Inflate::LZ4_Inflate() { }

LZ4_Inflate::~LZ4_Inflate() { }


int LZ4_Inflate::decompress(const void* in, int in_size, void* out, int out_buffer_size)
{
  int ret;
  int in_buffer_size = *(int*)in;

  if (in_buffer_size > 0)
  {
    ret = LZ4_decompress_safe((const char*) in + 4, (char*) out, in_buffer_size, out_buffer_size);
    if (ret < 0)
      throw Error (ret);
  }
  else
  {
    in_buffer_size *= -1;
    if (in_buffer_size > out_buffer_size)
      throw Error (1);

    std::memcpy ((char*) out, (const char*) in + 4, in_buffer_size);
    ret = in_buffer_size;
  }
  return ret;
}


const char* LZ4_Deflate::Error::what() const throw()
{
  std::ostringstream out;
  out<<"LZ4_Deflate: "<<error_code;
  return out.str().c_str();
}


const char* LZ4_Inflate::Error::what() const throw()
{
  std::ostringstream out;
  out<<"LZ4_Inflate: "<<error_code;
  return out.str().c_str();
}
