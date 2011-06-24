#ifndef DE__OSM3S___OVERPASS_API__DISPATCH__RESOURCE_MANAGER_H
#define DE__OSM3S___OVERPASS_API__DISPATCH__RESOURCE_MANAGER_H

#include <ctime>
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../osm-backend/area_updater.h"

using namespace std;

struct Statement;

class Resource_Manager
{
public:
  Resource_Manager(Transaction& transaction_)
      : transaction(&transaction_), start_time(time(NULL)),
        max_allowed_time(0), max_allowed_space(0) {}
  
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
  void set_limits(uint32 max_allowed_time_, uint64 max_allowed_space_)
  {
    max_allowed_time = max_allowed_time_;
    max_allowed_space = max_allowed_space_;
  }
  
  Transaction& get_transaction() { return *transaction; }
  
private:
  map< string, Set > sets_;
  vector< const Set* > set_stack;
  Area_Updater area_updater_;
  Transaction* transaction;
  int start_time;
  uint32 max_allowed_time;
  uint64 max_allowed_space;
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
