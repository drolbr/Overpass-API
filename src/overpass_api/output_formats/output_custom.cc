
#include "../../expat/escape_xml.h"
#include "../frontend/basic_formats.h"
#include "output_custom.h"

#include <cmath>
#include <fstream>


bool Output_Custom::write_http_headers()
{
  return false;
}


void Output_Custom::write_payload_header
    (const std::string& db_dir_, const std::string& timestamp_, const std::string& area_timestamp_)
{
  db_dir = db_dir_;
  timestamp = timestamp_;
  area_timestamp = area_timestamp_;
}


std::string process_template(const std::string& raw_template, unsigned int count)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    
    if (raw_template.substr(new_pos + 3, 8) == "count}}}")
    {
      result<<count;
      old_pos = new_pos + 11;
    }
    else
    {
      result<<"{{{";
      old_pos = new_pos + 3;
    }
    new_pos = raw_template.find("{{{", old_pos);
  }
  result<<raw_template.substr(old_pos);
  
  return result.str();
}


void Output_Custom::display_remark(const std::string& text)
{
  //TODO
}


void Output_Custom::display_error(const std::string& text)
{
  //TODO
}


std::string::size_type find_block_end(std::string data, std::string::size_type pos)
{
  if (pos == std::string::npos || data.substr(pos, 2) != "{{")
    return pos;
  
  std::string::size_type curly_brace_count = 2;
  if (data.substr(pos, 3) == "{{{")
    curly_brace_count = 3;
  pos += curly_brace_count;
  
  while (pos < data.size())
  {
    if (data[pos] == '{' && data.substr(pos, 2) == "{{")
      pos = find_block_end(data, pos);
    else if ((data[pos] == '}') && curly_brace_count == 2 && data.substr(pos, 2) == "}}")
      return pos+2;
    else if ((data[pos] == '}') && curly_brace_count == 3 && data.substr(pos, 3) == "}}}")
      return pos+3;
    else
      ++pos;
  }
  
  return std::string::npos;
}


void Output_Custom::set_output_templates()
{
  std::string data;
  
  std::ifstream in((db_dir + "/templates/" + template_name).c_str());
  while (in.good())
  {
    std::string buf;
    std::getline(in, buf);
    data += buf + '\n';
  }
  
  if (data.find("<includeonly>") != std::string::npos)
  {
    std::string::size_type start = data.find("<includeonly>") + 13;
    std::string::size_type end = data.find("</includeonly>");
    data = data.substr(start, end - start);
  }

  template_contains_js = (data.find("<script") != std::string::npos);

  if (data == "")
    data = "\n<p>Template not found.</p>\n";

  node_template = "\n<p>No {{node:..}} found in template.</p>\n";
  way_template = "\n<p>No {{way:..}} found in template.</p>\n";
  relation_template = "\n<p>No {{relation:..}} found in template.</p>\n";
  
  bool header_written = false;
  std::string::size_type pos = 0;
  while (pos < data.size())
  {
    if (data[pos] == '{')
    {
      if (data.substr(pos, 7) == "{{node:")
      {
	std::string::size_type end_pos = find_block_end(data, pos);
	
	if (!header_written)
	{
	  header = data.substr(0, pos);
	  header_written = true;
	}
	
	if (end_pos != std::string::npos)
	{
	  node_template = data.substr(pos + 7, end_pos - pos - 9);
	  pos = end_pos;
	}
	else
	  pos = data.size();
      }
      else if (data.substr(pos, 6) == "{{way:")
      {
	std::string::size_type end_pos = find_block_end(data, pos);

	if (!header_written)
	{
	  header = data.substr(0, pos);
	  header_written = true;
	}
	
	if (end_pos != std::string::npos)
	{
	  way_template = data.substr(pos + 6, end_pos - pos - 8);
	  pos = end_pos;
	}
	else
	  pos = data.size();
      }
      else if (data.substr(pos, 11) == "{{relation:")
      {
	std::string::size_type end_pos = find_block_end(data, pos);
	
	if (!header_written)
	{
	  header = data.substr(0, pos);
	  header_written = true;
	}
	
	if (end_pos != std::string::npos)
	{
	  relation_template = data.substr(pos + 11, end_pos - pos - 13);
	  pos = end_pos;
	}
	else
	  pos = data.size();
      }
      else
	++pos;
    }
    else
      ++pos;
  }
  
  if (!header_written)
    header = data.substr(0, pos);
}


std::string process_members(const std::string& raw_template, Node::Id_Type ref)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 9) == "{{{ref}}}")
      result<<ref.val();
    else if (raw_template.substr(new_pos, 8) == "{{first:")
      ;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}


std::string process_members(const std::string& raw_template, uint32 id, const Relation_Entry& entry,
		       const std::map< uint32, std::string >& roles)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 8) == "{{{id}}}")
      result<<id;
    else if (raw_template.substr(new_pos, 9) == "{{{ref}}}")
      result<<entry.ref.val();
    else if (raw_template.substr(new_pos, 10) == "{{{type}}}")
      result<<member_type_name(entry.type);
    else if (raw_template.substr(new_pos, 10) == "{{{role}}}")
    {
      std::map< uint32, std::string >::const_iterator rit = roles.find(entry.role);
      result<<escape_xml(rit != roles.end() ? rit->second : "???");
    }
    else if (raw_template.substr(new_pos, 8) == "{{first:")
      ;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}


