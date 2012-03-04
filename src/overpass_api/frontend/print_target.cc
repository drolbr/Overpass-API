/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "output.h"
#include "print_target.h"

using namespace std;

class Print_Target_Xml : public Print_Target
{
  public:
    Print_Target_Xml(uint32 mode, Transaction& transaction)
        : Print_Target(mode, transaction) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
};

class Print_Target_Json : public Print_Target
{
  public:
    Print_Target_Json(uint32 mode, Transaction& transaction, bool first_target = true)
        : Print_Target(mode, transaction), first_elem(first_target) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
			    
  private:
    mutable bool first_elem;
};

class Print_Target_Custom : public Print_Target
{
  public:
    Print_Target_Custom(uint32 mode, Transaction& transaction, bool first_target,
			const string& node_template_, const string& way_template_,
			const string& relation_template_)
        : Print_Target(mode, transaction), written_elements_count(0), first_id(0),
	node_template(node_template_), way_template(way_template_),
	relation_template(relation_template_) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0);
			    
    string get_output() const { return output; }
    uint32 get_written_elements_count() const { return written_elements_count; }
    string get_first_type() const { return first_type; }
    uint32 get_first_id() const { return first_id; }
    
  private:
    string output;
    uint32 written_elements_count;
    string first_type;
    uint32 first_id;
    string node_template;
    string way_template;
    string relation_template;
};

//-----------------------------------------------------------------------------

const char* MEMBER_TYPE[] = { 0, "node", "way", "relation" };

void print_meta_xml(const OSM_Element_Metadata_Skeleton& meta,
		    const map< uint32, string >& users)
{
  uint32 year = (meta.timestamp)>>26;
  uint32 month = ((meta.timestamp)>>22) & 0xf;
  uint32 day = ((meta.timestamp)>>17) & 0x1f;
  uint32 hour = ((meta.timestamp)>>12) & 0x1f;
  uint32 minute = ((meta.timestamp)>>6) & 0x3f;
  uint32 second = meta.timestamp & 0x3f;
  string timestamp("    -  -  T  :  :  Z");
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
  cout<<" version=\""<<meta.version<<"\" timestamp=\""<<timestamp
      <<"\" changeset=\""<<meta.changeset<<"\" uid=\""<<meta.user_id<<"\"";
  map< uint32, string >::const_iterator it = users.find(meta.user_id);
  if (it != users.end())
    cout<<" user=\""<<escape_xml(it->second)<<"\"";
}

void Print_Target_Xml::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  cout<<"  <node";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if (mode & PRINT_COORDS)
    cout<<" lat=\""<<fixed<<setprecision(7)<<Node::lat(ll_upper, skel.ll_lower)
        <<"\" lon=\""<<fixed<<setprecision(7)<<Node::lon(ll_upper, skel.ll_lower)<<'\"';
  if (meta)
    print_meta_xml(*meta, *users);
  if ((tags == 0) || (tags->empty()))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< pair< string, string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
	cout<<"    <tag k=\""<<escape_xml(it->first)
	    <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    cout<<"  </node>\n";
  }
}

void Print_Target_Xml::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  cout<<"  <way";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if (meta)
    print_meta_xml(*meta, *users);
  if (((tags == 0) || (tags->empty())) && ((mode & PRINT_NDS) == 0))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    if (mode & PRINT_NDS)
    {
      for (uint i(0); i < skel.nds.size(); ++i)
	cout<<"    <nd ref=\""<<skel.nds[i]<<"\"/>\n";
    }
    if ((tags != 0) && (!tags->empty()))
    {
      for (vector< pair< string, string > >::const_iterator it(tags->begin());
          it != tags->end(); ++it)
	  cout<<"    <tag k=\""<<escape_xml(it->first)
	      <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    }
    cout<<"  </way>\n";
  }
}

void Print_Target_Xml::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{ 
  cout<<"  <relation";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if (meta)
    print_meta_xml(*meta, *users);
  if (((tags == 0) || (tags->empty())) && ((mode & PRINT_NDS) == 0))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    if (mode & PRINT_MEMBERS)
    {
      for (uint i(0); i < skel.members.size(); ++i)
      {
	map< uint32, string >::const_iterator it = roles.find(skel.members[i].role);
	cout<<"    <member type=\""<<MEMBER_TYPE[skel.members[i].type]
	    <<"\" ref=\""<<skel.members[i].ref
	    <<"\" role=\""<<escape_xml(it != roles.end() ? it->second : "???")<<"\"/>\n";
      }
    }
    if ((tags != 0) && (!tags->empty()))
    {
      for (vector< pair< string, string > >::const_iterator it(tags->begin());
          it != tags->end(); ++it)
	  cout<<"    <tag k=\""<<escape_xml(it->first)
	      <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    }
    cout<<"  </relation>\n";
  }
}

