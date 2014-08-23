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

#include "../frontend/output.h"
#include "statement_dump.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

Statement_Dump::~Statement_Dump()
{
  for (vector< Statement_Dump* >::iterator it = substatements.begin();
      it != substatements.end(); ++it)
    delete *it;
}

void Statement_Dump::add_statement(Statement_Dump* statement, string text)
{
  substatements.push_back(statement);
}

string indent(const string& subresult)
{
  string result;

  string::size_type pos = 0;
  string::size_type next = subresult.find('\n', pos);
  while (next != string::npos)
  {
    result += "  " + subresult.substr(pos, next-pos) + '\n';
    pos = next + 1;
    next = subresult.find('\n', pos);
  }
  if (subresult.substr(pos) != "")
    result += "  " + subresult.substr(pos);
  
  return result;
}

string Statement_Dump::dump_xml() const
{
  string result;
  
  if (substatements.empty())
  {
    result = string("<") + name;
    for (map< string, string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
      result += string(" ") + it->first + "=\"" + escape_xml(it->second) + "\"";
    result += "/>\n";
  }
  else
  {
    result = string("<") + name;
    for (map< string, string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
      result += string(" ") + it->first + "=\"" + escape_xml(it->second) + "\"";
    result += ">\n";

    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      result += indent((*it)->dump_xml());
/*      string subresult = (*it)->dump_xml();
      string::size_type pos = 0;
      string::size_type next = subresult.find('\n', pos);
      while (next != string::npos)
      {
	result += string("  ") + subresult.substr(pos, next-pos) + '\n';
	pos = next + 1;
	next = subresult.find('\n', pos);
      }*/
    }
    
    result += string("</") + name + ">\n";
  }

  return result;
}

string dump_print_map_ql(const map< string, string >& attributes, bool pretty = false)
{
  string result;
  
  for (map< string, string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "from" && it->second != "_")
      result += "." + it->second + " ";
  }
  result += "out";
  for (map< string, string >::const_iterator it = attributes.begin();
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
      else if (it->second == "meta")
	result += " meta";
      else if (it->second == "quirks")
	result += " quirks";
    }
  }
  for (map< string, string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "order" && it->second != "id")
    {
      if (it->second == "quadtile")
	result += " qt";
    }
  }
  for (map< string, string >::const_iterator it = attributes.begin();
      it != attributes.end(); ++it)
  {
    if (it->first == "limit" && it->second != "")
      result += " " + it->second + "";
  }
  
  return result;
}

string escape_quotation_marks(const string& input)
{
  string result = input;
  
  string::size_type pos = result.find('\\');
  while (pos != string::npos)
  {
    result = result.substr(0, pos) + "\\\\" + (pos < result.size()-1 ? input.substr(pos+1) : "");
    if (pos < result.size()-2)
      pos = result.find('\\', pos + 2);
    else
      break;
  }
  
  pos = result.find('\"');
  while (pos != string::npos)
  {
    result = result.substr(0, pos) + "\\\"" + (pos < result.size()-1 ? input.substr(pos+1) : "");
    if (pos < result.size()-2)
      pos = result.find('\\', pos + 2);
    else
      break;
  }
  
  return result;
}

