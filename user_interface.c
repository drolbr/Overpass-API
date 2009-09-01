#include <fstream>
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

string current_db;

string timestamp_of(uint32 timestamp)
{
  int month_borders[] =
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365,
  31+365, 59+365, 90+365, 120+365, 151+365, 181+365,
  212+365, 243+365, 273+365, 304+365, 334+365, 365+365,
  31+2*365, 59+2*365, 90+2*365, 120+2*365, 151+2*365, 181+2*365,
  212+2*365, 243+2*365, 273+2*365, 304+2*365, 334+2*365, 365+2*365,
  31+3*365, 60+3*365, 91+3*365, 121+3*365, 152+3*365, 182+3*365,
  213+3*365, 244+3*365, 274+3*365, 305+3*365, 335+3*365, 366+3*365 };
  
  int hour_number(timestamp);
  int day_number(hour_number / 24);
  int four_year_remainder(day_number % (4 * 365));
  int year(day_number / 365 / 4 + 2009);
  
  int month(1);
  while (four_year_remainder >= month_borders[month])
    ++month;
  
  hour_number = hour_number % 24;
  int day(four_year_remainder - month_borders[month-1] + 1);
  year += (month-1) / 12;
  month = (month-1) % 12 + 1;
  
  ostringstream temp;
  temp<<(long long)year*1000000 + month*10000 + day*100 + hour_number;
  return temp.str();
}

void out_header(int type)
{
  User_Output& out(get_output());
  
  ifstream status_in("/tmp/big_status");
  uint32 db_timestamp, db_rule_version;
  status_in>>db_timestamp;
  status_in>>db_timestamp;
  if (current_db == "osm_1")
  {
    status_in>>db_timestamp;
    status_in>>db_rule_version;
  }
  else if (current_db == "osm_2")
  {
    status_in>>db_timestamp;
    status_in>>db_timestamp;
    status_in>>db_timestamp;
    status_in>>db_timestamp;
    status_in>>db_rule_version;
  }
  
  if (header_state != 0)
    return;
  if (type == MIXED_XML)
  {
    out.print("Content-type: application/osm3s\n");
    out.finish_header();
    out.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-derived>\n"
	"<note>The data included in this document is from www.openstreetmap.org. "
	"It has there been collected by a large group of contributors. For individual "
	"attribution of each item please refer to "
	"http://www.openstreetmap.org/api/0.6/[node|way|relation]/#id/history </note>\n"
	"<meta data_included_until=\"");
    out.print(timestamp_of(db_timestamp));
    out.print("\" last_rule_applied=\"");
    out.print((long long)db_rule_version);
    out.print("\"/>\n\n");
    header_state = WRITTEN_XML;
  }
  else if (type == HTML)
  {
    header_state = WRITTEN_HTML;
    out.print("Content-Type: text/html; charset=utf-8\n");
    out.finish_header();
    out.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
	"    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
	"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
	"<head>\n"
	"  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
	"  <title>Query Results</title>\n"
	"</head>\n"
	"<body>\n\n"
	"<p>Data included until: ");
    out.print(timestamp_of(db_timestamp));
    out.print("<br/>Last rule applied: ");
    out.print((long long)db_rule_version);
    out.print("</p>\n\n");
  }
  else
  {
    header_state = WRITTEN_HTML;
    out.print("Content-Type: text/html; charset=utf-8\n");
    out.finish_header();
    out.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
	"    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
	"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
	"<head>\n"
	"  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
	"  <title>Nothing</title>\n"
	"</head>\n"
	"<body>\n\n"
	"<p>Data included until: ");
    out.print(timestamp_of(db_timestamp));
    out.print("<br/>Last rule applied: ");
    out.print((long long)db_rule_version);
    out.print("</p>\n"
	"<p>Your query doesn't contain any statement that produces output.</p>\n");
  }
  
  return;
}

void out_error_header(string title)
{
  User_Output& out(get_output());
  
  if (header_state != 0)
    return;
  header_state = WRITTEN_HTML;
  out.print("Content-Type: text/html; charset=utf-8\n");
  out.finish_header();
  out.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
      "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
      "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
      "<head>\n"
      "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
      "  <title>");
  out.print(title);
  out.print("</title>\n"
      "</head>\n"
      "<body>\n");
  return;
}

