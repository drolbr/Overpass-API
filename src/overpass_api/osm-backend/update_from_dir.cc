/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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


struct Node_Caller
{
  public:
    static void parse(FILE* osc_file) { parse_nodes_only(osc_file); }
};

struct Way_Caller
{
  public:
    static void parse(FILE* osc_file) { parse_ways_only(osc_file); }
};

struct Relation_Caller
{
  public:
    static void parse(FILE* osc_file) { parse_relations_only(osc_file); }
};

template < class Caller >
void process_source_files(const string& source_dir,
		          const vector< string >& source_file_names)
{
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
      Caller::parse(osc_file);
      
      fclose(osc_file);
    }
    else
      throw File_Error(errno, (source_dir + *it), "update_from_dir:1");
    ++it;
  }
};

int main(int argc, char* argv[])
{
  // read command line arguments
  string source_dir, db_dir, data_version;
  vector< string > source_file_names;
  meta_modes meta = only_data;
  bool abort = false;
  unsigned int flush_limit = 16*1024*1024;
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    else if (!(strncmp(argv[argpos], "--osc-dir=", 10)))
    {
      source_dir = ((string)argv[argpos]).substr(10);
      if ((source_dir.size() > 0) && (source_dir[source_dir.size()-1] != '/'))
	source_dir += '/';
    }
    else if (!(strncmp(argv[argpos], "--version=", 10)))
      data_version = ((string)argv[argpos]).substr(10);
    else if (!(strncmp(argv[argpos], "--meta", 6)))
      meta = keep_meta;
    else if (!(strncmp(argv[argpos], "--keep-attic", 12)))
      meta = keep_attic;
    else if (!(strncmp(argv[argpos], "--flush-size=", 13)))
    {
      flush_limit = atoll(string(argv[argpos]).substr(13).c_str()) *1024*1024;
      if (flush_limit == 0)
        flush_limit = std::numeric_limits< unsigned int >::max();
    }
    else
    {
      cerr<<"Unkown argument: "<<argv[argpos]<<'\n';
      abort = true;
    }
    ++argpos;
  }
  if (abort)
  {
    cerr<<"Usage: "<<argv[0]<<" --osc-dir=DIR"
          " [--db-dir=DIR] [--version=VER] [--meta|--keep-attic] [--flush-size=FLUSH_SIZE]\n";
    return -1;
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
    if (db_dir == "")
    {
      Osm_Updater osm_updater(get_verbatim_callback(), data_version, meta, flush_limit);
      get_verbatim_callback()->parser_started();
      
      process_source_files< Node_Caller >(source_dir, source_file_names);
      process_source_files< Way_Caller >(source_dir, source_file_names);
      process_source_files< Relation_Caller >(source_dir, source_file_names);
      
      osm_updater.finish_updater();
    }
    else
    {
      Osm_Updater osm_updater(get_verbatim_callback(), db_dir, data_version, meta, flush_limit);
      get_verbatim_callback()->parser_started();
      
      process_source_files< Node_Caller >(source_dir, source_file_names);
      process_source_files< Way_Caller >(source_dir, source_file_names);
      process_source_files< Relation_Caller >(source_dir, source_file_names);
      
      osm_updater.finish_updater();
    }
  }
  catch(Context_Error e)
  {
    std::cerr<<"Context error: "<<e.message<<'\n';
    return 3;
  }
  catch (const File_Error& e)
  {
    report_file_error(e);
    return -1;
  }
  
  return 0;
}
