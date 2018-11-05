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
#include "../data/utils.h"
#include "../frontend/output.h"
#include "api_key_updater.h"
#include "osm_updater.h"


void download_and_parse(Api_Key_Updater& api_key_updater, const std::string& url_from_cmdline)
{
  std::string url = url_from_cmdline;
  if (file_present(api_key_updater.effective_db_dir() + api_key_settings().source_url_file))
  {
    if (!url.empty())
      throw Context_Error("File " + api_key_updater.effective_db_dir() + api_key_settings().source_url_file
          + " present which conflicts with URL by parameter.\n");

    std::ifstream in((api_key_updater.effective_db_dir() + api_key_settings().source_url_file).c_str());
    std::getline(in, url);
  }
  Api_Keys_Url_And_Params url_and_params(url);
  if (!url_and_params.valid())
    throw Context_Error("Bad URL: " + url_and_params.error());

  std::string downloaded = Curl_Client().send_request(url);
  if (api_key_updater.is_transactional())
    Logger(api_key_updater.effective_db_dir()).annotated_log(
        "Downloaded " + to_string(downloaded.size()) + " bytes from '" + url + "'");
  else
    std::cerr<<"Downloaded "<<downloaded.size()<<" bytes from '"<<url<<"'\n";

  api_key_updater.parse_completely(downloaded, url_and_params);
}


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

  try
  {
    if (transactional)
    {
      Api_Key_Updater api_key_updater;
      download_and_parse(api_key_updater, url);
    }
    else
    {
      Api_Key_Updater api_key_updater(db_dir);
      download_and_parse(api_key_updater, url);
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
  catch (const Curl_Client::Setup_Error& e)
  {
    std::cerr<<"Could not setup libcurl: "<<e.what()<<'\n';
    return 5;
  }
  catch (const Curl_Client::Download_Error& e)
  {
    std::cerr<<"Could not download "<<url<<": "<<e.what()<<"\n\n";
    std::cerr<<e.payload()<<'\n';
    return 6;
  }

  return 0;
}
