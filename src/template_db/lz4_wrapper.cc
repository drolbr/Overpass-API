/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lz4_wrapper.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>


namespace
{
  template < typename T >
  std::string to_string(T t)
  {
    std::ostringstream out;
    out<<std::setprecision(14)<<t;
    return out.str();
  }
}


LZ4_Deflate::Error::Error(int error_code_)
    : std::runtime_error("LZ4_Deflate: " + to_string(error_code_)), error_code(error_code_)
{}


LZ4_Deflate::LZ4_Deflate() { }

LZ4_Deflate::~LZ4_Deflate() { }

int LZ4_Deflate::compress(const void* in, int in_size, void* out, int out_buffer_size)
{
#ifdef HAVE_LZ4

   int ret = LZ4_compress_limitedOutput((const char*) in, (char *) out + 4, in_size, out_buffer_size - 4);

   if (ret == 0)
   { // compression failed
     if (in_size > out_buffer_size - 4)
       throw std::runtime_error("LZ4: output buffer too small during compression");

     *(int*)out = in_size * -1;
     std::memcpy ((char *) out + 4, (const char*)in, in_size);
     ret = in_size;
   }
   else
     *(int*)out = ret;

   return ret + 4;

#else

  throw std::runtime_error("Overpass API was compiled without lz4 compression library support");

#endif
}


LZ4_Inflate::Error::Error(int error_code_)
    : std::runtime_error("LZ4_Inflate: " + to_string(error_code_)), error_code(error_code_)
{}


LZ4_Inflate::LZ4_Inflate() { }

LZ4_Inflate::~LZ4_Inflate() { }


int LZ4_Inflate::decompress(const void* in, int in_size, void* out, int out_buffer_size)
{
#ifdef HAVE_LZ4

  int ret;
  int in_buffer_size = *(int*)in;

  if (in_buffer_size > 0)
  {
    ret = LZ4_decompress_safe((const char*) in + 4, (char*) out, in_buffer_size, out_buffer_size);
    if (ret < 0)
      throw std::runtime_error("LZ4_decompress_safe failed");
  }
  else
  {
    in_buffer_size *= -1;
    if (in_buffer_size > out_buffer_size)
      throw std::runtime_error ("LZ4: output buffer too small during decompression");

    std::memcpy ((char*) out, (const char*) in + 4, in_buffer_size);
    ret = in_buffer_size;
  }
  return ret;

#else

  throw std::runtime_error("Overpass API was compiled without lz4 compression library support");

#endif
}
