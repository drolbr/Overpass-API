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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_HANDLER_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_HANDLER_H


#include "../core/output_handler_params.h"


struct OSM_Data_Printer;
struct OSM_Diff_Printer;


class Output_Handler : public Output_Handler_Params
{
public:  
  // shall return true if it really has written a content declaration
  virtual bool write_http_headers() = 0;

  virtual void write_payload_header(const std::string& db_dir,
				    const std::string& timestamp, const std::string& area_timestamp) = 0;
  virtual void write_footer() = 0;
  virtual void display_remark(const std::string& text) = 0;
  virtual void display_error(const std::string& text) = 0;

  virtual OSM_Data_Printer* get_data_printer() = 0;
  virtual bool supports_diff() const { return false; }
  virtual OSM_Diff_Printer* get_diff_printer() { return nullptr; }

  virtual std::string dump_config() const { return ""; }

  virtual ~Output_Handler() {}
};


#endif