string dump_subquery_map_ql(const string& name, const map< string, string >& attributes)
{
  string result;

  if (name == "bbox-query")
  {
    result += "(";
    if (attributes.find("s") != attributes.end())
      result += attributes.find("s")->second;
    result += ",";
    if (attributes.find("w") != attributes.end())
      result += attributes.find("w")->second;
    result += ",";
    if (attributes.find("n") != attributes.end())
      result += attributes.find("n")->second;
    result += ",";
    if (attributes.find("e") != attributes.end())
      result += attributes.find("e")->second;
    result += ")";
  }
  else if (name == "has-kv")
  {
    result += "[";
    if (attributes.find("k") != attributes.end())
      result += "\"" + escape_quotation_marks(attributes.find("k")->second) + "\"";
    if (attributes.find("regk") != attributes.end() && attributes.find("regk")->second != "")
      result += "~\"" + escape_quotation_marks(attributes.find("regk")->second) + "\"";
    if (attributes.find("modv") != attributes.end() && attributes.find("modv")->second == "not")
      result += "!";
    if (attributes.find("v") != attributes.end() && attributes.find("v")->second != "")
      result += "=\"" + escape_quotation_marks(attributes.find("v")->second) + "\"";
    if (attributes.find("regv") != attributes.end() && attributes.find("regv")->second != "")
      result += "~\"" + escape_quotation_marks(attributes.find("regv")->second) + "\"";
    result += "]";
  }
  else if (name == "recurse")
  {
    result += "(";
    if (attributes.find("type") != attributes.end())
    {
      string type = attributes.find("type")->second;
      if (type == "way-node")
	result += "w";
      else if (type == "relation-node" || type == "relation-way" || type == "relation-relation")
	result += "r";
      else if (type == "node-way" || type == "node-relation")
	result += "bn";
      else if (type == "way-relation")
	result += "bw";
      else if (type == "relation-backwards")
	result += "br";
      else if (type == "down")
	result += ">";
      else if (type == "down-rel")
	result += ">>";
      else if (type == "up")
	result += "<";
      else if (type == "up-rel")
	result += "<<";
    }
    if (attributes.find("from") != attributes.end() && attributes.find("from")->second != "_")
      result += "." + attributes.find("from")->second;
    result += ")";
  }
  else if (name == "id-query")
  {
    result += "(";
    if (attributes.find("ref") != attributes.end())
      result += attributes.find("ref")->second;
    result += ")";
  }
  else if (name == "around")
  {
    result += "(around";
    if (attributes.find("from") != attributes.end() && attributes.find("from")->second != "_")
      result += "." + attributes.find("from")->second;
    result += ":";
    if (attributes.find("radius") != attributes.end())
      result += attributes.find("radius")->second;
    if (attributes.find("lat") != attributes.end() && attributes.find("lat")->second != "")
      result += "," + attributes.find("lat")->second;
    if (attributes.find("lon") != attributes.end() && attributes.find("lon")->second != "")
      result += "," + attributes.find("lon")->second;
    result += ")";
  }
  else if (name == "user")
  {
    if (attributes.find("name") != attributes.end() && attributes.find("name")->second != "")
      result += "(user:\"" + escape_quotation_marks(attributes.find("name")->second) + "\")";
    else if (attributes.find("uid") != attributes.end() && attributes.find("uid")->second != "")
      result += "(uid:" + attributes.find("uid")->second + ")";
  }
  else if (name == "newer")
  {
    if (attributes.find("than") != attributes.end() && attributes.find("than")->second != "")
      result += "(newer:\"" + escape_quotation_marks(attributes.find("than")->second) + "\")";
  }
  else if (name == "area-query")
  {
    result += "(area";
    if (attributes.find("from") != attributes.end() && attributes.find("from")->second != "_")
      result += "." + attributes.find("from")->second;
    if (attributes.find("ref") != attributes.end() && attributes.find("ref")->second != "")
      result += ":" + attributes.find("ref")->second;
    result += ")";
  }
  else
    result += "(" + name + ":)";
  
  return result;
}

string Statement_Dump::dump_compact_map_ql() const
{
  string result;
  if (name == "osm-script")
  {
    for (map< string, string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
    {
      if (it->first == "timeout")
	result += "[timeout:" + it->second + "]";
      else if (it->first == "element-limit")
	result += "[maxsize:" + it->second + "]";
      else if (it->first == "output")
	result += "[out:" + it->second + "]";
    }
    if (!attributes.empty())
      result += ";";
    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += (*it)->dump_compact_map_ql();
  }
  else if (name == "union")
  {
    result += "(";
    
    vector< Statement_Dump* >::const_iterator it = substatements.begin();
    if (it == substatements.end())
      return result + ");";
    
    result += (*it)->dump_compact_map_ql();
    for (++it; it != substatements.end(); ++it)
      result += (*it)->dump_compact_map_ql();
    result += ")";
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "item")
  {
    if (attributes.find("set") != attributes.end())
      result += "." + attributes.find("set")->second;
    else
      result += "._";
  }
  else if (name == "foreach")
  {
    result += name;
    
    if (attributes.find("from") != attributes.end() && attributes.find("from")->second != "_")
      result += "." + attributes.find("from")->second;
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
    
    result += "(";
    
    vector< Statement_Dump* >::const_iterator it = substatements.begin();
    if (it == substatements.end())
      return result + ");";
    
    result += (*it)->dump_compact_map_ql();
    for (++it; it != substatements.end(); ++it)
      result += (*it)->dump_compact_map_ql();
    result += ")";
  }
  else if (name == "query")
  {
    if (attributes.find("type") != attributes.end())
      result += attributes.find("type")->second;

    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      if ((*it)->name == "item")
      {
	if ((*it)->attributes.find("set") != (*it)->attributes.end())
	  result += "." + (*it)->attributes.find("set")->second;
	else
	  result += "._";
      }
    }

    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      if ((*it)->name != "item")
        result += dump_subquery_map_ql((*it)->name, (*it)->attributes);
    }
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "print")
    return dump_print_map_ql(attributes, false) + ";";
  else if (name == "bbox-query" || name == "around" || name == "id_query")
  {
    result += "node";
    result += dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "id-query" || name == "user" || name == "newer")
  {
    if (attributes.find("type") == attributes.end())
      result += "all";
    else
      result += attributes.find("type")->second;
    
    result += dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "recurse")
  {
    if (attributes.find("type") != attributes.end())
    {
      string rel_type = attributes.find("type")->second;
      if (rel_type == "way-node" || rel_type == "relation-node")
	result += "node" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "relation-way" || rel_type == "node-way")
	result += "way" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "relation-relation" || rel_type == "relation-backwards"
	  || rel_type == "node-relation" || rel_type == "way-relation")
	result += "rel" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "down")
	result += ">";
      else if (rel_type == "down-rel")
	result += ">>";
      else if (rel_type == "up")
	result += "<";
      else if (rel_type == "up-rel")
	result += "<<";
    }
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "area-query")
  {
    result += "node" + dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else
    result += "(" + name + ":)";
  
  if (name != "osm-script")
    result += ";";
  return result;
}

