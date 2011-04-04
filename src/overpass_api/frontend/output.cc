#include <iostream>
#include <string>

#include "output.h"

using namespace std;

string escape_xml(const string& s)
{
  string result;
  for (int i(0); i < s.length(); ++i)
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
