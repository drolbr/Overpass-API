#ifndef PROCESS_RULES_DEFINED
#define PROCESS_RULES_DEFINED

#include <string>

#include <mysql.h>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

void process_rules(MYSQL* mysql, const string& current_db, uint32 max_version);

#endif
