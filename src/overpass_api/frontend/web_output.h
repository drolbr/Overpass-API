/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__WEB_OUTPUT_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__WEB_OUTPUT_H

#include "../core/datatypes.h"

using namespace std;

struct Web_Output : public Error_Output
{
  Web_Output(uint log_level_) : http_method(http_get), has_origin(false), header_written(not_yet),
      encoding_errors(false), parse_errors(false), static_errors(false), log_level(log_level_) {}
  
  ~Web_Output() { write_footer(); }
  
  virtual void add_encoding_error(const string& error);
  virtual void add_parse_error(const string& error, int line_number);
  virtual void add_static_error(const string& error, int line_number);
  
  virtual void add_encoding_remark(const string& error);
  virtual void add_parse_remark(const string& error, int line_number);
  virtual void add_static_remark(const string& error, int line_number);
  
  virtual void runtime_error(const string& error);
  virtual void runtime_remark(const string& error);
  
  virtual void display_statement_progress
      (uint timer, const string& name, int progress, int line_number,
       const vector< pair< uint, uint > >& stack) {}
       
  virtual bool display_encoding_errors() { return encoding_errors; }
  virtual bool display_parse_errors() { return parse_errors; }
  virtual bool display_static_errors() { return static_errors; }
  
  virtual void add_padding(const string& padding_) { padding = padding_; }
  
  void enforce_header(uint write_mime);
  void write_html_header
      (const string& timestamp = "", const string& area_timestamp = "", uint write_mime = 200,
       bool write_js_init = false, bool write_remarks = true);
  void write_xml_header
      (const string& timestamp = "", const string& area_timestamp = "", bool write_mime = true);
  void write_json_header
      (const string& timestamp = "", const string& area_timestamp = "", bool write_mime = true);
  void write_footer();
  
public:
  typedef enum { http_get, http_post, http_head, http_options } Http_Methods;
  Http_Methods http_method;
  string allow_headers;
  bool has_origin;
  
private:
  enum { not_yet, xml, html, json, final } header_written;
  bool encoding_errors;
  bool parse_errors;
  bool static_errors;
  uint log_level;
  string padding;
  string messages;
  
  void display_remark(const string& text);
  void display_error(const string& text, uint write_mime);
};

#endif
