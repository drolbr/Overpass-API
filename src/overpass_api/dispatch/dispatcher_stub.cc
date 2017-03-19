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
#include "../frontend/user_interface.h"
#include "../statements/statement_dump.h"

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


std::string de_escape(std::string input)
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

  //TODO: Set soft limits for FastCGI instead of hard limits!

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
    (std::string db_dir_, Error_Output* error_output_, std::string xml_raw, meta_modes meta_, int area_level,
     uint32 max_allowed_time, uint64 max_allowed_space, Parsed_Query& global_settings)
:  Dispatcher_Stub(db_dir_, error_output_, xml_raw, meta_, area_level,
    max_allowed_time, max_allowed_space, global_settings, nullptr) {}


Dispatcher_Stub::Dispatcher_Stub
    (std::string db_dir_, Error_Output* error_output_, std::string xml_raw, meta_modes meta_, int area_level,
     uint32 max_allowed_time, uint64 max_allowed_space, Parsed_Query& global_settings, Index_Cache* ic)
    : db_dir(db_dir_), error_output(error_output_),
      dispatcher_client(0), area_dispatcher_client(0),
      transaction(0), area_transaction(0), rman(0), meta(meta_)
{
  if (max_allowed_time > 0)
    set_limits(2*max_allowed_time + 60, 2*max_allowed_space + 1024*1024*1024);

  if (db_dir == "")
  {
    uint32 client_token = probe_client_token();
    dispatcher_client = new Dispatcher_Client(osm_base_settings().shared_name);
    Logger logger(dispatcher_client->get_db_dir());
    try
    {
      logger.annotated_log("request_read_and_idx() start");
      dispatcher_client->request_read_and_idx(max_allowed_time, max_allowed_space, client_token);
      logger.annotated_log("request_read_and_idx() end");
    }
    catch (const File_Error& e)
    {
      std::ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
      if (e.origin == "Dispatcher_Client::request_read_and_idx::rate_limited"
          || e.origin == "Dispatcher_Client::request_read_and_idx::timeout")
	out<<' '<<probe_client_identifier();
      logger.annotated_log(out.str());
      throw;
    }

    if (ic != nullptr)
      transaction = new Nonsynced_Transaction
        (false, false, dispatcher_client->get_db_dir(), "", ic);
    else
      transaction = new Nonsynced_Transaction
          (false, false, dispatcher_client->get_db_dir(), "");

    {
      std::ifstream version((dispatcher_client->get_db_dir() + "osm_base_version").c_str());
      getline(version, timestamp);
      timestamp = de_escape(timestamp);
    }

    {
      std::ifstream replicate_id_stream((dispatcher_client->get_db_dir() + "replicate_id").c_str());
      std::string replicate_id;
      getline(replicate_id_stream, replicate_id);
      transaction->set_replicate_id(replicate_id);
    }

    transaction->flush_outdated_index_cache();

    transaction->data_index(osm_base_settings().NODES);
    transaction->random_index(osm_base_settings().NODES);
    transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL);
    transaction->data_index(osm_base_settings().NODE_TAGS_GLOBAL);
    transaction->data_index(osm_base_settings().NODE_KEYS);
    transaction->data_index(osm_base_settings().WAYS);
    transaction->random_index(osm_base_settings().WAYS);
    transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL);
    transaction->data_index(osm_base_settings().WAY_TAGS_GLOBAL);
    transaction->data_index(osm_base_settings().WAY_KEYS);
    transaction->data_index(osm_base_settings().RELATIONS);
    transaction->random_index(osm_base_settings().RELATIONS);
    transaction->data_index(osm_base_settings().RELATION_ROLES);
    transaction->data_index(osm_base_settings().RELATION_TAGS_LOCAL);
    transaction->data_index(osm_base_settings().RELATION_TAGS_GLOBAL);
    transaction->data_index(osm_base_settings().RELATION_KEYS);

    if (meta == keep_meta || meta == keep_attic)
    {
      transaction->data_index(meta_settings().NODES_META);
      transaction->data_index(meta_settings().WAYS_META);
      transaction->data_index(meta_settings().RELATIONS_META);
      transaction->data_index(meta_settings().USER_DATA);
      transaction->data_index(meta_settings().USER_INDICES);
    }

    if (meta == keep_attic)
    {
      transaction->data_index(attic_settings().NODES);
      transaction->data_index(attic_settings().NODES_UNDELETED);
      transaction->data_index(attic_settings().NODE_IDX_LIST);
      transaction->data_index(attic_settings().NODE_TAGS_LOCAL);
      transaction->data_index(attic_settings().NODE_TAGS_GLOBAL);
      transaction->data_index(attic_settings().NODES_META);
      transaction->data_index(attic_settings().NODE_CHANGELOG);
      transaction->data_index(attic_settings().WAYS);
      transaction->data_index(attic_settings().WAYS_UNDELETED);
      transaction->data_index(attic_settings().WAY_IDX_LIST);
      transaction->data_index(attic_settings().WAY_TAGS_LOCAL);
      transaction->data_index(attic_settings().WAY_TAGS_GLOBAL);
      transaction->data_index(attic_settings().WAYS_META);
      transaction->data_index(attic_settings().WAY_CHANGELOG);
      transaction->data_index(attic_settings().RELATIONS);
      transaction->data_index(attic_settings().RELATIONS_UNDELETED);
      transaction->data_index(attic_settings().RELATION_IDX_LIST);
      transaction->data_index(attic_settings().RELATION_TAGS_LOCAL);
      transaction->data_index(attic_settings().RELATION_TAGS_GLOBAL);
      transaction->data_index(attic_settings().RELATIONS_META);
      transaction->data_index(attic_settings().RELATION_CHANGELOG);
    }

    try
    {
      logger.annotated_log("read_idx_finished() start");
      dispatcher_client->read_idx_finished();
      logger.annotated_log("read_idx_finished() end");
      logger.annotated_log('\n' + xml_raw);
    }
    catch (const File_Error& e)
    {
      std::ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
      logger.annotated_log(out.str());
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
          logger.annotated_log("request_read_and_idx() area start");
	  area_dispatcher_client->request_read_and_idx(max_allowed_time, max_allowed_space, client_token);
          logger.annotated_log("request_read_and_idx() area end");
        }
	catch (const File_Error& e)
	{
	  std::ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
	  logger.annotated_log(out.str());
	  throw;
	}
	area_transaction = new Nonsynced_Transaction
            (false, false, area_dispatcher_client->get_db_dir(), "");
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
	  logger.annotated_log("write_start() area start");
	  area_dispatcher_client->write_start();
	  logger.annotated_log("write_start() area end");
	}
	catch (const File_Error& e)
	{
	  std::ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
	  logger.annotated_log(out.str());
	  throw;
	}
	area_transaction = new Nonsynced_Transaction
	    (true, true, area_dispatcher_client->get_db_dir(), "");
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
          logger.annotated_log("read_idx_finished() area start");
          area_dispatcher_client->read_idx_finished();
          logger.annotated_log("read_idx_finished() area end");
	}
	catch (const File_Error& e)
	{
	  std::ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
	  logger.annotated_log(out.str());
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

    transaction = new Nonsynced_Transaction(false, false, db_dir, "");
    if (area_level > 0)
    {
      area_transaction = new Nonsynced_Transaction(area_level == 2, false, db_dir, "");
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


Dispatcher_Stub::~Dispatcher_Stub()
{
  bool areas_written = (rman->area_updater() != 0);
  std::vector< uint64 > cpu_runtime = rman->cpu_time();
  delete rman;
  if (transaction)
    delete transaction;
  if (area_transaction)
    delete area_transaction;
  if (dispatcher_client)
  {
    Logger logger(dispatcher_client->get_db_dir());
    try
    {
      std::ostringstream out;
      out<<"read_finished() start "<<global_read_counter();
      if (rman)
        for (std::vector< uint64 >::const_iterator it = cpu_runtime.begin(); it != cpu_runtime.end(); ++it)
            out<<' '<<*it;
      logger.annotated_log(out.str());
      dispatcher_client->read_finished();
      logger.annotated_log("read_finished() end");
    }
    catch (const File_Error& e)
    {
      std::ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
      logger.annotated_log(out.str());
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
