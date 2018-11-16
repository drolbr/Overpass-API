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

#include "../../expat/expat_justparse_interface.h"
#include "../core/type_api_key.h"
#include "../data/utils.h"
#include "../frontend/basic_formats.h"
#include "api_key_updater.h"
#include "basic_updater.h"


#include <cstdio>
#include <fstream>
#include <vector>


namespace
{
  std::vector< Api_Key_Entry >* new_keys = 0;
  std::vector< Api_Key_Entry >* revoked_keys = 0;
  int64 maxkey = 0;
}


void start_api_keys(const char *el, const char **attr)
{
  if (!strcmp(el, "apikey"))
  {
    Api_Key_Entry api_key;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "key"))
        api_key.key = api_key_from_hex(attr[i+1]);
      else if (!strcmp(attr[i], "users"))
        api_key.users_allowed = (std::string(attr[i+1]) == "yes");
      else if (!strcmp(attr[i], "rate-limit"))
        api_key.rate_limit = atoi(attr[i+1]);
      else if (!strcmp(attr[i], "created"))
        api_key.timestamp = Timestamp(
            atol(attr[i+1]), //year
            atoi(attr[i+1]+5), //month
            atoi(attr[i+1]+8), //day
            atoi(attr[i+1]+11), //hour
            atoi(attr[i+1]+14), //minute
            atoi(attr[i+1]+17) //second
            ).timestamp;
    }
    new_keys->push_back(api_key);
  }
  else if (!strcmp(el, "revoked"))
  {
    Api_Key_Entry api_key;
    api_key.users_allowed = false;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "key"))
        api_key.key = api_key_from_hex(attr[i+1]);
    }
    revoked_keys->push_back(api_key);
  }
  else if (!strcmp(el, "keyid"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "max"))
        maxkey = atoll(attr[i+1]);
    }
  }
}


void end_api_keys(const char *el) {}


void Api_Key_Updater::finish_updater(const Api_Keys_Url_And_Params& url_and_params)
{
  std::map< Uint32_Index, std::set< Api_Key_Entry > > attic_api_keys;
  std::map< Uint32_Index, std::set< Api_Key_Entry > > new_api_keys;

  for (std::vector< Api_Key_Entry >::const_iterator it = revoked_keys->begin(); it != revoked_keys->end(); ++it)
    attic_api_keys[it->get_key()].insert(*it);
  for (std::vector< Api_Key_Entry >::const_iterator it = new_keys->begin(); it != new_keys->end(); ++it)
  {
    attic_api_keys[it->get_key()].insert(*it);
    new_api_keys[it->get_key()].insert(*it);
  }

  if (dispatcher_client)
  {
    {
      try
      {
        Nonsynced_Transaction transaction(true, true, dispatcher_client->get_db_dir(), "");
        update_elements(attic_api_keys, new_api_keys, transaction, *api_key_settings().API_KEYS);
      }
      catch (const File_Error& e)
      {
        std::ostringstream out;
        out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
        Logger(dispatcher_client->get_db_dir()).annotated_log(out.str());
      }
      catch (...)
      {
        Logger(dispatcher_client->get_db_dir()).annotated_log("Unknown error during API key sync.");
      }
    }
    // The transaction must be flushed before we copy back the files

    if (url_and_params.valid())
    {
      std::ofstream out((dispatcher_client->get_db_dir() + api_key_settings().source_url_file).c_str());
      out<<url_and_params.base_url
          <<"?service="<<url_and_params.service
          <<"&key="<<url_and_params.key
          <<"&beyond="<<std::max(url_and_params.beyond, maxkey)<<'\n';
    }
    Logger logger(dispatcher_client->get_db_dir());
    std::ostringstream out;
    out<<"write_commit() api_keys start "<<(revoked_keys ? revoked_keys->size() : 0)
        <<' '<<(new_keys ? new_keys->size() : 0);
    logger.annotated_log(out.str());

    dispatcher_client->write_commit();
    rename((dispatcher_client->get_db_dir() + "api_keys_beyond.shadow").c_str(),
        (dispatcher_client->get_db_dir() + "api_keys_beyond").c_str());

    logger.annotated_log("write_commit() api_keys end");
    delete dispatcher_client;
    dispatcher_client = 0;
  }
  else
  {
    Nonsynced_Transaction transaction(true, false, db_dir_, "");
    update_elements(attic_api_keys, new_api_keys, transaction, *api_key_settings().API_KEYS);

    if (url_and_params.valid())
    {
      std::ofstream out((db_dir_ + api_key_settings().source_url_file).c_str());
      out<<url_and_params.base_url
          <<"?service="<<url_and_params.service
          <<"&key="<<url_and_params.key
          <<"&beyond="<<std::max(url_and_params.beyond, maxkey)<<'\n';
    }
  }
}


