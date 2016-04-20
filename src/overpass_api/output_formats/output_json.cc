
#include "../../expat/escape_json.h"
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
        "  \"generator\": \"Overpass API\",\n"
        "  \"osm3s\": {\n"
	"    \"timestamp_osm_base\": \""<<timestamp<<"\",\n";
  if (area_timestamp != "")
    std::cout<<"    \"timestamp_areas_base\": \""<<area_timestamp<<"\",\n";
  std::cout<<"    \"copyright\": \"The data included in this document is from www.openstreetmap.org."
	" The data is made available under ODbL.\"\n"
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

//   if ((mode.mode & Output_Mode::GEOMETRY) != 0 && !skel.nds.empty())
//   {
//     std::cout<<",\n  \"geometry\": [";
//     for (uint i = 0; i < skel.nds.size(); ++i) 
//     {
//       if (geometry && !((*geometry)[i] == Quad_Coord(0u, 0u)))
//       {
//         std::cout<<"\n    { \"lat\": "<<std::fixed<<std::setprecision(7)
//             <<::lat((*geometry)[i].ll_upper, (*geometry)[i].ll_lower)
//             <<", \"lon\": "<<std::fixed<<std::setprecision(7)
//             <<::lon((*geometry)[i].ll_upper, (*geometry)[i].ll_lower)<<" }";
//       }
//       else
//         std::cout<<"\n    null";
//       if (i < skel.nds.size() - 1)
//         std::cout << ",";
//     }
//     std::cout<<"\n  ]";
//   }  

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

//       if (geometry && skel.members[i].type == Relation_Entry::NODE &&
//          !((*geometry)[i][0] == Quad_Coord(0u, 0u)))
//       {
//         std::cout<<",\n      \"lat\": "<<std::fixed<<std::setprecision(7)
//             <<::lat((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower)
//             <<",\n      \"lon\": "<<std::fixed<<std::setprecision(7)
//             <<::lon((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower);
//       }
//       if (geometry && skel.members[i].type == Relation_Entry::WAY && !(*geometry)[i].empty())
//       {
//         std::cout<<",\n      \"geometry\": [";
//         for (std::std::vector< Quad_Coord >::const_iterator it = (*geometry)[i].begin();
//              it != (*geometry)[i].end(); ++it)
//         {
//            if (!(*it == Quad_Coord(0u, 0u)))
//            {
//              std::cout<<"\n         { \"lat\": "<<std::fixed<<std::setprecision(7)
//                  <<::lat(it->ll_upper, it->ll_lower)
//                  <<", \"lon\": "<<std::fixed<<std::setprecision(7)
//                  <<::lon(it->ll_upper, it->ll_lower)<<" }";
//            } else
//              std::cout<<"\n         null";
//            if (it + 1 != (*geometry)[i].end())
//              std::cout << ",";
//         }
//         std::cout<<"\n      ]";
//       }
      std::cout<<"\n    }";
    }
    std::cout<<"\n  ]";
  }
  
  print_tags(tags);  
  std::cout<<"\n}";
}


void Output_JSON::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  handle_first_elem(first_elem);  
  std::cout<<"{\n"
        "  \"type\": \""<<skel.type_name<<"\"";
  if (mode.mode & Output_Mode::ID)
    std::cout<<",\n  \"id\": "<<skel.id.val();
  
  print_tags(tags);  
  std::cout<<"\n}";
}
