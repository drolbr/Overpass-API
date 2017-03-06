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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__OSM_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__OSM_UPDATER_H

#include "../../template_db/dispatcher_client.h"
#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


class Osm_Updater
{
  public:
    Osm_Updater(Osm_Backend_Callback* callback_, const std::string& data_version,
		meta_modes meta, unsigned int flush_limit);
    Osm_Updater(Osm_Backend_Callback* callback_, std::string db_dir, const std::string& data_version,
		meta_modes meta, unsigned int flush_limit);
    ~Osm_Updater();

    void finish_updater();
    void parse_file_completely(FILE* in);

  private:
    Nonsynced_Transaction* transaction;
    Dispatcher_Client* dispatcher_client;
    Node_Updater* node_updater_;
    Way_Updater* way_updater_;
    Relation_Updater* relation_updater_;
    std::string db_dir_;
    meta_modes meta;

    void flush();
};

void parse_nodes_only(FILE* in);
void parse_ways_only(FILE* in);
void parse_relations_only(FILE* in);

#endif
