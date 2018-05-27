#ifndef DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_OSMIUM_H
#define DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_OSMIUM_H


#include "../core/datatypes.h"
#include "../core/geometry.h"
#include "../frontend/output_handler.h"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/opl_output.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/io/error.hpp>
#include <osmium/io/gzip_compression.hpp>
#include <osmium/io/header.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/builder/attr.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/timestamp.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/types.hpp>


class Output_Osmium : public Output_Handler
{
public:
  Output_Osmium(std::string output_format_) : output_format(output_format_), writer(nullptr),
                                            output_file(nullptr), header(nullptr) {}

  ~Output_Osmium();

  virtual bool write_http_headers();
  virtual void write_payload_header(const std::string& db_dir,
                                    const std::string& timestamp, const std::string& area_timestamp);
  virtual void write_footer();
  virtual void display_remark(const std::string& text);
  virtual void display_error(const std::string& text);

  virtual void print_global_bbox(const Bbox_Double& bbox);

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
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0);

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
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0);

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
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0);

  virtual void print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode);

private:
  void maybe_flush();

  std::string output_format;
  osmium::memory::Buffer buffer{1024*1024};
  osmium::io::Writer* writer;
  osmium::io::File * output_file;
  osmium::io::Header* header;
};


#endif
