/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
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
      output_handle(0), factory(0), template_name("default.wiki"), template_contains_js_(false)
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

  if (attributes["output"] == "xml" || attributes["output"] == "json" || attributes["output"] == "custom"
      || attributes["output"] == "popup")
    type = attributes["output"];
  else
  {
    ostringstream temp;
    temp<<"For the attribute \"output\" of the element \"osm-script\""
        <<" the only allowed values are \"xml\", \"json\", \"custom\", or \"popup\".";
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


bool set_output_templates
    (Output_Handle& output, string& header, const string& name, Transaction& transaction)
{
  string data;
  
  ifstream in((transaction.get_db_dir() + "/templates/" + name).c_str());
  while (in.good())
  {
    string buf;
    getline(in, buf);
    data += buf + '\n';
  }
  
  if (data.find("<includeonly>") != string::npos)
  {
    string::size_type start = data.find("<includeonly>") + 13;
    string::size_type end = data.find("</includeonly>");
    data = data.substr(start, end - start);
  }

  bool template_contains_js = (data.find("<script") != string::npos);

  if (data == "")
    data = "\n<p>Template not found.</p>\n";

  string node_template = "\n<p>No {{node:..}} found in template.</p>\n";
  string way_template = "\n<p>No {{way:..}} found in template.</p>\n";
  string relation_template = "\n<p>No {{relation:..}} found in template.</p>\n";
  
  bool header_written = false;
  string::size_type pos = 0;
  while (pos < data.size())
  {
    if (data[pos] == '{')
    {
      if (data.substr(pos, 7) == "{{node:")
      {
	string::size_type end_pos = find_block_end(data, pos);
	
	if (!header_written)
	{
	  header = data.substr(0, pos);
	  header_written = true;
	}
	
	if (end_pos != string::npos)
	{
	  node_template = data.substr(pos + 7, end_pos - pos - 9);
	  pos = end_pos;
	}
	else
	  pos = data.size();
      }
      else if (data.substr(pos, 6) == "{{way:")
      {
	string::size_type end_pos = find_block_end(data, pos);

	if (!header_written)
	{
	  header = data.substr(0, pos);
	  header_written = true;
	}
	
	if (end_pos != string::npos)
	{
	  way_template = data.substr(pos + 6, end_pos - pos - 8);
	  pos = end_pos;
	}
	else
	  pos = data.size();
      }
      else if (data.substr(pos, 11) == "{{relation:")
      {
	string::size_type end_pos = find_block_end(data, pos);
	
	if (!header_written)
	{
	  header = data.substr(0, pos);
	  header_written = true;
	}
	
	if (end_pos != string::npos)
	{
	  relation_template = data.substr(pos + 11, end_pos - pos - 13);
	  pos = end_pos;
	}
	else
	  pos = data.size();
      }
      else
	++pos;
    }
    else
      ++pos;
  }
  
  if (!header_written)
    header = data.substr(0, pos);
  
  output.set_templates(node_template, way_template, relation_template);
  
  return template_contains_js;
}

void Osm_Script_Statement::execute(Resource_Manager& rman)
{
  rman.set_limits(max_allowed_time, max_allowed_space);

  if (factory)
  {
    if (!output_handle)
      output_handle = new Output_Handle(type);
    if (type == "custom")
      template_contains_js_ =
          set_output_templates(*output_handle, header, template_name, *rman.get_transaction());
    else if (type == "popup")
      output_handle->set_categories(categories);
    for (vector< Statement* >::iterator it = factory->created_statements.begin();
        it != factory->created_statements.end(); ++it)
    {
      Print_Statement* print = dynamic_cast< Print_Statement* >(*it);
      if (print)
	print->set_output_handle(output_handle);
    }
  }
  
  for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
    (*it)->execute(rman);
  
  if (rman.area_updater())
    rman.area_updater()->flush();
  rman.health_check(*this);
}

string Osm_Script_Statement::adapt_url(const string& url) const
{
  if (output_handle)
    return output_handle->adapt_url(url);
  return 0;
}

string process_template(const string& raw_template, uint32 count)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    
    if (raw_template.substr(new_pos + 3, 8) == "count}}}")
    {
      result<<count;
      old_pos = new_pos + 11;
    }
    else
    {
      result<<"{{{";
      old_pos = new_pos + 3;
    }
    new_pos = raw_template.find("{{{", old_pos);
  }
  result<<raw_template.substr(old_pos);
  
  return result.str();
}

void Osm_Script_Statement::write_output() const
{
  if (output_handle)
  {
    if (output_handle->get_written_elements_count() > 0)
      cout<<process_template(header, output_handle->get_written_elements_count());
    cout<<'\n'<<output_handle->get_output();
  }
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
