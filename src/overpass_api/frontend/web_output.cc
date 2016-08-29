/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#include "output.h"
#include "web_output.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

void Web_Output::add_encoding_error(const string& error)
{
  if (log_level != Error_Output::QUIET)
  {
    ostringstream out;
    out<<"encoding error: "<<error;
    display_error(out.str(), 400);
  }
  encoding_errors = true;
}

void Web_Output::add_parse_error(const string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
  {
    ostringstream out;
    out<<"line "<<line_number<<": parse error: "<<error;
    display_error(out.str(), 400);
  }
  parse_errors = true;
}

void Web_Output::add_static_error(const string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
  {
    ostringstream out;
    out<<"line "<<line_number<<": static error: "<<error;
    display_error(out.str(), 400);
  }
  static_errors = true;
}

void Web_Output::add_encoding_remark(const string& error)
{
  if (log_level == Error_Output::VERBOSE)
  {
    ostringstream out;
    out<<"encoding remark: "<<error;
    display_remark(out.str());
  }
}

void Web_Output::add_parse_remark(const string& error, int line_number)
{
  if (log_level == Error_Output::VERBOSE)
  {
    ostringstream out;
    out<<"line "<<line_number<<": parse remark: "<<error;
    display_remark(out.str());
  }
}

void Web_Output::add_static_remark(const string& error, int line_number)
{
  if (log_level == Error_Output::VERBOSE)
  {
    ostringstream out;
    out<<"line "<<line_number<<": static remark: "<<error;
    display_remark(out.str());
  }
}

void Web_Output::runtime_error(const string& error)
{
  if (log_level != Error_Output::QUIET)
  {
    ostringstream out;
    out<<"runtime error: "<<error;
    display_error(out.str(), 200);
  }
}

void Web_Output::runtime_remark(const string& error)
{
  if (log_level == Error_Output::VERBOSE)
  {
    ostringstream out;
    out<<"runtime remark: "<<error;
    display_remark(out.str());
  }
}

void Web_Output::enforce_header(uint write_mime)
{
  if (header_written == not_yet)
    write_html_header("", "", write_mime);
}

void Web_Output::write_html_header
    (const string& timestamp, const string& area_timestamp, uint write_mime, bool write_js_init,
     bool write_remarks)
{
  if (header_written != not_yet)
    return;    
  header_written = html;
  
  if (write_mime > 0)
  {
    if (write_mime != 200)
    {
      if (write_mime == 504)
        cout<<"Status: "<<write_mime<<" Gateway Timeout\n";
      else if (write_mime == 400)
        cout<<"Status: "<<write_mime<<" Bad Request\n";
      else if (write_mime == 429)
        cout<<"Status: "<<write_mime<<" Too Many Requests\n";
      else
        cout<<"Status: "<<write_mime<<"\n";
    }
    if (allow_headers != "")
      cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      cout<<"Access-Control-Allow-Origin: *\n"
            "Access-Control-Max-Age: 600\n";
    if (http_method == http_options)
      cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    cout<<"Content-type: text/html; charset=utf-8\n\n";
    if (http_method == http_options || http_method == http_head)
      return;
  }
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
  "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
  "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
  "<head>\n"
  "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
  "  <title>OSM3S Response</title>\n"
  "</head>\n";
  cout<<(write_js_init ? "<body onload=\"init()\">\n\n" : "<body>\n\n");
  if (write_remarks)
  {
    cout<<
    "<p>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</p>\n";
    if (timestamp != "")
    {
      cout<<"<p>Data included until: "<<timestamp;
      if (area_timestamp != "")
        cout<<"<br/>Areas based on data until: "<<area_timestamp;
      cout<<"</p>\n";
    }
  }
}

