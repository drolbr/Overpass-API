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
			    const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP);

    virtual void print_item_count(const Output_Item_Count& item_count);
};


class Print_Target_Json : public Print_Target
{
  public:
    Print_Target_Json(uint32 mode, Transaction& transaction, bool first_target = true)
        : Print_Target(mode, transaction), first_elem(first_target) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP);
    
    virtual void print_item_count(const Output_Item_Count& item_count);

    bool nothing_written() const { return first_elem; }
			    
  private:
    mutable bool first_elem;
};


struct Object_Type
{
  typedef enum { invalid, node, way, relation, area } _;
};


class Print_Target_Csv : public Print_Target
{
  public:
    Print_Target_Csv(uint32 mode, Transaction& transaction, bool first_target,
         const Csv_Settings& csv_settings_)
        : Print_Target(mode, transaction) {
      csv_settings = csv_settings_;
      needs_headerline = first_target ? csv_settings.with_headerline : false;
    }
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP);

    virtual void print_item_count(const Output_Item_Count& item_count);

  private:
    template< typename OSM_Element_Metadata_Skeleton >
    void process_csv_line(std::ostream& result,
			  Object_Type::_ otype,
                          const OSM_Element_Metadata_Skeleton* meta,
                          const vector< pair< string, string > >* tags,
                          const map< uint32, string >* users,
                          uint32 id, double lat, double lon);

    void process_csv_line(std::ostream& result, const Output_Item_Count& item_count);

    void print_headerline_if_needed();

    Csv_Settings csv_settings;
    mutable bool needs_headerline;
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
			    const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP);

    virtual void print_item_count(const Output_Item_Count& item_count);
			    
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
    Element_Collector(const string& title_key_);
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
    bool empty() const { return (output == ""); }
    
  private:
    string title_key;
    
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
			    const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP);

    virtual void print_item_count(const Output_Item_Count& item_count);
			    
    string get_output() const;
    
  private:
    vector< pair< string, vector< Element_Collector* > > > collector;
};

//-----------------------------------------------------------------------------

const char* MEMBER_TYPE[] = { 0, "node", "way", "relation" };

template< typename Id_Type >
void print_meta_xml(const OSM_Element_Metadata_Skeleton< Id_Type >& meta,
		    const map< uint32, string >& users)
{
  uint32 year = Timestamp::year(meta.timestamp);
  uint32 month = Timestamp::month(meta.timestamp);
  uint32 day = Timestamp::day(meta.timestamp);
  uint32 hour = Timestamp::hour(meta.timestamp);
  uint32 minute = Timestamp::minute(meta.timestamp);
  uint32 second = Timestamp::second(meta.timestamp);
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
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  if (action == KEEP)
    ;
  else if (action == MODIFY_OLD)
    std::cout<<"<action type=\"modify\">\n<old>\n";
  else if (action == MODIFY_NEW)
    std::cout<<"<new>\n";
  else if (action == DELETE)
    std::cout<<"<action type=\"delete\">\n<old>\n";
  else if (action == CREATE)
    std::cout<<"<action type=\"create\">\n";
  
  cout<<"  <node";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
  if (mode & (PRINT_COORDS | PRINT_GEOMETRY | PRINT_BOUNDS | PRINT_CENTER))
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
  
  if (action == KEEP)
    ;
  else if (action == MODIFY_OLD)
    std::cout<<"</old>\n";
  else if (action == MODIFY_NEW)
    std::cout<<"</new>\n</action>\n";
  else if (action == DELETE)
  {
    if (show_new_elem == visible_void)
      std::cout<<"</old>\n</action>\n";
    else
    {
      std::cout<<"</old>\n"
          "<new>\n"
	  "  <node id=\""<<skel.id.val()<<"\" visible=\""<<(show_new_elem == visible_true ? "true" : "false")<<"\"";
      if (new_meta)
        print_meta_xml(*new_meta, *users);
      std::cout<<"/>\n"
	  "</new>\n</action>\n";
    }
  }
  else if (action == CREATE)
    std::cout<<"</action>\n";
}