string Statement_Dump::dump_bbox_map_ql() const
{
  string result;
  bool auto_timeout = true;
  if (name == "osm-script")
  {
    for (map< string, string >::const_iterator it = attributes.begin();
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
	result += "[out:" + it->second + "]";
    }
    if (auto_timeout)
      result += "[timeout:1]";
    result += ";";
    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += (*it)->dump_bbox_map_ql();
  }
  else if (name == "union")
  {
    result += "(";
    
    vector< Statement_Dump* >::const_iterator it = substatements.begin();
    if (it == substatements.end())
      return result + ");";
    
    result += (*it)->dump_bbox_map_ql();
    for (++it; it != substatements.end(); ++it)
      result += (*it)->dump_bbox_map_ql();
    result += ")";
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "item")
  {
    if (attributes.find("set") != attributes.end())
      result += "." + attributes.find("set")->second;
    else
      result += "._";
  }
  else if (name == "foreach")
  {
    result += name;
    
    if (attributes.find("from") != attributes.end() && attributes.find("from")->second != "_")
      result += "." + attributes.find("from")->second;
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
    
    result += "(";
    
    vector< Statement_Dump* >::const_iterator it = substatements.begin();
    if (it == substatements.end())
      return result + ");";
    
    result += (*it)->dump_bbox_map_ql();
    for (++it; it != substatements.end(); ++it)
      result += (*it)->dump_bbox_map_ql();
    result += ")";
  }
  else if (name == "query")
  {
    if (attributes.find("type") != attributes.end())
      result += attributes.find("type")->second;

    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      if ((*it)->name == "item")
      {
	if ((*it)->attributes.find("set") != (*it)->attributes.end())
	  result += "." + (*it)->attributes.find("set")->second;
	else
	  result += "._";
      }
    }

    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      if ((*it)->name != "item")
        result += dump_subquery_map_ql((*it)->name, (*it)->attributes);
    }
    
    if (attributes.find("type")->second =="node"
        || attributes.find("type")->second == "way"
        || attributes.find("type")->second == "relation")
      result += "(bbox)";
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "print")
    return dump_print_map_ql(attributes, false) + ";";
  else if (name == "bbox-query" || name == "around" || name == "id_query")
  {
    result += "node";
    result += dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "id-query" || name == "user" || name == "newer")
  {
    if (attributes.find("type") == attributes.end())
      result += "all";
    else
      result += attributes.find("type")->second;
    
    result += dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "recurse")
  {
    if (attributes.find("type") != attributes.end())
    {
      string rel_type = attributes.find("type")->second;
      if (rel_type == "way-node" || rel_type == "relation-node")
	result += "node" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "relation-way" || rel_type == "node-way")
	result += "way" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "relation-relation" || rel_type == "relation-backwards"
	  || rel_type == "node-relation" || rel_type == "way-relation")
	result += "rel" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "down")
	result += ">";
      else if (rel_type == "down-rel")
	result += ">>";
      else if (rel_type == "up")
	result += "<";
      else if (rel_type == "up-rel")
	result += "<<";
    }
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "area-query")
  {
    result += "node" + dump_subquery_map_ql(name, attributes) + "(bbox)";
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else
    result += "(" + name + ":)";
  
  if (name != "osm-script")
    result += ";";
  return result;
}

