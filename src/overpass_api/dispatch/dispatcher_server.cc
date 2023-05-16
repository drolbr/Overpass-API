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

#include <sys/time.h>
#include <sys/resource.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>


struct Default_Dispatcher_Logger : public Dispatcher_Logger
{
  Default_Dispatcher_Logger(Logger& logger_) : logger(&logger_) {}

  virtual void terminate_triggered(int32 countdown, pid_t writing_process);
  virtual void write_start(pid_t pid, const std::vector< ::pid_t >& registered);
  virtual void write_rollback(pid_t pid);
  virtual void write_pending(pid_t pid, const std::set< pid_t >& reading);
  virtual void write_commit(pid_t pid);
  virtual void migrate_start(pid_t pid, const std::vector< ::pid_t >& registered);
  virtual void migrate_rollback(pid_t pid);
  virtual void migrate_commit(pid_t pid);
  virtual void request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space);
  virtual void read_idx_finished(pid_t pid);
  virtual void prolongate(pid_t pid);
  virtual void idle_counter(uint32 idle_count);
  virtual void read_finished(pid_t pid);
  virtual void query_my_status(pid_t pid);
  virtual void read_aborted(pid_t pid);
  virtual void hangup(pid_t pid);
  virtual void purge(pid_t pid);

  private:
    Logger* logger;
};