void Print_Target_Xml::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< Quad_Coord >* geometry,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  if (action == KEEP)
    ;
  else if (action == MODIFY_OLD)
    std::cout<<"<action type=\"modify\">\n<old>\n";
  else if (action == MODIFY_NEW)
    std::cout<<"<new>\n";
  else if (action == DELETE)
    std::cout<<"<action type=\"delete\">\n<old>\n";
  else if (action == CREATE)
    std::cout<<"<action type=\"create\">\n";
  
  cout<<"  <way";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
  if (meta)
    print_meta_xml(*meta, *users);
  if (((tags == 0) || (tags->empty())) &&
      ((mode & (PRINT_NDS | PRINT_GEOMETRY | PRINT_BOUNDS | PRINT_CENTER)) == 0))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    if (bounds && !(bounds->first == Quad_Coord(0u, 0u)))
    {
      if (bounds->second)
        std::cout<<"    <bounds"
            " minlat=\""<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            " minlon=\""<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            " maxlat=\""<<fixed<<setprecision(7)<<::lat(bounds->second->ll_upper, bounds->second->ll_lower)<<"\""
            " maxlon=\""<<fixed<<setprecision(7)<<::lon(bounds->second->ll_upper, bounds->second->ll_lower)<<"\""
            "/>\n";
      else
        std::cout<<"    <center"
            " lat=\""<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            " lon=\""<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            "/>\n";
    }
    if (mode & PRINT_NDS)
    {
      for (uint i = 0; i < skel.nds.size(); ++i)
      {
	cout<<"    <nd ref=\""<<skel.nds[i].val()<<"\"";
        if (geometry && !((*geometry)[i] == Quad_Coord(0u, 0u)))
          cout<<" lat=\""<<fixed<<setprecision(7)
              <<::lat((*geometry)[i].ll_upper, (*geometry)[i].ll_lower)
              <<"\" lon=\""<<fixed<<setprecision(7)
              <<::lon((*geometry)[i].ll_upper, (*geometry)[i].ll_lower)<<'\"';
        cout<<"/>\n";
      }
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
  
  if (action == KEEP)
    ;
  else if (action == MODIFY_OLD)
    std::cout<<"</old>\n";
  else if (action == MODIFY_NEW)
    std::cout<<"</new>\n</action>\n";
  else if (action == DELETE)
  {
    if (show_new_elem == visible_void)
      std::cout<<"</old>\n</action>\n";
    else
    {
      std::cout<<"</old>\n"
          "<new>\n"
	  "  <way id=\""<<skel.id.val()<<"\" visible=\""<<(show_new_elem == visible_true ? "true" : "false")<<"\"";
      if (new_meta)
        print_meta_xml(*new_meta, *users);
      std::cout<<"/>\n"
	  "</new>\n</action>\n";
    }
  }
  else if (action == CREATE)
    std::cout<<"</action>\n";
}


