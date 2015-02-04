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
#include "../frontend/output_handler_parser.h"
#include "../frontend/print_target.h"
#include "bbox_query.h"
#include "osm_script.h"
#include "print.h"
// #include "area_query.h"


Generic_Statement_Maker< Osm_Script_Statement > Osm_Script_Statement::statement_maker("osm-script");


Osm_Script_Statement::Osm_Script_Statement
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_),
       desired_timestamp(NOW), comparison_timestamp(0), add_deletion_information(false),
       max_allowed_time(0), max_allowed_space(0),
       factory(0)
{
  map< string, string > attributes;
  
  attributes["bbox"] = "";
  attributes["timeout"] = "180";
  attributes["element-limit"] = "536870912";
  attributes["output"] = "xml";
  attributes["date"] = "";
  attributes["from"] = "";
  attributes["augmented"] = "";
  
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

  
  Output_Handler_Parser* format_parser = Output_Handler_Parser::get_format_parser(attributes["output"]);	
  if (!format_parser)
    add_static_error("Unknown output format: " + attributes["output"]);
  else
    global_settings.set_output_handler(format_parser->new_output_handler(0, 0));
  
  
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
      temp<<"Longitudes in bounding boxes must be between -180.0 and 180.0.";
      add_static_error(temp.str());
    }
    
    if (south >= -90.0 && south <= 90.0 && north >= -90.0 && north <= 90.0
        && west >= -180.0 && west <= 180.0 && east >= -180.0 && east <= 180.0)
      global_settings.set_global_bbox(Bbox_Double(south, west, north, east));
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
  
  if (attributes["augmented"] != "")
  {    
    if (attributes["augmented"] == "deletions" && attributes["from"] != "")      
      add_deletion_information = true;

    if (attributes["augmented"] != "deletions")
    {
      ostringstream temp;
      temp<<"The only allowed values for \"augmented\" are an empty value or \"deletions\".";
      add_static_error(temp.str());
    }    
    if (attributes["from"] == "")
    {
      ostringstream temp;
      temp<<"The attribute \"augmented\" can only be set if the attribute \"from\" is set.";
      add_static_error(temp.str());
    }      
  }
}

void Osm_Script_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if (statement)
  {
    if (statement->get_name() != "newer")
      substatements.push_back(statement);
    else
      add_static_error("\"newer\" can appear only inside \"query\" statements.");
  }
}


// bool set_output_templates
//     (Output_Handle& output, string& header, const string& name, Transaction& transaction)
// {
//   string data;
//   
//   ifstream in((transaction.get_db_dir() + "/templates/" + name).c_str());
//   while (in.good())
//   {
//     string buf;
//     getline(in, buf);
//     data += buf + '\n';
//   }
//   
//   if (data.find("<includeonly>") != string::npos)
//   {
//     string::size_type start = data.find("<includeonly>") + 13;
//     string::size_type end = data.find("</includeonly>");
//     data = data.substr(start, end - start);
//   }
// 
//   bool template_contains_js = (data.find("<script") != string::npos);
// 
//   if (data == "")
//     data = "\n<p>Template not found.</p>\n";
// 
//   string node_template = "\n<p>No {{node:..}} found in template.</p>\n";
//   string way_template = "\n<p>No {{way:..}} found in template.</p>\n";
//   string relation_template = "\n<p>No {{relation:..}} found in template.</p>\n";
//   
//   bool header_written = false;
//   string::size_type pos = 0;
//   while (pos < data.size())
//   {
//     if (data[pos] == '{')
//     {
//       if (data.substr(pos, 7) == "{{node:")
//       {
// 	string::size_type end_pos = find_block_end(data, pos);
// 	
// 	if (!header_written)
// 	{
// 	  header = data.substr(0, pos);
// 	  header_written = true;
// 	}
// 	
// 	if (end_pos != string::npos)
// 	{
// 	  node_template = data.substr(pos + 7, end_pos - pos - 9);
// 	  pos = end_pos;
// 	}
// 	else
// 	  pos = data.size();
//       }
//       else if (data.substr(pos, 6) == "{{way:")
//       {
// 	string::size_type end_pos = find_block_end(data, pos);
// 
// 	if (!header_written)
// 	{
// 	  header = data.substr(0, pos);
// 	  header_written = true;
// 	}
// 	
// 	if (end_pos != string::npos)
// 	{
// 	  way_template = data.substr(pos + 6, end_pos - pos - 8);
// 	  pos = end_pos;
// 	}
// 	else
// 	  pos = data.size();
//       }
//       else if (data.substr(pos, 11) == "{{relation:")
//       {
// 	string::size_type end_pos = find_block_end(data, pos);
// 	
// 	if (!header_written)
// 	{
// 	  header = data.substr(0, pos);
// 	  header_written = true;
// 	}
// 	
// 	if (end_pos != string::npos)
// 	{
// 	  relation_template = data.substr(pos + 11, end_pos - pos - 13);
// 	  pos = end_pos;
// 	}
// 	else
// 	  pos = data.size();
//       }
//       else
// 	++pos;
//     }
//     else
//       ++pos;
//   }
//   
//   if (!header_written)
//     header = data.substr(0, pos);
//   
//   output.set_templates(node_template, way_template, relation_template);
//   
//   return template_contains_js;
// }


void Osm_Script_Statement::execute(Resource_Manager& rman)
{
  rman.set_limits(max_allowed_time, max_allowed_space);
  rman.get_global_settings().trigger_print_bounds();

  if (comparison_timestamp > 0)
  {
    for (vector< Statement* >::iterator it = factory->created_statements.begin();
        it != factory->created_statements.end(); ++it)
    {
      Print_Statement* print = dynamic_cast< Print_Statement* >(*it);
      if (print)
        print->set_collect_lhs();
    }
    
    rman.set_diff_from_timestamp(comparison_timestamp);
    rman.set_diff_to_timestamp(desired_timestamp);
    rman.set_desired_timestamp(comparison_timestamp);
    
    for (vector< Statement* >::iterator it(substatements.begin());
        it != substatements.end(); ++it)
      (*it)->execute(rman);
    
    for (vector< Statement* >::iterator it = factory->created_statements.begin();
        it != factory->created_statements.end(); ++it)
    {
      Print_Statement* print = dynamic_cast< Print_Statement* >(*it);
      if (print)
        print->set_collect_rhs(add_deletion_information);
    }
    
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