void out_footer(int type)
{
  User_Output& out(get_output());
  
  if (type == MIXED_XML)
    out.print("\n</osm-derived>\n");
  else
    out.print("\n</body>\n</html>\n");
  out.finish_output();
  return;
}

void out_error_footer()
{
  User_Output& out(get_output());
  
  out.print("</body>\n</html>\n");
  out.finish_output();
  return;
}

void out_input_tagged(const string& input, const vector< Error >& errors)
{
  User_Output& out(get_output());
  
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
  
  out.print("<a id=\"input\"><h2>Your input formatted for XML parsing</h2>\n");
  out.print("<pre>\n");
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
      out.print(escape_xml(input.substr(pos, new_pos - pos)));
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
      out.print("<strong style=\"color:#FF0000\">");
    else
      out.print("<strong style=\"color:#00BB00\">");
    new_pos = input.find('\n', pos);
    if (new_pos >= input_size)
      out.print(escape_xml(input.substr(pos)));
    else
      out.print(escape_xml(input.substr(pos, new_pos - pos)));
    out.print("</strong> (");
    out.print("<a href=\"#err");
    out.print(abs(it->second));
    out.print("\">");
    out.print(((it++)->second > 0 ? "E" : "R"));
    out.print("</a>");
    while ((it != error_numbers.end()) && (it->first == line))
    {
      out.print(", <a href=\"#err");
      out.print(abs(it->second));
      out.print("\">");
      out.print(((it++)->second > 0 ? "E" : "R"));
      out.print("</a>");
    }
    out.print(")\n");
    if ((new_pos >= input_size) || (new_pos == string::npos))
    {
      pos = input_size;
      break;
    }
    pos = new_pos + 1;
    ++line;
  }
  out.print(escape_xml(input.substr(pos)));
  out.print("</pre></a>\n");
  
  return;
}


int display_encoding_errors()
{
  User_Output& out(get_output());
  
  if (!exist_encoding_errors)
    return 0;
  
  out_error_header("Encoding Error");
  
  out.print("<h1>Encoding Error</h1>\n");
  out.print("<p>The server was unable to convert your input to an XML file.\n"
      "The following happened so far:</p>\n");
  
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(it->text);
    out.print("</p>\n");
  }
  
  out_error_footer();
  return 1;
}

int display_parse_errors(const string& input)
{
  User_Output& out(get_output());
  
  if (!exist_parse_errors)
    return 0;
  
  out_error_header("Parse Error");
  
  out.print("<h1>Parse Error</h1>\n");
  out.print("<p>The server has converted your input to this <a href=\"#input\">XML document</a>.\n"
      "Then, <a href=\"#parse\">these errors</a> happened. For your information,\n"
      "<a href=\"#encoding\">the remarks</a> from decoding your input are also included.</p>\n");
  
  out.print("<a id=\"encoding\"><h2>Encoding Remarks</h2>\n");
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(it->text);
    out.print("</p>\n");
  }
  out.print("</a>\n");
  
  out_input_tagged(input, parse_errors);

  unsigned int i(0);
  out.print("<a id=\"parse\"><h2>Parse Remarks and Errors</h2></a>\n");
  for (vector< Error >::const_iterator it(parse_errors.begin());
       it != parse_errors.end(); ++it)
  {
    out.print("<a id=\"err");
    out.print((long long)++i);
    out.print("\">");
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>");
    if (it->line_number > 0)
    {
      out.print(" in line <strong>");
      out.print(it->line_number);
      out.print("</strong>");
    }
    out.print(":\n");
    out.print(it->text);
    out.print("</p></a>\n");
  }
  
  out_error_footer();
  return 1;
}

