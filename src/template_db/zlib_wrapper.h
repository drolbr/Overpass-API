#ifndef DE__OSM3S___TEMPLATE_DB__ZLIB_WRAPPER_H
#define DE__OSM3S___TEMPLATE_DB__ZLIB_WRAPPER_H

#include "zlib.h"

#include <exception>


class Zlib_Deflate
{
public:
  struct Error : public std::exception
  {
    Error(int error_code_) : error_code(error_code_) {}
    virtual const char* what() const throw();
    int error_code;
  };

  explicit Zlib_Deflate(int level);
  ~Zlib_Deflate();
  
  int compress(const void* in, int in_size, void* out, int out_buffer_size);
  
private:
  z_stream strm;
};


class Zlib_Inflate
{
public:
  struct Error : public std::exception
  {
    Error(int error_code_) : error_code(error_code_) {}
    virtual const char* what() const throw();
    int error_code;
  };

  Zlib_Inflate();
  ~Zlib_Inflate();
  
  int decompress(const void* in, int in_size, void* out, int out_buffer_size);
  
private:
  z_stream strm;
};


#endif
