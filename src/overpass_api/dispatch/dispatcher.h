#ifndef ORG__OVERPASS_API__DISPATCHER
#define ORG__OVERPASS_API__DISPATCHER

#include "../core/basic_types.h"

using namespace std;

const int SHM_SIZE = 20+12+2*(256+4);
const int OFFSET_BACK = 20;
const int OFFSET_DB_1 = OFFSET_BACK+12;
const int OFFSET_DB_2 = OFFSET_DB_1+(256+4);

const uint32 REGISTER_PID = 16;
const uint32 SET_LIMITS = 17;
const uint32 UNREGISTER_PID = 18;
const uint32 SERVER_STATE = 19;

const uint32 QUERY_REJECTED = 32;

#endif
