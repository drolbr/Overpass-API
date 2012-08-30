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

#include <iostream>
#include <string>

#include "output.h"

using namespace std;

string escape_xml(const string& s)
{
  string result;
  for (uint i(0); i < s.length(); ++i)
  {
    if (s[i] == '&')
      result += "&amp;";
    else if (s[i] == '\"')
      result += "&quot;";
    else if (s[i] == '<')
      result += "&lt;";
    else if (s[i] == '>')
      result += "&gt;";
    else if ((uint8)s[i] < 32)
      result += '?';
    else
      result += s[i];
  }
  return result;
}

string escape_cstr(const string& s)
{
  string result;
  result.reserve(s.length()*2);
  for (string::size_type i(0); i < s.size(); ++i)
  {
    if (s[i] == '\"')
      result += "\\\"";
    else if (s[i] == '\\')
      result += "\\\\";
    else if (s[i] == '\n')
      result += "\\n";
    else if (s[i] == '\t')
      result += "\\t";
    else if (s[i] == '\r')
      result += "\\r";
    else if ((unsigned char)s[i] < 32)
      result += '?';
    else
      result += s[i];
  }
  return result;
}

class Verbose_Osm_Backend_Callback : public Osm_Backend_Callback
{
  public:
    virtual void update_started() { cerr<<"Flushing to database ."; }
    virtual void compute_indexes_finished() { cerr<<'.'; }
    virtual void update_ids_finished() { cerr<<'.'; }
    virtual void update_coords_finished() { cerr<<'.'; }
    virtual void prepare_delete_tags_finished() { cerr<<'.'; }
    virtual void tags_local_finished() { cerr<<'.'; }
    virtual void tags_global_finished() { cerr<<'.'; }
    virtual void flush_roles_finished() { cerr<<'.'; }
    virtual void update_finished() { cerr<<" done.\n"; }
    virtual void partial_started() { cerr<<"Reorganizing the database ..."; }
    virtual void partial_finished() { cerr<<" done.\n"; }
    
    virtual void parser_started() { cerr<<"Reading XML file ..."; }
    virtual void node_elapsed(uint32 id) { cerr<<" elapsed node "<<id<<". "; }
    virtual void nodes_finished() { cerr<<" finished reading nodes. "; }
    virtual void way_elapsed(uint32 id) { cerr<<" elapsed way "<<id<<". "; }
    virtual void ways_finished() { cerr<<" finished reading ways. "; }
    virtual void relation_elapsed(uint32 id) { cerr<<" elapsed relation "<<id<<". "; }
    virtual void relations_finished() { cerr<<" finished reading relations. "; }

    virtual void parser_succeeded() { cerr<<"Update complete.\n"; }
};

Osm_Backend_Callback* get_verbatim_callback()
{
  return new Verbose_Osm_Backend_Callback;
}

class Quiet_Osm_Backend_Callback : public Osm_Backend_Callback
{
  public:
    virtual void update_started() {}
    virtual void compute_indexes_finished() {}
    virtual void update_ids_finished() {}
    virtual void update_coords_finished() {}
    virtual void prepare_delete_tags_finished() {}
    virtual void tags_local_finished() {}
    virtual void tags_global_finished() {}
    virtual void flush_roles_finished() {}
    virtual void update_finished() {}
    virtual void partial_started() {}
    virtual void partial_finished() {}
    
    virtual void parser_started() {}
    virtual void node_elapsed(uint32 id) {}
    virtual void nodes_finished() {}
    virtual void way_elapsed(uint32 id) {}
    virtual void ways_finished() {}
    virtual void relation_elapsed(uint32 id) {}
    virtual void relations_finished() {}
    
    virtual void parser_succeeded() {}
};

Osm_Backend_Callback* get_quiet_callback()
{
  return new Quiet_Osm_Backend_Callback;
}

void report_file_error(const File_Error& e)
{
  cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
}