std::string Api_Key_Updater::effective_db_dir() const
{
  if (dispatcher_client)
    return dispatcher_client->get_db_dir();
  return db_dir_;
}


void Api_Key_Updater::parse_file_completely(FILE* in)
{
  parse(stdin, start_api_keys, end_api_keys);

  finish_updater(Api_Keys_Url_And_Params());
}


void Api_Key_Updater::parse_completely(const std::string& file, const Api_Keys_Url_And_Params& url_and_params)
{
  Script_Parser().parse(file, start_api_keys, end_api_keys);

  finish_updater(url_and_params);
}


Api_Key_Updater::Api_Key_Updater() : dispatcher_client(0)
{
  delete(new_keys);
  new_keys = new std::vector< Api_Key_Entry >();
  delete(revoked_keys);
  revoked_keys = new std::vector< Api_Key_Entry >();

  dispatcher_client = new Dispatcher_Client(api_key_settings().shared_name);
  Logger logger(dispatcher_client->get_db_dir());
  logger.annotated_log("write_start() api_keys start");
  dispatcher_client->write_start();
  logger.annotated_log("write_start() api_keys end");
}


Api_Key_Updater::Api_Key_Updater(const std::string& db_dir) : db_dir_(db_dir), dispatcher_client(0)
{
  if (file_present(db_dir + api_key_settings().shared_name))
    throw Context_Error("File " + db_dir + api_key_settings().shared_name + " present, "
        "which indicates a running dispatcher. Delete file if no dispatcher is running.");

  delete(new_keys);
  new_keys = new std::vector< Api_Key_Entry >();
  delete(revoked_keys);
  revoked_keys = new std::vector< Api_Key_Entry >();
}


Api_Key_Updater::~Api_Key_Updater()
{
  delete new_keys;
  new_keys = 0;
  delete(revoked_keys);
  revoked_keys = 0;

  if (dispatcher_client)
  {
    Logger logger(dispatcher_client->get_db_dir());
    logger.annotated_log("write_rollback() api_keys start");
    dispatcher_client->write_rollback();
    logger.annotated_log("write_rollback() api_keys end");
    delete dispatcher_client;
  }
}


Api_Keys_Url_And_Params::Api_Keys_Url_And_Params() : beyond_valid(false) {}


Api_Keys_Url_And_Params::Api_Keys_Url_And_Params(const std::string& url) : beyond_valid(false)
{
  std::string::size_type start = url.find("?");
  if (start == std::string::npos)
  {
    base_url = url;
    return;
  }
  base_url = url.substr(0, start);

  std::string::size_type end = url.find("&", start+1);
  while (true)
  {
    std::string param = (end == std::string::npos ? url.substr(start+1) : url.substr(start+1, end-start-1));
    if (param.substr(0, 8) == "service=")
      service = param.substr(8);
    else if (param.substr(0, 4) == "key=")
      key = param.substr(4);
    else if (param.substr(0, 7) == "beyond=")
    {
      beyond = atoll(param.substr(7).c_str());
      beyond_valid = (beyond || (param.substr(7) == "0"));
    }
    else
      unknown_param = param;
    start = end;
    if (end == std::string::npos)
      break;
    end = url.find("&", start+1);
  }
}


bool Api_Keys_Url_And_Params::valid() const
{
  return !base_url.empty() && !service.empty() && !key.empty() && beyond_valid && unknown_param.empty();
}


std::string Api_Keys_Url_And_Params::error() const
{
  if (base_url.empty())
    return "Base URL is empty.";
  if (service.empty())
    return "Parameter service is empty.";
  if (key.empty())
    return "Parameter key is empty.";
  if (!beyond_valid)
    return "Parameter beyond is not a nonnegative integer.";
  if (!unknown_param.empty())
    return "Unexpected param \"" + unknown_param + "\" in URL.";
  return "";
}
