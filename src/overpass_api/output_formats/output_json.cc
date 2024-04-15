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

#include "../../expat/escape_json.h"
#include "../core/settings.h"
#include "../frontend/basic_formats.h"
#include "output_json.h"


bool Output_JSON::write_http_headers()
{
  std::cout<<"Content-type: application/json\n";
  return true;
}


void Output_JSON::write_payload_header
    (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp)
{
  if (padding != "")
    std::cout<<padding<<"(";

  std::cout<<"{\n"
        "  \"version\": 0.6,\n"
        "  \"generator\": \"Overpass API "<<basic_settings().version<<" "
            <<basic_settings().source_hash.substr(0, 8)<<"\",\n"
        "  \"osm3s\": {\n"
	"    \"timestamp_osm_base\": \""<<timestamp<<"\",\n";
  if (area_timestamp != "")
    std::cout<<"    \"timestamp_areas_base\": \""<<area_timestamp<<"\",\n";
  std::cout<<"    \"copyright\": \""<<copyright_notice(db_dir)<<"\"\n"
        "  },\n";
  std::cout<< "  \"elements\": [\n\n";
}


void Output_JSON::write_footer()
{
  std::cout<<"\n\n  ]"<<(messages != "" ? ",\n\"remark\": \"" + escape_cstr(messages) + "\"" : "")
      <<"\n}"<<(padding != "" ? ");\n" : "\n");
}


void Output_JSON::display_remark(const std::string& text)
{
  messages += text;
}


void Output_JSON::display_error(const std::string& text)
{
  messages += text;
}


void handle_first_elem(bool& first_elem)
{
  if (!first_elem)
    std::cout<<",\n";
  first_elem = false;
}


template< typename Id_Type >
void print_meta_json(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
		    const std::map< uint32, std::string >& users)
{
  std::cout<<",\n  \"timestamp\": \""<<iso_string(meta.timestamp)<<"\""
        ",\n  \"version\": "<<meta.version<<
	",\n  \"changeset\": "<<meta.changeset;
  std::map< uint32, std::string >::const_iterator it = users.find(meta.user_id);
  if (it != users.end())
    std::cout<<",\n  \"user\": \""<<escape_cstr(it->second)<<"\"";
  std::cout<<",\n  \"uid\": "<<meta.user_id;
}


void print_tags(const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (tags != 0 && !tags->empty())
  {
    std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
    std::cout<<",\n  \"tags\": {"
           "\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    for (++it; it != tags->end(); ++it)
      std::cout<<",\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    std::cout<<"\n  }";
  }
}


