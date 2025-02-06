/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__CORE__ERROR_OUTPUT_H
#define DE__OSM3S___OVERPASS_API__CORE__ERROR_OUTPUT_H

#include <string>
#include <utility>
#include <vector>


struct Error_Output
{
  virtual void add_encoding_error(const std::string& error) = 0;
  virtual void add_parse_error(const std::string& error, int line_number) = 0;
  virtual void add_static_error(const std::string& error, int line_number) = 0;
  // void add_sanity_error(const std::string& error);

  virtual void add_encoding_remark(const std::string& error) = 0;
  virtual void add_parse_remark(const std::string& error, int line_number) = 0;
  virtual void add_static_remark(const std::string& error, int line_number) = 0;
  // void add_sanity_remark(const std::string& error);

  virtual void runtime_error(const std::string& error) = 0;
  virtual void runtime_remark(const std::string& error) = 0;

  virtual void display_statement_progress
    (uint timer, const std::string& name, int progress, int line_number,
     const std::vector< std::pair< uint, uint > >& stack) = 0;

  virtual bool display_encoding_errors() = 0;
  virtual bool display_parse_errors() = 0;
  virtual bool display_static_errors() = 0;

  static const uint QUIET = 1;
  static const uint CONCISE = 2;
  static const uint PROGRESS = 3;
  static const uint ASSISTING = 4;
  static const uint VERBOSE = 5;
};


#endif
