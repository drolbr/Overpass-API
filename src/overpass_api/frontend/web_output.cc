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



void Web_Output::add_encoding_error(const std::string& error)
{
  if (log_level != Error_Output::QUIET)
  {
    std::ostringstream out;
    out<<"encoding error: "<<error;
    display_error(out.str(), 400);
  }
  encoding_errors = true;
}


void Web_Output::add_parse_error(const std::string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
  {
    std::ostringstream out;
    out<<"line "<<line_number<<": parse error: "<<error;
    display_error(out.str(), 400);
  }
  parse_errors = true;
}


void Web_Output::add_static_error(const std::string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
  {
    std::ostringstream out;
    out<<"line "<<line_number<<": static error: "<<error;
    display_error(out.str(), 400);
  }
  static_errors = true;
}


void Web_Output::add_encoding_remark(const std::string& error)
{
  if (log_level == Error_Output::VERBOSE)
  {
    std::ostringstream out;
    out<<"encoding remark: "<<error;
    display_remark(out.str());
  }
}


void Web_Output::add_parse_remark(const std::string& error, int line_number)
{
  if (log_level == Error_Output::VERBOSE)
  {
    std::ostringstream out;
    out<<"line "<<line_number<<": parse remark: "<<error;
    display_remark(out.str());
  }
}


void Web_Output::add_static_remark(const std::string& error, int line_number)
{
  if (log_level == Error_Output::VERBOSE)
  {
    std::ostringstream out;
    out<<"line "<<line_number<<": static remark: "<<error;
    display_remark(out.str());
  }
}


void Web_Output::runtime_error(const std::string& error)
{
  if (log_level != Error_Output::QUIET)
  {
    std::ostringstream out;
    out<<"runtime error: "<<error;
    display_error(out.str(), 200);
  }
}


void Web_Output::runtime_remark(const std::string& error)
{
  if (log_level == Error_Output::VERBOSE)
  {
    std::ostringstream out;
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
    (const std::string& timestamp, const std::string& area_timestamp, uint write_mime, bool write_js_init,
     bool write_remarks)
{
  if (header_written != not_yet)
    return;
  header_written = html;

  if (write_mime)
  {
    if (allow_headers != "")
      std::cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      std::cout<<"Access-Control-Allow-Origin: *\n"
            "Access-Control-Max-Age: 600\n";
    if (http_method == http_options)
      std::cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    if (http_method == http_options || http_method == http_head)
    {
      std::cout<<"Content-type: text/html; charset=utf-8\n";
      return;
    }
  }
  ::write_html_header(timestamp, area_timestamp, write_mime, write_js_init, write_remarks);
}


void Web_Output::write_payload_header
    (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp, bool write_mime)
{
  if (header_written != not_yet)
    return;
  header_written = payload;

  if (write_mime)
  {
    if (allow_headers != "")
      std::cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
    if (has_origin)
      std::cout<<"Access-Control-Allow-Origin: *\n"
            "Access-Control-Max-Age: 600\n";
    if (http_method == http_options)
      std::cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
            "Content-Length: 0\n";
    if (!output_handler || output_handler->write_http_headers())
      std::cout<<'\n';
    if (http_method == http_options || http_method == http_head)
      return;
  }

  if (output_handler)
    output_handler->write_payload_header(db_dir, timestamp, area_timestamp);
}


// void Web_Output::write_json_header
//     (const std::string& timestamp, const std::string& area_timestamp, bool write_mime)
// {
//   if (header_written != not_yet)
//     return;
//   header_written = json;
//
//   if (write_mime)
//   {
//     if (allow_headers != "")
//       std::cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
//     if (has_origin)
//       std::cout<<"Access-Control-Allow-Origin: *\n"
//             "Access-Control-Max-Age: 600\n";
//     if (http_method == http_options)
//       std::cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
//             "Content-Length: 0\n";
//     std::cout<<"Content-type: application/json\n\n";
//     if (http_method == http_options || http_method == http_head)
//       return;
//   }
//
//   if (padding != "")
//     std::cout<<padding<<"(";
//
//   std::cout<<"{\n"
//         "  \"version\": 0.6,\n"
//         "  \"generator\": \"Overpass API\",\n"
//         "  \"osm3s\": {\n"
// 	"    \"timestamp_osm_base\": \""<<timestamp<<"\",\n";
//   if (area_timestamp != "")
//     std::cout<<"    \"timestamp_areas_base\": \""<<area_timestamp<<"\",\n";
//   std::cout<<"    \"copyright\": \"The data included in this document is from www.openstreetmap.org."
// 	" The data is made available under ODbL.\"\n"
//         "  },\n";
//  std::cout<< "  \"elements\": [\n\n";
// }


// void Web_Output::write_text_header
//     (const std::string& timestamp, const std::string& area_timestamp, bool write_mime)
// {
//   if (header_written != not_yet)
//     return;
//   header_written = text;
//
//   if (write_mime)
//   {
//     if (allow_headers != "")
//       std::cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
//     if (has_origin)
//       std::cout<<"Access-Control-Allow-Origin: *\n"
//             "Access-Control-Max-Age: 600\n";
//     if (http_method == http_options)
//       std::cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
//             "Content-Length: 0\n";
//     std::cout<<"Content-type: text/plain\n\n";
//     if (http_method == http_options || http_method == http_head)
//       return;
//   }
//
//   std::cout<<timestamp<<"\n";
// }


// void Web_Output::write_csv_header
//     (const std::string& timestamp, const std::string& area_timestamp, bool write_mime)
// {
//   if (header_written != not_yet)
//     return;
//   header_written = csv;
//
//   if (write_mime)
//   {
//     if (allow_headers != "")
//       std::cout<<"Access-Control-Allow-Headers: "<<allow_headers<<'\n';
//     if (has_origin)
//       std::cout<<"Access-Control-Allow-Origin: *\n";
//     if (http_method == http_options)
//       std::cout<<"Access-Control-Allow-Methods: GET, POST, OPTIONS\n"
//             "Content-Length: 0\n";
//     std::cout<<"Content-type: text/csv\n\n";
//     if (http_method == http_options || http_method == http_head)
//       return;
//   }
// }


void Web_Output::write_footer()
{
  if (header_written == html)
    std::cout<<"\n</body>\n</html>\n";
  else if (header_written != final && output_handler)
    output_handler->write_footer();

  header_written = final;
}


void Web_Output::display_remark(const std::string& text)
{
  enforce_header(200);
  if (http_method == http_options || http_method == http_head)
    return;
  if (header_written == html)
    std::cout<<"<p><strong style=\"color:#00BB00\">Remark</strong>: "
        <<text<<" </p>\n";
  else if (output_handler)
    output_handler->display_remark(text);
}


void Web_Output::display_error(const std::string& text, uint write_mime)
{
  enforce_header(write_mime);
  if (http_method == http_options || http_method == http_head)
    return;
  if (header_written == html)
    std::cout<<"<p><strong style=\"color:#FF0000\">Error</strong>: "
        <<text<<" </p>\n";
  else if (output_handler)
    output_handler->display_error(text);
}
