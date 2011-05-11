#ifndef ORG__OVERPASS_API__OSM_UPDATER
#define ORG__OVERPASS_API__OSM_UPDATER

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"

using namespace std;

class Osm_Updater
{
  public:
    Osm_Updater(Osm_Backend_Callback* callback_);
    
  private:
    //Nonsynced_Transaction transaction;
    Node_Updater node_updater_;
    Way_Updater way_updater_;
    Relation_Updater relation_updater_;
};

void finish_updater();

void parse_file_completely(FILE* in);

void parse_nodes_only(FILE* in);
void parse_ways_only(FILE* in);
void parse_relations_only(FILE* in);

#endif
