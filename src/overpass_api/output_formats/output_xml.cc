
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


void Output_XML::print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      const Feature_Action& action,
      const Opaque_Geometry* new_geometry,
      const std::vector< std::pair< std::string, std::string > >* new_tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
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
  
  std::cout<<"  <node id=\""<<skel.id.val()<<'\"';
  if (geometry.has_center())
    std::cout<<" lat=\""<<fixed<<setprecision(7)<<geometry.center_lat()
        <<"\" lon=\""<<fixed<<setprecision(7)<<geometry.center_lon()<<'\"';
  if (meta && users)
    print_meta_xml(*meta, *users);
  if ((tags == 0) || (tags->empty()))
    std::cout<<"/>\n";
  else
  {
    std::cout<<">\n";
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
	 it != tags->end(); ++it)
      std::cout<<"    <tag k=\""<<escape_xml(it->first)<<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    std::cout<<"  </node>\n";
  }
  
  if (action == Output_Handler::keep)
    ;
//   else if (action == MODIFY_OLD)
//     std::cout<<"</old>\n";
//   else if (action == MODIFY_NEW)
//     std::cout<<"</new>\n</action>\n";
//   else if (action == DELETE)
//   {
//     if (show_new_elem == visible_void)
//       std::cout<<"</old>\n</action>\n";
//     else
//     {
//       std::cout<<"</old>\n"
//           "<new>\n"
// 	  "  <node id=\""<<skel.id.val()<<"\" visible=\""<<(show_new_elem == visible_true ? "true" : "false")<<"\"";
//       if (new_meta)
//         print_meta_xml(*new_meta, *users);
//       std::cout<<"/>\n"
// 	  "</new>\n</action>\n";
//     }
//   }
//   else if (action == CREATE)
//     std::cout<<"</action>\n";
}


void Output_XML::print_item(const Way_Skeleton& skel,
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


void Output_XML::print_item(const Relation_Skeleton& skel,
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


void Output_XML::print_item(uint32 ll_upper, const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  //TODO
}
