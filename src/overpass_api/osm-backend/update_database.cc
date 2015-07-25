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

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../core/settings.h"
#include "../frontend/output.h"
#include "osm_updater.h"

using namespace std;

int main(int argc, char* argv[])
{
  // read command line arguments
  string db_dir, data_version;
  bool transactional = true;
  meta_modes meta = only_data;
  bool produce_augmented_diffs = false;
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
      transactional = false;
    }
    else if (!(strncmp(argv[argpos], "--version=", 10)))
      data_version = ((string)argv[argpos]).substr(10);
    else if (!(strncmp(argv[argpos], "--meta", 6)))
      meta = keep_meta;
    else if (!(strncmp(argv[argpos], "--keep-attic", 12)))
      meta = keep_attic;
    else if (!(strncmp(argv[argpos], "--produce-diff", 14)))
      produce_augmented_diffs = true;
    else if (!(strncmp(argv[argpos], "--flush-size=", 13)))
    {
      flush_limit = atoll(string(argv[argpos]).substr(13).c_str()) *1024*1024;
      if (flush_limit == 0)
        flush_limit = std::numeric_limits< unsigned int >::max();
    }
    else if (!(strncmp(argv[argpos], "--compression_method=", 21)))
    {
      if (string(argv[argpos]).substr(21) == "no")
	basic_settings().compression_method = File_Blocks_Index< Uint31_Index >::NO_COMPRESSION;
      else if (string(argv[argpos]).substr(21) == "gz")
	basic_settings().compression_method = File_Blocks_Index< Uint31_Index >::ZLIB_COMPRESSION;
      else
      {
        cerr<<"For --compression_method, please use \"no\" or \"gz\" as value.\n";
        abort = true;
      }
    }
    else
    {
      cerr<<"Unkown argument: "<<argv[argpos]<<'\n';
      abort = true;
    }
    ++argpos;
  }
  if (!transactional && produce_augmented_diffs)
  {
    cerr<<"Augmented diffs can only be produced with running dispatcher.\n";
    abort = true;
  }
  if (abort)
  {
    cerr<<"Usage: "<<argv[0]<<" [--db-dir=DIR] [--version=VER] [--meta|--keep-attic] [--compression_method=(no|gz)]\n";
    return 0;
  }
  
  try
  {
    if (transactional)
    {
      Osm_Updater osm_updater(get_verbatim_callback(), data_version, meta,
                              produce_augmented_diffs, flush_limit);
      //reading the main document
      osm_updater.parse_file_completely(stdin);
    }
    else
    {
      Osm_Updater osm_updater(get_verbatim_callback(), db_dir, data_version, meta,
                              produce_augmented_diffs, flush_limit);
      //reading the main document
      osm_updater.parse_file_completely(stdin);
    }
  }
  catch (File_Error e)
  {
    report_file_error(e);
  }
  
  return 0;
}