void Output_JSON::print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Node_Skeleton* new_skel,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
{
  handle_first_elem(first_elem);
  std::cout<<"{\n"
        "  \"type\": \"node\"";
  if (mode.mode & Output_Mode::ID)
    std::cout<<",\n  \"id\": "<<skel.id.val();

  if (mode.mode & (Output_Mode::COORDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
    std::cout<<",\n  \"lat\": "<<std::fixed<<std::setprecision(7)<<geometry.center_lat()
        <<",\n  \"lon\": "<<std::fixed<<std::setprecision(7)<<geometry.center_lon();
  if (meta)
    print_meta_json(*meta, *users);

  print_tags(tags);
  std::cout<<"\n}";
}


void print_bounds(const Opaque_Geometry& geometry, Output_Mode mode)
{
  if ((mode.mode & Output_Mode::BOUNDS) && geometry.has_bbox())
    std::cout<<",\n  \"bounds\": {\n"
        "    \"minlat\": "<<std::fixed<<std::setprecision(7)<<geometry.south()<<",\n"
        "    \"minlon\": "<<std::fixed<<std::setprecision(7)<<geometry.west()<<",\n"
        "    \"maxlat\": "<<std::fixed<<std::setprecision(7)<<geometry.north()<<",\n"
        "    \"maxlon\": "<<std::fixed<<std::setprecision(7)<<geometry.east()<<"\n"
        "  }";
  else if ((mode.mode & Output_Mode::CENTER) && geometry.has_center())
    std::cout<<",\n  \"center\": {\n"
        "    \"lat\": "<<std::fixed<<std::setprecision(7)<<geometry.center_lat()<<",\n"
        "    \"lon\": "<<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<"\n"
        "  }";
}


void Output_JSON::print_item(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Way_Skeleton* new_skel,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta)
{
  handle_first_elem(first_elem);
  std::cout<<"{\n"
        "  \"type\": \"way\"";
  if (mode.mode & Output_Mode::ID)
    std::cout<<",\n  \"id\": "<<skel.id.val();

  if (meta)
    print_meta_json(*meta, *users);

  print_bounds(geometry, mode);

  if ((mode.mode & Output_Mode::NDS) != 0 && !skel.nds.empty())
  {
    std::vector< Node::Id_Type >::const_iterator it = skel.nds.begin();
    std::cout<<",\n  \"nodes\": ["
           "\n    "<<it->val();
    for (++it; it != skel.nds.end(); ++it)
      std::cout<<",\n    "<<it->val();
    std::cout<<"\n  ]";
  }

  if ((mode.mode & Output_Mode::GEOMETRY) != 0 && geometry.has_faithful_way_geometry())
  {
    std::cout<<",\n  \"geometry\": [";
    for (uint i = 0; i < geometry.way_size(); ++i)
    {
      if (geometry.way_pos_is_valid(i))
        std::cout<<"\n    { \"lat\": "<<std::fixed<<std::setprecision(7)
            <<geometry.way_pos_lat(i)
            <<", \"lon\": "<<std::fixed<<std::setprecision(7)
            <<geometry.way_pos_lon(i)<<" }";
      else
        std::cout<<"\n    null";

      if (i < geometry.way_size() - 1)
        std::cout << ",";
    }
    std::cout<<"\n  ]";
  }

  print_tags(tags);
  std::cout<<"\n}";
}


void Output_JSON::print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Relation_Skeleton* new_skel,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  handle_first_elem(first_elem);
  std::cout<<"{\n"
        "  \"type\": \"relation\"";
  if (mode.mode & Output_Mode::ID)
    std::cout<<",\n  \"id\": "<<skel.id.val();

  if (meta)
    print_meta_json(*meta, *users);

  print_bounds(geometry, mode);

  if (roles && (mode.mode & Output_Mode::MEMBERS) != 0 && !skel.members.empty())
  {
    std::cout<<",\n  \"members\": [";
    for (uint i = 0; i < skel.members.size(); i++)
    {
      std::map< uint32, std::string >::const_iterator rit = roles->find(skel.members[i].role);
      std::cout<< (i == 0 ? "" : ",");
      std::cout <<"\n    {"
            "\n      \"type\": \""<<member_type_name(skel.members[i].type)<<
            "\",\n      \"ref\": "<<skel.members[i].ref.val()<<
            ",\n      \"role\": \""<<escape_cstr(rit != roles->end() ? rit->second : "???") << "\"";

      if (skel.members[i].type == Relation_Entry::NODE &&
          geometry.has_faithful_relation_geometry() && geometry.relation_pos_is_valid(i))
        std::cout<<",\n      \"lat\": "<<std::fixed<<std::setprecision(7)<<geometry.relation_pos_lat(i)
            <<",\n      \"lon\": "<<std::fixed<<std::setprecision(7)<<geometry.relation_pos_lon(i);

      if (skel.members[i].type == Relation_Entry::WAY && geometry.has_faithful_relation_geometry())
      {
        std::cout<<",\n      \"geometry\": [";
        for (uint j = 0; j < geometry.relation_way_size(i); ++j)
        {
          if (geometry.relation_pos_is_valid(i, j))
          {
            std::cout<<"\n         { \"lat\": "<<std::fixed<<std::setprecision(7)
                <<geometry.relation_pos_lat(i, j)
                <<", \"lon\": "<<std::fixed<<std::setprecision(7)
                <<geometry.relation_pos_lon(i, j)<<" }";
          }
          else
            std::cout<<"\n         null";
          if (j < geometry.relation_way_size(i) - 1)
            std::cout << ",";
        }
        std::cout<<"\n      ]";
      }

      std::cout<<"\n    }";
    }
    std::cout<<"\n  ]";
  }

  print_tags(tags);
  std::cout<<"\n}";
}


void print_geometry(const Opaque_Geometry& geometry, const std::string& indent)
{
  if (geometry.has_components())
  {
    std::cout<<"{"
        "\n"<<indent<<"  \"type\": \"GeometryCollection\","
        "\n"<<indent<<"  \"geometries\": [";

    bool first_printed = true;
    const std::vector< Opaque_Geometry* >* components = geometry.get_components();
    for (std::vector< Opaque_Geometry* >::const_iterator it = components->begin(); it != components->end(); ++it)
    {
      if (*it && (*it)->has_center())
      {
        if (first_printed)
          first_printed = false;
        else
          std::cout<<",";

        std::cout<<"\n"<<indent<<"    ";
        print_geometry(**it, indent + "    ");
      }
    }

    std::cout<<"\n"<<indent<<"  ]\n"<<indent<<"}";
  }
  else if (geometry.has_line_geometry())
  {
    std::cout<<"{"
        "\n"<<indent<<"  \"type\": \"LineString\","
        "\n"<<indent<<"  \"coordinates\": [";

    const std::vector< Point_Double >* line = geometry.get_line_geometry();
    for (std::vector< Point_Double >::const_iterator it = line->begin(); it != line->end(); ++it)
      std::cout<<(it == line->begin() ? "" : ",")<<"\n"<<indent<<"    ["
          <<std::fixed<<std::setprecision(7)<<it->lon<<", "
          <<std::fixed<<std::setprecision(7)<<it->lat<<"]";

    std::cout<<"\n"<<indent<<"  ]\n"<<indent<<"}";
  }
  else if (geometry.has_multiline_geometry())
  {
    std::cout<<"{"
        "\n"<<indent<<"  \"type\": \"Polygon\","
        "\n"<<indent<<"  \"coordinates\": [";

    const std::vector< std::vector< Point_Double > >* linestrings = geometry.get_multiline_geometry();
    for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings->begin();
        iti != linestrings->end(); ++iti)
    {
      std::cout<<(iti == linestrings->begin() ? "" : ",")<<"\n"<<indent<<"    [";
      for (std::vector< Point_Double >::const_iterator it = iti->begin(); it != iti->end(); ++it)
        std::cout<<(it == iti->begin() ? "" : ",")<<"\n"<<indent<<"      ["
            <<std::fixed<<std::setprecision(7)<<it->lon<<", "
            <<std::fixed<<std::setprecision(7)<<it->lat<<"]";
      std::cout<<"\n"<<indent<<"    ]";
    }

    std::cout<<"\n"<<indent<<"  ]\n"<<indent<<"}";
  }
  else if (geometry.has_center())
    std::cout<<"{"
        "\n"<<indent<<"  \"type\": \"Point\","
        "\n"<<indent<<"  \"coordinates\": [ "
        <<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<", "
        <<std::fixed<<std::setprecision(7)<<geometry.center_lat()<<" ]"
    "\n"<<indent<<"}";
}


