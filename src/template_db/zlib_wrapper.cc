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

#include "zlib_wrapper.h"

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>


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


Zlib_Deflate::Error::Error(int error_code_)
    : std::runtime_error("Zlib_Deflate: " + to_string(error_code_)), error_code(error_code_)
{}


Zlib_Deflate::Zlib_Deflate(int level)
{
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  int ret = deflateInit(&strm, level);
  if (ret != Z_OK)
    throw Error(ret);
}


Zlib_Deflate::~Zlib_Deflate()
{
  deflateEnd(&strm);
}


int Zlib_Deflate::compress(const void* in, int in_size, void* out, int out_buffer_size)
{
  strm.avail_in = in_size;
  strm.next_in = (unsigned char*) in;
  strm.avail_out = out_buffer_size;
  strm.next_out = (unsigned char*) out;

  int ret = deflate(&strm, Z_FINISH);

  if (strm.avail_out == 0 || strm.avail_in != 0 || ret != Z_STREAM_END)
    throw Error(Z_BUF_ERROR); // Too few output space

  return out_buffer_size - strm.avail_out;
}


Zlib_Inflate::Error::Error(int error_code_)
    : std::runtime_error("Zlib_Inflate: " + to_string(error_code_)), error_code(error_code_)
{}


Zlib_Inflate::Zlib_Inflate()
{
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  int ret = inflateInit(&strm);
  if (ret != Z_OK)
    throw Error(ret);
}


Zlib_Inflate::~Zlib_Inflate()
{
  inflateEnd(&strm);
}


int Zlib_Inflate::decompress(const void* in, int in_size, void* out, int out_buffer_size)
{
  strm.avail_in = in_size;
  strm.next_in = (unsigned char*) in;
  strm.avail_out = out_buffer_size;
  strm.next_out = (unsigned char*) out;

  int ret = inflate(&strm, Z_FINISH);

  if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
    throw Error(ret);
  if (ret != Z_STREAM_END)
    throw Error(Z_BUF_ERROR); // Decompression incomplete

  return out_buffer_size - strm.avail_out;
}