void Print_Target_Xml::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< std::vector< Quad_Coord > >* geometry,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{ 
  if (action == KEEP)
    ;
  else if (action == MODIFY_OLD)
    std::cout<<"<action type=\"modify\">\n<old>\n";
  else if (action == MODIFY_NEW)
    std::cout<<"<new>\n";
  else if (action == DELETE)
    std::cout<<"<action type=\"delete\">\n<old>\n";
  else if (action == CREATE)
    std::cout<<"<action type=\"create\">\n";
  
  cout<<"  <relation";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id.val()<<'\"';
  if (meta)
    print_meta_xml(*meta, *users);
  if (((tags == 0) || (tags->empty())) &&
      ((mode & (PRINT_NDS | PRINT_GEOMETRY | PRINT_BOUNDS | PRINT_CENTER)) == 0))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    if (bounds && !(bounds->first == Quad_Coord(0u, 0u)))
    {
      if (bounds->second)
        std::cout<<"    <bounds"
            " minlat=\""<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            " minlon=\""<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            " maxlat=\""<<fixed<<setprecision(7)<<::lat(bounds->second->ll_upper, bounds->second->ll_lower)<<"\""
            " maxlon=\""<<fixed<<setprecision(7)<<::lon(bounds->second->ll_upper, bounds->second->ll_lower)<<"\""
            "/>\n";
      else
        std::cout<<"    <center"
            " lat=\""<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            " lon=\""<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<"\""
            "/>\n";
    }
    if (mode & PRINT_MEMBERS)
    {
      for (uint i = 0; i < skel.members.size(); ++i)
      {
	map< uint32, string >::const_iterator it = roles.find(skel.members[i].role);
	cout<<"    <member type=\""<<MEMBER_TYPE[skel.members[i].type]
	    <<"\" ref=\""<<skel.members[i].ref.val()
	    <<"\" role=\""<<escape_xml(it != roles.end() ? it->second : "???")<<"\"";
            
        if (geometry && skel.members[i].type == Relation_Entry::NODE
            && !((*geometry)[i][0] == Quad_Coord(0u, 0u)))
          cout<<" lat=\""<<fixed<<setprecision(7)
              <<::lat((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower)
              <<"\" lon=\""<<fixed<<setprecision(7)
              <<::lon((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower)<<'\"';
              
        if (geometry && skel.members[i].type == Relation_Entry::WAY && !(*geometry)[i].empty())
        {
          cout<<">\n";
          for (std::vector< Quad_Coord >::const_iterator it = (*geometry)[i].begin();
               it != (*geometry)[i].end(); ++it)
          {
            cout<<"      <nd";
            if (!(*it == Quad_Coord(0u, 0u)))
              cout<<" lat=\""<<fixed<<setprecision(7)
                  <<::lat(it->ll_upper, it->ll_lower)
                  <<"\" lon=\""<<fixed<<setprecision(7)
                  <<::lon(it->ll_upper, it->ll_lower)<<"\"";
            cout<<"/>\n";
          }
          cout<<"    </member>\n";
        }
        else
          cout<<"/>\n";
//         if (geometry)
//         {
//           for (uint j = 0; j < (*geometry)[i].size(); ++j)
//             std::cout<<fixed<<setprecision(7)<<::lat((*geometry)[i][j].ll_upper, (*geometry)[i][j].ll_lower)<<", "
//                 <<fixed<<setprecision(7)<<::lon((*geometry)[i][j].ll_upper, (*geometry)[i][j].ll_lower)<<'\n';
//         }
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
  
  if (action == KEEP)
    ;
  else if (action == MODIFY_OLD)
    std::cout<<"</old>\n";
  else if (action == MODIFY_NEW)
    std::cout<<"</new>\n</action>\n";
  else if (action == DELETE)
  {
    if (show_new_elem == visible_void)
      std::cout<<"</old>\n</action>\n";
    else
    {
      std::cout<<"</old>\n"
          "<new>\n"
	  "  <relation id=\""<<skel.id.val()<<"\" visible=\""<<(show_new_elem == visible_true ? "true" : "false")<<"\"";
      if (new_meta)
        print_meta_xml(*new_meta, *users);
      std::cout<<"/>\n"
	  "</new>\n</action>\n";
    }
  }
  else if (action == CREATE)
    std::cout<<"</action>\n";
}


void Print_Target_Xml::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action)
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

void Print_Target_Xml::print_item_count(const Output_Item_Count& item_count)
{
  cout<<"  <count total=\"" << item_count.total << "\" "
        "nodes=\"" << item_count.nodes << "\" "
        "ways=\"" << item_count.ways << "\" "
        "relations=\"" << item_count.relations << "\" "
        "areas=\"" << item_count.areas << "\"/>\n";
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
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";
    
  cout<<"{\n"
        "  \"type\": \"node\"";
  if (mode & PRINT_IDS)
    cout<<",\n  \"id\": "<<skel.id.val();
  if (mode & (PRINT_COORDS | PRINT_GEOMETRY | PRINT_BOUNDS | PRINT_CENTER))
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
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< Quad_Coord >* geometry,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
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

  if (mode & (PRINT_BOUNDS | PRINT_CENTER))
  {
    if (bounds && !(bounds->first == Quad_Coord(0u, 0u)))
    {
      if (bounds->second)
        std::cout<<",\n  \"bounds\": {\n"
            "    \"minlat\": "<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<",\n"
            "    \"minlon\": "<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<",\n"
            "    \"maxlat\": "<<fixed<<setprecision(7)<<::lat(bounds->second->ll_upper, bounds->second->ll_lower)<<",\n"
            "    \"maxlon\": "<<fixed<<setprecision(7)<<::lon(bounds->second->ll_upper, bounds->second->ll_lower)<<"\n"
            "  }";
      else
        std::cout<<",\n  \"center\": {\n"
            "    \"lat\": "<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<",\n"
            "    \"lon\": "<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<"\n"
            "  }";
    }
  }
  
  if ((mode & PRINT_NDS) != 0 && !skel.nds.empty())
  {
    vector< Node::Id_Type >::const_iterator it = skel.nds.begin();
    cout<<",\n  \"nodes\": ["
           "\n    "<<it->val();
    for (++it; it != skel.nds.end(); ++it)
      cout<<",\n    "<<it->val();
    cout<<"\n  ]";
  }

  if ((mode & PRINT_GEOMETRY) != 0 && !skel.nds.empty())
  {
    cout<<",\n  \"geometry\": [";
    for (uint i = 0; i < skel.nds.size(); ++i) 
    {
      if (geometry && !((*geometry)[i] == Quad_Coord(0u, 0u)))
      {
        cout<<"\n    { \"lat\": "<<fixed<<setprecision(7)
            <<::lat((*geometry)[i].ll_upper, (*geometry)[i].ll_lower)
            <<", \"lon\": "<<fixed<<setprecision(7)
            <<::lon((*geometry)[i].ll_upper, (*geometry)[i].ll_lower)<<" }";
      }
      else
        cout<<"\n    null";
      if (i < skel.nds.size() - 1)
        cout << ",";
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


void Print_Target_Json::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< std::vector< Quad_Coord > >* geometry,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
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

  if (mode & (PRINT_BOUNDS | PRINT_CENTER))
  {
    if (bounds && !(bounds->first == Quad_Coord(0u, 0u)))
    {
      if (bounds->second)
        std::cout<<",\n  \"bounds\": {\n"
            "    \"minlat\": "<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<",\n"
            "    \"minlon\": "<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<",\n"
            "    \"maxlat\": "<<fixed<<setprecision(7)<<::lat(bounds->second->ll_upper, bounds->second->ll_lower)<<",\n"
            "    \"maxlon\": "<<fixed<<setprecision(7)<<::lon(bounds->second->ll_upper, bounds->second->ll_lower)<<"\n"
            "  }";
      else
        std::cout<<",\n  \"center\": {\n"
            "    \"lat\": "<<fixed<<setprecision(7)<<::lat(bounds->first.ll_upper, bounds->first.ll_lower)<<",\n"
            "    \"lon\": "<<fixed<<setprecision(7)<<::lon(bounds->first.ll_upper, bounds->first.ll_lower)<<"\n"
            "  }";
    }
  }
  
  if ((mode & PRINT_MEMBERS) != 0 && !skel.members.empty())
  {
    cout<<",\n  \"members\": [";
    for (uint i = 0; i < skel.members.size(); i++)
    {
      map< uint32, string >::const_iterator rit = roles.find(skel.members[i].role);
      cout<< (i == 0 ? "" : ",");
      cout <<"\n    {"
            "\n      \"type\": \""<<MEMBER_TYPE[skel.members[i].type]<<
            "\",\n      \"ref\": "<<skel.members[i].ref.val()<<
            ",\n      \"role\": \""<<escape_cstr(rit != roles.end() ? rit->second : "???") << "\"";

      if (geometry && skel.members[i].type == Relation_Entry::NODE &&
         !((*geometry)[i][0] == Quad_Coord(0u, 0u)))
      {
        cout<<",\n      \"lat\": "<<fixed<<setprecision(7)
            <<::lat((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower)
            <<",\n      \"lon\": "<<fixed<<setprecision(7)
            <<::lon((*geometry)[i][0].ll_upper, (*geometry)[i][0].ll_lower);
      }
      if (geometry && skel.members[i].type == Relation_Entry::WAY && !(*geometry)[i].empty())
      {
        cout<<",\n      \"geometry\": [";
        for (std::vector< Quad_Coord >::const_iterator it = (*geometry)[i].begin();
             it != (*geometry)[i].end(); ++it)
        {
           if (!(*it == Quad_Coord(0u, 0u)))
           {
             cout<<"\n         { \"lat\": "<<fixed<<setprecision(7)
                 <<::lat(it->ll_upper, it->ll_lower)
                 <<", \"lon\": "<<fixed<<setprecision(7)
                 <<::lon(it->ll_upper, it->ll_lower)<<" }";
           } else
             cout<<"\n         null";
           if (it + 1 != (*geometry)[i].end())
             cout << ",";
        }
        cout<<"\n      ]";
      }
      cout<<"\n    }";
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
		const map< uint32, string >* users, const Action& action)
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


void Print_Target_Json::print_item_count(const Output_Item_Count& item_count)
{
  if (first_elem)
    first_elem = false;
  else
    cout<<",\n";

  cout<<"{\n"
        "  \"count\": {";
  cout<<"\n    \"total\": " << item_count.total
      <<",\n    \"nodes\": " << item_count.nodes
      <<",\n    \"ways\": " << item_count.ways
      <<",\n    \"relations\": " << item_count.relations
      <<",\n    \"areas\": " << item_count.areas
      <<"\n  }";
  cout<<"\n}\n";
}

//-----------------------------------------------------------------------------

void Print_Target_Csv::print_headerline_if_needed()
{
  if (needs_headerline)
  {
    for (std::vector< std::pair< std::string, bool > >::const_iterator it = csv_settings.keyfields.begin();
        it != csv_settings.keyfields.end(); ++it)
    {
      cout << (it->second ? "@" : "") << it->first;
      if (it + 1 != csv_settings.keyfields.end())
        cout << csv_settings.separator;
    }
    cout << "\n";
    needs_headerline = false;
  }
}

template< typename OSM_Element_Metadata_Skeleton >
void Print_Target_Csv::process_csv_line(std::ostream& result, Object_Type::_ otype,
    const OSM_Element_Metadata_Skeleton* meta,
    const vector<pair<string, string> >* tags, const map<uint32, string>* users,
    uint32 id, double lat, double lon)
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
	    result << it_tags->second;
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
          result << meta->version;
        else if (it->first == "timestamp")
          result << Timestamp(meta->timestamp).str();
        else if (it->first == "changeset")
          result << meta->changeset;
        else if (it->first == "uid")
          result << meta->user_id;
        else if (it->first == "user")
        {
          map<uint32, string>::const_iterator uit = users->find(meta->user_id);
          if (uit != users->end())
            result << uit->second;
        }
      }

      if (it->first == "id")
      {
        if (mode & PRINT_IDS)
          result << id;
      }
      else if (it->first == "otype")
        result << int(otype);
      else if (it->first == "type")
      {
        if (otype == Object_Type::node)
          result << "node";
        else if (otype == Object_Type::way)
          result << "way";
        else if (otype == Object_Type::relation)
          result << "relation";
        else if (otype == Object_Type::area)
          result << "area";
        else
          result << "unknown object";
      }
      else if (it->first == "lat")
      {
        if ((mode & (PRINT_COORDS | PRINT_GEOMETRY | PRINT_BOUNDS | PRINT_CENTER)) && lat < 100)
          result << fixed << setprecision(7) << lat;
      }
      else if (it->first == "lon")
      {
        if ((mode & (PRINT_COORDS | PRINT_GEOMETRY | PRINT_BOUNDS | PRINT_CENTER)) && lon < 200)
          result << fixed << setprecision(7) << lon;
      }
    }
      
    if (++it == csv_settings.keyfields.end())
      break;
    result << csv_settings.separator;
  }
  result << "\n";
}


void Print_Target_Csv::process_csv_line(std::ostream& result, const Output_Item_Count& item_count)
{
  for (std::vector< std::pair< std::string, bool > >::const_iterator it = csv_settings.keyfields.begin();
       it != csv_settings.keyfields.end(); ++it)
  {
    if (it->first == "count")
      result << item_count.total;
    else if (it->first == "count:nodes")
      result << item_count.nodes;
    else if (it->first == "count:ways")
      result << item_count.ways;
    else if (it->first == "count:relations")
      result << item_count.relations;
    else if (it->first == "count:areas")
      result << item_count.areas;
    else if (it->first == "otype")
      result << Object_Type::invalid;
    else if (it->first == "type")
      result << "count";

    if (it + 1 != csv_settings.keyfields.end())
      result << csv_settings.separator;
  }
  result << "\n";
}


void Print_Target_Csv::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  print_headerline_if_needed();
  process_csv_line(std::cout, Object_Type::node, meta, tags, users, skel.id.val(),
          ::lat(ll_upper, skel.ll_lower), ::lon(ll_upper, skel.ll_lower));
}

void Print_Target_Csv::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< Quad_Coord >* geometry,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  double lat(100.0), lon(200.0);

  print_headerline_if_needed();

  if (bounds && !(bounds->first == Quad_Coord(0u, 0u)) && !(bounds->second))
  {
    lat = ::lat(bounds->first.ll_upper, bounds->first.ll_lower);
    lon = ::lon(bounds->first.ll_upper, bounds->first.ll_lower);
  }

  process_csv_line(std::cout, Object_Type::way, meta, tags, users, skel.id.val(), lat, lon);
}

void Print_Target_Csv::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< std::vector< Quad_Coord > >* geometry,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  double lat(100.0), lon(200.0);

  print_headerline_if_needed();

  if (bounds && !(bounds->first == Quad_Coord(0u, 0u)) && !(bounds->second))
  {
    lat = ::lat(bounds->first.ll_upper, bounds->first.ll_lower);
    lon = ::lon(bounds->first.ll_upper, bounds->first.ll_lower);
  }

  process_csv_line(std::cout, Object_Type::relation, meta, tags, users, skel.id.val(), lat, lon);
}


void Print_Target_Csv::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action)

