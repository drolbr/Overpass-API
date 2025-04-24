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

#ifndef DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_CSV_H
#define DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_CSV_H


#include "../core/datatypes.h"
#include "../core/geometry.h"
#include "../frontend/osm_printer.h"
#include "../frontend/output_handler.h"

#include <string>
#include <vector>


struct Csv_Settings
{
  std::vector< std::pair< std::string, bool > > keyfields;
  bool with_headerline;
  std::string separator;
};


class Output_CSV : public Output_Handler
{
public:
  Output_CSV(Csv_Settings csv_settings) : data_printer(csv_settings) {}

  virtual bool write_http_headers();
  virtual void write_payload_header(const std::string& db_dir,
				    const std::string& timestamp, const std::string& area_timestamp);
  virtual void write_footer();
  virtual void display_remark(const std::string& text);
  virtual void display_error(const std::string& text);

  virtual std::string dump_config() const;

  OSM_Data_Printer* get_data_printer() override { return &data_printer; }

private:
  struct Data_Printer : OSM_Data_Printer
  {
    Data_Printer(const Csv_Settings& csv_settings_) : csv_settings(csv_settings_) {}
    
    virtual void print_global_bbox(const Bbox_Double&) override {}

    virtual void print_item(
        const Node_Skeleton& skel,
        const Opaque_Geometry& geometry,
        const std::vector< std::pair< std::string, std::string > >* tags,
        const Full_Monotype_Meta* meta,
        const std::map< uint32, std::string >* users,
        Output_Mode mode,
        const Feature_Action& action = keep) override;

    virtual void print_item(
        const Way_Skeleton& skel,
        const Opaque_Geometry& geometry,
        const std::vector< std::pair< std::string, std::string > >* tags,
        const Full_Monotype_Meta* meta,
        const std::map< uint32, std::string >* users,
        Output_Mode mode,
        const Feature_Action& action = keep) override;

    virtual void print_item(
        const Relation_Skeleton& skel,
        const Opaque_Geometry& geometry,
        const std::vector< std::pair< std::string, std::string > >* tags,
        const Full_Monotype_Meta* meta,
        const std::map< uint32, std::string >* roles,
        const std::map< uint32, std::string >* users,
        Output_Mode mode,
        const Feature_Action& action = keep) override;

    void print_item(
        const Derived_Skeleton& skel,
        const Opaque_Geometry& geometry,
        const std::vector< std::pair< std::string, std::string > >* tags,
        Output_Mode mode,
        const Feature_Action& action = keep) override;
        
    void print_redacted(Output_Mode, uint32_t, const Full_Monotype_Meta*) override {}

    Csv_Settings csv_settings;
  };
  
  Data_Printer data_printer;
};


#endif
