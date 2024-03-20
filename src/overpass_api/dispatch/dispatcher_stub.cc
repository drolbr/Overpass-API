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

#include "dispatcher_stub.h"
#include "../frontend/hash_request.h"
#include "../frontend/user_interface.h"
#include "../statements/statement_dump.h"
#include "../../expat/escape_json.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


std::string de_escape(const std::string& input)
{
  std::string result;
  std::string::size_type pos = 0;
  while (pos < input.length())
  {
    if (input[pos] != '\\')
      result += input[pos];
    else
    {
      ++pos;
      if (pos >= input.length())
	break;
      if (input[pos] == 'n')
	result += '\n';
      else if (input[pos] == 't')
	result += '\t';
      else
	result += input[pos];
    }
    ++pos;
  }
  return result;
}


void set_limits(uint32 time, uint64 space)
{
  rlimit limit;

  int result = getrlimit(RLIMIT_CPU, &limit);
  if (result == 0 && time < limit.rlim_cur && time < limit.rlim_max)
  {
    limit.rlim_cur = time;
    limit.rlim_max = time;
    result = setrlimit(RLIMIT_CPU, &limit);
  }

  result = getrlimit(RLIMIT_AS, &limit);
  if (result == 0 && space < limit.rlim_cur && space < limit.rlim_max)
  {
    limit.rlim_cur = space;
    limit.rlim_max = space;
    result = setrlimit(RLIMIT_AS, &limit);
  }
}


