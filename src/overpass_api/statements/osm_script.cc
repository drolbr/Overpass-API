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
#include "bbox_query.h"
#include "osm_script.h"
#include "print.h"
// #include "area_query.h"

using namespace std;

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Osm_Script_Statement > Osm_Script_Statement::statement_maker("osm-script");

Osm_Script_Statement::Osm_Script_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation_)
    : Statement(line_number_), bbox_limitation(bbox_limitation_), bbox_statement(0),
       desired_timestamp(NOW), comparison_timestamp(0), max_allowed_time(0), max_allowed_space(0),
       type("xml"), output_handle(0), factory(0),
       template_name("default.wiki"), template_contains_js_(false)
{
  map< string, string > attributes;
  
  attributes["bbox"] = "";
  attributes["timeout"] = "180";
  attributes["element-limit"] = "536870912";
  attributes["output"] = "xml";
  attributes["date"] = "";
  attributes["from"] = "";
  
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
  
  if (attributes["bbox"] != "")
  {
    map< string, string > bbox_attributes;
    
    string& bbox_s = attributes["bbox"];
    string::size_type pos = bbox_s.find(",");
    string::size_type from = 0;
    if (pos != string::npos)
    {
      bbox_attributes["s"] = bbox_s.substr(0, pos);
      from = pos + 1;
      pos = bbox_s.find(",", from);
    }
    else
    {
      ostringstream temp;
      temp<<"A bounding box needs four comma separated values.";
      add_static_error(temp.str());
    }
    if (pos != string::npos)
    {
      bbox_attributes["w"] = bbox_s.substr(from, pos-from);
      from = pos + 1;
      pos = bbox_s.find(",", from);
    }
    else
    {
      ostringstream temp;
      temp<<"A bounding box needs four comma separated values.";
      add_static_error(temp.str());
    }
    if (pos != string::npos)
    {
      bbox_attributes["n"] = bbox_s.substr(from, pos-from);
      from = pos + 1;
    }
    else
    {
      ostringstream temp;
      temp<<"A bounding box needs four comma separated values.";
      add_static_error(temp.str());
    }
    bbox_attributes["e"] = bbox_s.substr(from);

    double south = atof(bbox_attributes["s"].c_str());
    double north = atof(bbox_attributes["n"].c_str());
    if (south < -90.0 || south > 90.0 || north < -90.0 || north > 90.0)
    {
      ostringstream temp;
      temp<<"Latitudes in bounding boxes must be between -90.0 and 90.0.";
      add_static_error(temp.str());
    }
    
    double west = atof(bbox_attributes["w"].c_str());
    double east = atof(bbox_attributes["e"].c_str());
    if (west < -180.0 || west > 180.0 || east < -180.0 || east > 180.0)
    {
      ostringstream temp;
      temp<<"Longitudes in bounding boxes must be between -1800.0 and 180.0.";
      add_static_error(temp.str());
    }
    
    if (south >= -90.0 && south <= 90.0 && north >= -90.0 && north <= 90.0
        && west >= -180.0 && west <= 180.0 && east >= -180.0 && east <= 180.0)
    {
      bbox_statement = new Bbox_Query_Statement(line_number_, bbox_attributes, 0);
      bbox_limitation = bbox_statement->get_query_constraint();
    }
  }
  
  if (attributes["date"] != "")
  {
    string timestamp = attributes["date"];
  
    desired_timestamp = 0;
    desired_timestamp |= (atoll(timestamp.c_str())<<26); //year
    desired_timestamp |= (atoi(timestamp.c_str()+5)<<22); //month
    desired_timestamp |= (atoi(timestamp.c_str()+8)<<17); //day
    desired_timestamp |= (atoi(timestamp.c_str()+11)<<12); //hour
    desired_timestamp |= (atoi(timestamp.c_str()+14)<<6); //minute
    desired_timestamp |= atoi(timestamp.c_str()+17); //second
  
    if (desired_timestamp == 0)
    {
      ostringstream temp;
      temp<<"The attribute \"date\" must be empty or contain a timestamp exactly in the form \"yyyy-mm-ddThh:mm:ssZ\".";
      add_static_error(temp.str());
    }
  }
  if (attributes["from"] != "")
  {
    string timestamp = attributes["from"];
  
    comparison_timestamp = 0;
    comparison_timestamp |= (atoll(timestamp.c_str())<<26); //year
    comparison_timestamp |= (atoi(timestamp.c_str()+5)<<22); //month
    comparison_timestamp |= (atoi(timestamp.c_str()+8)<<17); //day
    comparison_timestamp |= (atoi(timestamp.c_str()+11)<<12); //hour
    comparison_timestamp |= (atoi(timestamp.c_str()+14)<<6); //minute
    comparison_timestamp |= atoi(timestamp.c_str()+17); //second
  
    if (comparison_timestamp == 0)
    {
      ostringstream temp;
      temp<<"The attribute \"from\" must be empty or contain a timestamp exactly in the form \"yyyy-mm-ddThh:mm:ssZ\".";
      add_static_error(temp.str());
    }
  }
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
    
    if (bbox_statement)
      output_handle->print_bounds(bbox_statement->get_south(), bbox_statement->get_west(),
                                  bbox_statement->get_north(), bbox_statement->get_east());
  }
  
  if (comparison_timestamp > 0)
  {
    rman.set_desired_timestamp(comparison_timestamp);
    
    for (vector< Statement* >::iterator it(substatements.begin());
        it != substatements.end(); ++it)
      (*it)->execute(rman);
    
    rman.sets().clear();
    rman.set_desired_timestamp(desired_timestamp);
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
