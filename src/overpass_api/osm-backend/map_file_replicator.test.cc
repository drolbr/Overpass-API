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

#include "../core/datatypes.h"
#include "../frontend/output.h"
#include "map_file_replicator.h"


int main(int argc, char* argv[])
{
  // read command line arguments
  std::string db_dir;
  bool transactional = true;
  Database_Meta_State meta;
  bool abort = false;
  unsigned int flush_limit = 16*1024*1024;

  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((std::string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
        db_dir += '/';
      transactional = false;
    }
    else if (!(strncmp(argv[argpos], "--flush-size=", 13)))
    {
      flush_limit = atoll(std::string(argv[argpos]).substr(13).c_str()) *1024*1024;
      if (flush_limit == 0)
        flush_limit = std::numeric_limits< unsigned int >::max();
    }
    else
    {
      std::cerr<<"Unkown argument: "<<argv[argpos]<<'\n';
      abort = true;
    }
    ++argpos;
  }
  if (abort || transactional)
  {
    std::cerr<<"Usage: "<<argv[0]<<" --db-dir=DIR [--flush-size=FLUSH_SIZE]\n";
    return 1;
  }

  try
  {
    Nonsynced_Transaction transaction(Access_Mode::readonly, false, db_dir, "");

    replicate_current_map_file< Node::Index, Node_Skeleton >(nullptr, transaction);
    replicate_attic_map_file< Node::Index, Node_Skeleton >(nullptr, transaction);
    
    compare_current_map_files< Node::Index, Node_Skeleton >(transaction, 1000000);
    compare_attic_map_files< Node::Index, Node_Skeleton >(transaction, 1000000);
    compare_idx_lists< Node::Index, Node_Skeleton >(transaction);
  }
  catch (File_Error e)
  {
    report_file_error(e);
    return 2;
  }

  return 0;
}
