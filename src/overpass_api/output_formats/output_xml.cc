
#include "../../expat/escape_xml.h"
#include "output_xml.h"


void Output_XML::write_http_headers()
{
  std::cout<<"Content-type: application/osm3s+xml\n";
}


void Output_XML::write_payload_header
    (const std::string& timestamp, const std::string& area_timestamp)
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


template< typename Id_Type >
void print_meta_xml(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
		    const std::map< uint32, string >& users)
{
  uint32 year = Timestamp::year(meta.timestamp);
  uint32 month = Timestamp::month(meta.timestamp);
  uint32 day = Timestamp::day(meta.timestamp);
  uint32 hour = Timestamp::hour(meta.timestamp);
  uint32 minute = Timestamp::minute(meta.timestamp);
  uint32 second = Timestamp::second(meta.timestamp);
  std::string timestamp("    -  -  T  :  :  Z");
  timestamp[0] = (year / 1000) % 10 + '0';
  timestamp[1] = (year / 100) % 10 + '0';
  timestamp[2] = (year / 10) % 10 + '0';
  timestamp[3] = year % 10 + '0';
  timestamp[5] = (month / 10) % 10 + '0';
  timestamp[6] = month % 10 + '0';
  timestamp[8] = (day / 10) % 10 + '0';
  timestamp[9] = day % 10 + '0';
  timestamp[11] = (hour / 10) % 10 + '0';
  timestamp[12] = hour % 10 + '0';
  timestamp[14] = (minute / 10) % 10 + '0';
  timestamp[15] = minute % 10 + '0';
  timestamp[17] = (second / 10) % 10 + '0';
  timestamp[18] = second % 10 + '0';
  std::cout<<" version=\""<<meta.version<<"\" timestamp=\""<<timestamp
      <<"\" changeset=\""<<meta.changeset<<"\" uid=\""<<meta.user_id<<"\"";
  std::map< uint32, string >::const_iterator it = users.find(meta.user_id);
  if (it != users.end())
    std::cout<<" user=\""<<escape_xml(it->second)<<"\"";
}


void prepend_action(const Output_Handler::Feature_Action& action)
{
  if (action == Output_Handler::keep)
    ;
//   else if (action == MODIFY_OLD)
//     std::cout<<"<action type=\"modify\">\n<old>\n";
//   else if (action == MODIFY_NEW)
//     std::cout<<"<new>\n";
//   else if (action == DELETE)
//     std::cout<<"<action type=\"delete\">\n<old>\n";
//   else if (action == CREATE)
//     std::cout<<"<action type=\"create\">\n";
  //TODO
}


void insert_action(const Output_Handler::Feature_Action& action)
{
  if (action == Output_Handler::keep)
    ;
//   else if (action == MODIFY_OLD)
//     std::cout<<"<action type=\"modify\">\n<old>\n";
//   else if (action == MODIFY_NEW)
//     std::cout<<"<new>\n";
//   else if (action == DELETE)
//     std::cout<<"<action type=\"delete\">\n<old>\n";
//   else if (action == CREATE)
//     std::cout<<"<action type=\"create\">\n";
  //TODO
}


void append_action(const Output_Handler::Feature_Action& action)
{
  if (action == Output_Handler::keep)
    ;
//   else if (action == MODIFY_OLD)
//     std::cout<<"<action type=\"modify\">\n<old>\n";
//   else if (action == MODIFY_NEW)
//     std::cout<<"<new>\n";
//   else if (action == DELETE)
//     std::cout<<"<action type=\"delete\">\n<old>\n";
//   else if (action == CREATE)
//     std::cout<<"<action type=\"create\">\n";
  //TODO
}