void Print_Target_Xml::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  cout<<"  <area";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if ((tags == 0) || (tags->empty()))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< pair< string, string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
      cout<<"    <tag k=\""<<escape_xml(it->first)
          <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    cout<<"  </area>\n";
  }
}

//-----------------------------------------------------------------------------

void print_meta_json(const OSM_Element_Metadata_Skeleton& meta,
		const map< uint32, string >& users)
{
  uint32 year = (meta.timestamp)>>26;
  uint32 month = ((meta.timestamp)>>22) & 0xf;
  uint32 day = ((meta.timestamp)>>17) & 0x1f;
  uint32 hour = ((meta.timestamp)>>12) & 0x1f;
  uint32 minute = ((meta.timestamp)>>6) & 0x3f;
  uint32 second = meta.timestamp & 0x3f;
  string timestamp("    -  -  T  :  :  Z");
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
  cout<<",\n  \"timestamp\": \""<<timestamp<<"\""
        ",\n  \"version\": "<<meta.version<<
	",\n  \"changeset\": "<<meta.changeset;
  map< uint32, string >::const_iterator it = users.find(meta.user_id);
  if (it != users.end())
    cout<<",\n  \"user\": \""<<escape_cstr(it->second)<<"\"";
  cout<<",\n  \"uid\": "<<meta.user_id;
}

void Print_Target_Json::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
    
  cout<<"{\n"
        "  \"type\": \"node\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id;
  if (mode & PRINT_COORDS)
    cout<<",\n  \"lat\": "<<fixed<<setprecision(7)<<Node::lat(ll_upper, skel.ll_lower)
        <<",\n  \"lon\": "<<fixed<<setprecision(7)<<Node::lon(ll_upper, skel.ll_lower);
  if (meta)
    print_meta_json(*meta, *users);
  
  if (tags != 0 && !tags->empty())
  {
    vector< pair< string, string > >::const_iterator it = tags->begin();
    cout<<",\n  \"tags\": {"
           "\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    for (++it; it != tags->end(); ++it)
      cout<<",\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    cout<<"\n  }";
  }
  
  cout<<"\n}";
}

void Print_Target_Json::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
  
  cout<<"{\n"
        "  \"type\": \"way\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id;
  if (meta)
    print_meta_json(*meta, *users);
  
  if ((mode & PRINT_NDS) != 0 && !skel.nds.empty())
  {
    vector< uint32 >::const_iterator it = skel.nds.begin();
    cout<<",\n  \"nodes\": ["
           "\n    "<<*it;
    for (++it; it != skel.nds.end(); ++it)
      cout<<",\n    "<<*it;
    cout<<"\n  ]";
  }
  
  if (tags != 0 && !tags->empty())
  {
    vector< pair< string, string > >::const_iterator it = tags->begin();
    cout<<",\n  \"tags\": {"
           "\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    for (++it; it != tags->end(); ++it)
      cout<<",\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    cout<<"\n  }";
  }
  
  cout<<"\n}\n";
}

void Print_Target_Json::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{ 
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
  
  cout<<"{\n"
        "  \"type\": \"relation\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id;
  if (meta)
    print_meta_json(*meta, *users);
  
  if ((mode & PRINT_MEMBERS) != 0 && !skel.members.empty())
  {
    vector< Relation_Entry >::const_iterator it = skel.members.begin();
    map< uint32, string >::const_iterator rit = roles.find(it->role);
    cout<<",\n  \"members\": ["
           "\n    {"
	   "\n      \"type\": \""<<MEMBER_TYPE[it->type]<<
	   "\",\n      \"ref\": "<<it->ref<<
	   ",\n      \"role\": \""<<escape_cstr(rit != roles.end() ? rit->second : "???")<<
	   "\"\n    }";
    for (++it; it != skel.members.end(); ++it)
    {
      map< uint32, string >::const_iterator rit = roles.find(it->role);
      cout<<",\n    {"
            "\n      \"type\": \""<<MEMBER_TYPE[it->type]<<
            "\",\n      \"ref\": "<<it->ref<<
            ",\n      \"role\": \""<<escape_cstr(rit != roles.end() ? rit->second : "???")<<
            "\"\n    }";
    }
    cout<<"\n  ]";
  }
  
  if (tags != 0 && !tags->empty())
  {
    vector< pair< string, string > >::const_iterator it = tags->begin();
    cout<<",\n  \"tags\": {"
           "\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    for (++it; it != tags->end(); ++it)
      cout<<",\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    cout<<"\n  }";
  }
  
  cout<<"\n}\n";
}

