#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdlib.h>
#include "cgi-helper.h"
#include "expat_justparse_interface.h"
#include "user_interface.h"

using namespace std;

string get_xml_raw()
{
  int line_number(1);
  // If there is nonempty input from GET method, use GET
  string input(cgi_get_to_text());
  unsigned int pos(0);
  while ((pos < input.size()) && (isspace(input[pos])))
  {
    if (input[pos] == '\n')
      ++line_number;
    ++pos;
  }
  
  if (pos == input.size())
  {
    // otherwise use POST input
    if (pos == 0)
      add_encoding_remark("No input found from GET method. Trying to retrieve input by POST method.");
    else
      add_encoding_remark("Only whitespace found from GET method. Trying to retrieve input by POST method.");
    
    input = cgi_post_to_text();
    pos = 0;
    line_number = 1;
    while ((pos < input.size()) && (isspace(input[pos])))
    {
      if (input[pos] == '\n')
	++line_number;
      ++pos;
    }
  
    if (pos == input.size())
    {
      if (pos == 0)
	add_encoding_error("Your input is empty.");
      else
	add_encoding_error("Your input contains only whitespace.");
      return "";
    }
  }
  else
    add_encoding_remark("Found input from GET method. Thus, any input from POST is ignored.");

  // pos now points at the first non-whitespace character
  if (input[pos] != '<')
    // reduce input to the part between "data=" and "&"
    // and remove the character escapings from the form
  {
    add_encoding_remark("The server now removes the CGI character escaping.");
    int cgi_error(0);
    input = decode_cgi_to_plain(input, cgi_error);
    
    // no 'data=' found
    if (cgi_error)
    {
      add_encoding_error("Your input can neither be interpreted verbatim nor does it contain the string \"data=\".");
      return "";
    }
    
    pos = 0;
    line_number = 1;
    while ((pos < input.size()) && (isspace(input[pos])))
    {
      if (input[pos] == '\n')
	++line_number;
      ++pos;
    }
    
    // the input contains at most whitespace
    if (pos == input.size())
    {
      if (pos == 0)
	add_encoding_error("Your input is empty.");
      else
	add_encoding_error("Your input contains only whitespace.");
      return "";
    }
  }
  else
    add_encoding_remark("The first non-whitespace character is '<'. Thus, your input will be interpreted verbatim.");
  
  // assert length restriction
  if (input.size() > 1048576)
  {
    ostringstream temp;
    temp<<"Input too long (length: "<<input.size()<<", max. allowed: "<<1048576<<')';
    add_encoding_error(temp.str());
    return input;
  }
  
  // pos again points at the first non-whitespace character
  if (input.substr(pos, 11) == "<osm-script")
    // add a header line and remove trailing whitespace
  {
    ostringstream temp;
    temp<<"Your input starts with a 'osm-script' tag. Thus, a line with the\n"
	<<"datatype declaration is added. This shifts line numbering by "
	<<line_number - 2<<" line(s).";
    add_encoding_remark(temp.str());
    
    input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	+ input.substr(pos);
/*    set_line_offset(line_number - 2);*/
  }
  else if (input.substr(pos, 2) != "<?")
    // add a header line, the root tag 'osm-script' and remove trailing whitespace
  {
    ostringstream temp;
    temp<<"Your input starts with a tag but not the root tag. Thus, a line with the\n"
	<<"datatype declaration and a line with the root tag 'osm-script' is\n"
	<<"added. This shifts line numbering by "<<line_number-3<<" line(s).";
    add_encoding_remark(temp.str());
    
    input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-script>\n"
	+ input.substr(pos) + "\n</osm-script>\n";
/*    set_line_offset(line_number - 3);*/
  }

  return input;
}

//-----------------------------------------------------------------------------

struct Error
{
  public:
    Error(string text_, int type_, int line_number_)
  : text(text_), type(type_), line_number(line_number_) {}
    
    static const int ERROR = 1;
    static const int REMARK = 2;
    
    string text;
    int type;
    int line_number;
};

static vector< Error > encoding_errors;
bool exist_encoding_errors(false);
static vector< Error > parse_errors;
bool exist_parse_errors(false);
static vector< Error > static_errors;
bool exist_static_errors(false);
ostringstream sanity_out;
bool exist_sanity_errors(false);

static int header_state(0);
static const int WRITTEN_XML = 1;
static const int WRITTEN_HTML = 2;

