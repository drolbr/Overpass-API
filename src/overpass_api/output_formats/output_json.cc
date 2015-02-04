
#include "../../expat/escape_json.h"
#include "output_json.h"


void Output_JSON::write_http_headers()
{
  std::cout<<"Content-type: application/json\n";
}


void Output_JSON::write_payload_header
    (const std::string& timestamp, const std::string& area_timestamp)
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


template< typename Id_Type >
void print_meta_json(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
		    const std::map< uint32, string >& users)
{
  //TODO
}


void Output_JSON::print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
{
  //TODO
}


void Output_JSON::print_item(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta)
{
  //TODO
}


void Output_JSON::print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const map< uint32, string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  //TODO
}


void Output_JSON::print_item(uint32 ll_upper, const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  //TODO
}