Dispatcher_Stub::Dispatcher_Stub
    (std::string db_dir_, Error_Output* error_output_, const std::string& xml_raw,
     int area_level, uint32 max_allowed_time, uint64 max_allowed_space, Parsed_Query& global_settings)
    : db_dir(db_dir_), error_output(error_output_),
      dispatcher_client(0), area_dispatcher_client(0),
      transaction(0), area_transaction(0), rman(0),
      full_hash(hash(sanitize_string(xml_raw, false))),
      anon_hash(hash(sanitize_string(xml_raw, true))), client_token(0)
{
  if (max_allowed_time > 0)
    set_limits(2*max_allowed_time + 60, 2*max_allowed_space + 1024*1024*1024);

  if (db_dir == "")
  {
    client_token = probe_client_token();
    client_identifier = probe_client_identifier();
    if (client_identifier.empty())
      client_identifier = "::";
    dispatcher_client = new Dispatcher_Client(osm_base_settings().shared_name);
    Logger db_logger(dispatcher_client->get_db_dir());
    Logger client_logger(dispatcher_client->get_db_dir(), basic_settings().client_logfile_name);
    client_logger.annotated_log("requesting " + escape_cstr(xml_raw));
    try
    {
      db_logger.annotated_log("request_read_and_idx() start");
      dispatcher_client->request_read_and_idx(max_allowed_time, max_allowed_space, client_token, full_hash);
      db_logger.annotated_log("request_read_and_idx() end");
    }
    catch (const File_Error& e)
    {
      std::ostringstream out;
      if (e.origin == "Dispatcher_Client::request_read_and_idx::rate_limited"
          || e.origin == "Dispatcher_Client::request_read_and_idx::timeout"
          || e.origin == "Dispatcher_Client::request_read_and_idx::duplicate_query")
        out<<e.origin.substr(41)<<' '
            <<std::hex<<anon_hash<<' '<<full_hash<<' '<<std::dec<<client_token<<' '<<client_identifier;
      else
        out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
      client_logger.annotated_log(out.str());
      throw;
    }
    transaction = new Nonsynced_Transaction(Access_Mode::readonly, false, dispatcher_client->get_db_dir(), "");

    for (auto i : osm_base_settings().bin_idxs())
      transaction->data_index(i);
    for (auto i : osm_base_settings().map_idxs())
      transaction->random_index(i);
    for (auto i : meta_settings().bin_idxs())
      transaction->data_index(i);
    // meta_settings().map_idxs() is always empty
    for (auto i : attic_settings().bin_idxs())
      transaction->data_index(i);
    for (auto i : attic_settings().map_idxs())
      transaction->random_index(i);

    {
      std::ifstream version((dispatcher_client->get_db_dir() + "osm_base_version").c_str());
      getline(version, timestamp);
      timestamp = de_escape(timestamp);
    }
    try
    {
      db_logger.annotated_log("read_idx_finished() start");
      dispatcher_client->read_idx_finished();
      db_logger.annotated_log("read_idx_finished() end");
    }
    catch (const File_Error& e)
    {
      std::ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
      db_logger.annotated_log(out.str());
      throw;
    }

    if (area_level > 0)
    {
      area_dispatcher_client = new Dispatcher_Client(area_settings().shared_name);
      Logger logger(area_dispatcher_client->get_db_dir());

      if (area_level == 1)
      {
	try
	{
          db_logger.annotated_log("request_read_and_idx() area start");
	  area_dispatcher_client->request_read_and_idx(max_allowed_time, max_allowed_space, client_token, full_hash);
          db_logger.annotated_log("request_read_and_idx() area end");
        }
	catch (const File_Error& e)
	{
	  std::ostringstream out;
          if (e.origin == "Dispatcher_Client::request_read_and_idx::rate_limited"
              || e.origin == "Dispatcher_Client::request_read_and_idx::timeout"
              || e.origin == "Dispatcher_Client::request_read_and_idx::duplicate_query")
            out<<"Area::"<<e.origin.substr(41)<<' '
                <<std::hex<<anon_hash<<' '<<full_hash<<' '<<std::dec<<client_token<<' '<<client_identifier;
          else
            out<<"Area::"<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
	  client_logger.annotated_log(out.str());
	  throw;
	}
	area_transaction = new Nonsynced_Transaction(
            Access_Mode::readonly, false, area_dispatcher_client->get_db_dir(), "");
	{
	  std::ifstream version((area_dispatcher_client->get_db_dir() +
	      "area_version").c_str());
	  getline(version, area_timestamp);
	  area_timestamp = de_escape(area_timestamp);
	}
      }
      else if (area_level == 2)
      {
	try
	{
	  db_logger.annotated_log("write_start() area start");
	  area_dispatcher_client->write_start();
	  db_logger.annotated_log("write_start() area end");
	}
	catch (const File_Error& e)
	{
	  std::ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
	  db_logger.annotated_log(out.str());
	  throw;
	}
	area_transaction = new Nonsynced_Transaction(
	    Access_Mode::writeable, true, area_dispatcher_client->get_db_dir(), "");
	{
	  std::ofstream area_version((area_dispatcher_client->get_db_dir()
	      + "area_version.shadow").c_str());
	  area_version<<timestamp<<'\n';
	  area_timestamp = de_escape(timestamp);
	}
      }

      area_transaction->data_index(area_settings().AREAS);
      area_transaction->data_index(area_settings().AREA_BLOCKS);
      area_transaction->data_index(area_settings().AREA_TAGS_LOCAL);
      area_transaction->data_index(area_settings().AREA_TAGS_GLOBAL);

      if (area_level == 1)
      {
	try
	{
          db_logger.annotated_log("read_idx_finished() area start");
          area_dispatcher_client->read_idx_finished();
          db_logger.annotated_log("read_idx_finished() area end");
	}
	catch (const File_Error& e)
	{
	  std::ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
	  db_logger.annotated_log(out.str());
	  throw;
	}
      }

      rman = new Resource_Manager(*transaction, global_settings, area_level == 2 ? error_output : 0,
	  *area_transaction, this, area_level == 2 ? new Area_Updater(*area_transaction) : 0);
    }
    else
      rman = new Resource_Manager(*transaction, &global_settings, this, error_output);
  }
  else
  {
    if (file_present(db_dir + osm_base_settings().shared_name))
      throw Context_Error("File " + db_dir + osm_base_settings().shared_name + " present, "
          "which indicates a running dispatcher. Delete file if no dispatcher is running.");

    transaction = new Nonsynced_Transaction(Access_Mode::readonly, false, db_dir, "");
    if (area_level > 0)
    {
      area_transaction = new Nonsynced_Transaction(
          area_level == 2 ? Access_Mode::writeable : Access_Mode::readonly, false, db_dir, "");
      rman = new Resource_Manager(*transaction, global_settings, area_level == 2 ? error_output : 0,
	  *area_transaction, this, area_level == 2 ? new Area_Updater(*area_transaction) : 0);
    }
    else
      rman = new Resource_Manager(*transaction, &global_settings, this, error_output);

    {
      std::ifstream version((db_dir + "osm_base_version").c_str());
      getline(version, timestamp);
      timestamp = de_escape(timestamp);
    }
    if (area_level == 1)
    {
      std::ifstream version((db_dir + "area_version").c_str());
      getline(version, area_timestamp);
      area_timestamp = de_escape(timestamp);
    }
    else if (area_level == 2)
    {
      std::ofstream area_version((db_dir + "area_version").c_str());
      area_version<<timestamp<<'\n';
      area_timestamp = de_escape(timestamp);
    }
  }
}