string Statement_Dump::dump_pretty_map_ql() const
{
  string result;
  if (name == "osm-script")
  {
    for (map< string, string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
    {
      if (it->first == "timeout")
	result += "[timeout:" + it->second + "]\n";
      else if (it->first == "element-limit")
	result += "[maxsize:" + it->second + "]\n";
      else if (it->first == "output")
	result += "[out:" + it->second + "]\n";
    }
    if (result != "")
      result += ";\n";
    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
      result += (*it)->dump_pretty_map_ql() + "\n";
  }
  else if (name == "union")
  {
    result += "(";
    
    vector< Statement_Dump* >::const_iterator it = substatements.begin();
    if (it == substatements.end())
      return result + ");";
    
    result += "\n" + indent((*it)->dump_pretty_map_ql());
    for (++it; it != substatements.end(); ++it)
      result += "\n" + indent((*it)->dump_pretty_map_ql());
    result += "\n)";
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "item")
  {
    if (attributes.find("set") != attributes.end())
      result += "." + attributes.find("set")->second;
    else
      result += "._";
  }
  else if (name == "foreach")
  {
    result += name;
    
    if (attributes.find("from") != attributes.end() && attributes.find("from")->second != "_")
      result += "." + attributes.find("from")->second;
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
    
    result += "(";
    
    vector< Statement_Dump* >::const_iterator it = substatements.begin();
    if (it == substatements.end())
      return result + ");";
    
    result += "\n" + indent((*it)->dump_pretty_map_ql());
    for (++it; it != substatements.end(); ++it)
      result += "\n" + indent((*it)->dump_pretty_map_ql());
    result += "\n)";
  }
  else if (name == "query")
  {
    if (attributes.find("type") != attributes.end())
      result += attributes.find("type")->second;

    uint proper_substatement_count = 0;
    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      if ((*it)->name == "item")
      {
	if ((*it)->attributes.find("set") != (*it)->attributes.end())
	  result += "." + (*it)->attributes.find("set")->second;
	else
	  result += "._";
      }
      else
	++proper_substatement_count;
    }

    if (proper_substatement_count > 1)
    {
      for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
          it != substatements.end(); ++it)
      {
	if ((*it)->name != "item")
	  result += "\n  " + dump_subquery_map_ql((*it)->name, (*it)->attributes);
      }
    }
    else
    {
      for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
          it != substatements.end(); ++it)
      {
	if ((*it)->name != "item")
	  result += dump_subquery_map_ql((*it)->name, (*it)->attributes);
      }
    }
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
    {
      if (proper_substatement_count > 1)
	result += "\n";
      result += "->." + attributes.find("into")->second;
    }
  }
  else if (name == "print")
    return dump_print_map_ql(attributes, true) + ";";
  else if (name == "bbox-query" || name == "around" || name == "id_query")
  {
    result += "node";
    result += dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "id-query" || name == "user" || name == "newer")
  {
    if (attributes.find("type") == attributes.end())
      result += "all";
    else
      result += attributes.find("type")->second;
    
    result += dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "recurse")
  {
    if (attributes.find("type") != attributes.end())
    {
      string rel_type = attributes.find("type")->second;
      if (rel_type == "way-node" || rel_type == "relation-node")
	result += "node" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "relation-way" || rel_type == "node-way")
	result += "way" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "relation-relation" || rel_type == "relation-backwards"
	  || rel_type == "node-relation" || rel_type == "way-relation")
	result += "rel" + dump_subquery_map_ql(name, attributes);
      else if (rel_type == "down")
	result += ">";
      else if (rel_type == "down-rel")
	result += ">>";
      else if (rel_type == "up")
	result += "<";
      else if (rel_type == "up-rel")
	result += "<<";
    }
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else if (name == "area-query")
  {
    result += "node" + dump_subquery_map_ql(name, attributes);
    
    if (attributes.find("into") != attributes.end() && attributes.find("into")->second != "_")
      result += "->." + attributes.find("into")->second;
  }
  else
    result += "(" + name + ":)";
  
  if (name != "osm-script")
    result += ";";
  return result;
}

Statement_Dump* Statement_Dump::Factory::create_statement
    (string element, int line_number, const map< string, string >& attributes)
{
  return new Statement_Dump(element, attributes);
}
