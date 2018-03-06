
#include "../../expat/escape_xml.h"
#include "../frontend/basic_formats.h"
#include "output_xml.h"


bool Output_XML::write_http_headers()
{
  std::cout<<"Content-type: application/osm3s+xml\n";
  return true;
}


void Output_XML::write_payload_header
    (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp)
{
  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm version=\"0.6\" generator=\"Overpass API\">\n"
  "<note>The data included in this document is from www.openstreetmap.org. "
  "The data is made available under ODbL.</note>\n";
  std::cout<<"<meta osm_base=\""<<timestamp<<'\"';
  if (area_timestamp != "")
    std::cout<<" areas=\""<<area_timestamp<<"\"";
  std::cout<<"/>\n\n";
}


void Output_XML::write_footer()
{
  std::cout<<"\n</osm>\n";
}


void Output_XML::display_remark(const std::string& text)
{
  std::cout<<"<remark> "<<text<<" </remark>\n";
}


void Output_XML::display_error(const std::string& text)
{
  std::cout<<"<remark> "<<text<<" </remark>\n";
}


void Output_XML::print_global_bbox(const Bbox_Double& bbox)
{
  std::cout<<"  <bounds"
      " minlat=\""<<std::fixed<<std::setprecision(7)<<bbox.south<<"\""
      " minlon=\""<<std::fixed<<std::setprecision(7)<<bbox.west<<"\""
      " maxlat=\""<<std::fixed<<std::setprecision(7)<<bbox.north<<"\""
      " maxlon=\""<<std::fixed<<std::setprecision(7)<<bbox.east<<"\""
      "/>\n\n";
}


template< typename Id_Type >
void print_meta_xml(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
		    const std::map< uint32, std::string >& users)
{
  std::cout<<" version=\""<<meta.version<<"\" timestamp=\""<<iso_string(meta.timestamp)
      <<"\" changeset=\""<<meta.changeset<<"\" uid=\""<<meta.user_id<<"\"";
  std::map< uint32, std::string >::const_iterator it = users.find(meta.user_id);
  if (it != users.end())
    std::cout<<" user=\""<<escape_xml(it->second)<<"\"";
}


void prepend_action(const Output_Handler::Feature_Action& action, bool allow_delta = true)
{
  if (action == Output_Handler::keep)
    ;
  else if (action == Output_Handler::show_from)
    std::cout<<"<action type=\"show_initial\">\n";
  else if (action == Output_Handler::show_to)
    std::cout<<"<action type=\"show_final\">\n";

  if (allow_delta)
  {
    if (action == Output_Handler::modify)
      std::cout<<"<action type=\"modify\">\n<old>\n";
    else if (action == Output_Handler::create)
      std::cout<<"<action type=\"create\">\n";
    else if (action == Output_Handler::erase || action == Output_Handler::push_away)
      std::cout<<"<action type=\"delete\">\n<old>\n";
  }
}


void insert_action(const Output_Handler::Feature_Action& action)
{
  if (action == Output_Handler::keep)
    ;
  else if (action == Output_Handler::modify
      || action == Output_Handler::erase || action == Output_Handler::push_away)
    std::cout<<"</old>\n<new>\n";
}


void append_action(const Output_Handler::Feature_Action& action, bool is_new = false, bool allow_delta = true)
{
  if (action == Output_Handler::keep)
    ;
  else if (action == Output_Handler::show_from || action == Output_Handler::show_to)
    std::cout<<"</action>\n";

  if (allow_delta)
  {
    if (action == Output_Handler::modify)
      std::cout<<"</new>\n</action>\n";
    else if (action == Output_Handler::create)
      std::cout<<"</action>\n";
    else if (action == Output_Handler::erase || action == Output_Handler::push_away)
    {
      if (is_new)
        std::cout<<"</new>\n</action>\n";
      else
        std::cout<<"</old>\n</action>\n";
    }
  }
}


void print_tags(const std::vector< std::pair< std::string, std::string > >* tags,
		Output_Mode mode, bool& inner_tags_printed)
{
  if ((mode.mode & Output_Mode::TAGS) && tags && !tags->empty())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
	 it != tags->end(); ++it)
      std::cout<<"    <tag k=\""<<escape_xml(it->first)<<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
  }
}