int display_static_errors(const string& input)
{
  User_Output& out(get_output());
  
  if (!exist_static_errors)
    return 0;
  
  out_error_header("Static Error");
  
  out.print("<h1>Static Error</h1>\n");
  out.print("<p>The server has converted your input to this <a href=\"#input\">XML document</a>.\n"
      "It is well-formed XML but it has <a href=\"#static\">the following<a> semantic errors.\n"
      "For your information, <a href=\"#encoding\">the remarks</a> from decoding your input\n"
      "are also included.</p>\n");
  
  out.print("<a id=\"encoding\"><h2>Encoding Remarks</h2>\n");
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(it->text);
    out.print("</p>\n");
  }
  out.print("</a>\n");
  
  out_input_tagged(input, static_errors);

  unsigned int i(0);
  out.print("<a id=\"static\"><h2>Static Remarks and Errors</h2></a>\n");
  for (vector< Error >::const_iterator it(static_errors.begin());
       it != static_errors.end(); ++it)
  {
    out.print("<a id=\"err");
    out.print((long long)++i);
    out.print("\">");
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>");
    if (it->line_number > 0)
    {
      out.print(" in line <strong>");
      out.print(it->line_number);
      out.print("</strong>");
    }
    out.print(":\n");
    out.print(it->text);
    out.print("</p></a>\n");
  }
  
  out_error_footer();
  return 1;
}

int display_sanity_errors(const string& input)
{
  User_Output& out(get_output());
  
  if (!exist_sanity_errors)
    return 0;
  
  out_error_header("Flow Forecast Trouble");
  
  out.print("<h1>Flow Forecast Trouble</h1>\n");
  out.print("<p>The server has converted your input to this <a href=\"#input\">XML document</a>.\n"
      "To avoid congestions, the server tries a rough <a href=\"#flow\">forecast</a> "
      "for each script about its resource requirements. Unfortunatly, the server was unable "
      "to verify that your script could work on the server's resources. If you have a better "
      "prediction about your script's needs, try using the attributes \"timeout\" and/or "
      "\"element-limit\" on the root element.</p>\n");
  
  out.print("<a id=\"encoding\"><h2>Encoding Remarks</h2>\n");
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(it->text);
    out.print("</p>\n");
  }
  out.print("</a>\n");
  
  out_input_tagged(input, static_errors);

  unsigned int i(0);
  out.print("<a id=\"static\"><h2>Static Remarks</h2></a>\n");
  for (vector< Error >::const_iterator it(static_errors.begin());
       it != static_errors.end(); ++it)
  {
    out.print("<a id=\"err");
    out.print((long long)++i);
    out.print("\">");
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>");
    if (it->line_number > 0)
    {
      out.print(" in line <strong>");
      out.print(it->line_number);
      out.print("</strong>");
    }
    out.print(":\n");
    out.print(it->text);
    out.print("</p></a>\n");
  }
  
  out.print("<a id=\"flow\"><h2>Flow Forecast</h2></a>\n");
  out.print(sanity_out.str());
  
  out_error_footer();
  return 1;
}

void static_analysis(const string& input)
{
  User_Output& out(get_output());
  
  out_error_header("Static Script Analysis");
  
  out.print("<h1>Static Script Analysis</h1>\n");
  out.print("<p><a href=\"#input\">Your Input</a>.\n<br/>"
      "<p><a href=\"#encoding\">Encoding Remarks</a>.\n<br/>"
      "<p><a href=\"#static\">Static Remarks</a>.\n<br/>"
      "<p><a href=\"#flow\">Flow Forecast</a>.\n<br/></p>\n");
  
  out_input_tagged(input, static_errors);

  out.print("<a id=\"encoding\"><h2>Encoding Remarks</h2>\n");
  for (vector< Error >::const_iterator it(encoding_errors.begin());
       it != encoding_errors.end(); ++it)
  {
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(it->text);
    out.print("</p>\n");
  }
  out.print("</a>\n");
  
  unsigned int i(0);
  out.print("<a id=\"static\"><h2>Static Remarks</h2></a>\n");
  for (vector< Error >::const_iterator it(static_errors.begin());
       it != static_errors.end(); ++it)
  {
    out.print("<a id=\"err");
    out.print((long long)++i);
    out.print("\">");
    if (it->type == Error::ERROR)
      out.print("<p><strong style=\"color:#FF0000\">Error</strong>");
    else
      out.print("<p><strong style=\"color:#00BB00\">Remark</strong>");
    if (it->line_number > 0)
    {
      out.print(" in line <strong>");
      out.print(it->line_number);
      out.print("</strong>");
    }
    out.print(":\n");
    out.print(it->text);
    out.print("</p></a>\n");
  }
  
  out.print("<a id=\"flow\"><h2>Flow Forecast</h2></a>\n");
  out.print(sanity_out.str());
  
  out_error_footer();
}

