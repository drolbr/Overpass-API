#ifndef DE__OSM3S__RESOURCE_MANAGER_DEFINED
#define DE__OSM3S__RESOURCE_MANAGER_DEFINED

#include <ctime>
#include "../core/datatypes.h"
#include "../osm-backend/area_updater.h"

using namespace std;

struct Statement;

struct Resource_Manager
{
  Resource_Manager() : start_time(time(NULL)) {}
  
  map< string, Set >& sets()
  {
    return sets_;
  }
  
  Area_Updater& area_updater()
  {
    return area_updater_;
  }

  void push_reference(const Set& set_);
  void pop_reference();

  void health_check(const Statement& stmt);
  
private:
  map< string, Set > sets_;
  vector< const Set* > set_stack;
  Area_Updater area_updater_;
  int start_time;
};

struct Resource_Error
{
  bool timed_out;
  string stmt_name;
  uint line_number;
  uint64 size;
  uint runtime;
};

#endif
