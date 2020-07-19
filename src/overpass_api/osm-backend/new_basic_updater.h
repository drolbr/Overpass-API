#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NEW_BASIC_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NEW_BASIC_UPDATER_H


#include "../core/datatypes.h"

#include <cstdint>
#include <vector>


typedef std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > Id_Dates_Per_Idx;

typedef Uint32_Index Uint32;
typedef std::vector< Attic< Uint32 > > Coord_Dates_Per_Idx;


void keep_oldest_per_first(std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >& arg);

void keep_oldest_per_coord(Coord_Dates_Per_Idx& arg);


#endif