void Web_Output::write_xml_header
    (const string& timestamp, const string& area_timestamp, bool write_mime)
{
  if (header_written != not_yet)
    return;    
  header_written = xml;
  
  if (write_mime)
  {
    if (allow_headers != "")
      cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      cout<<"Access-Control-Allow-Origin: *\n"
            "Access-Control-Max-Age: 600\n";
    if (http_method == http_options)
      cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    cout<<"Content-type: application/osm3s+xml\n\n";
    if (http_method == http_options || http_method == http_head)
      return;
  }
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm version=\"0.6\" generator=\"Overpass API\">\n"
  "<note>The data included in this document is from www.openstreetmap.org. "
  "The data is made available under ODbL.</note>\n";
  cout<<"<meta osm_base=\""<<timestamp<<'\"';
  if (area_timestamp != "")
    cout<<" areas=\""<<area_timestamp<<"\"";
  cout<<"/>\n\n";
}

void Web_Output::write_json_header
    (const string& timestamp, const string& area_timestamp, bool write_mime)
{
  if (header_written != not_yet)
    return;    
  header_written = json;
  
  if (write_mime)
  {
    if (allow_headers != "")
      cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      cout<<"Access-Control-Allow-Origin: *\n"
            "Access-Control-Max-Age: 600\n";
    if (http_method == http_options)
      cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    cout<<"Content-type: application/json\n\n";
    if (http_method == http_options || http_method == http_head)
      return;
  }

  if (padding != "")
    cout<<padding<<"(";
    
  cout<<"{\n"
        "  \"version\": 0.6,\n"
        "  \"generator\": \"Overpass API\",\n"
        "  \"osm3s\": {\n"
	"    \"timestamp_osm_base\": \""<<timestamp<<"\",\n";
  if (area_timestamp != "")
    cout<<"    \"timestamp_areas_base\": \""<<area_timestamp<<"\",\n";
  cout<<"    \"copyright\": \"The data included in this document is from www.openstreetmap.org."
	" The data is made available under ODbL.\"\n"
        "  },\n";
//  cout<< "  \"elements\": [\n\n";
}


void Web_Output::write_text_header
    (const string& timestamp, const string& area_timestamp, bool write_mime)
{
  if (header_written != not_yet)
    return;
  header_written = text;
  
  if (write_mime)
  {
    if (allow_headers != "")
      cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      cout<<"Access-Control-Allow-Origin: *\n"
            "Access-Control-Max-Age: 600\n";
    if (http_method == http_options)
      cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    cout<<"Content-type: text/plain\n\n";
    if (http_method == http_options || http_method == http_head)
      return;
  }

  cout<<timestamp<<"\n";
}

void Web_Output::write_csv_header
    (const string& timestamp, const string& area_timestamp, bool write_mime)
{
  if (header_written != not_yet)
    return;
  header_written = csv;
  
  if (write_mime)
  {
    if (allow_headers != "")
      cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      cout<<"Access-Control-Allow-Origin: *\n";
    if (http_method == http_options)
      cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    cout<<"Content-type: text/csv\n\n";
    if (http_method == http_options || http_method == http_head)
      return;
  }
}


void Web_Output::write_footer()
{
  if (http_method == http_options || http_method == http_head)
    return;
  if (header_written == xml)
    cout<<"\n</osm>\n";
  else if (header_written == html)
    cout<<"\n</body>\n</html>\n";
  else if (header_written == json)
    cout<<"\n\n  ]"<<(messages != "" ? ",\n\"remark\": \"" + escape_cstr(messages) + "\"" : "")
        <<"\n}"<<(padding != "" ? ");\n" : "\n");
  header_written = final;
}

void Web_Output::display_remark(const string& text)
{
  enforce_header(200);
  if (http_method == http_options || http_method == http_head)
    return;
  if (header_written == xml)
    cout<<"<remark> "<<text<<" </remark>\n";
  else if (header_written == html)
    cout<<"<p><strong style=\"color:#00BB00\">Remark</strong>: "
        <<text<<" </p>\n";
}

void Web_Output::display_error(const string& text, uint write_mime)
{
  enforce_header(write_mime);
  if (http_method == http_options || http_method == http_head)
    return;
  if (header_written == xml)
    cout<<"<remark> "<<text<<" </remark>\n";
  else if (header_written == html)
    cout<<"<p><strong style=\"color:#FF0000\">Error</strong>: "
        <<text<<" </p>\n";
  else if (header_written == json)
    messages += text;
}