{
  double lat(100.0), lon(200.0);

  print_headerline_if_needed();

  process_csv_line(std::cout, Object_Type::area, meta, tags, users, skel.id.val(), lat, lon);
}

void Print_Target_Csv::print_item_count(const Output_Item_Count& item_count)
{
  print_headerline_if_needed();

  process_csv_line(std::cout, item_count);
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
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
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
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< Quad_Coord >* geometry,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
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
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< std::vector< Quad_Coord > >* geometry,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
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
		const map< uint32, string >* users, const Action& action)
{
}

void Print_Target_Custom::print_item_count(const Output_Item_Count& item_count)
{

}

//-----------------------------------------------------------------------------

Element_Collector::Element_Collector(const string& title_key_)
    : title_key(title_key_) {}


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
      output += "<strong>" + it->second + "</strong>";
      if (link != "")
	output += "</a>";
      output += "<br/>\n";
      title_key_found = true;
    }
  }
  if (!title_key_found)
  {
    if (link != "")
      output += "<a href=\"" + link + "\" target=\"_blank\">";
    ostringstream out;
    out<<skel.id.val();
    output += "<strong>" + elem_type< TSkel >() + " " + out.str() + "</strong>";
    if (link != "")
      output += "</a>";
    output += "<br/>\n";
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
  for (vector< Element_Collector_Condition* >::const_iterator it = constraints.begin();
       it != constraints.end(); ++it)
  {
    if (!check_tag_criterion(tags, **it))
      return false;
  }
  
  print(skel, tags);

  return true;
}

