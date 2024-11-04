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
#include "../core/type_meta.h"
#include "../frontend/output.h"
#include "map_file_replicator.h"
#include "tags_global_writer.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>


struct File_Format_Version_Checker
{
  File_Format_Version_Checker() {}
  void check_bin_up_to_date(Transaction& transaction, int min_acceptable, File_Properties& file_properties);
  void check_map_present(Transaction& transaction, File_Properties& file_properties);

  bool empty() const { return bin_files_to_update.empty() && map_files_to_replicate.empty(); }

  std::vector< File_Properties* > bin_files_to_update;
  std::vector< File_Properties* > map_files_to_replicate;
};


void File_Format_Version_Checker::check_bin_up_to_date(
    Transaction& transaction, int min_acceptable, File_Properties& file_properties)
{
  constexpr auto current_version = File_Blocks_Index_Structure_Params::FILE_FORMAT_VERSION;

  auto idx = transaction.data_index(&file_properties);
  if (idx && !idx->empty() && idx->get_file_format_version() < min_acceptable)
  {
    std::cerr<<"Migrate "<<file_properties.get_file_name_trunk()
        <<" from "<<idx->get_file_format_version()<<" to "<<current_version<<'\n';
    bin_files_to_update.push_back(&file_properties);
  }
}


void File_Format_Version_Checker::check_map_present(
    Transaction& transaction, File_Properties& file_properties)
{
  auto idx = transaction.random_index(&file_properties);
  if (!file_exists(idx->get_map_file_name()))
  {
    std::cerr<<"Replicate "<<file_properties.get_file_name_trunk()<<'\n';
    map_files_to_replicate.push_back(&file_properties);
  }
}


void check_all_files(File_Format_Version_Checker& ver_checker, Transaction&& transaction)
{
  ver_checker.check_map_present(transaction, *osm_base_settings().NODES);
  ver_checker.check_map_present(transaction, *osm_base_settings().WAYS);
  ver_checker.check_map_present(transaction, *osm_base_settings().RELATIONS);

  ver_checker.check_map_present(transaction, *attic_settings().NODES);
  ver_checker.check_map_present(transaction, *attic_settings().WAYS);
  ver_checker.check_map_present(transaction, *attic_settings().RELATIONS);

  ver_checker.check_bin_up_to_date(transaction, 7561, *osm_base_settings().NODE_TAGS_GLOBAL);
  ver_checker.check_bin_up_to_date(transaction, 7561, *osm_base_settings().WAY_TAGS_GLOBAL);
  ver_checker.check_bin_up_to_date(transaction, 7561, *osm_base_settings().RELATION_TAGS_GLOBAL);

  ver_checker.check_bin_up_to_date(transaction, 7561, *attic_settings().NODE_TAGS_GLOBAL);
  ver_checker.check_bin_up_to_date(transaction, 7561, *attic_settings().WAY_TAGS_GLOBAL);
  ver_checker.check_bin_up_to_date(transaction, 7561, *attic_settings().RELATION_TAGS_GLOBAL);

  ver_checker.check_bin_up_to_date(transaction, 7562, *attic_settings().NODE_CHANGELOG);
  ver_checker.check_bin_up_to_date(transaction, 7562, *attic_settings().WAY_CHANGELOG);
  ver_checker.check_bin_up_to_date(transaction, 7562, *attic_settings().RELATION_CHANGELOG);
  
  ver_checker.check_bin_up_to_date(transaction, 7601, *meta_settings().NODES_META);
  ver_checker.check_bin_up_to_date(transaction, 7601, *meta_settings().WAYS_META);
  ver_checker.check_bin_up_to_date(transaction, 7601, *meta_settings().RELATIONS_META);
  
  ver_checker.check_bin_up_to_date(transaction, 7601, *attic_settings().NODES_META);
  ver_checker.check_bin_up_to_date(transaction, 7601, *attic_settings().WAYS_META);
  ver_checker.check_bin_up_to_date(transaction, 7601, *attic_settings().RELATIONS_META);
}


