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

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"
#include "resource_manager.h"
#include "scripting_core.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


template< class TIndex, class Id_Type >
struct Flat_Meta_Collector
{
  public:
    Flat_Meta_Collector(Transaction& transaction, const File_Properties* meta_file_prop = 0);

    void reset();
    const OSM_Element_Metadata_Skeleton< Id_Type >* get(const TIndex& index, Id_Type ref);

    ~Flat_Meta_Collector()
    {
      if (meta_db)
      {
	delete db_it;
	delete meta_db;
      }
    }

  private:
    Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >* meta_db;
    typename Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >::Flat_Iterator*
      db_it;
    TIndex* current_index;
    std::map< OSM_Element_Metadata_Skeleton< Id_Type >, bool > current_objects;
};

/** Implementation --------------------------------------------------------- */

template< class TIndex, class Id_Type >
Flat_Meta_Collector< TIndex, Id_Type >::Flat_Meta_Collector
    (Transaction& transaction, const File_Properties* meta_file_prop)
  : meta_db(0), db_it(0), current_index(0)
{
  meta_db = new Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >
      (transaction.data_index(meta_file_prop));
	
  reset();
}

template< class TIndex, class Id_Type >
void Flat_Meta_Collector< TIndex, Id_Type >::reset()
{
  if (!meta_db)
    return;

  if (db_it)
    delete db_it;
  if (current_index)
  {
    delete current_index;
    current_index = 0;
  }

  db_it = new typename Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >
      ::Flat_Iterator(meta_db->flat_begin());
	
  if (!(*db_it == meta_db->flat_end()))
    current_index = new TIndex(db_it->index());
  while (!(*db_it == meta_db->flat_end()) && (*current_index == db_it->index()))
  {
    current_objects.insert(std::make_pair(db_it->object(), false));
    ++(*db_it);
  }
}

