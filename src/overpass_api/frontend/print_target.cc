/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../data/regular_expression.h"
#include "output.h"
#include "print_target.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class Print_Target_Xml : public Print_Target
{
  public:
    Print_Target_Xml(uint32 mode, Transaction& transaction)
        : Print_Target(mode, transaction) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
};

class Print_Target_Json : public Print_Target
{
  public:
    Print_Target_Json(uint32 mode, Transaction& transaction, bool first_target = true)
        : Print_Target(mode, transaction), first_elem(first_target) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
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
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
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


class Element_Collector
{
  public:
    Element_Collector(const string& title_key_, const string& title_);
    Element_Collector& add_constraint(const string& key, const string& value, bool straight);
    
    ~Element_Collector();

    bool consider(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    bool consider(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    bool consider(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    bool consider(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
  
    string get_output() const { return output; }
    
  private:
    string title_key;
    string title;
    
    string output;
    
    struct Element_Collector_Condition
    {
      Element_Collector_Condition(const string& key_, const string& value_, bool straight_)
      : key(key_), value(value_, true), straight(straight_) {}
      
      string key;
      Regular_Expression value;
      bool straight;
    };
    
    vector< Element_Collector_Condition* > constraints;

    bool check_tag_criterion(const vector< pair< string, string > >* tags,
        const Element_Collector_Condition& constraint) const;

    template< typename TSkel >
    void print(const TSkel& skel, const vector< pair< string, string > >* tags);
};


class Print_Target_Popup : public Print_Target
{
  public:
    Print_Target_Popup(uint32 mode, Transaction& transaction,
                       const vector< Category_Filter >& categories);
    ~Print_Target_Popup();
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0);
			    
    string get_output() const;
    
  private:
    vector< Element_Collector* > collector;
};

//-----------------------------------------------------------------------------

const char* MEMBER_TYPE[] = { 0, "node", "way", "relation" };

template< typename Id_Type >
void print_meta_xml(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
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
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users)
{
  cout<<"  <node";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
  if (mode & PRINT_COORDS)
    cout<<" lat=\""<<fixed<<setprecision(7)<<::lat(ll_upper, skel.ll_lower)
        <<"\" lon=\""<<fixed<<setprecision(7)<<::lon(ll_upper, skel.ll_lower)<<'\"';
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
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users)
{
  cout<<"  <way";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
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
	cout<<"    <nd ref=\""<<skel.nds[i].val()<<"\"/>\n";
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
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users)
{ 
  cout<<"  <relation";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
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
	    <<"\" ref=\""<<skel.members[i].ref.val()
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
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users)
{
  cout<<"  <area";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
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

template< typename Id_Type >
void print_meta_json(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
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
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
    
  cout<<"{\n"
        "  \"type\": \"node\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id.val();
  if (mode & PRINT_COORDS)
    cout<<",\n  \"lat\": "<<fixed<<setprecision(7)<<::lat(ll_upper, skel.ll_lower)
        <<",\n  \"lon\": "<<fixed<<setprecision(7)<<::lon(ll_upper, skel.ll_lower);
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
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
  
  cout<<"{\n"
        "  \"type\": \"way\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id.val();
  if (meta)
    print_meta_json(*meta, *users);
  
  if ((mode & PRINT_NDS) != 0 && !skel.nds.empty())
  {
    vector< Node::Id_Type >::const_iterator it = skel.nds.begin();
    cout<<",\n  \"nodes\": ["
           "\n    "<<it->val();
    for (++it; it != skel.nds.end(); ++it)
      cout<<",\n    "<<it->val();
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
  
  cout<<"\n}";
}

void Print_Target_Json::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users)
{ 
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
  
  cout<<"{\n"
        "  \"type\": \"relation\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id.val();
  if (meta)
    print_meta_json(*meta, *users);
  
  if ((mode & PRINT_MEMBERS) != 0 && !skel.members.empty())
  {
    vector< Relation_Entry >::const_iterator it = skel.members.begin();
    map< uint32, string >::const_iterator rit = roles.find(it->role);
    cout<<",\n  \"members\": ["
           "\n    {"
	   "\n      \"type\": \""<<MEMBER_TYPE[it->type]<<
	   "\",\n      \"ref\": "<<it->ref.val()<<
	   ",\n      \"role\": \""<<escape_cstr(rit != roles.end() ? rit->second : "???")<<
	   "\"\n    }";
    for (++it; it != skel.members.end(); ++it)
    {
      map< uint32, string >::const_iterator rit = roles.find(it->role);
      cout<<",\n    {"
            "\n      \"type\": \""<<MEMBER_TYPE[it->type]<<
            "\",\n      \"ref\": "<<it->ref.val()<<
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
  
  cout<<"\n}";
}

void Print_Target_Json::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
  
  cout<<"{\n"
        "  \"type\": \"area\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id.val();
  
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

string process_members(const string& raw_template, Node::Id_Type ref)
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

string process_members(const string& raw_template, uint32 id, const Relation_Entry& entry,
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
    else if (raw_template.substr(new_pos, 8) == "{{{id}}}")
      result<<id;
    else if (raw_template.substr(new_pos, 9) == "{{{ref}}}")
      result<<entry.ref.val();
    else if (raw_template.substr(new_pos, 10) == "{{{type}}}")
      result<<MEMBER_TYPE[entry.type];
    else if (raw_template.substr(new_pos, 10) == "{{{role}}}")
    {
      map< uint32, string >::const_iterator rit = roles.find(entry.role);
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

string process_tags(const string& raw_template, uint32 id, const string& key, const string& value)
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

string process_coords(const string& raw_template, uint32 id, double lat, double lon)
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

string process_coords(const string& raw_template, uint32 id,
		      double south, double west, double north, double east, uint zoom)
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

string extract_first(const string& raw_template)
{
  string::size_type old_pos = 0;
  string::size_type new_pos = 0;
  
  new_pos = raw_template.find("{{", old_pos);
  while (new_pos != string::npos)
  {
    old_pos = find_block_end(raw_template, new_pos);
    if (old_pos == string::npos)
      return "";
    else if (raw_template.substr(new_pos, 8) == "{{first:")
      return raw_template.substr(new_pos + 8, old_pos - new_pos - 10);
    new_pos = raw_template.find("{{", old_pos);
  }
  
  return "";
}

string antiprocess_coords(const string& raw_template)
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
    else if (raw_template.substr(new_pos, 7) == "{{none:")
      return raw_template.substr(new_pos + 7, old_pos - new_pos - 9);
    else
      result<<raw_template.substr(new_pos, old_pos - new_pos);
    new_pos = raw_template.find("{{", old_pos);
  }
  result<<raw_template.substr(old_pos, new_pos - old_pos);
  
  return result.str();
}

string process_template(const string& raw_template, uint32 id, string type,
			double south, double west, double north, double east, uint zoom,
			const vector< pair< string, string > >* tags,
			const vector< Node::Id_Type >* nds,
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
	vector< pair< string, string > >::const_iterator it = tags->begin();
	
	string first = extract_first(raw_template.substr(new_pos + 7, old_pos - new_pos - 9));
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
	vector< Node::Id_Type >::const_iterator it = nds->begin();
	
	string first = extract_first(raw_template.substr(new_pos + 10, old_pos - new_pos - 12));
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
	vector< Relation_Entry >::const_iterator it = members->begin();
	
	string first = extract_first(raw_template.substr(new_pos + 10, old_pos - new_pos - 12));
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

struct Box_Coords
{
  Box_Coords() : south(100.0), west(200.0), north(0), east(0) {}
  
  Box_Coords(Uint31_Index ll_upper)
  {
    pair< Uint32_Index, Uint32_Index > bbox_bounds = calc_bbox_bounds(ll_upper);
    if (bbox_bounds.second.val() == 1)
    {
      south = 200.0;
      west = 200.0;
      north = 0;
      east = 0;
    }
    else
    {
      south = ::lat(bbox_bounds.first.val(), 0);
      west = ::lon(bbox_bounds.first.val(), 0);
      north = ::lat(bbox_bounds.second.val() - 1, 0xffffffffu);
      east = ::lon(bbox_bounds.second.val() - 1, 0xffffffffu);
    }
  }
  
  double south, west, north, east;
};

uint detect_zoom(Uint31_Index ll_upper)
{
  if (0x80000000 & ll_upper.val())
  {
    if (ll_upper.val() & 0x1)
      return 13;
    else if (ll_upper.val() & 0x2)
      return 11;
    else if (ll_upper.val() & 0x4)
      return 9;
    else if (ll_upper.val() & 0x8)
      return 7;
    else if (ll_upper.val() & 0x10)
      return 5;
    else if (ll_upper.val() & 0x20)
      return 3;
    else
      return 1;
  }
  else
    return 14;
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users)
{
  if (written_elements_count == 0)
  {
    first_type = "node";
    first_id = skel.id.val();
  }
  ++written_elements_count;
  
  double lat = 100.0;
  double lon = 200.0;
  if (mode & PRINT_COORDS)
  {
    lat = ::lat(ll_upper, skel.ll_lower);
    lon = ::lon(ll_upper, skel.ll_lower);
  }
  output += process_template(node_template, skel.id.val(), "node", lat, lon, 100.0, 0, 17, tags, 0, 0, 0);
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users)
{
  if (written_elements_count == 0)
  {
    first_type = "way";
    first_id = skel.id.val();
  }
  ++written_elements_count;
  
  Box_Coords coords;
  if (mode & PRINT_COORDS)
    coords = Box_Coords(ll_upper);
  
  output += process_template(way_template, skel.id.val(), "way",
			     coords.south, coords.west, coords.north, coords.east,
			     detect_zoom(ll_upper),
			     tags, mode & PRINT_NDS ? &skel.nds : 0, 0, 0);
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users)
{ 
  if (written_elements_count == 0)
  {
    first_type = "relation";
    first_id = skel.id.val();
  }
  ++written_elements_count;
  
  Box_Coords coords;
  if (mode & PRINT_COORDS)
    coords = Box_Coords(ll_upper);

  output += process_template(relation_template, skel.id.val(), "relation",
			     coords.south, coords.west, coords.north, coords.east,
			     detect_zoom(ll_upper),
			     tags, 0, mode & PRINT_MEMBERS ? &skel.members : 0, &roles);
}

void Print_Target_Custom::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users)
{
}

//-----------------------------------------------------------------------------

Element_Collector::Element_Collector(const string& title_key_, const string& title_)
    : title_key(title_key_), title(title_)
{
  if (title != "")
    output = "\n<h2>" + title + "</h2>\n\n";
}


Element_Collector& Element_Collector::add_constraint(const string& key, const string& value, bool straight)
{
  try
  {
    constraints.push_back(new Element_Collector_Condition(key, value, straight));
  }
  catch (Regular_Expression_Error e)
  {
    //TODO: report the error
  }
  return *this;
}


Element_Collector::~Element_Collector()
{
  for (vector< Element_Collector_Condition* >::iterator it = constraints.begin();
       it != constraints.end(); ++it)
    delete *it;
}


bool Element_Collector::check_tag_criterion
    (const vector< pair< string, string > >* tags,
     const Element_Collector_Condition& constraint) const
{
  if (!tags)
    return false;
  for (vector< pair< string, string > >::const_iterator it = tags->begin(); it != tags->end(); ++it)
  {
    if (it->first == constraint.key && (constraint.value.matches(it->second)))
      return constraint.straight;
  }
  return !constraint.straight;
}


template< typename TSkel > string elem_type() { return ""; }
template< > string elem_type< Node_Skeleton >() { return "Node"; }
template< > string elem_type< Way_Skeleton >() { return "Way"; }
template< > string elem_type< Relation_Skeleton >() { return "Relation"; }


template< typename TSkel >
void Element_Collector::print(const TSkel& skel,
		const vector< pair< string, string > >* tags)
{
  // Search for a weblink
  string link;
  for (vector< pair< string, string > >::const_iterator it = tags->begin(); it != tags->end(); ++it)
  {
    if (it->second.find("www.") != string::npos)
      link = "http://" + it->second;
  }
  for (vector< pair< string, string > >::const_iterator it = tags->begin(); it != tags->end(); ++it)
  {
    if (it->second.find("http://") != string::npos)
      link = it->second;
  }
  
  // Add a section for the element
  output += "<p>";
  bool title_key_found = false;
  for (vector< pair< string, string > >::const_iterator it = tags->begin(); it != tags->end(); ++it)
  {
    if (it->first == title_key)
    {
      if (link != "")
	output += "<a href=\"" + link + "\" target=\"_blank\">";
      output += "<strong>" + it->second + "</strong><br/>\n";
      if (link != "")
	output += "</a>";
      title_key_found = true;
    }
  }
  if (!title_key_found)
  {
    if (link != "")
      output += "<a href=\"" + link + "\" target=\"_blank\">";
    ostringstream out;
    out<<skel.id.val();
    output += "<strong>" + elem_type< TSkel >() + " " + out.str() + "</strong><br/>\n";
    if (link != "")
      output += "</a>";
  }

  for (vector< pair< string, string > >::const_iterator it = tags->begin(); it != tags->end(); ++it)
    output += it->first + ": " + it->second + "<br/>\n";
  output += "</p>\n\n";
}


bool Element_Collector::consider(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users)
{
  for (vector< Element_Collector_Condition* >::const_iterator it = constraints.begin();
       it != constraints.end(); ++it)
  {
    if (!check_tag_criterion(tags, **it))
      return false;
  }

  print(skel, tags);

  return true;
}


bool Element_Collector::consider(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users)
{
  for (vector< Element_Collector_Condition* >::const_iterator it = constraints.begin();
       it != constraints.end(); ++it)
  {
    if (!check_tag_criterion(tags, **it))
      return false;
  }
  
  print(skel, tags);

  return true;
}


bool Element_Collector::consider(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users)
{ 
  for (vector< Element_Collector_Condition* >::const_iterator it = constraints.begin();
       it != constraints.end(); ++it)
  {
    if (!check_tag_criterion(tags, **it))
      return false;
  }
  
  print(skel, tags);

  return true;
}


bool Element_Collector::consider(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users)
{
  return true;
}

//-----------------------------------------------------------------------------

Print_Target_Popup::Print_Target_Popup(uint32 mode, Transaction& transaction,
                                       const vector< Category_Filter >& categories)
        : Print_Target(mode, transaction)
{
  if (categories.empty())
  {
    collector.push_back(&(*new Element_Collector("name", "POIs"))
        .add_constraint("name", ".", true)
        .add_constraint("highway", ".", false)
        .add_constraint("railway", ".", false)
        .add_constraint("landuse", ".", false)
        .add_constraint("type", "route|network|associatedStreet", false)
        .add_constraint("public_transport", ".", false)
        .add_constraint("route", "bus|ferry|railway|train|tram|trolleybus|subway|light_rail", false));
    collector.push_back(&(*new Element_Collector("name", "Streets"))
        .add_constraint("highway", "primary|secondary|tertiary|residential|unclassified", true));
    collector.push_back(&(*new Element_Collector("name", "Public Transport Stops"))
        .add_constraint("name", ".", true)
        .add_constraint("highway", "bus_stop|tram_stop", true));
    collector.push_back(&(*new Element_Collector("name", ""))
        .add_constraint("name", ".", true)
        .add_constraint("railway", "halt|station|tram_stop", true));
    collector.push_back(&(*new Element_Collector("ref", "Public Transport Lines"))
        .add_constraint("route", "bus|ferry|railway|train|tram|trolleybus|subway|light_rail", true));
  }
  else
  {
    for (vector< Category_Filter >::const_iterator it = categories.begin(); it != categories.end(); ++it)
    {
      if (it->filter_disjunction.empty())
	continue;
      
      string title_key = it->title_key;
      
      vector< vector< Tag_Filter > >::const_iterator it2
          = it->filter_disjunction.begin();
	  
      Element_Collector* new_collector
          = new Element_Collector(title_key, it->title);
      for (vector< Tag_Filter >::const_iterator it3 = it2->begin();
	  it3 != it2->end(); ++it3)
	new_collector->add_constraint(it3->key, it3->value, it3->straight);
      collector.push_back(new_collector);
      
      for (++it2; it2 != it->filter_disjunction.end(); ++it2)
      {
        Element_Collector* new_collector
            = new Element_Collector(title_key, "");
        for (vector< Tag_Filter >::const_iterator it3 = it2->begin();
	    it3 != it2->end(); ++it3)
	  new_collector->add_constraint(it3->key, it3->value, it3->straight);
        collector.push_back(new_collector);
      }
    }
  }
}


Print_Target_Popup::~Print_Target_Popup()
{
  for (vector< Element_Collector* >::iterator it = collector.begin();
       it != collector.end(); ++it)
    delete *it;
}

void Print_Target_Popup::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users)
{
  for (vector< Element_Collector* >::iterator it = collector.begin();
       it != collector.end(); ++it)
  {
    if ((*it)->consider(ll_upper, skel, tags, meta, users))
      break;
  }
}

void Print_Target_Popup::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users)
{
  for (vector< Element_Collector* >::iterator it = collector.begin();
       it != collector.end(); ++it)
  {
    if ((*it)->consider(ll_upper, skel, tags, meta, users))
      break;
  }
}

void Print_Target_Popup::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users)
{ 
  for (vector< Element_Collector* >::iterator it = collector.begin();
       it != collector.end(); ++it)
  {
    if ((*it)->consider(ll_upper, skel, tags, meta, users))
      break;
  }
}

void Print_Target_Popup::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users)
{
  for (vector< Element_Collector* >::iterator it = collector.begin();
       it != collector.end(); ++it)
  {
    if ((*it)->consider(ll_upper, skel, tags, meta, users))
      break;
  }
}

string Print_Target_Popup::get_output() const
{
  string result;
  for (vector< Element_Collector* >::const_iterator it = collector.begin();
       it != collector.end(); ++it)
    result += (*it)->get_output();
  return result;
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
      if (dynamic_cast< Print_Target_Custom* >(print_target)
	  || dynamic_cast< Print_Target_Xml* >(print_target)
	  || dynamic_cast< Print_Target_Json* >(print_target))
      {
        delete print_target;
        print_target = 0;
        first_target = false;
      }
    }
  }
  