//-----------------------------------------------------------------------------

Print_Target_Popup::Print_Target_Popup(uint32 mode, Transaction& transaction,
                                       const vector< Category_Filter >& categories)
        : Print_Target(mode, transaction)
{
  if (categories.empty())
  {
    collector.push_back(make_pair("POIs", vector< Element_Collector* >(1,
        &(*new Element_Collector("name"))
        .add_constraint("name", ".", true)
        .add_constraint("highway", ".", false)
        .add_constraint("railway", ".", false)
        .add_constraint("landuse", ".", false)
        .add_constraint("type", "route|network|associatedStreet", false)
        .add_constraint("public_transport", ".", false)
        .add_constraint("route", "bus|ferry|railway|train|tram|trolleybus|subway|light_rail", false))));
    collector.push_back(make_pair("Streets", vector< Element_Collector* >(1,
        &(*new Element_Collector("name"))
        .add_constraint("highway", "primary|secondary|tertiary|residential|unclassified", true))));
    collector.push_back(make_pair("Public Transport Stops", vector< Element_Collector* >(1,
        &(*new Element_Collector("name"))
        .add_constraint("name", ".", true)
        .add_constraint("highway", "bus_stop|tram_stop", true))));
    collector.back().second.push_back(&(*new Element_Collector("name"))
        .add_constraint("name", ".", true)
        .add_constraint("railway", "halt|station|tram_stop", true));
    collector.push_back(make_pair("Public Transport Lines", vector< Element_Collector* >(1,
        &(*new Element_Collector("ref"))
        .add_constraint("route", "bus|ferry|railway|train|tram|trolleybus|subway|light_rail", true))));
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
          = new Element_Collector(title_key);
      for (vector< Tag_Filter >::const_iterator it3 = it2->begin();
	  it3 != it2->end(); ++it3)
	new_collector->add_constraint(it3->key, it3->value, it3->straight);
      collector.push_back(make_pair(it->title, vector< Element_Collector* >(1, new_collector)));
      
      for (++it2; it2 != it->filter_disjunction.end(); ++it2)
      {
        Element_Collector* new_collector
            = new Element_Collector(title_key);
        for (vector< Tag_Filter >::const_iterator it3 = it2->begin();
	    it3 != it2->end(); ++it3)
	  new_collector->add_constraint(it3->key, it3->value, it3->straight);
        collector.back().second.push_back(new_collector);
      }
    }
  }
}