void Default_Dispatcher_Logger::terminate_triggered(int32 countdown, pid_t writing_process)
{
  std::ostringstream out;
  out<<"Shutdown triggered. Waiting "<<countdown<<" cycles for writing process "<<writing_process<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::write_start(pid_t pid, const std::vector< ::pid_t >& registered)
{
  std::ostringstream out;
  out<<"write_start of process "<<pid<<". Considered as reading:";
  for (auto i : registered)
    out<<' '<<i;
  out<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::write_rollback(pid_t pid)
{
  std::ostringstream out;
  out<<"write_rollback of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::write_pending(pid_t pid, const std::set< pid_t >& registered)
{
  std::ostringstream out;
  out<<"write_pending of process "<<pid<<". Considered as reading:";
  for (auto i : registered)
    out<<' '<<i;
  out<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::write_commit(pid_t pid)
{
  std::ostringstream out;
  out<<"write_commit of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::migrate_start(pid_t pid, const std::vector< ::pid_t >& registered)
{
  std::ostringstream out;
  out<<"migrate_start of process "<<pid<<". Considered as reading:";
  for (auto i : registered)
    out<<' '<<i;
  out<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::migrate_rollback(pid_t pid)
{
  std::ostringstream out;
  out<<"migrate_rollback of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::migrate_commit(pid_t pid)
{
  std::ostringstream out;
  out<<"migrate_commit of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::request_read_and_idx
    (pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space)
{
  std::ostringstream out;
  out<<"request_read_and_idx of process "<<pid<<" timeout "<<max_allowed_time
      <<" space "<<max_allowed_space<<'.';
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


void Default_Dispatcher_Logger::query_my_status(pid_t pid)
{
  std::ostringstream out;
  out<<"query_my_status by process "<<pid<<'.';
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


void Default_Dispatcher_Logger::hangup(pid_t pid)
{
  std::ostringstream out;
  out<<"hangup of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Default_Dispatcher_Logger::purge(pid_t pid)
{
  std::ostringstream out;
  out<<"purge of process "<<pid<<'.';
  logger->annotated_log(out.str());
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
  bool osm_base(false), areas(false), meta(false), attic(false),
      terminate(false), status(false), show_dir(false);
  uint32 purge_id = 0;
  bool query_token = false;
  uint64 max_allowed_space = 0;
  uint64 max_allowed_time_units = 0;
  int rate_limit = -1;
  std::string server_name;

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
    else if (std::string("--meta") == argv[argpos])
      meta = true;
    else if (std::string("--attic") == argv[argpos])
      attic = true;
    else if (std::string("--terminate") == argv[argpos])
      terminate = true;
    else if (std::string("--status") == argv[argpos])
      status = true;
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
    else if (!(strncmp(argv[argpos], "--server-name=", 14)))
      server_name = ((std::string)argv[argpos]).substr(14);
    else
    {
      std::cout<<"Unknown argument: "<<argv[argpos]<<"\n\n"
      "Accepted arguments are:\n"
      "  --osm-base: Start or talk to the dispatcher for the osm data.\n"
      "  --areas: Start or talk to the dispatcher for the areas data.\n"
      "  --meta: When starting the osm data dispatcher, also care for meta data.\n"
      "  --attic: When starting the osm data dispatcher, also care for meta and museum data.\n"
      "  --db-dir=$DB_DIR: The directory where the database resides.\n"
      "  --terminate: Stop the adressed dispatcher.\n"
      "  --status: Let the adressed dispatcher dump its status into\n"
      "        $DB_DIR/osm_base_shadow.status or $DB_DIR/areas_shadow.status\n"
      "  --show-dir: Returns $DB_DIR\n"
      "  --purge=pid: Let the adressed dispatcher forget everything known about that pid.\n"
      "  --query_token: Returns the pid of a running query for the same client IP.\n"
      "  --space=number: Set the memory limit for the total of all running processes to this value in bytes.\n"
      "  --time=number: Set the time unit  limit for the total of all running processes to this value in bytes.\n"
      "  --rate-limit=number: Set the maximum allowed number of concurrent accesses from a single IP.\n"
      "  --server-name: Set the server name used in status and error messages.\n";

      return 0;
    }
    ++argpos;
  }

  if (osm_base && areas)
  {
    std::cout<<"\"--areas\" and \"--osm-base\" need separate instances.\n";
    return 0;
  }
  if (meta && areas)
  {
    std::cout<<"\"--areas\" and \"--meta\" cannot be combined.\n";
    return 0;
  }
  if (attic && areas)
  {
    std::cout<<"\"--areas\" and \"--attic\" cannot be combined.\n";
    return 0;
  }
  if (meta && attic)
  {
    std::cout<<"\"--attic\" and \"--meta\" cannot be combined.\n";
    return 0;
  }

  if (terminate)
  {
    try
    {
      Dispatcher_Client client
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
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
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
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
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
      std::cout<<client.get_db_dir();
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
      return e.error_number;
    }
    return 0;
  }

  if (purge_id > 0)
  {
    try
    {
      Dispatcher_Client client
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
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
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
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
  else if (db_dir == "" && (max_allowed_space > 0 || max_allowed_time_units > 0 || rate_limit > -1))
  {
    try
    {
      Dispatcher_Client client
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
      client.set_global_limits(max_allowed_space, max_allowed_time_units, rate_limit);
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }
  else if (db_dir.empty() && !server_name.empty())
  {
    try
    {
      Dispatcher_Client client
          (areas ? area_settings().shared_name : osm_base_settings().shared_name);
      db_dir = client.get_db_dir();
    }
    catch (File_Error e)
    {
      std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }

    try
    {
      set_server_name(db_dir, server_name);
    }
    catch (const std::exception& e)
    {
      std::cout<<"exception: "<<e.what()<<'\n';
    }
    return 0;
  }

  std::vector< File_Properties* > files_to_manage;
  std::vector< File_Properties* > files_to_avoid;
  bool suspicious_files_present = false;

  if (osm_base)
  {
    Database_Meta_State mode_;
    if (meta)
      mode_.set_mode(Database_Meta_State::keep_meta);
    if (attic)
      mode_.set_mode(Database_Meta_State::keep_attic);
    Database_Meta_State::Mode mode = mode_.value_or_autodetect(db_dir);
    
    files_to_manage = osm_base_settings().bin_idxs();

    if (mode >= Database_Meta_State::keep_meta)
      files_to_manage.insert(
          files_to_manage.end(), meta_settings().bin_idxs().begin(), meta_settings().bin_idxs().end());      
    else
      suspicious_files_present |= assure_files_absent(db_dir, meta_settings().bin_idxs(), "--meta");
    
    if (mode >= Database_Meta_State::keep_attic)
      files_to_manage.insert(
          files_to_manage.end(), attic_settings().bin_idxs().begin(), attic_settings().bin_idxs().end());
    else
      suspicious_files_present |= assure_files_absent(db_dir, attic_settings().bin_idxs(), "--attic");
  }
  else if (areas)
  {
    files_to_manage.push_back(area_settings().AREAS);
    files_to_manage.push_back(area_settings().AREA_BLOCKS);
    files_to_manage.push_back(area_settings().AREA_TAGS_LOCAL);
    files_to_manage.push_back(area_settings().AREA_TAGS_GLOBAL);
  }

  if (suspicious_files_present)
    return 3;

  if (!osm_base && !areas && !terminate)
  {
    std::cout<<"Usage: "<<argv[0]<<" (--terminate | (--osm-base | --areas | --osm-base (--meta | --attic)) --db-dir=Directory)\n";
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
    rlimit max_open_files;
    int result = getrlimit(RLIMIT_NOFILE, &max_open_files);
    if (result == -1)
    {
      std::cerr<<"getrlimit(RLIMIT_NOFILE, ..) failed: "<<errno<<' '<<strerror(errno)<<'\n';
      return errno;
    }
    
    Logger logger(db_dir);
    Default_Dispatcher_Logger disp_logger(logger);
    if (max_allowed_space <= 0)
      max_allowed_space = areas ? area_settings().total_available_space : osm_base_settings().total_available_space;
    if (max_allowed_time_units <= 0)
      max_allowed_time_units = areas ? area_settings().total_available_time_units
          : osm_base_settings().total_available_time_units;
    Dispatcher dispatcher(
        areas ? area_settings().shared_name : osm_base_settings().shared_name,
        "", db_dir + (areas ? "areas_shadow" : "osm_base_shadow"), db_dir,
        areas ? area_settings().max_num_processes : osm_base_settings().max_num_processes,
        max_open_files.rlim_cur > 256 ? max_open_files.rlim_cur - 64 : max_open_files.rlim_cur*3/4,
        areas ? area_settings().purge_timeout : osm_base_settings().purge_timeout,
        max_allowed_space, max_allowed_time_units,
        files_to_manage, &disp_logger);

    if (rate_limit > -1)
      dispatcher.set_rate_limit(rate_limit);
    
    if (!server_name.empty())
    {
      try
      {
        set_server_name(db_dir, server_name);
      }
      catch (const std::exception& e)
      {
        std::cout<<"exception: "<<e.what()<<'\n';
      }
    }
    
    dispatcher.standby_loop(0);
  }
  catch (File_Error e)
  {
    std::cout<<"File_Error "<<strerror(e.error_number)<<' '<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }

  return 0;
}