void print_bounds(const Opaque_Geometry& geometry, Output_Mode mode, bool& inner_tags_printed)
{
  if ((mode.mode & Output_Mode::BOUNDS) && geometry.has_bbox())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    std::cout<<"    <bounds"
        " minlat=\""<<std::fixed<<std::setprecision(7)<<geometry.south()<<"\""
        " minlon=\""<<std::fixed<<std::setprecision(7)<<geometry.west()<<"\""
        " maxlat=\""<<std::fixed<<std::setprecision(7)<<geometry.north()<<"\""
        " maxlon=\""<<std::fixed<<std::setprecision(7)<<geometry.east()<<"\""
        "/>\n";
  }
  else if ((mode.mode & Output_Mode::CENTER) && geometry.has_center())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    std::cout<<"    <center"
        " lat=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lat()<<"\""
        " lon=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<"\""
        "/>\n";
  }
}


void print_geometry(const Opaque_Geometry& geometry, Output_Mode mode, bool& inner_tags_printed,
    const std::string& indent)
{
  if ((mode.mode & Output_Mode::GEOMETRY) && geometry.has_components())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    const std::vector< Opaque_Geometry* >* components = geometry.get_components();
    for (std::vector< Opaque_Geometry* >::const_iterator it = components->begin(); it != components->end(); ++it)
    {
      if (*it)
      {
        std::cout<<indent<<"<group>\n";
        print_geometry(**it, mode, inner_tags_printed, indent + "  ");
        std::cout<<indent<<"</group>\n";
      }
    }
  }
  else if ((mode.mode & Output_Mode::GEOMETRY) && geometry.has_line_geometry())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    const std::vector< Point_Double >* line = geometry.get_line_geometry();
    for (std::vector< Point_Double >::const_iterator it = line->begin(); it != line->end(); ++it)
      std::cout<<indent<<"<vertex"
          " lat=\""<<std::fixed<<std::setprecision(7)<<it->lat<<"\""
          " lon=\""<<std::fixed<<std::setprecision(7)<<it->lon<<"\""
          "/>\n";
  }
  else if ((mode.mode & Output_Mode::GEOMETRY) && geometry.has_multiline_geometry())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    const std::vector< std::vector< Point_Double > >* linestrings = geometry.get_multiline_geometry();
    for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings->begin();
        iti != linestrings->end(); ++iti)
    {
      std::cout<<indent<<"<linestring>\n";
      for (std::vector< Point_Double >::const_iterator it = iti->begin(); it != iti->end(); ++it)
        std::cout<<indent<<"  <vertex"
            " lat=\""<<std::fixed<<std::setprecision(7)<<it->lat<<"\""
            " lon=\""<<std::fixed<<std::setprecision(7)<<it->lon<<"\""
            "/>\n";
      std::cout<<indent<<"</linestring>\n";
    }
  }
  else if ((mode.mode & Output_Mode::GEOMETRY) && geometry.has_center())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    std::cout<<indent<<"<point"
        " lat=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lat()<<"\""
        " lon=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<"\""
        "/>\n";
  }
  else if ((mode.mode & Output_Mode::BOUNDS) && geometry.has_bbox())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    std::cout<<"    <bounds"
        " minlat=\""<<std::fixed<<std::setprecision(7)<<geometry.south()<<"\""
        " minlon=\""<<std::fixed<<std::setprecision(7)<<geometry.west()<<"\""
        " maxlat=\""<<std::fixed<<std::setprecision(7)<<geometry.north()<<"\""
        " maxlon=\""<<std::fixed<<std::setprecision(7)<<geometry.east()<<"\""
        "/>\n";
  }
  else if ((mode.mode & Output_Mode::CENTER) && geometry.has_center())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    std::cout<<indent<<"<center"
        " lat=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lat()<<"\""
        " lon=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<"\""
        "/>\n";
  }
}