void display_verbatim(const string& text)
{
  sanity_out<<"<pre>";
  escape_xml(sanity_out, text);
  sanity_out<<"</pre>\n";
}

void display_state(const string& text)
{
  sanity_out<<"<p><strong style=\"color:#00BB00\">Forecast</strong>: "<<text<<"</p>\n";
}

int debug_mode(0);

void runtime_error(const string& error)
{
  User_Output& out(get_output());
  
  if (debug_mode == QUIET)
    return;
  if (header_state == 0)
  {
    out_error_header("Runtime Error");
  
    out.print("<h1>Runtime Error</h1>\n");
    out.print("<p>The following error occured while running your script:</p>\n");
    out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    out.print(error);
    out.print("</p>\n");
  
    out_error_footer();
  }
  else if (header_state == WRITTEN_HTML)
  {
    out.print("<p><strong style=\"color:#FF0000\">Error</strong>: ");
    out.print(error);
    out.print("</p>\n");
  }
  else
  {
    out.print("<error type=\"runtime\">");
    out.print(error);
    out.print("</error>\n");
  }
}

void runtime_remark(const string& error)
{
  User_Output& out(get_output());
  
  if (debug_mode != VERBOSE)
    return;
  if (header_state == 0)
  {
    out_error_header("Runtime Error");
  
    out.print("<h1>Runtime Remark</h1>\n");
    out.print("<p>The following remark occured while running your script:</p>\n");
    out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(error);
    out.print("</p>\n");
    
    out_error_footer();
  }
  else if (header_state == WRITTEN_HTML)
  {
    out.print("<p><strong style=\"color:#00BB00\">Remark</strong>: ");
    out.print(error);
    out.print("</p>\n");
  }
  else
  {
    out.print("<remark type=\"runtime\">");
    out.print(error);
    out.print("</remark>\n");
  }
}

void statement_finished(const Statement* stmt)
{
  User_Output& out(get_output());
  
  if (debug_mode != VERBOSE)
    return;
  if (header_state == 0)
  {
    out_error_header("Runtime Error");
  
    out.print("<h1>Runtime Error</h1>\n");
    out.print("<p>The following error occured while running your script:</p>\n");
  
    out.print("<p><strong style=\"color:#00BB00\">Runtime state</strong>: "
	"Your program finished \"");
    out.print(stmt->get_name());
    out.print("\" from line ");
    out.print(stmt->get_line_number());
    out.print(" at ");
    out.print((long long)(uintmax_t)time(NULL));
    out.print(" seconds after Epoch.");
    out.print(" Stack: ");
    for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
	 it != get_stack().end(); ++it)
    {
      out.print(it->first);
      out.print(" ");
      out.print(it->second);
      out.print(" ");
    }
    out.print("</p>\n");
    
    out_error_footer();
  }
  else if (header_state == WRITTEN_HTML)
  {
    out.print("<p><strong style=\"color:#00BB00\">Runtime state</strong>: "
	"Your program finished \"");
    out.print(stmt->get_name());
    out.print("\" from line ");
    out.print(stmt->get_line_number());
    out.print(" at ");
    out.print((long long)(uintmax_t)time(NULL));
    out.print(" seconds after Epoch.");
    out.print(" Stack: ");
    for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
	 it != get_stack().end(); ++it)
    {
      out.print(it->first);
      out.print(" ");
      out.print(it->second);
      out.print(" ");
    }
    out.print("</p>\n");
  }
  else
  {
    out.print("<remark type=\"state\">: "
	"Your program finished \"");
    out.print(stmt->get_name());
    out.print("\" from line ");
    out.print(stmt->get_line_number());
    out.print(" at ");
    out.print((long long)(uintmax_t)time(NULL));
    out.print(" seconds after Epoch.");
    out.print(" Stack: ");
    for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
	 it != get_stack().end(); ++it)
    {
      out.print(it->first);
      out.print(" ");
      out.print(it->second);
      out.print(" ");
    }
    out.print("</remark>\n");
  }
}

void set_debug_mode(int mode)
{
  debug_mode = mode;
}

int get_debug_mode()
{
  return debug_mode;
}

void set_meta_db(string current_db_)
{
  current_db = current_db_;
}
