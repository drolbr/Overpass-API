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
      : transaction(&transaction_), start_time(time(NULL)) {}
  
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
  
  Transaction& get_transaction() { return *transaction; }
  
private:
  map< string, Set > sets_;
  vector< const Set* > set_stack;
  Area_Updater area_updater_;
  Transaction* transaction;
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