template< class TIndex, class Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Flat_Meta_Collector< TIndex, Id_Type >::get
    (const TIndex& index, Id_Type ref)
{
  if (!meta_db)
    return 0;

  if ((current_index) && (*current_index < index))
  {
    for (typename std::map< OSM_Element_Metadata_Skeleton< Id_Type >, bool >::const_iterator
        it = current_objects.begin(); it != current_objects.end(); ++it)
    {
      if (!it->second)
        std::cout<<"Skipping meta data of "<<std::dec<<it->first.ref.val()
            <<" at "<<std::hex<<current_index->val()<<'\n';
    }
    current_objects.clear();

    while (!(*db_it == meta_db->flat_end()) && (db_it->index() < index))
    {
      std::cout<<"Skipping meta data of "<<std::dec<<db_it->object().ref.val()
          <<" at "<<std::hex<<db_it->index().val()<<'\n';
      ++(*db_it);
    }
    if (!(*db_it == meta_db->flat_end()))
      *current_index = db_it->index();
    while (!(*db_it == meta_db->flat_end()) && (*current_index == db_it->index()))
    {
      current_objects.insert(std::make_pair(db_it->object(), false));
      ++(*db_it);
    }
  }

  typename std::map< OSM_Element_Metadata_Skeleton< Id_Type >, bool >::iterator it
      = current_objects.find(OSM_Element_Metadata_Skeleton< Id_Type >(ref));
  it->second = true;
  if (it != current_objects.end())
    return &it->first;
  else
    return 0;
}

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  // read command line arguments
  std::string db_dir;
  uint log_level = Error_Output::ASSISTING;

  int area_level = 0;

  int argpos = 1;
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((std::string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    else if (!(strcmp(argv[argpos], "--quiet")))
      log_level = Error_Output::QUIET;
    else if (!(strcmp(argv[argpos], "--concise")))
      log_level = Error_Output::CONCISE;
    else if (!(strcmp(argv[argpos], "--progress")))
      log_level = Error_Output::PROGRESS;
    else if (!(strcmp(argv[argpos], "--verbose")))
      log_level = Error_Output::VERBOSE;
    ++argpos;
  }

  Error_Output* error_output(new Console_Output(log_level));
  Statement::set_error_output(error_output);

  // connect to dispatcher and get database dir
  try
  {
    // open read transaction and log this.
    Parsed_Query global_settings;
    Dispatcher_Stub dispatcher(db_dir, error_output, "-- consistency check --\n", keep_meta, area_level,
			       24*60*60, 1024*1024*1024, global_settings);
    Resource_Manager& rman = dispatcher.resource_manager();

    // perform check
    {
      uint32 count = 0;
      Flat_Meta_Collector< Uint32_Index, Node::Id_Type > meta_collector
          (*rman.get_transaction(), meta_settings().NODES_META);
      Block_Backend< Uint32_Index, Node_Skeleton > db
          (rman.get_transaction()->data_index(osm_base_settings().NODES));
      for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
          it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        if (++count >= 1000*1000)
        {
	  dispatcher.ping();
          count = 0;
          std::cout<<"Processed 1000000 nodes.\n";
        }
        if (!meta_collector.get(it.index(), it.object().id))
	  std::cout<<"Missing meta data of "<<std::dec<<it.object().id.val()
	      <<" at "<<std::hex<<it.index().val()<<'\n';
      }
    }

    {
      uint32 count = 0;
      File_Properties* props = meta_settings().WAYS_META;
      File_Blocks_Index_Base* index_base = rman.get_transaction()->data_index(props);
      std::cout<<"ways_meta address "<<index_base<<'\n';
      File_Blocks_Index< Uint31_Index >* index = (File_Blocks_Index< Uint31_Index >*)index_base;
      std::cout<<"ways_meta";
      for (int i = 0; i < (int)index->get_void_blocks().size(); ++i)
	std::cout<<' '<<index->get_void_blocks()[i].first<<' '<<index->get_void_blocks()[i].second;
      std::cout<<'\n';
      Flat_Meta_Collector< Uint31_Index, Way::Id_Type > meta_collector
          (*rman.get_transaction(), props);
      Block_Backend< Uint31_Index, Way_Skeleton > db
          (rman.get_transaction()->data_index(osm_base_settings().WAYS));
      for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
          it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        if (++count >= 100*1000)
        {
	  dispatcher.ping();
	  count = 0;
          std::cout<<"Processed 100000 ways.\n";
        }
        if (!meta_collector.get(it.index(), it.object().id))
	  std::cout<<"Missing meta data of "<<std::dec<<it.object().id.val()
	      <<" at "<<std::hex<<it.index().val()<<'\n';
      }
    }

    {
      uint32 count = 0;
      Flat_Meta_Collector< Uint31_Index, Relation::Id_Type > meta_collector
          (*rman.get_transaction(), meta_settings().RELATIONS_META);
      Block_Backend< Uint31_Index, Relation_Skeleton > db
          (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
      for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
          it(db.flat_begin()); !(it == db.flat_end()); ++it)
      {
        if (++count >= 100*1000)
        {
	  dispatcher.ping();
	  count = 0;
          std::cout<<"Processed 100000 relations.\n";
        }
        if (!meta_collector.get(it.index(), it.object().id))
	  std::cout<<"Missing meta data of "<<std::dec<<it.object().id.val()
	      <<" at "<<std::hex<<it.index().val()<<'\n';
      }
    }

    std::cout<<"done\n";

    return 0;
  }
  catch(File_Error e)
  {
    std::ostringstream temp;
    if (e.origin != "Dispatcher_Stub::Dispatcher_Stub::1")
    {
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
      if (error_output)
        error_output->runtime_error(temp.str());
    }

    return 1;
  }
  catch(Resource_Error e)
  {
    std::ostringstream temp;
    if (e.timed_out)
      temp<<"Query timed out in \""<<e.stmt_name<<"\" at line "<<e.line_number
          <<" after "<<e.runtime<<" seconds.";
    else
      temp<<"Query run out of memory in \""<<e.stmt_name<<"\" at line "
          <<e.line_number<<" using about "<<e.size/(1024*1024)<<" MB of RAM.";
    if (error_output)
      error_output->runtime_error(temp.str());

    return 2;
  }
  catch(Exit_Error e)
  {
    return 3;
  }
}