void Dispatcher_Stub::ping() const
{
  if (dispatcher_client)
    dispatcher_client->ping();
  if (area_dispatcher_client)
    area_dispatcher_client->ping();
}


bool Dispatcher_Stub::all_meta_empty() const
{
  for (auto i : meta_settings().bin_idxs())
  {
    if (transaction->data_index(i) && !transaction->data_index(i)->empty())
      return false;
  }
  return true;
}


std::string basename(const std::string& filename)
{
  uint start = filename.size();
  while (start > 0 && filename[--start] != '/')
    ;
  if (start < filename.size() && filename[start] == '/')
    ++start;
  uint end = start;
  while (end < filename.size() && filename[end] != '.')
    ++end;
  return filename.substr(start, end-start);
}


bool Dispatcher_Stub::is_meta_file(const std::string& filename) const
{
  std::string trunk = basename(filename);
  for (auto i : meta_settings().bin_idxs())
  {
    if (i && i->get_file_name_trunk() == trunk)
      return true;
  }
  return false;
}


bool Dispatcher_Stub::all_attic_empty() const
{
  for (auto i : attic_settings().bin_idxs())
  {
    if (transaction->data_index(i) && !transaction->data_index(i)->empty())
      return false;
  }
  return true;
}


bool Dispatcher_Stub::is_attic_file(const std::string& filename) const
{
  std::string trunk = basename(filename);
  for (auto i : attic_settings().bin_idxs())
  {
    if (i && i->get_file_name_trunk() == trunk)
      return true;
  }
  return false;
}


Dispatcher_Stub::~Dispatcher_Stub()
{
  bool areas_written = (rman->area_updater() != 0);
  std::vector< uint64 > cpu_runtime = rman ? rman->cpu_time() : std::vector< uint64 >();
  delete rman;
  if (transaction)
    delete transaction;
  if (area_transaction)
    delete area_transaction;
  if (dispatcher_client)
  {
    Logger db_logger(dispatcher_client->get_db_dir());
    Logger client_logger(dispatcher_client->get_db_dir(), basic_settings().client_logfile_name);
    try
    {
      std::ostringstream out;
      out<<"read_finished "<<std::hex<<anon_hash<<' '<<full_hash<<' '
          <<std::dec<<client_token<<' '<<client_identifier<<' '<<global_read_counter();
      for (std::vector< uint64 >::const_iterator it = cpu_runtime.begin(); it != cpu_runtime.end(); ++it)
        out<<' '<<*it;
      client_logger.annotated_log(out.str());
      db_logger.annotated_log("read_finished() start");
      dispatcher_client->read_finished();
      db_logger.annotated_log("read_finished() end");
    }
    catch (const File_Error& e)
    {
      std::ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
      db_logger.annotated_log(out.str());
    }
    delete dispatcher_client;
  }
  if (area_dispatcher_client)
  {
    if (areas_written)
    {
      Logger logger(area_dispatcher_client->get_db_dir());
      try
      {
        logger.annotated_log("write_commit() area start");
        area_dispatcher_client->write_commit();
        rename((area_dispatcher_client->get_db_dir() + "area_version.shadow").c_str(),
	       (area_dispatcher_client->get_db_dir() + "area_version").c_str());
        logger.annotated_log("write_commit() area end");
      }
      catch (const File_Error& e)
      {
        std::ostringstream out;
        out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
        logger.annotated_log(out.str());
      }
    }
    else
    {
      Logger logger(area_dispatcher_client->get_db_dir());
      try
      {
        logger.annotated_log("read_finished() area start");
        area_dispatcher_client->read_finished();
        logger.annotated_log("read_finished() area end");
      }
      catch (const File_Error& e)
      {
        std::ostringstream out;
        out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
        logger.annotated_log(out.str());
      }
    }
    delete area_dispatcher_client;
  }
}