Print_Target_Popup::~Print_Target_Popup()
{
  for (vector< pair< string, vector< Element_Collector* > > >::iterator it2 = collector.begin();
       it2 != collector.end(); ++it2)
  {
    for (vector< Element_Collector* >::iterator it = it2->second.begin();
        it != it2->second.end(); ++it)
      delete *it;
  }
}

void Print_Target_Popup::print_item(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  for (vector< pair< string, vector< Element_Collector* > > >::iterator it2 = collector.begin();
       it2 != collector.end(); ++it2)
  {
    for (vector< Element_Collector* >::iterator it = it2->second.begin();
        it != it2->second.end(); ++it)
    {
      if ((*it)->consider(ll_upper, skel, tags, meta, users))
        return;
    }
  }
}


void Print_Target_Popup::print_item(uint32 ll_upper, const Way_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< Quad_Coord >* geometry,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{
  for (vector< pair< string, vector< Element_Collector* > > >::iterator it2 = collector.begin();
       it2 != collector.end(); ++it2)
  {
    for (vector< Element_Collector* >::iterator it = it2->second.begin();
        it != it2->second.end(); ++it)
    {
      if ((*it)->consider(ll_upper, skel, tags, meta, users))
        return;
    }
  }
}


void Print_Target_Popup::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
		const vector< pair< string, string > >* tags,
                const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                const std::vector< std::vector< Quad_Coord > >* geometry,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
		Show_New_Elem show_new_elem)
{ 
  for (vector< pair< string, vector< Element_Collector* > > >::iterator it2 = collector.begin();
       it2 != collector.end(); ++it2)
  {
    for (vector< Element_Collector* >::iterator it = it2->second.begin();
        it != it2->second.end(); ++it)
    {
      if ((*it)->consider(ll_upper, skel, tags, meta, users))
        return;
    }
  }
}