void print_geometry(const Opaque_Geometry& geometry, Output_Mode mode)
{
  if ((mode.mode & Output_Mode::GEOMETRY) && (geometry.has_center()))
  {
    std::cout<<",\n  \"geometry\": ";
    print_geometry(geometry, "  ");
  }
  else if ((mode.mode & Output_Mode::BOUNDS) && geometry.has_bbox())
    std::cout<<",\n  \"geometry\": {"
        "\n    \"type\": \"Polygon\","
        "\n    \"coordinates\": ["
        "\n      ["
            <<std::fixed<<std::setprecision(7)<<geometry.west()<<", "
            <<std::fixed<<std::setprecision(7)<<geometry.south()<<"]"
        ",\n      ["
            <<std::fixed<<std::setprecision(7)<<geometry.east()<<", "
            <<std::fixed<<std::setprecision(7)<<geometry.south()<<"]"
        ",\n      ["
            <<std::fixed<<std::setprecision(7)<<geometry.east()<<", "
            <<std::fixed<<std::setprecision(7)<<geometry.north()<<"]"
        ",\n      ["
            <<std::fixed<<std::setprecision(7)<<geometry.west()<<", "
            <<std::fixed<<std::setprecision(7)<<geometry.north()<<"]"
        ",\n      ["
            <<std::fixed<<std::setprecision(7)<<geometry.west()<<", "
            <<std::fixed<<std::setprecision(7)<<geometry.south()<<"]"
        "\n    ]\n  }";
  else if ((mode.mode & Output_Mode::CENTER) && geometry.has_center())
    std::cout<<",\n  \"geometry\": {"
        "\n    \"type\": \"Point\","
        "\n    \"coordinates\": [ "
        <<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<", "
        <<std::fixed<<std::setprecision(7)<<geometry.center_lat()<<" ]"
        "\n  }";
}


void Output_JSON::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode,
      const Feature_Action& action)
{
  handle_first_elem(first_elem);
  std::cout<<"{\n"
        "  \"type\": \""<<skel.type_name<<"\"";
  if (mode.mode & Output_Mode::ID)
    std::cout<<",\n  \"id\": "<<skel.id.val();

  print_geometry(geometry, mode);
  print_tags(tags);
  std::cout<<"\n}";
}