  if (!print_target)
  {
    if (type == "xml")
      print_target = new Print_Target_Xml(mode, transaction);
    else if (type == "json")
      print_target = new Print_Target_Json(mode, transaction, first_target);
    else if (type == "custom")
      print_target = new Print_Target_Custom(mode, transaction, first_target,
					     node_template, way_template, relation_template);
    else if (type == "popup")
      print_target = new Print_Target_Popup(mode, transaction, categories);
  }

  return *print_target;
}

string Output_Handle::adapt_url(const string& url) const
{
  if (print_target && written_elements_count == 0)
    return process_template(url,
			    dynamic_cast< Print_Target_Custom* >(print_target)->get_first_id(),
			    dynamic_cast< Print_Target_Custom* >(print_target)->get_first_type(),
			    100.0, 200.0, 0, 0, 17, 0, 0, 0, 0);
  else
    return process_template(url, first_id, first_type, 100.0, 200.0, 0, 17, 0, 0, 0, 0, 0);
}

string Output_Handle::get_output() const
{
  if (print_target && dynamic_cast< Print_Target_Custom* >(print_target))
    return output
        + dynamic_cast< Print_Target_Custom* >(print_target)->get_output();
  else if (print_target && dynamic_cast< Print_Target_Popup* >(print_target))
    return output
        + dynamic_cast< Print_Target_Popup* >(print_target)->get_output();
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
  if (print_target)
    delete print_target;
}
