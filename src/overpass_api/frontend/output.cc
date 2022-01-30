/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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


class Verbose_Osm_Backend_Callback : public Osm_Backend_Callback
{
  public:
    virtual void update_started() { std::cerr<<"Flushing to database ."; }
    virtual void attic_update_started() { std::cerr<<"Flushing to database ."; }
    virtual void compute_started() { std::cerr<<"Compute current ..."; }
    virtual void compute_attic_started() { std::cerr<<"Compute attic ..."; }
    virtual void compute_finished() { std::cerr<<" ready. "; }
    virtual void compute_attic_finished() { std::cerr<<" ready. "; }
    virtual void compute_indexes_finished() { std::cerr<<'.'; }
    virtual void update_ids_finished() { std::cerr<<'.'; }
    virtual void update_coords_finished() { std::cerr<<'.'; }
    virtual void prepare_delete_tags_finished() { std::cerr<<'.'; }
    virtual void undeleted_finished() { std::cerr<<'.'; }
    virtual void meta_finished() { std::cerr<<'.'; }
    virtual void tags_local_finished() { std::cerr<<'.'; }
    virtual void tags_global_finished() { std::cerr<<'.'; }
    virtual void flush_roles_finished() { std::cerr<<'.'; }
    virtual void changelog_finished() { std::cerr<<'.'; }
    virtual void update_finished() { std::cerr<<" done.\n"; }
    virtual void current_update_finished() { std::cerr<<" done. "; }
    virtual void partial_started() { std::cerr<<"Reorganizing the database ..."; }
    virtual void partial_finished() { std::cerr<<" done.\n"; }

    virtual void parser_started() { std::cerr<<"Reading XML file ..."; }
    virtual void node_elapsed(Node::Id_Type id) { std::cerr<<" elapsed node "<<id.val()<<". "; }
    virtual void nodes_finished() { std::cerr<<" finished reading nodes. "; }
    virtual void way_elapsed(Way::Id_Type id) { std::cerr<<" elapsed way "<<id.val()<<". "; }
    virtual void ways_finished() { std::cerr<<" finished reading ways. "; }
    virtual void relation_elapsed(Relation::Id_Type id) { std::cerr<<" elapsed relation "<<id.val()<<". "; }
    virtual void relations_finished() { std::cerr<<" finished reading relations. "; }

    virtual void parser_succeeded() { std::cerr<<"Update complete.\n"; }
};


Osm_Backend_Callback* get_verbatim_callback()
{
  return new Verbose_Osm_Backend_Callback;
}


class Quiet_Osm_Backend_Callback : public Osm_Backend_Callback
{
  public:
    virtual void update_started() {}
    virtual void attic_update_started() {}
    virtual void compute_started() {}
    virtual void compute_attic_started() {}
    virtual void compute_finished() {}
    virtual void compute_attic_finished() {}
    virtual void compute_indexes_finished() {}
    virtual void update_ids_finished() {}
    virtual void update_coords_finished() {}
    virtual void prepare_delete_tags_finished() {}
    virtual void undeleted_finished() {}
    virtual void meta_finished() {}
    virtual void tags_local_finished() {}
    virtual void tags_global_finished() {}
    virtual void flush_roles_finished() {}
    virtual void changelog_finished() {}
    virtual void update_finished() {}
    virtual void current_update_finished() {}
    virtual void partial_started() {}
    virtual void partial_finished() {}

    virtual void parser_started() {}
    virtual void node_elapsed(Node::Id_Type id) {}
    virtual void nodes_finished() {}
    virtual void way_elapsed(Way::Id_Type id) {}
    virtual void ways_finished() {}
    virtual void relation_elapsed(Relation::Id_Type id) {}
    virtual void relations_finished() {}

    virtual void parser_succeeded() {}
};


Osm_Backend_Callback* get_quiet_callback()
{
  return new Quiet_Osm_Backend_Callback;
}


void report_file_error(const File_Error& e)
{
  if (e.error_number)
    std::cerr<<"File error caught: "
        <<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
  else
    std::cerr<<"File error caught: "<<e.filename<<' '<<e.origin<<'\n';
}
