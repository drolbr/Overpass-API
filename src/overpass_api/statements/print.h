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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H

#include <map>
#include <string>
#include <vector>
#include "../data/collect_members.h"
#include "../frontend/output_handler.h"
#include "statement.h"


struct Output_Item_Count
{
  uint32 nodes;
  uint32 ways;
  uint32 relations;
  uint32 areas;
  uint32 total;
};


class Print_Target;
class Collection_Print_Target;

class Relation_Geometry_Store;
class Way_Bbox_Geometry_Store;

class Output_Handle;


class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_, const map< string, string >& attributes, Parsed_Query& global_settings);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Print_Statement();

    static Generic_Statement_Maker< Print_Statement > statement_maker;
      
    void print_item(Print_Target& target, uint32 ll_upper, const Node_Skeleton& skel,
                    const vector< pair< string, string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta = 0,
                    const map< uint32, string >* users = 0);
    
    void print_item(Print_Target& target, uint32 ll_upper, const Way_Skeleton& skel,
                    const vector< pair< string, string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0,
                    const map< uint32, string >* users = 0);
    void print_item(Print_Target& target, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const vector< pair< string, string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0,
                    const map< uint32, string >* users = 0);
    
    void print_item(Print_Target& target, uint32 ll_upper, const Relation_Skeleton& skel,
                    const vector< pair< string, string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0,
                    const map< uint32, string >* users = 0);
    void print_item(Print_Target& target, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const vector< pair< string, string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0,
                    const map< uint32, string >* users = 0);
    
    void print_item(Print_Target& target, uint32 ll_upper, const Area_Skeleton& skel,
                    const vector< pair< string, string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Area_Skeleton::Id_Type >* meta = 0,
                    const map< uint32, string >* users = 0);

    void print_item_count(Print_Target& target, const Output_Item_Count & item_count);
    
    void set_collect_lhs();
    void set_collect_rhs(bool add_deletion_information);
    
  private:
    string input;
    unsigned int mode;
    enum { order_by_id, order_by_quadtile } order;
    unsigned int limit;
    Way_Bbox_Geometry_Store* way_geometry_store;
    Way_Bbox_Geometry_Store* attic_way_geometry_store;
    Relation_Geometry_Store* relation_geometry_store;
    Relation_Geometry_Store* attic_relation_geometry_store;
    Collection_Print_Target* collection_print_target;
    enum { dont_collect, collect_lhs, collect_rhs } collection_mode;
    bool add_deletion_information;
    
    double south;
    double north;
    double west;
    double east;

    template< class Index, class Object >
    void tags_quadtile
      (const map< Index, vector< Object > >& items,
       const File_Properties& file_prop, Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* meta_file_prop, uint32& element_count);
    
    template< class Index, class Object >
    void tags_quadtile_attic
      (const map< Index, vector< Attic< Object > > >& items,
       Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* current_meta_file_prop, const File_Properties* attic_meta_file_prop,
       uint32& element_count);
    
    template< class TIndex, class TObject >
    void tags_by_id
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop,
       uint32 FLUSH_SIZE, Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* meta_file_prop, uint32& element_count);
    
    template< class Index, class Object >
    void tags_by_id_attic
      (const map< Index, vector< Object > >& current_items,
       const map< Index, vector< Attic< Object > > >& attic_items,
       uint32 FLUSH_SIZE, Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* current_meta_file_prop, const File_Properties* attic_meta_file_prop,
       uint32& element_count);
};

#endif
