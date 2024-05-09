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
#include "../frontend/user_interface.h"
#include "../../template_db/dispatcher_client.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>


std::string to_date(time_t time)
{
  char result[21];
  strftime(result, 21, "%FT%TZ", gmtime(&time));
  return result;
}


int main(int argc, char* argv[])
{
  // read command line arguments
  bool areas = false;

  try
  {
    time_t now = time(0);
    uint32 client_token = probe_client_token();

    std::ostringstream out;
    out<<"Connected as: "<<client_token<<'\n';
    out<<"Current time: "<<to_date(now)<<'\n';

    Dispatcher_Client client
        (areas ? area_settings().shared_name : osm_base_settings().shared_name);
    std::string server_name = get_server_name(client.get_db_dir());
    out<<"Announced endpoint: "<<(server_name == "/api/" ? "none" : server_name)<<'\n';
    Client_Status status = client.query_my_status(client_token);
    out<<"Rate limit: "<<status.rate_limit<<'\n';
    if (status.slot_starts.size() + status.queries.size() < status.rate_limit)
      out<<(status.rate_limit - status.slot_starts.size() - status.queries.size())<<" slots available now.\n";
    for (std::vector< time_t >::const_iterator it = status.slot_starts.begin(); it != status.slot_starts.end();
        ++it)
      out<<"Slot available after: "<<to_date(*it)<<", in "<<*it - now<<" seconds.\n";
    out<<"Currently running queries (pid, space limit, time limit, start time):\n";
    for (std::vector< Running_Query >::const_iterator it = status.queries.begin(); it != status.queries.end(); ++it)
      out<<it->pid<<'\t'<<it->max_space<<'\t'<<it->max_time<<'\t'<<to_date(it->start_time)<<'\n';

    std::cout<<"Content-Type: text/plain; charset=utf-8\n"
        "Access-Control-Allow-Origin: *\n\n";

    std::cout<<out.str();
  }
  catch (File_Error e)
  {
    if (e.origin.substr(e.origin.size()-20) == "Dispatcher_Client::1")
    {
      std::cout<<"Status: 504 Gateway Timeout\n"
          "Content-Type: text/plain; charset=utf-8\n"
          "Access-Control-Allow-Origin: *\n\n";
      std::cout<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Probably the server is down.\n";
    }
    else
    {
      std::cout<<"Content-Type: text/plain; charset=utf-8\n"
          "Access-Control-Allow-Origin: *\n\n";
      std::cout<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  return 0;
}