void add_encoding_error(const string& error)
{
  encoding_errors.push_back(Error(error, Error::ERROR, 0));
  exist_encoding_errors = true;
}

void add_parse_error(const string& error)
{
  parse_errors.push_back(Error(error, Error::ERROR, current_line_number()));
  exist_parse_errors = true;
}

void add_static_error(const string& error)
{
  static_errors.push_back(Error(error, Error::ERROR, current_line_number()));
  exist_static_errors = true;
}

void add_sanity_error(const string& error)
{
  sanity_out<<"<p><strong style=\"color:#FF0000\">Error</strong>: "<<error<<"</p>\n";
  exist_sanity_errors = true;
}

void add_encoding_remark(const string& error)
{
  encoding_errors.push_back(Error(error, Error::REMARK, 0));
}

void add_parse_remark(const string& error)
{
  parse_errors.push_back(Error(error, Error::REMARK, current_line_number()));
}

void add_static_remark(const string& error)
{
  static_errors.push_back(Error(error, Error::REMARK, current_line_number()));
}

void add_sanity_remark(const string& error)
{
  sanity_out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: "<<error<<"</p>\n";
}

ostream& out_header(ostream& out, int type)
{
  if (header_state != 0)
    return out;
  if (type == MIXED_XML)
  {
    out<<"Content-type: application/xml\n\n"
	<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not-osm>\n\n";
    header_state = WRITTEN_XML;
  }
  else if (type == HTML)
  {
    header_state = WRITTEN_HTML;
    out<<"Content-Type: text/html; charset=utf-8\n\n"
	<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
	<<"    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
	<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
	<<"<head>\n"
	<<"  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
	<<"  <title>Query Results</title>\n"
	<<"</head>\n"
	<<"<body>\n\n";
  }
  else
  {
    header_state = WRITTEN_HTML;
    out<<"Content-Type: text/html; charset=utf-8\n\n"
	<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
	<<"    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
	<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
	<<"<head>\n"
	<<"  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
	<<"  <title>Nothing</title>\n"
	<<"</head>\n"
	<<"<body>\n\n"
	<<"Your query doesn't contain any statement that produces output.\n";
  }
  
  return out;
}

ostream& out_error_header(ostream& out, string title)
{
  if (header_state != 0)
    return out;
  header_state = WRITTEN_HTML;
  out<<"Content-Type: text/html; charset=utf-8\n\n"
      <<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      <<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
      <<"    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
      <<"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
      <<"<head>\n"
      <<"  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
      <<"  <title>"<<title<<"</title>\n"
      <<"</head>\n"
      <<"<body>\n";
  return out;
}

ostream& out_footer(ostream& out, int type)
{
  if (type == MIXED_XML)
    out<<"\n</not-osm>\n";
  else
    out<<"\n</body>\n</html>\n";
  return out;
}

ostream& out_error_footer(ostream& out)
{
  out<<"</body>\n</html>\n";
  return out;
}

ostream& out_input_tagged(ostream& out, const string& input, const vector< Error >& errors)
{
  // collect line numbers with errors
  int i(0);
  multimap< int, int > error_numbers;
  for (vector< Error >::const_iterator it(errors.begin());
       it != errors.end(); ++it)
  {
    if (it->line_number > 0)
    {
      if (it->type == Error::ERROR)
	error_numbers.insert(make_pair< int, int >(it->line_number, ++i));
      else
	error_numbers.insert(make_pair< int, int >(it->line_number, -(++i)));
    }
  }
  
  out<<"<a id=\"input\"><h2>Your input formatted for XML parsing</h2>\n";
  out<<"<pre>\n";
  // mark line numbers with errors
  int line(1);
  string::size_type pos(0), new_pos(0), input_size(input.size());
  for (multimap< int, int >::const_iterator it(error_numbers.begin());
       it != error_numbers.end(); )
  {
    while (line < it->first)
    {
      new_pos = input.find('\n', pos)+1;
      if ((new_pos >= input_size) || (new_pos == string::npos))
      {
	pos = input_size;
	break;
      }
      escape_xml(out, input.substr(pos, new_pos - pos));
      pos = new_pos;
      ++line;
    }
    if ((new_pos >= input_size) || (new_pos == string::npos))
      break;
    bool is_error(false);
    multimap< int, int >::const_iterator it2(it);
    while ((it2 != error_numbers.end()) && (it2->first == line))
      is_error = ((it2++)->second > 0);
    if (is_error)
      out<<"<strong style=\"color:#FF0000\">";
    else
      out<<"<strong style=\"color:#00BB00\">";
    new_pos = input.find('\n', pos);
    if (new_pos >= input_size)
      escape_xml(out, input.substr(pos));
    else
      escape_xml(out, input.substr(pos, new_pos - pos));
    out<<"</strong> (";
    out<<"<a href=\"#err"<<abs(it->second)<<"\">";
    out<<((it++)->second > 0 ? "E" : "R");
    out<<"</a>";
    while ((it != error_numbers.end()) && (it->first == line))
    {
      out<<", <a href=\"#err"<<abs(it->second)<<"\">";
      out<<((it++)->second > 0 ? "E" : "R");
      out<<"</a>";
    }
    out<<")\n";
    if ((new_pos >= input_size) || (new_pos == string::npos))
    {
      pos = input_size;
      break;
    }
    pos = new_pos + 1;
    ++line;
  }
  escape_xml(out, input.substr(pos));
  out<<"</pre></a>\n";
  
  return out;
}


