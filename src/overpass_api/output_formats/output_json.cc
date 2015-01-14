
#include "../../expat/escape_json.h"
#include "output_json.h"


void Output_JSON::write_http_headers()
{
  //TODO
}


void Output_JSON::write_payload_header
    (const std::string& timestamp, const std::string& area_timestamp)
{
  //TODO
}


void Output_JSON::write_footer()
{
  //TODO
}


void Output_JSON::display_remark(const std::string& text)
{
  //TODO
}


void Output_JSON::display_error(const std::string& text)
{
  //TODO
  else if (header_written == json)
    messages += text;
}


template< typename Id_Type >
void print_meta_json(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
		    const std::map< uint32, string >& users)
{
}


void Output_JSON::print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
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
      const Feature_Action& action,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  //TODO
}


void Output_JSON::print_item(uint32 ll_upper, const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  //TODO
}
