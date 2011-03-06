#include <iomanip>
#include <iostream>
#include <string>

#include "console_output.h"

using namespace std;

void Console_Output::add_encoding_error(const string& error)
{
  if (!quiet)
    cerr<<"encoding error: "<<error<<'\n';
  encoding_errors = true;
}

void Console_Output::add_parse_error(const string& error, int line_number)
{
  if (!quiet)
    cerr<<"line "<<line_number<<": parse error: "<<error<<'\n';
  parse_errors = true;
}

void Console_Output::add_static_error(const string& error, int line_number)
{
  if (!quiet)
    cerr<<"line "<<line_number<<": static error: "<<error<<'\n';
  static_errors = true;
}

void Console_Output::add_encoding_remark(const string& error)
{
  if (!quiet)
    cerr<<"encoding remark: "<<error<<'\n';
}

void Console_Output::add_parse_remark(const string& error, int line_number)
{
  if (!quiet)
    cerr<<"line "<<line_number<<": parse remark: "<<error<<'\n';
}

void Console_Output::add_static_remark(const string& error, int line_number)
{
  if (!quiet)
    cerr<<"line "<<line_number<<": static remark: "<<error<<'\n';
}

void Console_Output::runtime_error(const string& error)
{
  if (!quiet)
    cerr<<"runtime error: "<<error<<'\n';
}

void Console_Output::runtime_remark(const string& error)
{
  if (!quiet)
    cerr<<"runtime remark: "<<error<<'\n';
}
  
void Console_Output::display_statement_stopwatch
    (const string& name,
     const vector< double >& stopwatches,
     const vector< uint >& read_counts)
{
  if (quiet)
    return;
  cerr<<"Stopwatch "<<name;
  vector< uint >::const_iterator rit(read_counts.begin());
  for (vector< double >::const_iterator it(stopwatches.begin());
      it != stopwatches.end(); ++it)
  {
    cerr<<setprecision(3)<<fixed<<'\t'<<*it<<' '<<*rit;
    ++rit;
  }
  cerr<<'\n';
}
