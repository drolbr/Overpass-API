#include "../core/settings.h"
#include "../../template_db/dispatcher.h"

#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
  // read command line arguments
  string db_dir;
  bool osm_base(false), terminate(false);
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
      set_basedir(db_dir);
    }
    else if (!(strncmp(argv[argpos], "--osm-base", 10)))
      osm_base = true;
    else if (!(strncmp(argv[argpos], "--terminate", 11)))
      terminate = true;  
    ++argpos;
  }
  
  if (terminate)
  {
    try
    {
      Dispatcher_Client client(shared_name);
      client.terminate();
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
    files_to_manage.push_back(de_osm3s_file_ids::NODES);
    files_to_manage.push_back(de_osm3s_file_ids::NODE_TAGS_LOCAL);
    files_to_manage.push_back(de_osm3s_file_ids::NODE_TAGS_GLOBAL);
    files_to_manage.push_back(de_osm3s_file_ids::WAYS);
    files_to_manage.push_back(de_osm3s_file_ids::WAY_TAGS_LOCAL);
    files_to_manage.push_back(de_osm3s_file_ids::WAY_TAGS_GLOBAL);
    files_to_manage.push_back(de_osm3s_file_ids::RELATIONS);
    files_to_manage.push_back(de_osm3s_file_ids::RELATION_ROLES);
    files_to_manage.push_back(de_osm3s_file_ids::RELATION_TAGS_LOCAL);
    files_to_manage.push_back(de_osm3s_file_ids::RELATION_TAGS_GLOBAL);
  }
  
  if (!osm_base && !terminate)
  {
    cout<<"Usage: "<<args[0]<<" (--terminate | --osm-base --db-dir=Directory)\n";
    return 0;
  }
  
  Dispatcher dispatcher(shared_name, "", db_dir + "shadow", db_dir,
			files_to_manage);
  dispatcher.standby_loop(0);
}