void Print_Target_Json::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
  
  cout<<"{\n"
        "  \"type\": \"area\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id;
  
  if (tags != 0 && !tags->empty())
  {
    vector< pair< string, string > >::const_iterator it = tags->begin();
    cout<<",\n  \"tags\": {"
           "\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    for (++it; it != tags->end(); ++it)
      cout<<",\n    \""<<escape_cstr(it->first)<<"\": \""<<escape_cstr(it->second)<<"\"";
    cout<<"\n  }";
  }
  
  cout<<"\n}\n";
}

//-----------------------------------------------------------------------------

string::size_type find_block_end(string data, string::size_type pos)
{
  if (pos == string::npos || data.substr(pos, 2) != "{{")
    return pos;
  
  string::size_type curly_brace_count = 2;
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
  
  return string::npos;
}

string process_members(const string& raw_template, uint32 ref)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 9) == "{{{ref}}}")
      result<<ref;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

string process_members(const string& raw_template, const Relation_Entry& entry,
		       const map< uint32, string >& roles)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 9) == "{{{ref}}}")
      result<<entry.ref;
    else if (raw_template.substr(new_pos, 10) == "{{{type}}}")
      result<<MEMBER_TYPE[entry.type];
    else if (raw_template.substr(new_pos, 10) == "{{{role}}}")
    {
      map< uint32, string >::const_iterator rit = roles.find(entry.role);
      result<<escape_xml(rit != roles.end() ? rit->second : "???");
    }
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

string process_tags(const string& raw_template, const string& key, const string& value)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 9) == "{{{key}}}")
      result<<key;
    else if (raw_template.substr(new_pos, 11) == "{{{value}}}")
      result<<value;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

string process_coords(const string& raw_template, double lat, double lon)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 9) == "{{{lat}}}")
      result<<fixed<<setprecision(7)<<lat;
    else if (raw_template.substr(new_pos, 9) == "{{{lon}}}")
      result<<fixed<<setprecision(7)<<lon;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

string process_coords(const string& raw_template,
		      double south, double west, double north, double east)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
    {
      result<<raw_template.substr(new_pos);
      return result.str();
    }
    else if (raw_template.substr(new_pos, 11) == "{{{south}}}")
      result<<fixed<<setprecision(7)<<south;
    else if (raw_template.substr(new_pos, 10) == "{{{west}}}")
      result<<fixed<<setprecision(7)<<west;
    else if (raw_template.substr(new_pos, 11) == "{{{north}}}")
      result<<fixed<<setprecision(7)<<north;
    else if (raw_template.substr(new_pos, 10) == "{{{east}}}")
      result<<fixed<<setprecision(7)<<east;
    else if (raw_template.substr(new_pos, 9) == "{{{lat}}}")
      result<<fixed<<setprecision(7)<<(south + north)/2.0;
    else if (raw_template.substr(new_pos, 9) == "{{{lon}}}")
      result<<fixed<<setprecision(7)<<(east + west)/2.0;
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

string process_template(const string& raw_template, uint32 id, string type,
			double south, double west, double north, double east,
			const vector< pair< string, string > >* tags,
			const vector< uint32 >* nds,
			const vector< Relation_Entry >* members,
			const map< uint32, string >* roles)
{
  ostringstream result;
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    result<<raw_template.substr(old_pos, new_pos - old_pos);
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
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
	result<<process_coords(raw_template.substr(new_pos + 9, old_pos - new_pos - 11),
			       south, west);
    }
    else if (raw_template.substr(new_pos, 7) == "{{bbox:")
    {
      if (south < 100.0 && north < 100.0)
	result<<process_coords(raw_template.substr(new_pos + 7, old_pos - new_pos - 9),
			       south, west, north, east);
    }
    else if (raw_template.substr(new_pos, 7) == "{{tags:")
    {
      if (tags != 0 && !tags->empty())
      {
	for (vector< pair< string, string > >::const_iterator it = tags->begin();
	    it != tags->end(); ++it)
	  result<<process_tags(raw_template.substr(new_pos + 7, old_pos - new_pos - 9),
			       escape_xml(it->first), escape_xml(it->second));
      }
    }
    else if (raw_template.substr(new_pos, 10) == "{{members:")
    {
      if (nds != 0 && !nds->empty())
      {
	for (vector< uint32 >::const_iterator it = nds->begin(); it != nds->end(); ++it)
	  result<<process_members(raw_template.substr(new_pos + 10, old_pos - new_pos - 12), *it);
      }
      else if (members != 0 && !members->empty())
      {
	for (vector< Relation_Entry >::const_iterator it = members->begin();
	    it != members->end(); ++it)
	  result<<process_members(raw_template.substr(new_pos + 10, old_pos - new_pos - 12),
				  *it, *roles);
      }
    }
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

