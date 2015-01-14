#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_HANDLER_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_HANDLER_H


#include "../core/datatypes.h"
#include "../core/geometry.h"


class Output_Handler
{
public:
  enum Feature_Action { keep, modify, move_away, erase, create };
  
  virtual void write_http_headers() = 0;
  virtual void write_payload_header(const string& timestamp, const string& area_timestamp) = 0;
  virtual void write_footer() = 0;
  virtual void display_remark(const std::string& text) = 0;
  virtual void display_error(const std::string& text) = 0;

  virtual void print_global_bbox(const Bbox& bbox) = 0;
  
  virtual void print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags = 0,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
      const std::map< uint32, std::string >* users = 0,
      const Feature_Action& action = keep,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0) = 0;
  
  virtual void print_item(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags = 0,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
      const std::map< uint32, std::string >* users = 0,
      const Feature_Action& action = keep,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0) = 0;
      
  virtual void print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags = 0,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
      const std::map< uint32, std::string >* roles = 0,
      const map< uint32, string >* users = 0,
      const Feature_Action& action = keep,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0) = 0;
                            
  virtual void print_item(uint32 ll_upper, const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags = 0) = 0;

  virtual ~Output_Handler() {}
};


#endif
