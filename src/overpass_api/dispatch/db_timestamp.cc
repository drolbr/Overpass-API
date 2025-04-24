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

#include "../core/settings.h"
#include "../frontend/decode_text.h"
#include "../frontend/output_handler.h"
#include "../frontend/web_output.h"
#include "../../template_db/dispatcher_client.h"

#include <cstring> 
#include <fstream>
#include <iostream>
#include <sstream>


struct OSM_Data_Printer;


class Output_Timestamp : public Output_Handler
{
public:
  Output_Timestamp() {}

  virtual bool write_http_headers();
  virtual void write_payload_header(const std::string& db_dir,
				    const std::string& timestamp, const std::string& area_timestamp);
  virtual void write_footer() {}
  virtual void display_remark(const std::string&) {}
  virtual void display_error(const std::string&) {}

  OSM_Data_Printer* get_data_printer() override { return nullptr; }
};


bool Output_Timestamp::write_http_headers()
{
  std::cout<<"Content-type: text/plain\n";
  return true;
}


void Output_Timestamp::write_payload_header(
    const std::string&, const std::string& timestamp, const std::string&)
{
  std::cout<<timestamp<<"\n";
}


int main(int, char**)
{
  Output_Timestamp output;
  Web_Output error_output(Error_Output::ASSISTING);
  error_output.set_output_handler(&output);
  std::string db_dir;

  try
  {
    if (error_output.http_method == http_options
        || error_output.http_method == http_head)
      error_output.write_payload_header("", "", "", true);
    else
    {
      // open read transaction and log this.
      Dispatcher_Client dispatcher_client(osm_base_settings().shared_name);
      db_dir = dispatcher_client.get_db_dir();
      Logger logger(dispatcher_client.get_db_dir());
      logger.annotated_log("\n-- db-timestamp --");

      std::string timestamp;
      {
        std::ifstream version((dispatcher_client.get_db_dir() + "osm_base_version").c_str());
        std::getline(version, timestamp);
        timestamp = decode_json(timestamp, 0, 0);
      }
      error_output.write_payload_header(dispatcher_client.get_db_dir(), timestamp, "", true);
    }
  }
  catch(File_Error e)
  {
    std::ostringstream temp;
    if (e.origin.substr(e.origin.size()-9) == "::timeout")
    {
      error_output.write_html_header("", "", 504, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Probably the server is overcrowded.\n";
    }
    else if (e.origin.substr(e.origin.size()-20) == "Dispatcher_Client::1")
    {
      error_output.write_html_header("", "", 504, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Probably the server is down.\n";
    }
    else if (e.origin.substr(e.origin.size()-14) == "::rate_limited")
    {
      error_output.write_html_header("", "", 429, false);
      std::string server_name = get_server_name(db_dir);
      if (error_output.http_method == http_get || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Please check "<<server_name<<"status for the quota of your IP address.\n";
    }
    else
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
    error_output.runtime_error(temp.str());
  }

  return 0;
}
