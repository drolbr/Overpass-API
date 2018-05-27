
#include "../../expat/escape_xml.h"
#include "../frontend/basic_formats.h"
#include "output_osmium.h"

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

Output_Osmium::~Output_Osmium()
{
  delete writer;
  delete output_file;
  delete header;
}


bool Output_Osmium::write_http_headers()
{
  if (output_format == "pbf")
     std::cout<<"Content-type: application/vnd.openstreetmap.data.pbf\n" << std::flush;
  else if (output_format == "opl")
     std::cout<<"Content-type: application/vnd.openstreetmap.data.opl\n" << std::flush;
  return true;
}

void Output_Osmium::write_payload_header
    (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp)
{
  output_file = new osmium::io::File("", output_format);
  header = new osmium::io::Header();
  header->set("generator","Overpass API");
  header->set("osmosis_replication_timestamp", timestamp);
  writer = new osmium::io::Writer(*output_file, *header);
}

void Output_Osmium::write_footer()
{
  (*writer)(std::move(buffer));
  writer->flush();
  writer->close();

  delete writer;
  delete output_file;
  delete header;

  writer = nullptr;
  output_file = nullptr;
  header = nullptr;
}

void Output_Osmium::display_remark(const std::string& text)
{
  // Intentionally empty
}


void Output_Osmium::display_error(const std::string& text)
{
  // Intentionally empty
}


void Output_Osmium::print_global_bbox(const Bbox_Double& bbox)
{
  // Intentionally empty
}

void Output_Osmium::print_item(const Node_Skeleton& skel,
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
  using namespace osmium::builder::attr;

  if (!meta)
    return;

  std::map< uint32, std::string >::const_iterator it = users->find(meta->user_id);
  std::string user =  (it != users->end() ? it->second : "???" );

  osmium::Location loc;

  if (mode & Output_Mode::ID)
    ;
  if (mode & (Output_Mode::COORDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
  {
    loc.set_lat(geometry.center_lat());
    loc.set_lon(geometry.center_lon());
  }

  std::vector<pair_of_cstrings> tag_list;

  if ((tags != 0) && (!tags->empty()))
  {
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
      tag_list.push_back(std::make_pair(it->first.c_str(), it->second.c_str()));
  }

  const auto pos = osmium::builder::add_node(buffer,
      _id(skel.id.val()),
      _version(meta->version),
      _timestamp(osmium::Timestamp(iso_string(meta->timestamp))),
      _cid(meta->changeset),
      _uid(meta->user_id),
      _location(loc),
      _user(user),
      _tags(tag_list)
  );

  maybe_flush();
}

void Output_Osmium::print_item(const Way_Skeleton& skel,
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
  using namespace osmium::builder::attr;

  std::vector<osmium::NodeRef> nrvec;

  if (!meta)
    return;

  std::map< uint32, std::string >::const_iterator it = users->find(meta->user_id);
  std::string user = (it != users->end() ? it->second : "???" );

  if (((tags == 0) || (tags->empty())) &&
      ((mode & (Output_Mode::NDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER)) == 0))
    ;
  else
  {
    if (mode & Output_Mode::NDS)
    {

      for (uint i = 0; i < skel.nds.size(); ++i)
      {
        osmium::Location location;
        osmium::object_id_type ref = skel.nds[i].val();

        if (geometry.has_faithful_way_geometry() && geometry.way_pos_is_valid(i))
        {
          location.set_lat(geometry.way_pos_lat(i));
          location.set_lon(geometry.way_pos_lon(i));
        }

        nrvec.push_back(osmium::NodeRef{ref, location});
      }
    }
  }

  std::vector<pair_of_cstrings> tag_list;

  if ((tags != 0) && (!tags->empty()))
  {
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
      tag_list.push_back(std::make_pair(it->first.c_str(), it->second.c_str()));
  }

  const auto pos = osmium::builder::add_way(buffer,
      _id(skel.id.val()),
      _version(meta->version),
      _timestamp(osmium::Timestamp(iso_string(meta->timestamp))),
      _cid(meta->changeset),
      _uid(meta->user_id),
      _nodes(nrvec),
      _user(user),
      _tags(tag_list)
  );

  maybe_flush();
}

void Output_Osmium::print_item(const Relation_Skeleton& skel,
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
  using namespace osmium::builder::attr;

  std::vector<member_type> members;

  if (!meta)
    return;

  std::map< uint32, std::string >::const_iterator it = users->find(meta->user_id);
  std::string user =  (it != users->end() ? it->second : "???" );

  if (((tags == 0) || (tags->empty())) &&
      ((mode & (Output_Mode::NDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER)) == 0))
    ;
  else
  {
    if (mode & Output_Mode::MEMBERS)
    {
      for (uint i = 0; i < skel.members.size(); ++i)
      {
        std::map< uint32, std::string >::const_iterator it = roles->find(skel.members[i].role);

        osmium::item_type type;

        switch(skel.members[i].type) {

        case 1:
          type = osmium::item_type::node;
          break;
        case 2:
          type = osmium::item_type::way;
          break;
        case 3:
          type = osmium::item_type::relation;
          break;
        }

        members.push_back(member_type{type,
          (osmium::object_id_type) skel.members[i].ref.val(),
          it != roles->end() ? it->second.c_str() : "???" });
      }
    }
  }

  std::vector<pair_of_cstrings> tag_list;

  if ((tags != 0) && (!tags->empty()))
  {
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
      tag_list.push_back(std::make_pair(it->first.c_str(), it->second.c_str()));
  }

  const auto pos = osmium::builder::add_relation(buffer,
      _id(skel.id.val()),
      _version(meta->version),
      _timestamp(osmium::Timestamp(iso_string(meta->timestamp))),
      _cid(meta->changeset),
      _uid(meta->user_id),
      _members(members),
      _user(user),
      _tags(tag_list)
  );

  maybe_flush();
}

void Output_Osmium::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  // Intentionally empty
}

void Output_Osmium::maybe_flush()
{
  if (buffer.committed() > 800*1024) {
    osmium::memory::Buffer _buffer{1024*1024};
    using std::swap;
    swap(_buffer, buffer);
    (*writer)(std::move(_buffer));
  }
}

