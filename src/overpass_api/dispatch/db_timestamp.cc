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

#include "dispatcher_stub.h"
#include "../frontend/web_output.h"


class Output_Timestamp : public Output_Handler
{
public:
  Output_Timestamp() {}
  
  virtual bool write_http_headers();
  virtual void write_payload_header(const std::string& db_dir,
				    const std::string& timestamp, const std::string& area_timestamp);
  virtual void write_footer() {}
  virtual void display_remark(const std::string& text) {}
  virtual void display_error(const std::string& text) {}

  virtual void print_global_bbox(const Bbox_Double& bbox) {}
  
  virtual void print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Node_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0) {}
  
  virtual void print_item(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Way_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0) {}
      
  virtual void print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Relation_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0) {}
                            
  virtual void print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode) {}
};


bool Output_Timestamp::write_http_headers()
{
  std::cout<<"Content-type: text/plain\n";
  return true;
}


void Output_Timestamp::write_payload_header
    (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp)
{
  std::cout<<timestamp<<"\n";
}


int main(int argc, char *argv[])
{
  Parsed_Query global_settings;
  Output_Timestamp output;
  Web_Output error_output(Error_Output::ASSISTING);
  error_output.set_output_handler(&output);
  
  try
  {
    if (error_output.http_method == http_options
        || error_output.http_method == http_head)
      error_output.write_payload_header("", "", "", true);
    else
    {
      // open read transaction and log this.
      Dispatcher_Stub dispatcher("", &error_output, "-- db-timestamp --", only_data, 0, 5, 256, global_settings);
      error_output.write_payload_header(dispatcher.get_db_dir(), dispatcher.get_timestamp(), "", true);
    }
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin.substr(e.origin.size()-9) == "::timeout")
    {
      error_output.write_html_header("", "", 504, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Probably the server is overcrowded.\n";
    }
    else if (e.origin.substr(e.origin.size()-14) == "::rate_limited")
    {
      error_output.write_html_header("", "", 429, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Please check /api/status for the quota of your IP address.\n";
    }
    else
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
    error_output.runtime_error(temp.str());
  }
  catch(Resource_Error e)
  {
    ostringstream temp;
    if (e.timed_out)
      temp<<"Query timed out in \""<<e.stmt_name<<"\" at line "<<e.line_number
          <<" after "<<e.runtime<<" seconds.";
    else
      temp<<"Query run out of memory in \""<<e.stmt_name<<"\" at line "
          <<e.line_number<<" using about "<<e.size/(1024*1024)<<" MB of RAM.";
    error_output.runtime_error(temp.str());
  }
  catch(Exit_Error e) {}

  return 0;
}
