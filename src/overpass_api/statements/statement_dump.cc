/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#include "../frontend/output.h"
#include "statement_dump.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


Statement_Dump::~Statement_Dump()
{
  for (std::vector< Statement_Dump* >::iterator it = substatements.begin();
      it != substatements.end(); ++it)
    delete *it;
}


void Statement_Dump::add_statement(Statement_Dump* statement, std::string text)
{
  substatements.push_back(statement);
}


Statement* Statement_Dump::create_non_dump_stmt(Statement::Factory& stmt_factory)
{
  if (non_dump_stmt)
    return non_dump_stmt;
  
  non_dump_stmt = stmt_factory.create_statement(name_, line_number, attributes);
  
  if (non_dump_stmt)
  {
    for (std::vector< Statement_Dump* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
    {
      Statement* substmt = (*it)->create_non_dump_stmt(stmt_factory);
      if (substmt)
        non_dump_stmt->add_statement(substmt, "");
    }
  }
  
  return non_dump_stmt;
}


std::string indent(const std::string& subresult)
{
  std::string result;

  std::string::size_type pos = 0;
  std::string::size_type next = subresult.find('\n', pos);
  while (next != std::string::npos)
  {
    result += "  " + subresult.substr(pos, next-pos) + '\n';
    pos = next + 1;
    next = subresult.find('\n', pos);
  }
  if (subresult.substr(pos) != "")
    result += "  " + subresult.substr(pos);
  
  return result;
}


std::string Statement_Dump::dump_xml() const
{
  if (non_dump_stmt)
    return non_dump_stmt->dump_xml("");
  
  std::string result;
  
  if (substatements.empty())
  {
    result = std::string("<") + name_;
    for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
      result += std::string(" ") + it->first + "=\"" + escape_xml(it->second) + "\"";
    result += "/>\n";
  }
  else
  {
    result = std::string("<") + name_;
    for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
      result += std::string(" ") + it->first + "=\"" + escape_xml(it->second) + "\"";
    result += ">\n";

    for (std::vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += indent((*it)->dump_xml());
    
    result += std::string("</") + name_ + ">\n";
  }

  return result;
}


std::string dump_print_map_ql(const std::map< std::string, std::string >& attributes, bool pretty = false)
{
  std::string result;
  
  for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "from" && it->second != "_")
      result += "." + it->second + " ";
  }
  result += "out";
  for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "mode")
    {
      if (it->second == "ids_only")
	result += " ids";
      else if (it->second == "skeleton")
	result += " skel";
      else if (it->second == "body")
	result += " body";
      else if (it->second == "tags")
        result += " tags";
      else if (it->second == "meta")
	result += " meta";
      else if (it->second == "quirks")
	result += " quirks";
    }
  }
  for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "geometry")
    {
      if (it->second == "full")
      {
	result += " geom";
        if (attributes.find("s") != attributes.end() && attributes.find("s")->second != "" &&
            attributes.find("w") != attributes.end() && attributes.find("w")->second != "" &&
            attributes.find("n") != attributes.end() && attributes.find("n")->second != "" && 
            attributes.find("e") != attributes.end() && attributes.find("e")->second != "")
        {
          result += "(";
          result += attributes.find("s")->second;
          result += ",";
          result += attributes.find("w")->second;
          result += ",";
          result += attributes.find("n")->second;
          result += ",";
          result += attributes.find("e")->second;
          result += ")";
        }
      }
      else if (it->second == "center")
        result += " center";
      else if (it->second == "bounds")
        result += " bb";
    }
  }
  for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "order" && it->second != "id")
    {
      if (it->second == "quadtile")
	result += " qt";
    }
  }
  for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "limit" && it->second != "")
      result += " " + it->second + "";
  }
  
  return result;
}


