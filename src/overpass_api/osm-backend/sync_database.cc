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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../curl/curl_client.h"
#include "../../expat/expat_justparse_interface.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "api_key_updater.h"
#include "osm_updater.h"


int main(int argc, char* argv[])
{
  // read command line arguments
  std::string db_dir, url;
  bool transactional = true;
  bool abort = false;
  bool api_keys = false;

  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((std::string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
      transactional = false;
    }
    else if (!(strncmp(argv[argpos], "--url=", 6)))
      url = std::string(argv[argpos]).substr(6);
    else if (!(strncmp(argv[argpos], "--api-keys", 10)))
      api_keys = true;
    else
    {
      std::cerr<<"Unkown argument: "<<argv[argpos]<<'\n';
      abort = true;
    }
    ++argpos;
  }
  if (!api_keys)
  {
    std::cerr<<"Parameter api_keys required.\n";
    abort = true;
  }
  if (abort)
  {
    std::cerr<<"Usage: "<<argv[0]<<" [--db-dir=DIR] [--version=VER]\n";
    return 1;
  }

  std::string delta_file;
  try
  {
    Curl_Client client;
    try
    {
      delta_file = client.send_request(url);
    }
    catch (const Curl_Client::Error& e)
    {
      std::cerr<<"Could not download "<<url<<": "<<e.what()<<"\n\n";
      std::cerr<<client.get_payload_if_error()<<'\n';
      return 6;
    }
  }
  catch (const Curl_Client::Error& e)
  {
    std::cerr<<"Could not setup libcurl: "<<e.what()<<'\n';
    return 5;
  }
  std::cerr<<"Downloaded "<<delta_file.size()<<" bytes.\n";

  try
  {
    if (transactional)
    {
      Api_Key_Updater api_key_updater;
      api_key_updater.parse_completely(delta_file);
    }
    else
    {
      Api_Key_Updater api_key_updater(db_dir);
      api_key_updater.parse_completely(delta_file);
    }
  }
  catch(Context_Error e)
  {
    std::cerr<<"Context error: "<<e.message<<'\n';
    return 3;
  }
  catch (File_Error e)
  {
    report_file_error(e);
    return 2;
  }
  catch (Parse_Error e)
  {
    std::cerr<<"Parser error: "<<e.message<<'\n';
    return 4;
  }

  return 0;
}