void Print_Target_Popup::print_item(uint32 ll_upper, const Area_Skeleton& skel,
		const vector< pair< string, string > >* tags,
		const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
		const map< uint32, string >* users, const Action& action)
{
  for (vector< pair< string, vector< Element_Collector* > > >::iterator it2 = collector.begin();
       it2 != collector.end(); ++it2)
  {
    for (vector< Element_Collector* >::iterator it = it2->second.begin();
        it != it2->second.end(); ++it)
    {
      if ((*it)->consider(ll_upper, skel, tags, meta, users))
        return;
    }
  }
}

void Print_Target_Popup::print_item_count(const Output_Item_Count& item_count)
{

}

string Print_Target_Popup::get_output() const
{
  string result;
  for (vector< pair< string, vector< Element_Collector* > > >::const_iterator it2 = collector.begin();
       it2 != collector.end(); ++it2)
  {
    bool empty = true;
    for (vector< Element_Collector* >::const_iterator it = it2->second.begin();
        it != it2->second.end(); ++it)
      empty &= (*it)->empty();
    if (!empty)
    {
      result += "\n<h2>" + it2->first + "</h2>\n\n";
      
      for (vector< Element_Collector* >::const_iterator it = it2->second.begin();
          it != it2->second.end(); ++it)
        result += (*it)->get_output();
    }
  }
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
      else if (dynamic_cast< Print_Target_Custom* >(print_target)
	  || dynamic_cast< Print_Target_Xml* >(print_target)
          || dynamic_cast< Print_Target_Csv* >(print_target))
      {
        delete print_target;
        print_target = 0;
        first_target = false;
      }
      else if (dynamic_cast< Print_Target_Json* >(print_target))
      {
        first_target = dynamic_cast< Print_Target_Json* >(print_target)->nothing_written();
        delete print_target;
        print_target = 0;
      }
    }
  }
  
  if (!print_target)
  {
    if (type == "xml")
      print_target = new Print_Target_Xml(mode, transaction);
    else if (type == "json")
      print_target = new Print_Target_Json(mode, transaction, first_target);
    else if (type == "csv")
      print_target = new Print_Target_Csv(mode, transaction, first_target, csv_settings);
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

    
void Output_Handle::print_bounds(double south, double west, double north, double east)
{
  if (type == "xml")
    cout<<"  <bounds minlat=\""<<south<<"\" minlon=\""<<west<<"\" "
          "maxlat=\""<<north<<"\" maxlon=\""<<east<<"\"/>\n\n";
  else if (type == "json")
    cout<<"  \"bounds\": {\n"
          "    \"minlat\": " << south << ",\n"
          "    \"minlon\": " << west  << ",\n"
          "    \"maxlat\": " << north << ",\n"
          "    \"maxlon\": " << east  << "\n"
          "  },\n";
}

void Output_Handle::print_elements_header()
{
  if (type == "json")
    cout<< "  \"elements\": [\n\n";
}


Output_Handle::~Output_Handle()
{
  if (print_target)
    delete print_target;
}
