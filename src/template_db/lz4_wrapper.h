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

#ifndef DE__OSM3S___TEMPLATE_DB__LZ4_WRAPPER_H
#define DE__OSM3S___TEMPLATE_DB__LZ4_WRAPPER_H

#include <exception>

#include <lz4.h>


class LZ4_Deflate
{
public:
  struct Error : public std::exception
  {
    Error(int error_code_) : error_code(error_code_) {}
    virtual const char* what() const throw();
    int error_code;
  };

  explicit LZ4_Deflate();
  ~LZ4_Deflate();

  int compress(const void* in, int in_size, void* out, int out_buffer_size);
};


class LZ4_Inflate
{
public:
  struct Error : public std::exception
  {
    Error(int error_code_) : error_code(error_code_) {}
    virtual const char* what() const throw();
    int error_code;
  };

  LZ4_Inflate();
  ~LZ4_Inflate();
  
  int decompress(const void* in, int in_size, void* out, int out_buffer_size);
};


#endif
