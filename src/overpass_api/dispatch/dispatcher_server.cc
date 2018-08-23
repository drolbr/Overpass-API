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
#include "../../template_db/dispatcher.h"
#include "../../template_db/dispatcher_client.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>


struct Default_Dispatcher_Logger : public Dispatcher_Logger
{
  Default_Dispatcher_Logger(Logger& logger_) : logger(&logger_) {}

  virtual void write_start(pid_t pid, const std::vector< pid_t >& registered);
  virtual void write_rollback(pid_t pid);
  virtual void write_commit(pid_t pid);
  virtual void request_read_and_idx(
      pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space, const std::string& client_token);
  virtual void read_idx_finished(pid_t pid);
  virtual void prolongate(pid_t pid);
  virtual void idle_counter(uint32 idle_count);
  virtual void read_finished(pid_t pid);
  virtual void read_aborted(pid_t pid);
  virtual void purge(pid_t pid);

  private:
    Logger* logger;
};

void Default_Dispatcher_Logger::write_start(pid_t pid, const std::vector< pid_t >& registered)
{
  std::ostringstream out;
  out<<"write_start of process "<<pid<<". Considered as reading:";
  for (std::vector< pid_t >::const_iterator it = registered.begin(); it != registered.end(); ++it)
    out<<' '<<*it;
  out<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::write_rollback(pid_t pid)
{
  std::ostringstream out;
  out<<"write_rollback of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::write_commit(pid_t pid)
{
  std::ostringstream out;
  out<<"write_commit of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::request_read_and_idx
    (pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space, const std::string& client_token)
{
  std::ostringstream out;
  out<<"request_read_and_idx of process "<<pid<<" timeout "<<max_allowed_time
      <<" space "<<max_allowed_space<<" token "<<resolve_client_token(client_token)<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::read_idx_finished(pid_t pid)
{
  std::ostringstream out;
  out<<"read_idx_finished "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::prolongate(pid_t pid)
{
  std::ostringstream out;
  out<<"prolongate of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::idle_counter(uint32 idle_count)
{
  std::ostringstream out;
  out<<"waited idle for "<<idle_count<<" cycles.";
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::read_finished(pid_t pid)
{
  std::ostringstream out;
  out<<"read_finished of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::read_aborted(pid_t pid)
{
  std::ostringstream out;
  out<<"read_aborted of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::purge(pid_t pid)
{
  std::ostringstream out;
  out<<"purge of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


std::string to_date(time_t time)
{
  char result[21];
  strftime(result, 21, "%FT%TZ", gmtime(&time));
  return result;
}


bool assure_files_absent(const std::string& db_dir, const std::vector< File_Properties* >& files_to_avoid,
    const std::string& parameter)
{
  bool suspicious_files_present = false;

  for (std::vector< File_Properties* >::const_iterator it = files_to_avoid.begin(); it != files_to_avoid.end(); ++it)
  {
    if (file_present(db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()))
    {
      std::cerr<<"File "<<(db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix())
          <<" present. Please use parameter "<<parameter<<'\n';
      suspicious_files_present = true;
    }
    if (file_present(db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()))
    {
      std::cerr<<"File "<<(db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix())
          <<" present. Please use parameter "<<parameter<<'\n';
      suspicious_files_present = true;
    }
  }

  return suspicious_files_present;
}


int main(int argc, char* argv[])
{
  // read command line arguments
  std::string db_dir;
  bool osm_base(false), areas(false), api_keys(false), meta(false), attic(false),
      terminate(false), status(false), my_status(false), show_dir(false);
  uint32 purge_id = 0;
  bool query_token = false;
  uint64 max_allowed_space = 0;
  uint64 max_allowed_time_units = 0;
  int rate_limit = -1;

  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((std::string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    else if (std::string("--osm-base") == argv[argpos])
      osm_base = true;
    else if (std::string("--areas") == argv[argpos])
      areas = true;
    else if (std::string("--api-keys") == argv[argpos])
      api_keys = true;
    else if (std::string("--meta") == argv[argpos])
      meta = true;
    else if (std::string("--attic") == argv[argpos])
      attic = true;
    else if (std::string("--terminate") == argv[argpos])
      terminate = true;
    else if (std::string("--status") == argv[argpos])
      status = true;
    else if (std::string("--my-status") == argv[argpos])
      my_status = true;
    else if (std::string("--show-dir") == argv[argpos])
      show_dir = true;
    else if (!(strncmp(argv[argpos], "--purge=", 8)))
      purge_id = atoll(((std::string)argv[argpos]).substr(8).c_str());
    else if (std::string("--query_token") == argv[argpos])
      query_token = true;
    else if (!(strncmp(argv[argpos], "--space=", 8)))
      max_allowed_space = atoll(((std::string)argv[argpos]).substr(8).c_str());
    else if (!(strncmp(argv[argpos], "--time=", 7)))
      max_allowed_time_units = atoll(((std::string)argv[argpos]).substr(7).c_str());
    else if (!(strncmp(argv[argpos], "--rate-limit=", 13)))
      rate_limit = atoll(((std::string)argv[argpos]).substr(13).c_str());
    else
    {
      std::cout<<"Unknown argument: "<<argv[argpos]<<"\n\n"
      "Accepted arguments are:\n"
      "  --osm-base: Start or talk to the dispatcher for the osm data.\n"
      "  --areas: Start or talk to the dispatcher for the areas data.\n"
      "  --api-keys: Start or talk to the dispatcher for the api keys data.\n"
      "  --meta: When starting the osm data dispatcher, also care for meta data.\n"
      "  --attic: When starting the osm data dispatcher, also care for meta and museum data.\n"
      "  --db-dir=$DB_DIR: The directory where the database resides.\n"
      "  --terminate: Stop the adressed dispatcher.\n"
      "  --status: Let the adressed dispatcher dump its status into\n"
      "        $DB_DIR/osm_base_shadow.status or $DB_DIR/areas_shadow.status\n"
      "  --my-status: Let the adressed dispatcher return everything known about this client token\n"
      "  --show-dir: Returns $DB_DIR\n"
      "  --purge=pid: Let the adressed dispatcher forget everything known about that pid.\n"
      "  --query_token: Returns the pid of a running query for the same client IP.\n"
      "  --space=number: Set the memory limit for the total of all running processes to this value in bytes.\n"
      "  --time=number: Set the time unit  limit for the total of all running processes to this value in bytes.\n"
      "  --rate-limit=number: Set the maximum allowed number of concurrent accesses from a single IP.\n";

      return 0;
    }
    ++argpos;
  }

  if (osm_base && (areas || api_keys))
  {
    std::cout<<"\""<<(areas ? "--areas" : "--api-keys")<<"\" and \"--osm-base\" need separate instances.\n";
    return 0;
  }
  if (meta && (areas || api_keys))
  {
    std::cout<<"\""<<(areas ? "--areas" : "--api-keys")<<"\" and \"--meta\" cannot be combined.\n";
    return 0;
  }
  if (attic && (areas || api_keys))
  {
    std::cout<<"\""<<(areas ? "--areas" : "--api-keys")<<"\" and \"--attic\" cannot be combined.\n";
    return 0;
  }
  if (meta && attic)
  {
    std::cout<<"\"--attic\" and \"--meta\" cannot be combined.\n";
    return 0;
  }
  if (areas && api_keys)
  {
    std::cout<<"\"--areas\" and \"--api_keys\" need separate instances.\n";
    return 0;
  }

  if (terminate)
  {
    try
    {
      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      client.terminate();
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }

  if (status)
  {
    try
    {
      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      client.output_status();
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }

  if (show_dir)
  {
    try
    {
      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      std::cout<<client.get_db_dir();
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }

  if (purge_id > 0)
  {
    try
    {
      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      client.purge(purge_id);
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }
  else if (query_token)
  {
    try
    {
      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      pid_t pid = client.query_by_token(probe_client_token());
      if (pid > 0)
        std::cout<<pid<<'\n';
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }
  else if (my_status)
  {
    try
    {
      time_t now = time(0);
      std::string client_token = probe_client_token();
      std::cout<<"Connected as: "<<resolve_client_token(client_token)<<'\n';
      std::cout<<"Current time: "<<to_date(now)<<'\n';

      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      Client_Status status = client.query_my_status(probe_client_token());
      std::cout<<"Rate limit: "<<status.rate_limit<<'\n';
      if (status.slot_starts.size() + status.queries.size() < status.rate_limit)
        std::cout<<(status.rate_limit - status.slot_starts.size() - status.queries.size())<<" slots available now.\n";
      for (std::vector< time_t >::const_iterator it = status.slot_starts.begin(); it != status.slot_starts.end();
          ++it)
        std::cout<<"Slot available after: "<<to_date(*it)<<", in "<<*it - now<<" seconds.\n";
      std::cout<<"Currently running queries (pid, space limit, time limit, start time):\n";
      for (std::vector< Running_Query >::const_iterator it = status.queries.begin(); it != status.queries.end(); ++it)
        std::cout<<it->pid<<'\t'<<it->max_space<<'\t'<<it->max_time<<'\t'<<to_date(it->start_time)<<'\n';
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }
  else if (db_dir == "" && (max_allowed_space > 0 || max_allowed_time_units > 0 || rate_limit > -1))
  {
    try
    {
      Dispatcher_Client client
          (osm_base ? osm_base_settings().shared_name :
           areas ? area_settings().shared_name : api_key_settings().shared_name);
      client.set_global_limits(max_allowed_space, max_allowed_time_units, rate_limit);
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }

  std::vector< File_Properties* > files_to_manage;
  std::vector< File_Properties* > files_to_avoid;
  bool suspicious_files_present = false;

  if (osm_base)
  {
    files_to_manage.push_back(osm_base_settings().NODES);
    files_to_manage.push_back(osm_base_settings().NODE_TAGS_LOCAL);
    files_to_manage.push_back(osm_base_settings().NODE_TAGS_GLOBAL);
    files_to_manage.push_back(osm_base_settings().NODE_KEYS);
    files_to_manage.push_back(osm_base_settings().WAYS);
    files_to_manage.push_back(osm_base_settings().WAY_TAGS_LOCAL);
    files_to_manage.push_back(osm_base_settings().WAY_TAGS_GLOBAL);
    files_to_manage.push_back(osm_base_settings().WAY_KEYS);
    files_to_manage.push_back(osm_base_settings().RELATIONS);
    files_to_manage.push_back(osm_base_settings().RELATION_ROLES);
    files_to_manage.push_back(osm_base_settings().RELATION_TAGS_LOCAL);
    files_to_manage.push_back(osm_base_settings().RELATION_TAGS_GLOBAL);
    files_to_manage.push_back(osm_base_settings().RELATION_KEYS);

    std::vector< File_Properties* >* file_target = (meta || attic) ? &files_to_manage : &files_to_avoid;

    file_target->push_back(meta_settings().NODES_META);
    file_target->push_back(meta_settings().WAYS_META);
    file_target->push_back(meta_settings().RELATIONS_META);
    file_target->push_back(meta_settings().USER_DATA);
    file_target->push_back(meta_settings().USER_INDICES);

    suspicious_files_present |= assure_files_absent(db_dir, files_to_avoid, "--meta");
    files_to_avoid.clear();
    file_target = attic ? &files_to_manage : &files_to_avoid;

    file_target->push_back(attic_settings().NODES);
    file_target->push_back(attic_settings().NODES_UNDELETED);
    file_target->push_back(attic_settings().NODE_IDX_LIST);
    file_target->push_back(attic_settings().NODE_TAGS_LOCAL);
    file_target->push_back(attic_settings().NODE_TAGS_GLOBAL);
    file_target->push_back(attic_settings().NODES_META);
    file_target->push_back(attic_settings().NODE_CHANGELOG);
    file_target->push_back(attic_settings().WAYS);
    file_target->push_back(attic_settings().WAYS_UNDELETED);
    file_target->push_back(attic_settings().WAY_IDX_LIST);
    file_target->push_back(attic_settings().WAY_TAGS_LOCAL);
    file_target->push_back(attic_settings().WAY_TAGS_GLOBAL);
    file_target->push_back(attic_settings().WAYS_META);
    file_target->push_back(attic_settings().WAY_CHANGELOG);
    file_target->push_back(attic_settings().RELATIONS);
    file_target->push_back(attic_settings().RELATIONS_UNDELETED);
    file_target->push_back(attic_settings().RELATION_IDX_LIST);
    file_target->push_back(attic_settings().RELATION_TAGS_LOCAL);
    file_target->push_back(attic_settings().RELATION_TAGS_GLOBAL);
    file_target->push_back(attic_settings().RELATIONS_META);
    file_target->push_back(attic_settings().RELATION_CHANGELOG);

    suspicious_files_present |= assure_files_absent(db_dir, files_to_avoid, "--attic");
  }
  else if (areas)
  {
    files_to_manage.push_back(area_settings().AREAS);
    files_to_manage.push_back(area_settings().AREA_BLOCKS);
    files_to_manage.push_back(area_settings().AREA_TAGS_LOCAL);
    files_to_manage.push_back(area_settings().AREA_TAGS_GLOBAL);
  }
  else if (api_keys)
    files_to_manage.push_back(api_key_settings().API_KEYS);

  if (suspicious_files_present)
    return 3;

  if (!osm_base && !areas && !api_keys && !terminate)
  {
    std::cout<<"Usage: "<<argv[0]<<" (--terminate | (--osm-base | --areas | --api-keys | --osm-base (--meta | --attic)) --db-dir=Directory)\n";
    return 0;
  }

  {
    Logger log(db_dir);
    log.annotated_log("Dispatcher version " + basic_settings().version + " "
        + basic_settings().source_hash + " just started.");
  }
  int chmod_res = chmod((db_dir + basic_settings().logfile_name).c_str(), S_666);
  if (chmod_res)
  {
    Logger log(db_dir);
    log.annotated_log("Warning: chmod for logfile failed.");
  }

  try
  {
    Logger logger(db_dir);
    Default_Dispatcher_Logger disp_logger(logger);
    if (max_allowed_space <= 0)
      max_allowed_space = osm_base ? osm_base_settings().total_available_space :
          areas ? area_settings().total_available_space : api_key_settings().total_available_space;
    if (max_allowed_time_units <= 0)
      max_allowed_time_units = osm_base ? osm_base_settings().total_available_time_units :
          areas ? area_settings().total_available_time_units : api_key_settings().total_available_time_units;
    Dispatcher dispatcher
        (osm_base ? osm_base_settings().shared_name :
        areas ? area_settings().shared_name : api_key_settings().shared_name,
        "", db_dir + (areas ? "areas_shadow" : "osm_base_shadow"), db_dir,
	osm_base ? osm_base_settings().max_num_processes :
            areas ? area_settings().max_num_processes : api_key_settings().max_num_processes,
	max_allowed_space,
	max_allowed_time_units,
	files_to_manage, &disp_logger);
    if (rate_limit > -1)
      dispatcher.set_rate_limit(rate_limit);
    dispatcher.standby_loop(0);
  }
  catch (File_Error e)
  {
    std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }

  return 0;
}
