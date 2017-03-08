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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__WEB_OUTPUT_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__WEB_OUTPUT_H


#include "../core/datatypes.h"
#include "basic_formats.h"
#include "output_handler.h"


struct Web_Output : public Error_Output
{
  Web_Output(uint log_level_) : http_method(http_get), has_origin(false), header_written(not_yet),
      encoding_errors(false), parse_errors(false), static_errors(false), log_level(log_level_),
      output_handler(0) {}
  
  ~Web_Output() { write_footer(); }
  
  virtual void add_encoding_error(const std::string& error);
  virtual void add_parse_error(const std::string& error, int line_number);
  virtual void add_static_error(const std::string& error, int line_number);
  
  virtual void add_encoding_remark(const std::string& error);
  virtual void add_parse_remark(const std::string& error, int line_number);
  virtual void add_static_remark(const std::string& error, int line_number);
  
  virtual void runtime_error(const std::string& error);
  virtual void runtime_remark(const std::string& error);
  
  virtual void display_statement_progress
      (uint timer, const std::string& name, int progress, int line_number,
       const std::vector< std::pair< uint, uint > >& stack) {}
       
  virtual bool display_encoding_errors() { return encoding_errors; }
  virtual bool display_parse_errors() { return parse_errors; }
  virtual bool display_static_errors() { return static_errors; }
  
  void enforce_header(uint write_mime);
  void write_html_header
      (const std::string& timestamp = "", const std::string& area_timestamp = "", uint write_mime = 200,
       bool write_js_init = false, bool write_remarks = true);
  void write_payload_header
      (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp,
       bool write_mime);
  void write_footer();
  
  void set_output_handler(Output_Handler* output_handler_) { output_handler = output_handler_; }
  
public:
  Http_Methods http_method;
  std::string allow_headers;
  bool has_origin;
  
private:
  enum { not_yet, payload, html, final } header_written;
  bool encoding_errors;
  bool parse_errors;
  bool static_errors;
  uint log_level;
  std::string messages;
  
  Output_Handler* output_handler;
  
  void display_remark(const std::string& text);
  void display_error(const std::string& text, uint write_mime);
};


#endif