template< typename Skeleton >
void migrate_changelog(Osm_Backend_Callback* callback, Transaction& transaction)
{
  callback->migration_started(changelog_file_properties< Skeleton >()->get_file_name_trunk());

  Block_Backend< Timestamp, Change_Entry< typename Skeleton::Id_Type > >
      from_db(transaction.data_index(changelog_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::writeable, false, transaction.get_db_dir(), ".next");
  Block_Backend< Timestamp, typename Skeleton::Id_Type >
      into_db(into_transaction.data_index(changelog_file_properties< Skeleton >()));

  std::map< Timestamp, std::vector< typename Skeleton::Id_Type > > db_to_insert;
  uint64_t elem_cnt = 0;
  for (auto from_it = from_db.flat_begin(); !(from_it == from_db.flat_end()); ++from_it)
  {
    db_to_insert[from_it.index()].push_back(from_it.object().elem_id);

    if (++elem_cnt >= 256*1024*1024)
    {
      callback->migration_flush();
      for (auto& i : db_to_insert)
        std::sort(i.second.begin(), i.second.end());
      into_db.update({}, db_to_insert);
      db_to_insert.clear();
      elem_cnt = 0;
    }
  }

  callback->migration_flush();
  for (auto& i : db_to_insert)
    std::sort(i.second.begin(), i.second.end());
  into_db.update({}, db_to_insert);

  callback->migration_completed();
}


template< typename Index, typename From_Db >
void migrate_meta(
    From_Db& from_db, Block_Backend< Index, Meta_Per_Changeset_Skeleton >& into_db,
    Osm_Backend_Callback* callback)
{
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > db_to_insert;

  auto it = from_db.flat_begin();
  uint64_t obj_cnt = 0;
  std::map< uint64_t, Meta_Per_Changeset_Skeleton > changesets_by_id;
  
  if (!(it == from_db.flat_end()))
  {
    Index last_idx = it.index();
    for (; !(it == from_db.flat_end()); ++it)
    {
      if (!(it.index() == last_idx))
      {
        auto& into = db_to_insert[last_idx];
        for (const auto& j : changesets_by_id)
          into.push_back(j.second);

        changesets_by_id.clear();
        last_idx = it.index();

        if (obj_cnt >= 64*1024*1024)
        {
          callback->migration_flush();
          into_db.update({}, db_to_insert);
          db_to_insert.clear();
          obj_cnt = 0;
        }
      }
      
      ++obj_cnt;
      auto cit = changesets_by_id.insert({
          it.object().changeset,
          Meta_Per_Changeset_Skeleton(it.object().changeset, false, it.object().user_id) }).first;
      cit->second.add_ref({ it.object().ref.val(), it.object().version, it.object().timestamp });
    }

    auto& into = db_to_insert[last_idx];
    for (const auto& j : changesets_by_id)
      into.push_back(j.second);

    into_db.update({}, db_to_insert);
  }
}


template< typename Index, typename Skeleton >
void migrate_current_meta(Osm_Backend_Callback* callback, Transaction& transaction)
{
  callback->migration_started(current_meta_file_properties< Skeleton >()->get_file_name_trunk());

  Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
      from_db(transaction.data_index(current_meta_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  Block_Backend< Index, Meta_Per_Changeset_Skeleton >
      into_db(into_transaction.data_index(current_meta_file_properties< Skeleton >()));

  migrate_meta< Index, Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > >(
      from_db, into_db, callback);

  callback->migration_completed();
}


template< typename Index, typename Skeleton >
void migrate_attic_meta(Osm_Backend_Callback* callback, Transaction& transaction)
{
  callback->migration_started(attic_meta_file_properties< Skeleton >()->get_file_name_trunk());

  Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
      from_db(transaction.data_index(attic_meta_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  Block_Backend< Index, Meta_Per_Changeset_Skeleton >
      into_db(into_transaction.data_index(attic_meta_file_properties< Skeleton >()));

  migrate_meta< Index, Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > >(
      from_db, into_db, callback);

  callback->migration_completed();
}


void migrate_listed_files(
    File_Format_Version_Checker& ver_checker, Transaction&& transaction, Osm_Backend_Callback* callback,
    uint64_t flush_limit)
{
  for (auto i : ver_checker.bin_files_to_update)
  {
    if (i == osm_base_settings().NODE_TAGS_GLOBAL)
      migrate_current_global_tags< Node_Skeleton >(callback, transaction);
    else if (i == osm_base_settings().WAY_TAGS_GLOBAL)
      migrate_current_global_tags< Way_Skeleton >(callback, transaction);
    else if (i == osm_base_settings().RELATION_TAGS_GLOBAL)
      migrate_current_global_tags< Relation_Skeleton >(callback, transaction);
    else if (i == attic_settings().NODE_TAGS_GLOBAL)
      migrate_attic_global_tags< Node_Skeleton >(callback, transaction);
    else if (i == attic_settings().WAY_TAGS_GLOBAL)
      migrate_attic_global_tags< Way_Skeleton >(callback, transaction);
    else if (i == attic_settings().RELATION_TAGS_GLOBAL)
      migrate_attic_global_tags< Relation_Skeleton >(callback, transaction);
    else if (i == meta_settings().NODES_META)
      migrate_current_meta< Uint32_Index, Node_Skeleton >(callback, transaction);
    else if (i == meta_settings().WAYS_META)
      migrate_current_meta< Uint31_Index, Way_Skeleton >(callback, transaction);
    else if (i == meta_settings().RELATIONS_META)
      migrate_current_meta< Uint31_Index, Relation_Skeleton >(callback, transaction);
    else if (i == attic_settings().NODES_META)
      migrate_attic_meta< Uint32_Index, Node_Skeleton >(callback, transaction);
    else if (i == attic_settings().WAYS_META)
      migrate_attic_meta< Uint31_Index, Way_Skeleton >(callback, transaction);
    else if (i == attic_settings().RELATIONS_META)
      migrate_attic_meta< Uint31_Index, Relation_Skeleton >(callback, transaction);
    else if (i == attic_settings().NODE_CHANGELOG)
      migrate_changelog< Node_Skeleton >(callback, transaction);
    else if (i == attic_settings().WAY_CHANGELOG)
      migrate_changelog< Way_Skeleton >(callback, transaction);
    else if (i == attic_settings().RELATION_CHANGELOG)
      migrate_changelog< Relation_Skeleton >(callback, transaction);
  }
  for (auto i : ver_checker.map_files_to_replicate)
  {
    if (i == osm_base_settings().NODES)
      replicate_current_map_file< Node::Index, Node_Skeleton >(callback, transaction, flush_limit/4);
    else if (i == osm_base_settings().WAYS)
      replicate_current_map_file< Way::Index, Way_Skeleton >(callback, transaction, flush_limit/4);
    else if (i == osm_base_settings().RELATIONS)
      replicate_current_map_file< Relation::Index, Relation_Skeleton >(callback, transaction, flush_limit/4);
    else if (i == attic_settings().NODES)
      replicate_attic_map_file< Node::Index, Node_Skeleton, Node_Skeleton >(callback, transaction, flush_limit/4);
    else if (i == attic_settings().WAYS)
      replicate_attic_map_file< Way::Index, Way_Skeleton, Way_Delta >(callback, transaction, flush_limit/4);
    else if (i == attic_settings().RELATIONS)
      replicate_attic_map_file< Relation::Index, Relation_Skeleton, Relation_Delta >(
          callback, transaction, flush_limit/4);
  }
}


class Dispatcher_Write_Guard
{
public:
  Dispatcher_Write_Guard(Dispatcher_Client* dispatcher_client, Logger& logger);
  ~Dispatcher_Write_Guard();
  void commit();

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
    dispatcher_client->migrate_start();
    logger->annotated_log("migrate_start() end");
  }
}


void Dispatcher_Write_Guard::commit()
{
  if (dispatcher_client)
  {
    logger->annotated_log("migrate_commit() start");
    dispatcher_client->migrate_commit();
    logger->annotated_log("migrate_commit() end");
  }
  dispatcher_client = 0;
}


Dispatcher_Write_Guard::~Dispatcher_Write_Guard()
{
  if (dispatcher_client)
  {
    logger->annotated_log("write_rollback() start");
    dispatcher_client->migrate_rollback();
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
  uint64_t flush_limit = 16ull*1024*1024*1024;

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
        dispatcher_client.request_read_and_idx(1, 1, 0, 0);
        logger.annotated_log("migrate_request_read_and_idx() end");

        check_all_files(ver_checker, Nonsynced_Transaction(
            Access_Mode::readonly, false, dispatcher_client.get_db_dir(), ""));

        logger.annotated_log("migrate_read_idx_finished() start");
        dispatcher_client.read_idx_finished();
        logger.annotated_log("migrate_read_idx_finished() end");

        Osm_Backend_Callback* callback = get_verbatim_callback();
        callback->set_db_dir(dispatcher_client.get_db_dir());

        if (migrate && !ver_checker.empty())
        {
          Dispatcher_Write_Guard guard(&dispatcher_client, logger);
          migrate_listed_files(
            ver_checker, Nonsynced_Transaction(
                Access_Mode::writeable, false, dispatcher_client.get_db_dir(), ""), callback, flush_limit);
          guard.commit();
        }
        delete callback;
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
      Osm_Backend_Callback* callback = get_verbatim_callback();
      callback->set_db_dir(db_dir);

      check_all_files(ver_checker, Nonsynced_Transaction(
          Access_Mode::readonly, false, db_dir, ""));

      if (migrate && !ver_checker.empty())
        migrate_listed_files(
            ver_checker, Nonsynced_Transaction(Access_Mode::writeable, false, db_dir, ""), callback, flush_limit);

      delete callback;
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
