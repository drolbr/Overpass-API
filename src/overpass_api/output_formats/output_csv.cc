
#include "../../expat/escape_json.h"
#include "output_csv.h"


namespace
{
  struct Object_Type
  {
    enum _ { invalid, node, way, relation, area };
    
    static std::string name(Object_Type::_ otype)
    {
      if (otype == Object_Type::node)
        return "node";
      else if (otype == Object_Type::way)
        return "way";
      else if (otype == Object_Type::relation)
        return "relation";
      else if (otype == Object_Type::area)
        return "area";
      return "unknown object";
    }
  };
}


bool Output_CSV::write_http_headers()
{
  std::cout<<"Content-type: text/csv\n";
  return true;
}


// Escape CSV output according to RFC 4180
std::string escape_csv(const std::string& input)
{
  std::string result;
  bool quotes_needed = false;
  
  for (int i = input.size() - 1; i >= 0; --i)
  {
    if (input[i] == '\n' || input[i] == ',')
      quotes_needed = true;
    else if (input[i] == '\"')
    {
      quotes_needed = true;
      if (result.empty())
        result = input.substr(0, i) + "\"" + input.substr(i);
      else
        result = result.substr(0, i) + "\"" + result.substr(i);
    }
  }
  
  if (!quotes_needed)
    return input;
  else if (result.empty())
    return std::string("\"") + input + "\"";
  else
    return std::string("\"") + result + "\"";
}


void Output_CSV::write_payload_header
    (const std::string& db_dir, const std::string& timestamp, const std::string& area_timestamp)
{
  if (csv_settings.with_headerline)
  {
    for (std::vector< std::pair< std::string, bool > >::const_iterator it = csv_settings.keyfields.begin();
        it != csv_settings.keyfields.end(); ++it)
    {
      std::cout<<(it->second ? "@" : "")<<escape_csv(it->first);
      if (it + 1 != csv_settings.keyfields.end())
        std::cout<<csv_settings.separator;
    }
    std::cout<<'\n';
  }
}


void Output_CSV::write_footer()
{
  // Intentionally empty
}


void Output_CSV::display_remark(const std::string& text)
{
  // Intentionally empty
}


void Output_CSV::display_error(const std::string& text)
{
  // Intentionally empty
}


std::string Output_CSV::dump_config() const
{
  std::string result = "(";
  
  for (std::vector< std::pair< std::string, bool > >::const_iterator it = csv_settings.keyfields.begin();
      it != csv_settings.keyfields.end(); ++it)
  {
    if (it != csv_settings.keyfields.begin())
      result += ",";
      
    if (it->second)
      result += "::" + it->first;
    else
      result += "\"" + escape_cstr(it->first) + "\"";
  }
  
  if (csv_settings.separator != "\t")
    result += std::string(";") + (csv_settings.with_headerline ? "true" : "false") + ";\""
        + escape_cstr(csv_settings.separator) + "\"";
  else if (!csv_settings.with_headerline)
    result += ";false";
    
  return result + ")";
}


template< typename Id_Type, typename OSM_Element_Metadata_Skeleton >
void process_csv_line(Object_Type::_ otype, Id_Type id, const Opaque_Geometry& geometry,
    const OSM_Element_Metadata_Skeleton* meta,
    const std::vector< std::pair< std::string, std::string> >* tags,
    const std::map< uint32, std::string >* users,
    const Csv_Settings& csv_settings,
    Output_Mode mode)
{
  std::vector< std::pair< std::string, bool > >::const_iterator it = csv_settings.keyfields.begin();
  while (true)
  {
    if (!it->second)
    {
      if (tags)
      {
	for (std::vector< std::pair< std::string, std::string> >::const_iterator it_tags = tags->begin();
	     it_tags != tags->end(); ++it_tags)
	{
	  if (it_tags->first == it->first)
	  {
	    std::cout<<escape_csv(it_tags->second);
	    break;
	  }
	}
      }
    }
    else
    {
      if (meta)
      {
        if (it->first == "version")
          std::cout<<meta->version;
        else if (it->first == "timestamp")
          std::cout<<Timestamp(meta->timestamp).str();
        else if (it->first == "changeset")
          std::cout<<meta->changeset;
        else if (it->first == "uid")
          std::cout<<meta->user_id;
        else if (it->first == "user")
        {
          std::map< uint32, std::string >::const_iterator uit = users->find(meta->user_id);
          if (uit != users->end())
            std::cout<<uit->second;
        }
      }

      if (it->first == "id")
      {
        if (mode.mode & Output_Mode::ID)
          std::cout<<id.val();
      }
      else if (it->first == "otype")
        std::cout<<int(otype);
      else if (it->first == "type")
	std::cout<<Object_Type::name(otype);
      else if (it->first == "lat")
      {
        if ((mode.mode & (Output_Mode::COORDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
	    && geometry.has_center())
          std::cout<<std::fixed<<std::setprecision(7)<<geometry.center_lat();
      }
      else if (it->first == "lon")
      {
        if ((mode.mode & (Output_Mode::COORDS | Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
	    && geometry.has_center())
          std::cout<<std::fixed<<std::setprecision(7)<<geometry.center_lon();
      }
    }
      
    if (++it == csv_settings.keyfields.end())
      break;
    std::cout<<csv_settings.separator;
  }
  std::cout<<"\n";
}


void Output_CSV::print_item(const Node_Skeleton& skel,
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
  process_csv_line(Object_Type::node, skel.id, geometry, meta, tags, users, csv_settings, mode);
}


void Output_CSV::print_item(const Way_Skeleton& skel,
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
  process_csv_line(Object_Type::way, skel.id, geometry, meta, tags, users, csv_settings, mode);
}


void Output_CSV::print_item(const Relation_Skeleton& skel,
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
  process_csv_line(Object_Type::relation, skel.id, geometry, meta, tags, users, csv_settings, mode);
}


void Output_CSV::print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode)
{
  //TODO
}