int display_encoding_errors(ostream& out)
{
  if (!exist_encoding_errors)
    return 0;
  
  out_error_header(out, "Encoding Error");
  
  out<<"<h1>Encoding Error</h1>\n";
  out<<"<p>The server was unable to convert your input to an XML file.\n"
      <<"The following happened so far:</p>\n";
  
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>: ";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: ";
    out<<it->text<<"</p>\n";
  }
  
  out_error_footer(out);
  return 1;
}

int display_parse_errors(ostream& out, const string& input)
{
  if (!exist_parse_errors)
    return 0;
  
  out_error_header(out, "Parse Error");
  
  out<<"<h1>Parse Error</h1>\n";
  out<<"<p>The server has converted your input to this <a href=\"#input\">XML document</a>.\n"
      <<"Then, <a href=\"#parse\">these errors</a> happened. For your information,\n"
      <<"<a href=\"#encoding\">the remarks</a> from decoding your input are also included.</p>\n";
  
  out<<"<a id=\"encoding\"><h2>Encoding Remarks</h2>\n";
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>: ";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: ";
    out<<it->text<<"</p>\n";
  }
  out<<"</a>\n";
  
  out_input_tagged(out, input, parse_errors);

  unsigned int i(0);
  out<<"<a id=\"parse\"><h2>Parse Remarks and Errors</h2></a>\n";
  for (vector< Error >::const_iterator it(parse_errors.begin());
       it != parse_errors.end(); ++it)
  {
    out<<"<a id=\"err"<<++i<<"\">";
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>";
    if (it->line_number > 0)
      out<<" in line <strong>"<<it->line_number<<"</strong>";
    out<<":\n"<<it->text<<"</p></a>\n";
  }
  
  out_error_footer(out);
  return 1;
}

int display_static_errors(ostream& out, const string& input)
{
  if (!exist_static_errors)
    return 0;
  
  out_error_header(out, "Static Error");
  
  out<<"<h1>Static Error</h1>\n";
  out<<"<p>The server has converted your input to this <a href=\"#input\">XML document</a>.\n"
      <<"It is well-formed XML but it has <a href=\"#static\">the following<a> semantic errors.\n"
      <<"For your information, <a href=\"#encoding\">the remarks</a> from decoding your input\n"
      <<"are also included.</p>\n";
  
  out<<"<a id=\"encoding\"><h2>Encoding Remarks</h2>\n";
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>: ";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: ";
    out<<it->text<<"</p>\n";
  }
  out<<"</a>\n";
  
  out_input_tagged(out, input, static_errors);

  unsigned int i(0);
  out<<"<a id=\"static\"><h2>Static Remarks and Errors</h2></a>\n";
  for (vector< Error >::const_iterator it(static_errors.begin());
       it != static_errors.end(); ++it)
  {
    out<<"<a id=\"err"<<++i<<"\">";
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>";
    if (it->line_number > 0)
      out<<" in line <strong>"<<it->line_number<<"</strong>";
    out<<":\n"<<it->text<<"</p></a>\n";
  }
  
  out_error_footer(out);
  return 1;
}