void print_members(const Way_Skeleton& skel, const Opaque_Geometry& geometry,
		   Output_Mode mode, bool& inner_tags_printed)
{
  if ((mode.mode & Output_Mode::NDS) && !skel.nds.empty())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    for (uint i = 0; i < skel.nds.size(); ++i)
    {
      std::cout<<"    <nd ref=\""<<skel.nds[i].val()<<"\"";
      if (geometry.has_faithful_way_geometry() && geometry.way_pos_is_valid(i))
        std::cout<<" lat=\""<<std::fixed<<std::setprecision(7)<<geometry.way_pos_lat(i)
            <<"\" lon=\""<<std::fixed<<std::setprecision(7)<<geometry.way_pos_lon(i)<<'\"';
      std::cout<<"/>\n";
    }
  }
}


void print_members(const Relation_Skeleton& skel, const Opaque_Geometry& geometry,
		   const std::map< uint32, std::string >& roles,
		   Output_Mode mode, bool& inner_tags_printed)
{
  if ((mode.mode & Output_Mode::MEMBERS) && !skel.members.empty())
  {
    if (!inner_tags_printed)
    {
      std::cout<<">\n";
      inner_tags_printed = true;
    }
    for (uint i = 0; i < skel.members.size(); ++i)
    {
      std::map< uint32, std::string >::const_iterator it = roles.find(skel.members[i].role);
      std::cout<<"    <member type=\""<<member_type_name(skel.members[i].type)
	  <<"\" ref=\""<<skel.members[i].ref.val()
	  <<"\" role=\""<<escape_xml(it != roles.end() ? it->second : "???")<<"\"";

      if (skel.members[i].type == Relation_Entry::NODE)
      {
	if (geometry.has_faithful_relation_geometry() && geometry.relation_pos_is_valid(i))
          std::cout<<" lat=\""<<std::fixed<<std::setprecision(7)<<geometry.relation_pos_lat(i)
              <<"\" lon=\""<<std::fixed<<std::setprecision(7)<<geometry.relation_pos_lon(i)<<'\"';
        std::cout<<"/>\n";
      }
      else if (skel.members[i].type == Relation_Entry::WAY)
      {
	if (!geometry.has_faithful_relation_geometry())
	  std::cout<<"/>\n";
	else
	{
	  bool has_some_geometry = false;
	  for (uint j = 0; j < geometry.relation_way_size(i); ++j)
	    has_some_geometry |= geometry.relation_pos_is_valid(i, j);

	  if (!has_some_geometry)
	    std::cout<<"/>\n";
	  else
	  {
            std::cout<<">\n";
	    for (uint j = 0; j < geometry.relation_way_size(i); ++j)
	    {
	      if (geometry.relation_pos_is_valid(i, j))
                  std::cout<<"      <nd lat=\""<<std::fixed<<std::setprecision(7)<<geometry.relation_pos_lat(i, j)
                      <<"\" lon=\""<<std::fixed<<std::setprecision(7)<<geometry.relation_pos_lon(i, j)<<"\"/>\n";
              else
                  std::cout<<"      <nd/>\n";
	    }
            std::cout<<"    </member>\n";
	  }
	}
      }
      else
        std::cout<<"/>\n";
    }
  }
}