std::string Statement_Dump::dump_compact_map_ql(Statement::Factory& stmt_factory)
{
  std::string result;
  if (name_ == "osm-script")
  {
    std::string output_val;
    std::string output_config;
    for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
    {
      if (it->first == "timeout")
	result += "[timeout:" + it->second + "]";
      else if (it->first == "element-limit")
	result += "[maxsize:" + it->second + "]";
      else if (it->first == "output")
      {
        if (stmt_factory.global_settings.get_output_handler())
          result += "[out:" + it->second + stmt_factory.global_settings.get_output_handler()->dump_config() + "]";
        else
          output_val = it->second;
      }
      else if (it->first == "output-config")
        output_config = it->second;
      else if (it->first == "bbox")
	result += "[bbox:" + it->second + "]";
    }
    if (output_val != "")
      result += "[out:" + output_val + output_config + "]";

    if (attributes.find("augmented") != attributes.end() && 
        attributes.find("augmented")->second == "deletions" &&
        attributes.find("from") != attributes.end())
    {
      result += "[adiff:\"" + attributes.find("from")->second;
      if (attributes.find("date") != attributes.end())
        result += "\",\"" + attributes.find("date")->second;
      result += "\"]";
    }
    else if (attributes.find("from") != attributes.end())
    {
      result += "[diff:\"" + attributes.find("from")->second;
      if (attributes.find("date") != attributes.end())
        result += "\",\"" + attributes.find("date")->second;
      result += "\"]";
    }
    else if (attributes.find("date") != attributes.end())
      result += "[date:\"" + attributes.find("date")->second + "\"]";

    if (!attributes.empty())
      result += ";";
    for (std::vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += (*it)->dump_compact_map_ql(stmt_factory);
  }
  else if (name_ == "area-query" || name_ == "around"|| name_ == "bbox-query" || name_ == "convert"
      || name_ == "coord-query" || name_ == "difference" || name_ == "foreach" || name_ == "id-query"
      || name_ == "item" || name_ == "make" || name_ == "newer" || name_ == "pivot"
      || name_ == "polygon-query" || name_ == "print" || name_ == "query" || name_ == "recurse"
      || name_ == "union" || name_ == "user")
  {
    Statement* stmt = create_non_dump_stmt(stmt_factory);
    if (stmt)
      result += stmt->dump_compact_ql("");
  }
  else
    result += "(" + name_ + ":)";
  
  if (name_ != "osm-script")
    result += ";";
  return result;
}


std::string Statement_Dump::dump_bbox_map_ql(Statement::Factory& stmt_factory)
{
  std::string result;
  bool auto_timeout = true;
  if (name_ == "osm-script")
  {
    std::string output_val;
    std::string output_config;
    for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
    {
      if (it->first == "timeout")
      {
	result += "[timeout:" + it->second + "]";
	auto_timeout = false;
      }
      else if (it->first == "element-limit")
	result += "[maxsize:" + it->second + "]";
      else if (it->first == "output")
      {
        if (stmt_factory.global_settings.get_output_handler())
          result += "[out:" + it->second + stmt_factory.global_settings.get_output_handler()->dump_config() + "]";
        else
          output_val = it->second;
      }
      else if (it->first == "output-config")
        output_config = it->second;
      else if (it->first == "bbox")
	result += "[bbox:" + it->second + "]";
    }
    if (output_val != "")
      result += "[out:" + output_val + output_config + "]";

    if (attributes.find("augmented") != attributes.end() && 
        attributes.find("augmented")->second == "deletions" &&
        attributes.find("from") != attributes.end())
    {
      result += "[adiff:\"" + attributes.find("from")->second;
      if (attributes.find("date") != attributes.end())
        result += "\",\"" + attributes.find("date")->second;
      result += "\"]";
    }
    else if (attributes.find("from") != attributes.end())
    {
      result += "[diff:\"" + attributes.find("from")->second;
      if (attributes.find("date") != attributes.end())
        result += "\",\"" + attributes.find("date")->second;
      result += "\"]";
    }
    else if (attributes.find("date") != attributes.end())
      result += "[date:\"" + attributes.find("date")->second + "\"]";

    if (auto_timeout)
      result += "[timeout:1]";
    result += ";";
    for (std::vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += (*it)->dump_bbox_map_ql(stmt_factory);
  }
  else if (name_ == "area-query" || name_ == "around"|| name_ == "bbox-query" || name_ == "convert"
      || name_ == "coord-query" || name_ == "difference" || name_ == "foreach" || name_ == "id-query"
      || name_ == "item" || name_ == "make" || name_ == "newer" || name_ == "pivot"
      || name_ == "polygon-query" || name_ == "print" || name_ == "query" || name_ == "recurse"
      || name_ == "union" || name_ == "user")
  {
    Statement* stmt = create_non_dump_stmt(stmt_factory);
    if (stmt)
      result += stmt->dump_compact_ql(name_ == "difference" || name_ == "foreach" || name_ == "query" || name_ == "union" ? "(bbox)" : "");
  }
  else
    result += "(" + name_ + ":)";
  
  if (name_ != "osm-script")
    result += ";";
  return result;
}


std::string Statement_Dump::dump_pretty_map_ql(Statement::Factory& stmt_factory)
{
  std::string result;
  if (name_ == "osm-script")
  {
    std::string output_val;
    std::string output_config;
    for (std::map< std::string, std::string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
    {
      if (it->first == "timeout")
	result += "[timeout:" + it->second + "]\n";
      else if (it->first == "element-limit")
	result += "[maxsize:" + it->second + "]\n";
      else if (it->first == "output")
      {
        if (stmt_factory.global_settings.get_output_handler())
          result += "[out:" + it->second + stmt_factory.global_settings.get_output_handler()->dump_config() + "]\n";
        else
          output_val = it->second;
      }
      else if (it->first == "output-config")
        output_config = it->second;
      else if (it->first == "bbox")
	result += "[bbox:" + it->second + "]\n";
    }
    if (output_val != "")
      result += "[out:" + output_val + output_config + "]\n";

    if (attributes.find("augmented") != attributes.end() && 
        attributes.find("augmented")->second == "deletions" &&
        attributes.find("from") != attributes.end())
    {
      result += "[adiff:\"" + attributes.find("from")->second;
      if (attributes.find("date") != attributes.end())
        result += "\",\"" + attributes.find("date")->second;
      result += "\"]\n";
    }
    else if (attributes.find("from") != attributes.end())
    {
      result += "[diff:\"" + attributes.find("from")->second;
      if (attributes.find("date") != attributes.end())
        result += "\",\"" + attributes.find("date")->second;
      result += "\"]\n";
    }
    else if (attributes.find("date") != attributes.end())
      result += "[date:\"" + attributes.find("date")->second + "\"]\n";

    if (result != "")
      result += ";\n";

    for (std::vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += (*it)->dump_pretty_map_ql(stmt_factory) + "\n";
  }
  else if (name_ == "area-query" || name_ == "around"|| name_ == "bbox-query" || name_ == "convert"
      || name_ == "coord-query" || name_ == "difference" || name_ == "foreach" || name_ == "id-query"
      || name_ == "item" || name_ == "make" || name_ == "newer" || name_ == "pivot"
      || name_ == "polygon-query" || name_ == "print" || name_ == "query" || name_ == "recurse"
      || name_ == "union" || name_ == "user")
  {
    Statement* stmt = create_non_dump_stmt(stmt_factory);
    if (stmt)
      result += stmt->dump_pretty_ql("");
  }
  else
    result += "(" + name_ + ":)";
  
  if (name_ != "osm-script")
    result += ";";
  return result;
}


Statement_Dump* Statement_Dump::Factory::create_statement
    (std::string element, int line_number, const std::map< std::string, std::string >& attributes)
{
  return new Statement_Dump(element, attributes, line_number);
}


Statement_Dump* Statement_Dump::Factory::create_statement(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context)
{
  Statement* stmt = stmt_factory->create_statement(tree_it, tree_context);
  if (stmt)
    return new Statement_Dump("universal_dump", std::map< std::string, std::string >(), tree_it->line_col.first, stmt);
  
  return 0;
}


std::string Statement_Dump::attribute(const std::string& key) const
{
  std::map< std::string, std::string >::const_iterator it = attributes.find(key);
  if (it == attributes.end())
    return "";
  else
    return it->second;
}
