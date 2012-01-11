#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "foreach.h"

using namespace std;

Generic_Statement_Maker< Foreach_Statement > Foreach_Statement::statement_maker("foreach");

Foreach_Statement::Foreach_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  output = attributes["into"];
}

void Foreach_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  substatements.push_back(statement);
}

void Foreach_Statement::forecast()
{
}

void Foreach_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();
  
  Set base_set(rman.sets()[input]);
  rman.push_reference(base_set);
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(base_set.nodes.begin()); it != base_set.nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].nodes[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(base_set.ways.begin()); it != base_set.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].ways[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
    it(base_set.relations.begin()); it != base_set.relations.end(); ++it)
  {
    for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].relations[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
    it(base_set.areas.begin()); it != base_set.areas.end(); ++it)
  {
    for (vector< Area_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].areas[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  
  if (input == output)
    rman.sets()[output] = base_set;
  
  stopwatch.stop(Stopwatch::NO_DISK);
  stopwatch.report(get_name());
  rman.pop_reference();
  rman.health_check(*this);
}
