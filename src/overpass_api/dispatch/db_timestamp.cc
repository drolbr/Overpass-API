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

#include "dispatcher_stub.h"
#include "../frontend/web_output.h"


int main(int argc, char *argv[])
{
  Parsed_Query global_settings;
  Web_Output error_output(Error_Output::ASSISTING);
  
  try
  {
    if (error_output.http_method == error_output.http_options
        || error_output.http_method == error_output.http_head)
      error_output.write_text_header("");
    else
    {
      // open read transaction and log this.
      Dispatcher_Stub dispatcher("", &error_output, "-- db-timestamp --", only_data, 0, 5, 256, global_settings);
      error_output.write_text_header(dispatcher.get_timestamp());
    }
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin.substr(e.origin.size()-9) == "::timeout")
    {
      error_output.write_html_header("", "", 504, false);
      if (error_output.http_method == error_output.http_get
          || error_output.http_method == error_output.http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Probably the server is overcrowded.\n";
    }
    else if (e.origin.substr(e.origin.size()-14) == "::rate_limited")
    {
      error_output.write_html_header("", "", 429, false);
      if (error_output.http_method == error_output.http_get
          || error_output.http_method == error_output.http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Another request from your IP is still running.\n";
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
