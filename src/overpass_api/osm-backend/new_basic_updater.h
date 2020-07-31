#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NEW_BASIC_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NEW_BASIC_UPDATER_H


#include "../core/datatypes.h"

#include <cstdint>
#include <vector>


struct Pre_Event_Ref
{
  Node_Skeleton::Id_Type ref;
  uint64_t timestamp;
  unsigned int offset;
};


typedef std::vector< Pre_Event_Ref > Pre_Event_Refs;

typedef std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > Id_Dates;

typedef Uint32_Index Uint32;
typedef std::vector< Attic< Uint32 > > Coord_Dates;


void keep_oldest_per_first(std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >& arg);

void keep_oldest_per_coord(Coord_Dates& arg);


#endif