int display_sanity_errors(ostream& out, const string& input)
{
/*  if (!exist_sanity_errors)
    return 0;*/
  
  out_error_header(out, "Flow Forecast Trouble");
  
  out<<"<h1>Flow Forecast Trouble</h1>\n";
  out<<"<p>The server has converted your input to this <a href=\"#input\">XML document</a>.\n"
      <<"To avoid congestions, the server tries a rough <a href=\"#flow\">forecast</a> "
      <<"for each script about its resource requirements. Unfortunatly, the server was unable "
      <<"to verify that your script could work on the server's resources. If you have a better "
      <<"prediction about your script's needs, try using the attributes \"timeout\" and/or "
      <<"\"element-limit\" on the root element.</p>\n";
  
  out<<"<a id=\"encoding\"><h2>Encoding Remarks</h2>\n";
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>: ";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: ";
    out<<it->text<<"</p>\n";
  }
  out<<"</a>\n";
  
  out_input_tagged(out, input, static_errors);

  unsigned int i(0);
  out<<"<a id=\"static\"><h2>Static Remarks</h2></a>\n";
  for (vector< Error >::const_iterator it(static_errors.begin());
       it != static_errors.end(); ++it)
  {
    out<<"<a id=\"err"<<++i<<"\">";
    if (it->type == Error::ERROR)
      out<<"<p><strong style=\"color:#FF0000\">Error</strong>";
    else
      out<<"<p><strong style=\"color:#00BB00\">Remark</strong>";
    if (it->line_number > 0)
      out<<" in line <strong>"<<it->line_number<<"</strong>";
    out<<":\n"<<it->text<<"</p></a>\n";
  }
  
  out<<"<a id=\"flow\"><h2>Flow Forecast</h2></a>\n";
  out<<sanity_out.str();
  
  out_error_footer(out);
  return 1;
}

void display_verbatim(const string& text, ostream& out)
{
  sanity_out<<"<pre>";
  escape_xml(sanity_out, text);
  sanity_out<<"</pre>\n";
}

void display_state(const string& text, ostream& out)
{
  sanity_out<<"<p><strong style=\"color:#00BB00\">Forecast</strong>: "<<text<<"</p>\n";
}

void runtime_error(const string& error, ostream& out)
{
  if (header_state == 0)
  {
    out_error_header(out, "Runtime Error");
  
    out<<"<h1>Runtime Error</h1>\n";
    out<<"<p>The following error occured while running your script:</p>\n";
    out<<"<p><strong style=\"color:#FF0000\">Error</strong>: "<<error<<"</p>\n";
  
    out_error_footer(out);
  }
  else if (header_state == WRITTEN_HTML)
  {
    out<<"<p><strong style=\"color:#FF0000\">Error</strong>: "<<error<<"</p>\n";
  }
  else
  {
    out<<"<error type=\"runtime\">"<<error<<"</error>\n";
  }
}

void runtime_remark(const string& error, ostream& out)
{
  if (header_state == 0)
  {
    out_error_header(out, "Runtime Error");
  
    out<<"<h1>Runtime Error</h1>\n";
    out<<"<p>The following error occured while running your script:</p>\n";
    out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: "<<error<<"</p>\n";
  
    out_error_footer(out);
  }
  else if (header_state == WRITTEN_HTML)
  {
    out<<"<p><strong style=\"color:#00BB00\">Remark</strong>: "<<error<<"</p>\n";
  }
  else
  {
    out<<"<remark type=\"runtime\">"<<error<<"</remark>\n";
  }
}

void statement_finished(const Statement* stmt)
{
  if (header_state == 0)
  {
    out_error_header(cout, "Runtime Error");
  
    cout<<"<h1>Runtime Error</h1>\n";
    cout<<"<p>The following error occured while running your script:</p>\n";
  
    out_error_footer(cout);
  }
  else if (header_state == WRITTEN_HTML)
  {
    cout<<"<p><strong style=\"color:#00BB00\">Runtime state</strong>: "
	<<"Your program finished \""<<stmt->get_name()<<"\" from line "<<stmt->get_line_number()
	<<" at "<<(uintmax_t)time(NULL)<<" seconds after Epoch.";
    cout<<" Stack: ";
    for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
	 it != get_stack().end(); ++it)
      cout<<it->first<<' '<<it->second<<' ';
    cout<<"</p>\n";
  }
  else
  {
    cout<<"<remark type=\"state\">: "
	<<"Your program finished \""<<stmt->get_name()<<"\" from line "<<stmt->get_line_number()
	<<" at "<<(uintmax_t)time(NULL)<<" seconds after Epoch.";
    cout<<" Stack: ";
    for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
	 it != get_stack().end(); ++it)
      cout<<it->first<<' '<<it->second<<' ';
    cout<<"</remark>\n";
  }
}
