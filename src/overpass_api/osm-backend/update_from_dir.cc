#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"
#include "osm_updater.h"

using namespace std;

int main(int argc, char* argv[])
{
  // read command line arguments
  string source_dir, db_dir, data_version;
  vector< string > source_file_names;
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    if (!(strncmp(argv[argpos], "--osc-dir=", 10)))
    {
      source_dir = ((string)argv[argpos]).substr(10);
      if ((source_dir.size() > 0) && (source_dir[source_dir.size()-1] != '/'))
	source_dir += '/';
    }
    if (!(strncmp(argv[argpos], "--version=", 10)))
      data_version = ((string)argv[argpos]).substr(10);
    ++argpos;
  }
  
  // read file names from source directory
  DIR *dp;
  struct dirent *ep;
  
  dp = opendir(source_dir.c_str());
  if (dp != NULL)
  {
    while ((ep = readdir (dp)))
      source_file_names.push_back(ep->d_name);
    closedir(dp);
  }
  else
  {
    report_file_error(File_Error(errno, source_dir, "update_from_dir:opendir"));
    return -1;
  }
  sort(source_file_names.begin(), source_file_names.end());
  
  try
  {
    Osm_Updater osm_updater(get_verbatim_callback(), data_version);
    get_verbatim_callback()->parser_started();
    
    vector< string >::const_iterator it(source_file_names.begin());
    while (it != source_file_names.end())
    {
      if ((*it == ".") || (*it == ".."))
      {
	++it;
	continue;
      }
      FILE* osc_file(fopen((source_dir + *it).c_str(), "r"));
      if (osc_file)
      {
	//reading the main document
	parse_nodes_only(osc_file);
	
	fclose(osc_file);
      }
      else
      {
	report_file_error(File_Error(errno, (source_dir + *it), "update_from_dir:1"));
	return -1;
      }
      ++it;
    }
    
    it = source_file_names.begin();
    while (it != source_file_names.end())
    {
      if ((*it == ".") || (*it == ".."))
      {
	++it;
	continue;
      }
      FILE* osc_file(fopen((source_dir + *it).c_str(), "r"));
      if (osc_file)
      {
	//reading the main document
	parse_ways_only(osc_file);

	fclose(osc_file);
      }
      else
      {
	report_file_error(File_Error(errno, (source_dir + *it), "update_from_dir:2"));
	return -1;
      }
      ++it;
    }
    
    it = source_file_names.begin();
    while (it != source_file_names.end())
    {
      if ((*it == ".") || (*it == ".."))
      {
	++it;
	continue;
      }
      FILE* osc_file(fopen((source_dir + *it).c_str(), "r"));
      if (osc_file)
      {
	//reading the main document
	parse_relations_only(osc_file);
	
	fclose(osc_file);
      }
      else
      {
	report_file_error(File_Error(errno, (source_dir + *it), "update_from_dir:3"));
	return -1;
      }
      ++it;
    }
    osm_updater.finish_updater();
  }
  catch (File_Error e)
  {
    report_file_error(e);
    return -1;
  }
  
  return 0;
}
