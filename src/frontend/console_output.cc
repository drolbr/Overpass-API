#include <iostream>
#include <string>

#include "console_output.h"

using namespace std;

void Console_Output::add_encoding_error(const string& error)
{
  cerr<<"encoding error: "<<error<<'\n';
  encoding_errors = true;
}

void Console_Output::add_parse_error(const string& error, int line_number)
{
  cerr<<"line "<<line_number<<": parse error: "<<error<<'\n';
  parse_errors = true;
}

void Console_Output::add_static_error(const string& error, int line_number)
{
  cerr<<"line "<<line_number<<": static error: "<<error<<'\n';
  static_errors = true;
}

void Console_Output::add_encoding_remark(const string& error)
{
  cerr<<"encoding remark: "<<error<<'\n';
}

void Console_Output::add_parse_remark(const string& error, int line_number)
{
  cerr<<"line "<<line_number<<": parse remark: "<<error<<'\n';
}

void Console_Output::add_static_remark(const string& error, int line_number)
{
  cerr<<"line "<<line_number<<": static remark: "<<error<<'\n';
}

void Console_Output::runtime_error(const string& error)
{
  cerr<<"runtime error: "<<error<<'\n';
}

void Console_Output::runtime_remark(const string& error)
{
  cerr<<"runtime remark: "<<error<<'\n';
}
