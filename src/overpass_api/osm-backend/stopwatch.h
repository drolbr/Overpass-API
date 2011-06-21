#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__STOPWATCH_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__STOPWATCH_H

#include "../core/datatypes.h"

using namespace std;

struct Stopwatch
{
  Stopwatch() : stopwatches(18), read_counts(18) {}
  
  const static int NO_DISK = 0;
  const static int NODES_MAP = 1;
  const static int NODES = 2;
  const static int NODE_TAGS_LOCAL = 3;
  const static int NODE_TAGS_GLOBAL = 4;
  const static int WAYS_MAP = 5;
  const static int WAYS = 6;
  const static int WAY_TAGS_LOCAL = 7;
  const static int WAY_TAGS_GLOBAL = 8;
  const static int RELATIONS_MAP = 9;
  const static int RELATIONS = 10;
  const static int RELATION_ROLES = 11;
  const static int RELATION_TAGS_LOCAL = 12;
  const static int RELATION_TAGS_GLOBAL = 13;
  const static int AREAS = 14;
  const static int AREA_BLOCKS = 15;
  const static int AREA_TAGS_LOCAL = 16;
  const static int AREA_TAGS_GLOBAL = 17;
  
  void start();
  void skip();
  void stop(uint32 account);
  void add(uint32 account, uint read_count)
  {
    read_counts[account] += read_count;
  }
  
  void report(string info) const;
  void sum(const Stopwatch& s);
  
  static void set_error_output(Error_Output* error_output_)
  {
    error_output = error_output_;
  }
  
private:
  static Error_Output* error_output;
  
  double stopwatch;
  vector< double > stopwatches;
  vector< uint > read_counts;
};

#endif
