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

#include "../../template_db/dispatcher_client.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "tags_global_writer.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>


struct File_Format_Version_Checker
{
  File_Format_Version_Checker() {}
  void check(Transaction& transaction, File_Properties& file_properties);

  std::vector< File_Properties* > files_to_update;
};


void File_Format_Version_Checker::check(Transaction& transaction, File_Properties& file_properties)
{
  constexpr auto current_version = File_Blocks_Index_Structure_Params::FILE_FORMAT_VERSION;
  
  auto idx = transaction.data_index(&file_properties);
  if (idx->get_file_format_version() <= 7560)
  {
    std::cerr<<"Migrate "<<file_properties.get_file_name_trunk()
        <<" from "<<idx->get_file_format_version()<<" to "<<current_version<<'\n';
    files_to_update.push_back(&file_properties);
  }
}


class Dispatcher_Write_Guard
{
public:
  Dispatcher_Write_Guard(Dispatcher_Client* dispatcher_client, Logger& logger);
  ~Dispatcher_Write_Guard();
  void clear();
  
private:
  Dispatcher_Client* dispatcher_client;
  Logger* logger;
};


Dispatcher_Write_Guard::Dispatcher_Write_Guard(Dispatcher_Client* dispatcher_client_, Logger& logger_)
    : dispatcher_client(dispatcher_client_), logger(&logger_)
{
  if (dispatcher_client)
  {
    logger->annotated_log("migrate_start() start");
    dispatcher_client->write_start();
    logger->annotated_log("migrate_start() end");
  }
}


void Dispatcher_Write_Guard::clear()
{
  if (dispatcher_client)
  {
    logger->annotated_log("migrate_commit() start");
    dispatcher_client->write_commit();
    logger->annotated_log("migrate_commit() end");
  }
  dispatcher_client = 0;
}


Dispatcher_Write_Guard::~Dispatcher_Write_Guard()
{
  if (dispatcher_client)
  {
    logger->annotated_log("write_rollback() start");
    dispatcher_client->write_rollback();
    logger->annotated_log("write_rollback() end");
  }
}


int main(int argc, char* argv[])
{
  // read command line arguments
  std::string db_dir;
  bool transactional = true;
  Database_Meta_State meta;
  bool abort = false;
  bool migrate = false;
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
    else if (!(strncmp(argv[argpos], "--migrate", 9)))
      migrate = true;
    else if (!(strncmp(argv[argpos], "--data-only", 11)))
      meta.set_mode(Database_Meta_State::only_data);
    else if (!(strncmp(argv[argpos], "--meta", 6)))
      meta.set_mode(Database_Meta_State::keep_meta);
    else if (!(strncmp(argv[argpos], "--keep-attic", 12)))
      meta.set_mode(Database_Meta_State::keep_attic);
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
  if (abort)
  {
    std::cerr<<"Usage: "<<argv[0]<<" [--migrate] [--db-dir=DIR] [--version=VER] [--meta|--keep-attic] [--flush-size=FLUSH_SIZE]\n";
    return 1;
  }

  File_Format_Version_Checker ver_checker;
  try
  {
    if (transactional)
    {
      Dispatcher_Client dispatcher_client(osm_base_settings().shared_name);
      Logger logger(dispatcher_client.get_db_dir());
      try
      {
        logger.annotated_log("migrate_request_read_and_idx() start");
        dispatcher_client.request_read_and_idx(1, 1, 0);
        logger.annotated_log("migrate_request_read_and_idx() end");

        {
          Nonsynced_Transaction transaction(false, false, dispatcher_client.get_db_dir(), "");
          ver_checker.check(transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
          ver_checker.check(transaction, *osm_base_settings().WAY_TAGS_GLOBAL);
          ver_checker.check(transaction, *osm_base_settings().RELATION_TAGS_GLOBAL);
          ver_checker.check(transaction, *attic_settings().NODE_TAGS_GLOBAL);
          ver_checker.check(transaction, *attic_settings().WAY_TAGS_GLOBAL);
          ver_checker.check(transaction, *attic_settings().RELATION_TAGS_GLOBAL);
        }

        logger.annotated_log("migrate_read_idx_finished() start");
        dispatcher_client.read_idx_finished();
        logger.annotated_log("migrate_read_idx_finished() end");

        if (migrate && !ver_checker.files_to_update.empty())
        {
          Dispatcher_Write_Guard guard(&dispatcher_client, logger);
          Nonsynced_Transaction transaction(true, false, dispatcher_client.get_db_dir(), "");

          for (auto i : ver_checker.files_to_update)
          {
            if (i == osm_base_settings().NODE_TAGS_GLOBAL)
              migrate_current_global_tags< Node_Skeleton >(transaction);
            else if (i == osm_base_settings().WAY_TAGS_GLOBAL)
              migrate_current_global_tags< Way_Skeleton >(transaction);
            else if (i == osm_base_settings().RELATION_TAGS_GLOBAL)
              migrate_current_global_tags< Relation_Skeleton >(transaction);
            // attic ...
          }
        }
      }
      catch (const File_Error& e)
      {
        std::ostringstream out;
        out<<e.origin<<' '<<e.filename<<' '<<e.error_number<<' '<<strerror(e.error_number);
        logger.annotated_log(out.str());
        throw;
      }
    }
    else
    {
      {
        Nonsynced_Transaction transaction(false, false, db_dir, "");
        ver_checker.check(transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
        ver_checker.check(transaction, *osm_base_settings().WAY_TAGS_GLOBAL);
        ver_checker.check(transaction, *osm_base_settings().RELATION_TAGS_GLOBAL);
        ver_checker.check(transaction, *attic_settings().NODE_TAGS_GLOBAL);
        ver_checker.check(transaction, *attic_settings().WAY_TAGS_GLOBAL);
        ver_checker.check(transaction, *attic_settings().RELATION_TAGS_GLOBAL);
      }

      if (migrate && !ver_checker.files_to_update.empty())
      {
        Nonsynced_Transaction transaction(true, false, db_dir, "");

        for (auto i : ver_checker.files_to_update)
        {
          if (i == osm_base_settings().NODE_TAGS_GLOBAL)
            migrate_current_global_tags< Node_Skeleton >(transaction);
          else if (i == osm_base_settings().WAY_TAGS_GLOBAL)
            migrate_current_global_tags< Way_Skeleton >(transaction);
          else if (i == osm_base_settings().RELATION_TAGS_GLOBAL)
            migrate_current_global_tags< Relation_Skeleton >(transaction);
          // attic ...
        }
      }
    }
  }
  catch(Context_Error e)
  {
    std::cerr<<"Context error: "<<e.message<<'\n';
    return 3;
  }
  catch (File_Error e)
  {
    report_file_error(e);
    return 2;
  }

  return 0;
}
