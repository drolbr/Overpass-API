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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__CONSOLE_OUTPUT_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__CONSOLE_OUTPUT_H

#include "../core/datatypes.h"


struct Console_Output : public Error_Output
{
  Console_Output(uint log_level_) : encoding_errors(false), parse_errors(false),
  static_errors(false), log_level(log_level_) {}
  
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
       const std::vector< std::pair< uint, uint > >& stack);
      
  virtual bool display_encoding_errors() { return encoding_errors; }
  virtual bool display_parse_errors() { return parse_errors; }
  virtual bool display_static_errors() { return static_errors; }
  
private:
  bool encoding_errors;
  bool parse_errors;
  bool static_errors;
  uint log_level;
};

#endif