std::string process_tags(const std::string& raw_template, uint32 id, const std::string& key, const std::string& value)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 8) == "{{{id}}}")
      result<<id;
    else if (raw_template.substr(new_pos, 9) == "{{{key}}}")
      result<<key;
    else if (raw_template.substr(new_pos, 11) == "{{{value}}}")
      result<<value;
    else if (raw_template.substr(new_pos, 8) == "{{first:")
      ;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}


std::string process_coords(const std::string& raw_template, uint32 id, double lat, double lon)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 8) == "{{{id}}}")
      result<<id;
    else if (raw_template.substr(new_pos, 9) == "{{{lat}}}")
      result<<std::fixed<<std::setprecision(7)<<lat;
    else if (raw_template.substr(new_pos, 9) == "{{{lon}}}")
      result<<std::fixed<<std::setprecision(7)<<lon;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}


std::string process_coords(const std::string& raw_template, uint32 id,
		      double south, double west, double north, double east, uint zoom)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 8) == "{{{id}}}")
      result<<id;
    else if (raw_template.substr(new_pos, 11) == "{{{south}}}")
      result<<std::fixed<<std::setprecision(7)<<south;
    else if (raw_template.substr(new_pos, 10) == "{{{west}}}")
      result<<std::fixed<<std::setprecision(7)<<west;
    else if (raw_template.substr(new_pos, 11) == "{{{north}}}")
      result<<std::fixed<<std::setprecision(7)<<north;
    else if (raw_template.substr(new_pos, 10) == "{{{east}}}")
      result<<std::fixed<<std::setprecision(7)<<east;
    else if (raw_template.substr(new_pos, 9) == "{{{lat}}}")
      result<<std::fixed<<std::setprecision(7)<<(south + north)/2.0;
    else if (raw_template.substr(new_pos, 9) == "{{{lon}}}")
      result<<std::fixed<<std::setprecision(7)<<(east + west)/2.0;
    else if (raw_template.substr(new_pos, 10) == "{{{zoom}}}")
      result<<zoom;
    else if (raw_template.substr(new_pos, 7) == "{{none:")
      ;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}


std::string extract_first(const std::string& raw_template)
{
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
      return "";
    else if (raw_template.substr(new_pos, 8) == "{{first:")
      return raw_template.substr(new_pos + 8, old_pos - new_pos - 10);
    new_pos = raw_template.find("{{", old_pos);
  }
  
  return "";
}


std::string antiprocess_coords(const std::string& raw_template)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 7) == "{{none:")
      return raw_template.substr(new_pos + 7, old_pos - new_pos - 9);
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}


std::string process_template(const std::string& raw_template, unsigned long long id, std::string type,
			double south, double west, double north, double east, uint zoom,
			const std::vector< std::pair< std::string, std::string > >* tags,
			const std::vector< Node::Id_Type >* nds,
			const std::vector< Relation_Entry >* members,
			const std::map< uint32, std::string >* roles)
{
  std::ostringstream result;
  std::string::size_type old_pos = 0;
  std::string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != std::string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == std::string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 8) == "{{{id}}}")
      result<<id;
    else if (raw_template.substr(new_pos, 10) == "{{{type}}}")
      result<<type;
    else if (raw_template.substr(new_pos, 9) == "{{coords:")
    {
      if (south < 100.0)
	result<<process_coords(raw_template.substr(new_pos + 9, old_pos - new_pos - 11), id,
			       south, west);
    }
    else if (raw_template.substr(new_pos, 7) == "{{bbox:")
    {
      if (south < 100.0 && north < 100.0)
	result<<process_coords(raw_template.substr(new_pos + 7, old_pos - new_pos - 9), id,
			       south, west, north, east, zoom);
      else if (south == 200.0)
	result<<antiprocess_coords(raw_template.substr(new_pos + 7, old_pos - new_pos - 9));
    }
    else if (raw_template.substr(new_pos, 7) == "{{tags:")
    {
      if (tags != 0 && !tags->empty())
      {
	std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
	
	std::string first = extract_first(raw_template.substr(new_pos + 7, old_pos - new_pos - 9));
	if (first != "" && it != tags->end())
	{
	  result<<process_tags(first, id, escape_xml(it->first), escape_xml(it->second));
	  ++it;
	}
	
	for (; it != tags->end(); ++it)
	  result<<process_tags(raw_template.substr(new_pos + 7, old_pos - new_pos - 9), id,
			       escape_xml(it->first), escape_xml(it->second));
      }
    }
    else if (raw_template.substr(new_pos, 10) == "{{members:")
    {
      if (nds != 0 && !nds->empty())
      {
	std::vector< Node::Id_Type >::const_iterator it = nds->begin();
	
	std::string first = extract_first(raw_template.substr(new_pos + 10, old_pos - new_pos - 12));
	if (first != "" && it != nds->end())
	{
	  result<<process_members(first, *it);
	  ++it;
	}
	
	for (; it != nds->end(); ++it)
	  result<<process_members(raw_template.substr(new_pos + 10, old_pos - new_pos - 12), *it);
      }
      else if (members != 0 && !members->empty())
      {
	std::vector< Relation_Entry >::const_iterator it = members->begin();
	
	std::string first = extract_first(raw_template.substr(new_pos + 10, old_pos - new_pos - 12));
	if (first != "" && it != members->end())
	{
	  result<<process_members(first, id, *it, *roles);
	  ++it;
	}
	
	for (; it != members->end(); ++it)
	  result<<process_members(raw_template.substr(new_pos + 10, old_pos - new_pos - 12), id,
				  *it, *roles);
      }
    }
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos);
  
  return result.str();
}