void print_tags(const std::vector< std::pair< std::string, std::string > >* tags,
		Output_Mode mode, bool& inner_tags_printed)
{
  if (Output_Mode::TAGS && tags && !tags->empty())
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
        " minlat=\""<<fixed<<setprecision(7)<<geometry.south()<<"\""
        " minlon=\""<<fixed<<setprecision(7)<<geometry.west()<<"\""
        " maxlat=\""<<fixed<<setprecision(7)<<geometry.north()<<"\""
        " maxlon=\""<<fixed<<setprecision(7)<<geometry.east()<<"\""
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
        " lat=\""<<fixed<<setprecision(7)<<geometry.center_lat()<<"\""
        " lon=\""<<fixed<<setprecision(7)<<geometry.center_lon()<<"\""
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
    const std::vector< Point_Double >* line_nodes = 0;
    if (geometry.has_line_geometry())
      line_nodes = geometry.get_line_geometry();
    for (uint i = 0; i < skel.nds.size(); ++i)
    {
      std::cout<<"    <nd ref=\""<<skel.nds[i].val()<<"\"";
      if (line_nodes)
        std::cout<<" lat=\""<<std::fixed<<std::setprecision(7)<<(*line_nodes)[i].lat
            <<"\" lon=\""<<std::fixed<<std::setprecision(7)<<(*line_nodes)[i].lon<<'\"';
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
//     const std::vector< Opaque_Geometry* >* member_geom = 0;
//     if (geometry.has_member_geometry())
//       member_geom = geometry.get_member_geometry();
    for (uint i = 0; i < skel.members.size(); ++i)
    {
      std::map< uint32, std::string >::const_iterator it = roles.find(skel.members[i].role);
      std::cout<<"    <member type=\""<<member_type_name(skel.members[i].type)
	  <<"\" ref=\""<<skel.members[i].ref.val()
	  <<"\" role=\""<<escape_xml(it != roles.end() ? it->second : "???")<<"\"";
//             
//         if (geometry && skel.members[i].type == Relation_Entry::NODE
//             && !((*geometry)[i][0] == Quad_Coord(0u, 0u)))
//           cout<<" lat=\""<<fixed<<setprecision(7)
//               <<::lat((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower)
//               <<"\" lon=\""<<fixed<<setprecision(7)
//               <<::lon((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower)<<'\"';
//               
//         if (geometry && skel.members[i].type == Relation_Entry::WAY && !(*geometry)[i].empty())
//         {
//           cout<<">\n";
//           for (std::vector< Quad_Coord >::const_iterator it = (*geometry)[i].begin();
//                it != (*geometry)[i].end(); ++it)
//           {
//             cout<<"      <nd";
//             if (!(*it == Quad_Coord(0u, 0u)))
//               cout<<" lat=\""<<fixed<<setprecision(7)
//                   <<::lat(it->ll_upper, it->ll_lower)
//                   <<"\" lon=\""<<fixed<<setprecision(7)
//                   <<::lon(it->ll_upper, it->ll_lower)<<"\"";
//             cout<<"/>\n";
//           }
//           cout<<"    </member>\n";
//         }
//         else
//           cout<<"/>\n";
// //         if (geometry)
// //         {
// //           for (uint j = 0; j < (*geometry)[i].size(); ++j)
// //             std::cout<<fixed<<setprecision(7)<<::lat((*geometry)[i][j].ll_upper, (*geometry)[i][j].ll_lower)<<", "
// //                 <<fixed<<setprecision(7)<<::lon((*geometry)[i][j].ll_upper, (*geometry)[i][j].ll_lower)<<'\n';
// //         }
	  
//       if (line_nodes)
//         std::cout<<" lat=\""<<std::fixed<<std::setprecision(7)<<(*line_nodes)[i].lat
//             <<"\" lon=\""<<std::fixed<<std::setprecision(7)<<(*line_nodes)[i].lon<<'\"';
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
    std::cout<<" lat=\""<<fixed<<setprecision(7)<<geometry.center_lat()
        <<"\" lon=\""<<fixed<<setprecision(7)<<geometry.center_lon()<<'\"';
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
  print_members(skel, geometry, mode, inner_tags_printed);
  print_tags(tags, mode, inner_tags_printed);
  print_bounds(geometry, mode, inner_tags_printed);
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
      const std::map< uint32, string >* users,
      Output_Mode mode)
{
  std::cout<<"  <relation";
  if (mode.mode & Output_Mode::ID)
    std::cout<<" id=\""<<skel.id.val()<<'\"';
  if ((mode.mode & (Output_Mode::VERSION | Output_Mode::META)) && meta && users)
    print_meta_xml(*meta, *users);
  
  bool inner_tags_printed = false;
  if (roles)
    print_members(skel, geometry, *roles, mode, inner_tags_printed);
  print_tags(tags, mode, inner_tags_printed);
  print_bounds(geometry, mode, inner_tags_printed);
  if (!inner_tags_printed)
    std::cout<<"/>\n";
  else
    std::cout<<"  </relation>\n";
}


void Output_XML::print_item(const Node_Skeleton& skel,
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
  prepend_action(action);
  
  print_node(skel, geometry, tags, meta, users, mode);
  
  if (action != Output_Handler::keep && action != Output_Handler::create)
  {
    insert_action(action);
    
    print_node(skel, *new_geometry, new_tags, new_meta, users, mode);    
  }
  
  append_action(action);
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
  
  if (action != Output_Handler::keep && action != Output_Handler::create)
  {
    insert_action(action);
    
    print_way(*new_skel, *new_geometry, new_tags, new_meta, users, mode);    
  }
  
  append_action(action);
}


void Output_XML::print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const map< uint32, string >* users,
      Output_Mode mode,
      const Feature_Action& action,
      const Relation_Skeleton* new_skel,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  prepend_action(action);
  
  print_relation(skel, geometry, tags, meta, roles, users, mode);
  
  if (action != Output_Handler::keep && action != Output_Handler::create)
  {
    insert_action(action);
    
    print_relation(*new_skel, *new_geometry, new_tags, new_meta, roles, users, mode);
  }
  
  append_action(action);
}


void Output_XML::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  //TODO
}
