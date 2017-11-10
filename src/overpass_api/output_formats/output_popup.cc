
#include "../../expat/escape_xml.h"
#include "../frontend/basic_formats.h"
#include "output_popup.h"

#include <cmath>
#include <fstream>


bool Tag_Filter::matches(const std::vector< std::pair< std::string, std::string > >* tags) const
{
  if (!tags)
    return false;
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin(); it != tags->end(); ++it)
  {
    if (it->first == key && condition.matches(it->second))
      return straight;
  }
  return !straight;
}


std::string link_if_any(const std::vector< std::pair< std::string, std::string > >& tags)
{
  std::string link;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin(); it != tags.end(); ++it)
  {
    if (it->second.find("www.") != std::string::npos)
      link = "http://" + it->second;
  }
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin(); it != tags.end(); ++it)
  {
    if (it->second.find("http://") != std::string::npos)
      link = it->second;
  }
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin(); it != tags.end(); ++it)
  {
    if (it->second.find("https://") != std::string::npos)
      link = it->second;
  }

  return link;
}


std::string title_if_any(const std::string& link, const std::string& title_key,
    const std::vector< std::pair< std::string, std::string > >& tags)
{
  std::string title;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags.begin(); it != tags.end(); ++it)
  {
    if (it->first == title_key)
    {
      if (link != "")
	title += "<a href=\"" + link + "\" target=\"_blank\">";
      title += "<strong>" + it->second + "</strong>";
      if (link != "")
	title += "</a>";
      title += "<br/>\n";
      
      break;
    }
  }
  
  return title;
}


void Category_Filter::set_title(const std::string& title)
{
  output = "\n<h2>" + title + "</h2>";
}


void Category_Filter::set_title_key(const std::string& title_key_)
{
  title_key = title_key_;
}


template< typename TSkel > std::string elem_type() { return ""; }
template< > std::string elem_type< Node_Skeleton >() { return "Node"; }
template< > std::string elem_type< Way_Skeleton >() { return "Way"; }
template< > std::string elem_type< Relation_Skeleton >() { return "Relation"; }


template< typename TSkel >
std::string print(const std::string& title_key,
    const TSkel& skel, const std::vector< std::pair< std::string, std::string > >* tags)
{
  std::string link = tags ? link_if_any(*tags) : "";
  std::string result = "\n<p>" + (tags ? title_if_any(link, title_key, *tags) : "");
  
  if (result == "\n<p>")
  {
    if (link != "")
      result += "<a href=\"" + link + "\" target=\"_blank\">";
    std::ostringstream out;
    out<<skel.id.val();
    result += "<strong>" + elem_type< TSkel >() + " " + out.str() + "</strong>";
    if (link != "")
      result += "</a>";
    result += "<br/>\n";
  }

  if (tags)
  {
    for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
        it != tags->end(); ++it)
      result += it->first + ": " + it->second + "<br/>\n";
  }
  return result + "<p/>\n";
}


bool Category_Filter::consider(const Node_Skeleton& skel,
    const std::vector< std::pair< std::string, std::string > >* tags)
{
  std::vector< std::vector< Tag_Filter* > >::const_iterator it_conj = filter_disjunction.begin();
  for (; it_conj != filter_disjunction.end(); ++it_conj)
  {
    std::vector< Tag_Filter* >::const_iterator it = it_conj->begin();
    for (; it != it_conj->end(); ++it)
    {
      if ((*it)->matches(tags))
        break;
    }
    if (it != it_conj->end())
      break;
  }
  
  if (it_conj != filter_disjunction.end())
    output += print(title_key, skel, tags);
  
  return it_conj != filter_disjunction.end();
}


bool Category_Filter::consider(const Way_Skeleton& skel,
    const std::vector< std::pair< std::string, std::string > >* tags)
{
  std::vector< std::vector< Tag_Filter* > >::const_iterator it_conj = filter_disjunction.begin();
  for (; it_conj != filter_disjunction.end(); ++it_conj)
  {
    std::vector< Tag_Filter* >::const_iterator it = it_conj->begin();
    for (; it != it_conj->end(); ++it)
    {
      if ((*it)->matches(tags))
        break;
    }
    if (it != it_conj->end())
      break;
  }
  
  if (it_conj != filter_disjunction.end())
    output += print(title_key, skel, tags);
  
  return it_conj != filter_disjunction.end();
}


bool Category_Filter::consider(const Relation_Skeleton& skel,
    const std::vector< std::pair< std::string, std::string > >* tags)
{
  std::vector< std::vector< Tag_Filter* > >::const_iterator it_conj = filter_disjunction.begin();
  for (; it_conj != filter_disjunction.end(); ++it_conj)
  {
    std::vector< Tag_Filter* >::const_iterator it = it_conj->begin();
    for (; it != it_conj->end(); ++it)
    {
      if ((*it)->matches(tags))
        break;
    }
    if (it != it_conj->end())
      break;
  }
  
  if (it_conj != filter_disjunction.end())
    output += print(title_key, skel, tags);
  
  return it_conj != filter_disjunction.end();
}


bool Output_Popup::write_http_headers()
{
  std::cout<<"Content-type: text/html; charset=utf-8\n";
  return true;
}


void Output_Popup::write_payload_header
    (const std::string& db_dir_, const std::string& timestamp_, const std::string& area_timestamp_)
{
  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
  "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
  "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
  "<head>\n"
  "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
  "  <title>OSM3S Response</title>\n"
  "</head>\n"
  "<body>\n";
}


void Output_Popup::display_remark(const std::string& text)
{
  //TODO
}


void Output_Popup::display_error(const std::string& text)
{
  //TODO
}


void Output_Popup::write_footer()
{
  for (std::vector< Category_Filter* >::iterator it = categories.begin(); it != categories.end(); ++it)
    std::cout<<(*it)->result();
  
  std::cout<<"\n</body>\n</html>\n";
}


void Output_Popup::print_item(const Node_Skeleton& skel,
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
  for (std::vector< Category_Filter* >::iterator it = categories.begin(); it != categories.end(); ++it)
  {
    if ((*it)->consider(skel, tags))
      return;
  }
}


void Output_Popup::print_item(const Way_Skeleton& skel,
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
  for (std::vector< Category_Filter* >::iterator it = categories.begin(); it != categories.end(); ++it)
  {
    if ((*it)->consider(skel, tags))
      return;
  }
}


void Output_Popup::print_item(const Relation_Skeleton& skel,
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
  for (std::vector< Category_Filter* >::iterator it = categories.begin(); it != categories.end(); ++it)
  {
    if ((*it)->consider(skel, tags))
      return;
  }
}


void Output_Popup::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  // Intentionally empty
}
