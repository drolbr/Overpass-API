#include "../core/settings.h"
#include "../../template_db/dispatcher.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

struct Default_Dispatcher_Logger : public Dispatcher_Logger
{
  Default_Dispatcher_Logger(Logger& logger_) : logger(&logger_) {}
  
  virtual void write_start(pid_t pid, const vector< pid_t >& registered);
  virtual void write_rollback(pid_t pid);
  virtual void write_commit(pid_t pid);
  virtual void request_read_and_idx(pid_t pid);
  virtual void read_idx_finished(pid_t pid);
  virtual void prolongate(pid_t pid);
  virtual void read_finished(pid_t pid);
  virtual void purge(pid_t pid);
  
  private:
    Logger* logger;
};

void Default_Dispatcher_Logger::write_start(pid_t pid, const vector< pid_t >& registered)
{
  ostringstream out;
  out<<"write_start of process "<<pid<<". Considered as reading:";
  for (vector< pid_t >::const_iterator it = registered.begin(); it != registered.end(); ++it)
    out<<' '<<*it;
  out<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::write_rollback(pid_t pid)
{
  ostringstream out;
  out<<"write_rollback of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::write_commit(pid_t pid)
{
  ostringstream out;
  out<<"write_commit of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::request_read_and_idx(pid_t pid)
{
  ostringstream out;
  out<<"request_read_and_idx of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::read_idx_finished(pid_t pid)
{
  ostringstream out;
  out<<"read_idx_finished "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::prolongate(pid_t pid)
{
  ostringstream out;
  out<<"prolongate of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::read_finished(pid_t pid)
{
  ostringstream out;
  out<<"read_finished of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

void Default_Dispatcher_Logger::purge(pid_t pid)
{
  ostringstream out;
  out<<"purge of process "<<pid<<'.';
  logger->annotated_log(out.str());
}

int main(int argc, char* argv[])
{
  // read command line arguments
  string db_dir;
  bool osm_base(false), areas(false), meta(false), terminate(false), status(false);
  uint32 purge_id = 0;
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    else if (!(strncmp(argv[argpos], "--osm-base", 10)))
      osm_base = true;
    else if (!(strncmp(argv[argpos], "--areas", 7)))
      areas = true;
    else if (!(strncmp(argv[argpos], "--meta", 6)))
      meta = true;
    else if (!(strncmp(argv[argpos], "--terminate", 11)))
      terminate = true;  
    else if (!(strncmp(argv[argpos], "--status", 8)))
      status = true;
    else if (!(strncmp(argv[argpos], "--purge=", 8)))
      purge_id = atoll(((string)argv[argpos]).substr(8).c_str());
    ++argpos;
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
      cout<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
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
      cout<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
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
      cout<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return 0;
  }

  vector< File_Properties* > files_to_manage;
  if (osm_base)
  {
    files_to_manage.push_back(osm_base_settings().NODES);
    files_to_manage.push_back(osm_base_settings().NODE_TAGS_LOCAL);
    files_to_manage.push_back(osm_base_settings().NODE_TAGS_GLOBAL);
    files_to_manage.push_back(osm_base_settings().WAYS);
    files_to_manage.push_back(osm_base_settings().WAY_TAGS_LOCAL);
    files_to_manage.push_back(osm_base_settings().WAY_TAGS_GLOBAL);
    files_to_manage.push_back(osm_base_settings().RELATIONS);
    files_to_manage.push_back(osm_base_settings().RELATION_ROLES);
    files_to_manage.push_back(osm_base_settings().RELATION_TAGS_LOCAL);
    files_to_manage.push_back(osm_base_settings().RELATION_TAGS_GLOBAL);
  }
  if (areas)
  {
    files_to_manage.push_back(area_settings().AREAS);
    files_to_manage.push_back(area_settings().AREA_BLOCKS);
    files_to_manage.push_back(area_settings().AREA_TAGS_LOCAL);
    files_to_manage.push_back(area_settings().AREA_TAGS_GLOBAL);
  }
  if (meta)
  {
    files_to_manage.push_back(meta_settings().NODES_META);
    files_to_manage.push_back(meta_settings().WAYS_META);
    files_to_manage.push_back(meta_settings().RELATIONS_META);
    files_to_manage.push_back(meta_settings().USER_DATA);
    files_to_manage.push_back(meta_settings().USER_INDICES);
  }
  
  if (!osm_base && !areas && !terminate)
  {
    cout<<"Usage: "<<argv[0]<<" (--terminate | (--osm-base | --areas | --meta) --db-dir=Directory)\n";
    return 0;
  }

  {
    Logger log(db_dir);
    log.annotated_log("Dispatcher just started.");
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
    Dispatcher dispatcher
        (areas ? area_settings().shared_name : osm_base_settings().shared_name,
         "", db_dir + (areas ? "areas_shadow" : "osm_base_shadow"), db_dir,
	 areas ? area_settings().max_num_processes : osm_base_settings().max_num_processes,
	 files_to_manage, &disp_logger);
    dispatcher.standby_loop(0);
  }
  catch (File_Error e)
  {
    cout<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
