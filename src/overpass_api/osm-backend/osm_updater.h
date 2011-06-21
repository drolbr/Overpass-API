#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__OSM_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__OSM_UPDATER_H

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"

using namespace std;

class Dispatcher_Client;

class Osm_Updater
{
  public:
    Osm_Updater(Osm_Backend_Callback* callback_, const string& data_version);
    Osm_Updater(Osm_Backend_Callback* callback_, string db_dir,
		const string& data_version);
    ~Osm_Updater();

    void finish_updater();
    void parse_file_completely(FILE* in);
    
  private:
    Nonsynced_Transaction* transaction;
    Dispatcher_Client* dispatcher_client;
    Node_Updater* node_updater_;
    Way_Updater* way_updater_;
    Relation_Updater* relation_updater_;
    string db_dir_;

    void flush();
};

void parse_nodes_only(FILE* in);
void parse_ways_only(FILE* in);
void parse_relations_only(FILE* in);

#endif