struct Box_Coords
{
  Box_Coords(Uint31_Index ll_upper)
  {
    pair< Uint32_Index, Uint32_Index > bbox_bounds = calc_bbox_bounds(ll_upper);
    south = Node::lat(bbox_bounds.first.val(), 0);
    west = Node::lon(bbox_bounds.first.val(), 0);
    north = Node::lat(bbox_bounds.second.val(), 0xffffffffu);
    east = Node::lon(bbox_bounds.second.val(), 0xffffffffu);
  }
  
  double south, north, west, east;
};

void Print_Target_Custom::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  if (written_elements_count == 0)
  {
    first_type = "node";
    first_id = skel.id;
  }
  ++written_elements_count;
  
  double lat = 100.0;
  double lon = 200.0;
  if (mode & PRINT_COORDS)
  {
    lat = Node::lat(ll_upper, skel.ll_lower);
    lon = Node::lon(ll_upper, skel.ll_lower);
  }
  output += process_template(node_template, skel.id, "node", lat, lon, 100.0, 0, tags, 0, 0, 0);
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
  if (written_elements_count == 0)
  {
    first_type = "way";
    first_id = skel.id;
  }
  ++written_elements_count;
  
  Box_Coords coords(ll_upper);
  if (mode & PRINT_COORDS)
    output += process_template(way_template, skel.id, "way",
			       coords.south, coords.west, coords.north, coords.east,
			       tags, &skel.nds, 0, 0);
  else
    output += process_template(way_template, skel.id, "way",
			       100.0, 200.0, 0, 0, tags, &skel.nds, 0, 0);
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{ 
  if (written_elements_count == 0)
  {
    first_type = "relation";
    first_id = skel.id;
  }
  ++written_elements_count;
  
  Box_Coords coords(ll_upper);
  if (mode & PRINT_COORDS)
    output += process_template(relation_template, skel.id, "relation",
			       coords.south, coords.west, coords.north, coords.east,
			       tags, 0, &skel.members, &roles);
  else
    output += process_template(relation_template, skel.id, "relation",
			       100.0, 200.0, 0, 0, tags, 0, &skel.members, &roles);
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton* meta,
		const map< uint32, string >* users)
{
}

//-----------------------------------------------------------------------------

Print_Target& Output_Handle::get_print_target(uint32 current_mode, Transaction& transaction)
{
  bool first_target = true;
  
  if (current_mode != mode)
  {
    mode = current_mode;
    if (print_target)
    {
      if (dynamic_cast< Print_Target_Custom* >(print_target))
      {
	output +=
	    dynamic_cast< Print_Target_Custom* >(print_target)->get_output();
	uint32 partly_elements_count =
	    dynamic_cast< Print_Target_Custom* >(print_target)->get_written_elements_count();
	if (written_elements_count == 0 && partly_elements_count > 0)
	{
	  first_type = dynamic_cast< Print_Target_Custom* >(print_target)->get_first_type();
	  first_id = dynamic_cast< Print_Target_Custom* >(print_target)->get_first_id();
	}
      }
      delete print_target;
      print_target = 0;
      first_target = false;
    }
  }
  
  if (print_target == 0)
  {
    if (type == "xml")
      print_target = new Print_Target_Xml(mode, transaction);
    else if (type == "json")
      print_target = new Print_Target_Json(mode, transaction, first_target);
    else if (type == "custom")
      print_target = new Print_Target_Custom(mode, transaction, first_target,
					     node_template, way_template, relation_template);
  }
  
  return *print_target;
}

string Output_Handle::adapt_url(const string& url) const
{
  if (written_elements_count == 0)
    return process_template(url,
			    dynamic_cast< Print_Target_Custom* >(print_target)->get_first_id(),
			    dynamic_cast< Print_Target_Custom* >(print_target)->get_first_type(),
			    100.0, 200.0, 0, 0, 0, 0, 0, 0);
  else
    return process_template(url, first_id, first_type, 100.0, 200.0, 0, 0, 0, 0, 0, 0);
}

string Output_Handle::get_output() const
{
  if (print_target && dynamic_cast< Print_Target_Custom* >(print_target))
    return output
        + dynamic_cast< Print_Target_Custom* >(print_target)->get_output();
  else
    return output;
}

uint32 Output_Handle::get_written_elements_count() const
{
  if (print_target && dynamic_cast< Print_Target_Custom* >(print_target))
    return written_elements_count
        + dynamic_cast< Print_Target_Custom* >(print_target)->get_written_elements_count();
  else
    return written_elements_count;
}

Output_Handle::~Output_Handle()
{
  delete print_target;
}
