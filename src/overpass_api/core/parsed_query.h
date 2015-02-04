#ifndef DE__OSM3S___OVERPASS_API__CORE__PARSED_QUERY_H
#define DE__OSM3S___OVERPASS_API__CORE__PARSED_QUERY_H


#include "../frontend/output_handler.h"
#include "geometry.h"


/* This class collects all the information gathered during parsing.
   Thus it models a query immediately before execution. */
class Parsed_Query
{
public:
  Parsed_Query() : output_handler(0), global_bbox_limitation(Bbox_Double::invalid) {}
  ~Parsed_Query() { delete output_handler; }
  
  Output_Handler* get_output_handler() const { return output_handler; }
  void set_output_handler(Output_Handler* handler);
  void set_global_bbox(const Bbox_Double& bbox) { global_bbox_limitation = bbox; }
  
  void trigger_print_bounds() const;
  const Bbox_Double& get_global_bbox_limitation() const { return global_bbox_limitation; }

private:
  // The class has ownership of objects - hence no assignment or copies are allowed
  Parsed_Query(const Parsed_Query&);
  Parsed_Query& operator=(const Parsed_Query&);
  
  Output_Handler* output_handler;
  Bbox_Double global_bbox_limitation;
};


inline void Parsed_Query::set_output_handler(Output_Handler* handler)
{
  delete output_handler;
  output_handler = handler;
}


inline void Parsed_Query::trigger_print_bounds() const
{
  if (global_bbox_limitation.valid() && output_handler)
    output_handler->print_global_bbox(global_bbox_limitation);
}


#endif