void print_node(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode)
{
  std::cout<<"  <node";
  if (mode.mode & Output_Mode::ID)
    std::cout<<" id=\""<<skel.id.val()<<'\"';
  if ((mode.mode & (Output_Mode::COORDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
      && geometry.has_center())
    std::cout<<" lat=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lat()
        <<"\" lon=\""<<std::fixed<<std::setprecision(7)<<geometry.center_lon()<<'\"';
  if ((mode.mode & (Output_Mode::VERSION | Output_Mode::META)) && meta && users)
    print_meta_xml(*meta, *users);

  bool inner_tags_printed = false;
  print_tags(tags, mode, inner_tags_printed);
  if (!inner_tags_printed)
    std::cout<<"/>\n";
  else
    std::cout<<"  </node>\n";
}


void print_way(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode)
{
  std::cout<<"  <way";
  if (mode.mode & Output_Mode::ID)
    std::cout<<" id=\""<<skel.id.val()<<'\"';
  if ((mode.mode & (Output_Mode::VERSION | Output_Mode::META)) && meta && users)
    print_meta_xml(*meta, *users);

  bool inner_tags_printed = false;
  print_bounds(geometry, mode, inner_tags_printed);
  print_members(skel, geometry, mode, inner_tags_printed);
  print_tags(tags, mode, inner_tags_printed);
  if (!inner_tags_printed)
    std::cout<<"/>\n";
  else
    std::cout<<"  </way>\n";
}


void print_relation(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const std::map< uint32, std::string >* users,
      Output_Mode mode)
{
  std::cout<<"  <relation";
  if (mode.mode & Output_Mode::ID)
    std::cout<<" id=\""<<skel.id.val()<<'\"';
  if ((mode.mode & (Output_Mode::VERSION | Output_Mode::META)) && meta && users)
    print_meta_xml(*meta, *users);

  bool inner_tags_printed = false;
  print_bounds(geometry, mode, inner_tags_printed);
  if (roles)
    print_members(skel, geometry, *roles, mode, inner_tags_printed);
  print_tags(tags, mode, inner_tags_printed);
  if (!inner_tags_printed)
    std::cout<<"/>\n";
  else
    std::cout<<"  </relation>\n";
}


template< typename Id_Type >
void print_deleted(const std::string& type_name, const Id_Type& id,
      const Output_Handler::Feature_Action& action,
      const OSM_Element_Metadata_Skeleton< Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode)
{
  std::cout<<"  <"<<type_name;
  if (mode.mode & Output_Mode::ID)
    std::cout<<" id=\""<<id.val()<<'\"';
  if (action == Output_Handler::erase)
    std::cout<<" visible=\"false\"";
  else
    std::cout<<" visible=\"true\"";
  if ((mode.mode & (Output_Mode::VERSION | Output_Mode::META)) && meta && users)
    print_meta_xml(*meta, *users);
  std::cout<<"/>\n";
}


void Output_XML::print_item(const Node_Skeleton& skel,
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
  prepend_action(action);

  print_node(skel, geometry, tags, meta, users, mode);

  if (new_skel)
  {
    insert_action(action);

    if (action == Output_Handler::erase || action == Output_Handler::push_away)
      print_deleted("node", new_skel->id, action, new_meta, users, mode);
    else
      print_node(*new_skel, *new_geometry, new_tags, new_meta, users, mode);
  }

  append_action(action, new_skel);
}


void Output_XML::print_item(const Way_Skeleton& skel,
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
  prepend_action(action);

  print_way(skel, geometry, tags, meta, users, mode);

  if (new_skel)
  {
    insert_action(action);

    if (action == Output_Handler::erase || action == Output_Handler::push_away)
      print_deleted("way", new_skel->id, action, new_meta, users, mode);
    else
      print_way(*new_skel, *new_geometry, new_tags, new_meta, users, mode);
  }

  append_action(action, new_skel);
}


void Output_XML::print_item(const Relation_Skeleton& skel,
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
  prepend_action(action);

  print_relation(skel, geometry, tags, meta, roles, users, mode);

  if (new_skel)
  {
    insert_action(action);

    if (action == Output_Handler::erase || action == Output_Handler::push_away)
      print_deleted("relation", new_skel->id, action, new_meta, users, mode);
    else
      print_relation(*new_skel, *new_geometry, new_tags, new_meta, roles, users, mode);
  }

  append_action(action, new_skel);
}


void Output_XML::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode,
      const Feature_Action& action)
{
  prepend_action(action, true);

  std::cout<<"  <"<<skel.type_name;
  if (mode.mode & Output_Mode::ID)
    std::cout<<" id=\""<<skel.id.val()<<'\"';

  bool inner_tags_printed = false;
  print_geometry(geometry, mode, inner_tags_printed, "    ");
  print_tags(tags, mode, inner_tags_printed);
  if (!inner_tags_printed)
    std::cout<<"/>\n";
  else
    std::cout<<"  </"<<skel.type_name<<">\n";

  append_action(action, false, true);
}
