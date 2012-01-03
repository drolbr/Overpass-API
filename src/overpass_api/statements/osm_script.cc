#include <iostream>
#include <sstream>

#include "../core/settings.h"
#include "../frontend/print_target.h"
#include "osm_script.h"
// #include "area_query.h"

using namespace std;

//-----------------------------------------------------------------------------

Osm_Script_Statement::Osm_Script_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_), max_allowed_time(0), max_allowed_space(0), type("xml")
{
  map< string, string > attributes;
  
  attributes["timeout"] = "180";
  attributes["element-limit"] = "536870912";
  attributes["output"] = "xml";
  
  /*attributes["name"] = "";
  attributes["replace"] = "0";
  attributes["version"] = "0";
  attributes["debug"] = "errors";*/
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  int32 timeout(atoi(attributes["timeout"].c_str()));
  if (timeout <= 0)
  {
    ostringstream temp;
    temp<<"For the attribute \"timeout\" of the element \"osm-script\""
        <<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
  max_allowed_time = timeout;
  
  int64 max_space(atoll(attributes["element-limit"].c_str()));
  if (max_space <= 0)
  {
    ostringstream temp;
    temp<<"For the attribute \"element-limit\" of the element \"osm-script\""
        <<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
  max_allowed_space = max_space;

  if (attributes["output"] == "xml" || attributes["output"] == "json")
    type = attributes["output"];
  else
  {
    ostringstream temp;
    temp<<"For the attribute \"output\" of the element \"osm-script\""
        <<" the only allowed values are \"xml\" or \"json\".";
    add_static_error(temp.str());
  }
    
/*  name = attributes["name"];
  replace = atoi(attributes["replace"].c_str());
  version = atoi(attributes["version"].c_str());
  
  if (attributes["debug"] == "quiet")
    set_debug_mode(QUIET);
  else if (attributes["debug"] == "errors")
    set_debug_mode(ERRORS);
  else if (attributes["debug"] == "verbose")
    set_debug_mode(VERBOSE);
  else if (attributes["debug"] == "static")
    set_debug_mode(STATIC_ANALYSIS);
  else
  {
    ostringstream temp;
    temp<<"For the attribute \"debug\" of the element \"osm-script\""
    <<" the only allowed values are \"quiet\", \"errors\", \"verbose\" or \"static\".";
    add_static_error(temp.str());
  }
  
  script_timeout = timeout;
  element_limit = elem_limit;
  ostringstream temp;
  temp<<"Timeout is set to "<<timeout<<", element_limit is "<<elem_limit;
  add_static_remark(temp.str());*/
}

void Osm_Script_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if ((statement->get_name() == "around") ||
      (statement->get_name() == "bbox-query") ||
      (statement->get_name() == "coord-query") ||
      (statement->get_name() == "foreach") ||
      (statement->get_name() == "id-query") ||
      (statement->get_name() == "print") ||
      (statement->get_name() == "query") ||
      (statement->get_name() == "recurse") ||
      (statement->get_name() == "union") ||
      (statement->get_name() == "area-query") ||
      (statement->get_name() == "make-area") ||
      (statement->get_name() == "user") /*||
    (statement->get_name() == "conflict") ||
    (statement->get_name() == "report") ||
    (statement->get_name() == "detect-odd-nodes")*/)
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Osm_Script_Statement::set_output_handle(Output_Handle* output)
{
  Statement::set_output_handle(output);
  
  for (vector< Statement* >::iterator it = substatements.begin();
      it != substatements.end(); ++it)
    (*it)->set_output_handle(output);
}

void Osm_Script_Statement::forecast()
{
/*  for (vector< Statement* >::iterator it(substatements.begin());
  it != substatements.end(); ++it)
  (*it)->forecast(mysql);*/
}

void Osm_Script_Statement::execute(Resource_Manager& rman)
{
  rman.set_limits(max_allowed_time, max_allowed_space);
  set_output_handle(new Output_Handle(type));
  
  stopwatch.start();
  stopwatch.stop(Stopwatch::NO_DISK);
  for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
  {
    (*it)->execute(rman);
    stopwatch.sum((*it)->stopwatch);
  }
  stopwatch.skip();
  
  if (rman.area_updater())
    rman.area_updater()->flush(&stopwatch);
  stopwatch.report(get_name());
  rman.health_check(*this);
}

Osm_Script_Statement::~Osm_Script_Statement()
{
  if (get_output_handle())
    delete get_output_handle();
}
