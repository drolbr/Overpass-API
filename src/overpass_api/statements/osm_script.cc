/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fstream>
#include <iostream>
#include <sstream>

#include "../core/settings.h"
#include "../frontend/print_target.h"
#include "osm_script.h"
#include "print.h"
// #include "area_query.h"

using namespace std;

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Osm_Script_Statement > Osm_Script_Statement::statement_maker("osm-script");

Osm_Script_Statement::Osm_Script_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_), max_allowed_time(0), max_allowed_space(0), type("xml"),
      factory(0),
      node_template_name("default.node"), way_template_name("default.way"),
      relation_template_name("default.relation")
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

  if (attributes["output"] == "xml" || attributes["output"] == "json" || attributes["output"] == "custom")
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
  
  substatements.push_back(statement);
}

void Osm_Script_Statement::forecast()
{
}

string load_template(const string& name, Transaction& transaction)
{
  string result;
  
  ifstream in((transaction.get_db_dir() + "/templates/" + name).c_str());
  while (in.good())
  {
    string buf;
    getline(in, buf);
    result += buf + '\n';
  }
  
  if (result == "")
    result = "\n<p>Template not found.</p>\n";
  
  return result;
}

void Osm_Script_Statement::execute(Resource_Manager& rman)
{
  rman.set_limits(max_allowed_time, max_allowed_space);

  if (factory)
  {
    output_handle = new Output_Handle(type);
    if (type == "custom")
    {
      output_handle->set_templates
          (load_template("default.node", *rman.get_transaction()),
	   load_template("default.way", *rman.get_transaction()),
	   load_template("default.relation", *rman.get_transaction()));
    }
    for (vector< Statement* >::iterator it = factory->created_statements.begin();
        it != factory->created_statements.end(); ++it)
    {
      Print_Statement* print = dynamic_cast< Print_Statement* >(*it);
      if (print)
	print->set_output_handle(output_handle);
    }
  }
  
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

string Osm_Script_Statement::adapt_url(const string& url) const
{
  if (output_handle)
    return output_handle->adapt_url(url);
  return 0;
}

string Osm_Script_Statement::get_output() const
{
  if (output_handle)
    return output_handle->get_output();
  return 0;
}

uint32 Osm_Script_Statement::get_written_elements_count() const
{
  if (output_handle)
    return output_handle->get_written_elements_count();
  return 0;
}

Osm_Script_Statement::~Osm_Script_Statement()
{
  if (output_handle)
    delete output_handle;
}
