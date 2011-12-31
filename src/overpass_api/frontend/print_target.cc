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

const char* MEMBER_TYPE[] = { 0, "node", "way", "relation" };

void print_meta(const OSM_Element_Metadata_Skeleton& meta,
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
    print_meta(*meta, *users);
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
    print_meta(*meta, *users);
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
    print_meta(*meta, *users);
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