void Output_Custom::write_footer()
{
  if (count == 0 && redirect)
  {
    ::write_html_header(timestamp, area_timestamp, 200, false, true);
    std::cout<<"<p>No results found.</p>\n";
    std::cout<<"\n</body>\n</html>\n";
  }
  else if (count == 1 && redirect)
  {
    std::cout<<"Status: 302 Moved\n";
    std::cout<<"Location: "
        <<process_template(url, first_id, first_type, 100.0, 200.0, 0, 17, 0, 0, 0, 0, 0)<<"\n\n";
  }
  else
  {
    ::write_html_header(timestamp, area_timestamp, 200, template_contains_js, true);
    std::cout<<process_template(header, count);
    std::cout<<'\n';
    std::cout<<output;
    std::cout<<"\n</body>\n</html>\n";
  }
}


void Output_Custom::print_item(const Node_Skeleton& skel,
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
  if (count == 0)
  {
    set_output_templates();
    
    first_type = "node";
    first_id = skel.id.val();
  }
  ++count;
  
  double lat = 100.0;
  double lon = 200.0;
  if (mode.mode & (Output_Mode::COORDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
  {
    lat = geometry.center_lat();
    lon = geometry.center_lon();
  }
  output += process_template(node_template, skel.id.val(), "node", lat, lon, 100.0, 0, 17, tags, 0, 0, 0);
}


// struct Box_Coords
// {
//   Box_Coords() : south(100.0), west(200.0), north(0), east(0) {}
//   
//   Box_Coords(Uint31_Index ll_upper)
//   {
//     pair< Uint32_Index, Uint32_Index > bbox_bounds = calc_bbox_bounds(ll_upper);
//     if (bbox_bounds.second.val() == 1)
//     {
//       south = 200.0;
//       west = 200.0;
//       north = 0;
//       east = 0;
//     }
//     else
//     {
//       south = ::lat(bbox_bounds.first.val(), 0);
//       west = ::lon(bbox_bounds.first.val(), 0);
//       north = ::lat(bbox_bounds.second.val() - 1, 0xffffffffu);
//       east = ::lon(bbox_bounds.second.val() - 1, 0xffffffffu);
//     }
//   }
//   
//   double south, west, north, east;
// };


unsigned int detect_zoom(const Opaque_Geometry& geometry)
{
  if (!geometry.has_bbox())
    return 16;
  
  double delta_lat = geometry.north() - geometry.south();
  double delta_lon = fabs(geometry.west() - geometry.east());
  
  double threshold = 0.012;
  unsigned int result = 15;
  while (delta_lat > threshold || delta_lon > threshold)
  {
    threshold *= 2;
    --result;
  }
  return result;
}


void Output_Custom::print_item(const Way_Skeleton& skel,
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
  if (count == 0)
  {
    set_output_templates();
    
    first_type = "way";
    first_id = skel.id.val();
  }
  ++count;
  
  if (geometry.has_bbox())
  {
    output += process_template(way_template, skel.id.val(), "way",
			      geometry.south(), geometry.west(), geometry.north(), geometry.east(),
			      detect_zoom(geometry),
			      tags, mode.mode & Output_Mode::NDS ? &skel.nds : 0, 0, 0);
  }
  else
  {
    output += process_template(way_template, skel.id.val(), "way",
			      200.0, 200.0, 200.0, 200.0,
			      detect_zoom(geometry),
			      tags, mode.mode & Output_Mode::NDS ? &skel.nds : 0, 0, 0);
  }
}


void Output_Custom::print_item(const Relation_Skeleton& skel,
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
  if (count == 0)
  {
    set_output_templates();
    
    first_type = "relation";
    first_id = skel.id.val();
  }
  ++count;
  
  if (geometry.has_bbox())
  {
    output += process_template(relation_template, skel.id.val(), "relation",
			       geometry.south(), geometry.west(), geometry.north(), geometry.east(),
			       detect_zoom(geometry),
			       tags, 0, mode.mode & Output_Mode::MEMBERS ? &skel.members : 0, roles);
  }
  else
  {
    output += process_template(relation_template, skel.id.val(), "relation",
			       200.0, 200.0, 200.0, 200.0,
			       detect_zoom(geometry),
			       tags, 0, mode.mode & Output_Mode::MEMBERS ? &skel.members : 0, roles);
  }
}


void Output_Custom::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  // Intentionally empty
}
