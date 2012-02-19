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
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
};

class Print_Target_Json : public Print_Target
{
  public:
    Print_Target_Json(uint32 mode, Transaction& transaction, bool first_target = true)
        : Print_Target(mode, transaction), first_elem(first_target) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
			    
  private:
    mutable bool first_elem;
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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
		const map< uint32, string >* users) const
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

Print_Target& Output_Handle::get_print_target(uint32 current_mode, Transaction& transaction)
{
  bool first_target = true;
  
  if (current_mode != mode)
  {
    mode = current_mode;
    if (print_target)
    {
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
  }
  
  return *print_target;
}

Output_Handle::~Output_Handle()
{
  delete print_target;
}
