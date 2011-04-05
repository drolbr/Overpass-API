#include <iomanip>
#include <iostream>
#include <string>

#include "web_output.h"

using namespace std;

void Web_Output::add_encoding_error(const string& error)
{
  if (log_level != Error_Output::QUIET)
  {
    write_xml_header();
    cout<<"encoding error: "<<error<<'\n';
  }
  encoding_errors = true;
}

void Web_Output::add_parse_error(const string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
  {
    write_xml_header();
    cout<<"line "<<line_number<<": parse error: "<<error<<'\n';
  }
  parse_errors = true;
}

void Web_Output::add_static_error(const string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
  {
    write_xml_header();
    cout<<"line "<<line_number<<": static error: "<<error<<'\n';
  }
  static_errors = true;
}

void Web_Output::add_encoding_remark(const string& error)
{
  if (log_level == Error_Output::VERBOSE)
  {
    write_xml_header();
    cout<<"encoding remark: "<<error<<'\n';
  }
}

void Web_Output::add_parse_remark(const string& error, int line_number)
{
  if (log_level == Error_Output::VERBOSE)
  {
    write_xml_header();
    cout<<"line "<<line_number<<": parse remark: "<<error<<'\n';
  }
}

void Web_Output::add_static_remark(const string& error, int line_number)
{
  if (log_level == Error_Output::VERBOSE)
  {
    write_xml_header();
    cout<<"line "<<line_number<<": static remark: "<<error<<'\n';
  }
}

void Web_Output::runtime_error(const string& error)
{
  if (log_level != Error_Output::QUIET)
  {
    write_xml_header();
    cout<<"runtime error: "<<error<<'\n';
  }
}

void Web_Output::runtime_remark(const string& error)
{
  if (log_level == Error_Output::VERBOSE)
  {
    write_xml_header();
    cout<<"runtime remark: "<<error<<'\n';
  }
}

void Web_Output::display_statement_stopwatch
    (const string& name,
     const vector< double >& stopwatches,
     const vector< uint >& read_counts)
{
  if (log_level != Error_Output::VERBOSE)
    return;
  write_xml_header();
  cout<<"Stopwatch "<<name;
  vector< uint >::const_iterator rit(read_counts.begin());
  for (vector< double >::const_iterator it(stopwatches.begin());
      it != stopwatches.end(); ++it)
  {
    cout<<setprecision(3)<<fixed<<'\t'<<*it<<' '<<*rit;
    ++rit;
  }
  cout<<'\n';
}

void Web_Output::write_xml_header(string timestamp)
{
  if (header_written)
    return;    
  header_written = true;
  
  cout<<
  "Content-type: application/osm3s\n\n";
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-derived>\n"
  "<note>The data included in this document is from www.openstreetmap.org. "
  "It has there been collected by a large group of contributors. For individual "
  "attribution of each item please refer to "
  "http://www.openstreetmap.org/api/0.6/[node|way|relation]/#id/history </note>\n";
  cout<<
  "<meta data_included_until=\""<<timestamp<<
  "\" last_rule_applied=\""<<0<<"\"/>\n"
  "\n";
}

void Web_Output::write_xml_footer()
{
  cout<<"\n</osm-derived>\n";
}
