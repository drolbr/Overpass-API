#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_HANDLER_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_HANDLER_H


#include "../core/datatypes.h"
#include "../core/geometry.h"


struct Output_Mode
{
  Output_Mode(unsigned int mode_) : mode(mode_) {}
  
  static const unsigned int ID = 0x1;
  static const unsigned int COORDS = 0x2;
  static const unsigned int NDS = 0x4;
  static const unsigned int MEMBERS = 0x8;
  static const unsigned int TAGS = 0x10;
  static const unsigned int VERSION = 0x20;
  static const unsigned int META = 0x40;
  static const unsigned int GEOMETRY = 0x80;
  static const unsigned int BOUNDS = 0x100;
  static const unsigned int CENTER = 0x200;
  static const unsigned int COUNT = 0x400;
  
  operator unsigned int() const { return mode; }
  unsigned int mode;
};


const std::string& member_type_name(uint32 type);


class Output_Handler
{
public:
  enum Feature_Action { keep, show_from, show_to, modify, push_away, pull_in, erase, create };
  
  // shall return true if it really has written a content declaration
  virtual bool write_http_headers() = 0;
  
  virtual void write_payload_header(const std::string& db_dir,
				    const std::string& timestamp, const std::string& area_timestamp) = 0;
  virtual void write_footer() = 0;
  virtual void display_remark(const std::string& text) = 0;
  virtual void display_error(const std::string& text) = 0;

  virtual void print_global_bbox(const Bbox_Double& bbox) = 0;
  
  virtual void print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Node_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0) = 0;
  
  virtual void print_item(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Way_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0) = 0;
      
  virtual void print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Relation_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0) = 0;
                            
  virtual void print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode,
      const Feature_Action& action = keep) = 0;
      
  virtual std::string dump_config() const { return ""; }

  virtual ~Output_Handler() {}
};


#endif
