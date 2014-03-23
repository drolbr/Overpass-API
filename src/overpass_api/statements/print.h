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
#include "statement.h"

using namespace std;

class Print_Target
{
  public:
    typedef enum { KEEP, MODIFY_OLD, MODIFY_NEW, CREATE, DELETE } Action;
    
    Print_Target(uint32 mode_, Transaction& transaction);
    virtual ~Print_Target() {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP) = 0;
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP) = 0;
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP) = 0;
                            
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0, const Action& action = KEEP) = 0;

    static const unsigned int PRINT_IDS = 1;
    static const unsigned int PRINT_COORDS = 2;
    static const unsigned int PRINT_NDS = 4;
    static const unsigned int PRINT_MEMBERS = 8;
    static const unsigned int PRINT_TAGS = 0x10;
    static const unsigned int PRINT_VERSION = 0x20;
    static const unsigned int PRINT_META = 0x40;
    static const unsigned int PRINT_GEOMETRY = 0x80;
    static const unsigned int PRINT_BOUNDS = 0x100;
    static const unsigned int PRINT_CENTER = 0x200;

  protected:
    uint32 mode;
    map< uint32, string > roles;
};


class Collection_Print_Target;


class Relation_Geometry_Store
{
public:
  Relation_Geometry_Store
      (const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
      const Statement& query, Resource_Manager& rman,
      double south_, double north_, double west_, double east_);
  Relation_Geometry_Store
      (const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& relations, uint64 timestamp,
      const Statement& query, Resource_Manager& rman,
      double south_, double north_, double west_, double east_);
      
  ~Relation_Geometry_Store();
  
  // return the empty vector if the relation is not found
  std::vector< std::vector< Quad_Coord > > get_geometry(const Relation_Skeleton& relation) const;
  
private:
  std::vector< Node > nodes;
  std::vector< Way_Skeleton > ways;
  Way_Geometry_Store* way_geometry_store;
  
  uint32 south;
  uint32 north;
  int32 west;
  int32 east;
  
  bool matches_bbox(uint32 ll_upper, uint32 ll_lower) const;
};


class Way_Bbox_Geometry_Store : public Way_Geometry_Store
{
public:
  Way_Bbox_Geometry_Store(const map< Uint31_Index, vector< Way_Skeleton > >& ways,
                     const Statement& query, Resource_Manager& rman,
                     double south_, double north_, double west_, double east_);
  Way_Bbox_Geometry_Store(const map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
                     const Statement& query, Resource_Manager& rman,
                     double south_, double north_, double west_, double east_);
  
  // return the empty vector if the way is not found
  vector< Quad_Coord > get_geometry(const Way_Skeleton& way) const;
  
private:
  uint32 south;
  uint32 north;
  int32 west;
  int32 east;
  
  bool matches_bbox(uint32 ll_upper, uint32 ll_lower) const;
};


class Output_Handle;


class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_, const map< string, string >& attributes,
                    Query_Constraint* bbox_limitation = 0);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Print_Statement();

    static Generic_Statement_Maker< Print_Statement > statement_maker;
    
    void set_output_handle(Output_Handle* output_handle_) { output_handle = output_handle_; }
      
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
    
    void set_collect_lhs();
    void set_collect_rhs();
    
  private:
    string input;
    unsigned int mode;
    enum { order_by_id, order_by_quadtile } order;
    unsigned int limit;
    Output_Handle* output_handle;
    Way_Bbox_Geometry_Store* way_geometry_store;
    Way_Bbox_Geometry_Store* attic_way_geometry_store;
    Relation_Geometry_Store* relation_geometry_store;
    Relation_Geometry_Store* attic_relation_geometry_store;
    Collection_Print_Target* collection_print_target;
    enum { dont_collect, collect_lhs, collect_rhs } collection_mode;
    
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
    
    template< class TIndex, class TObject >
    void tags_by_id_attic
      (const map< TIndex, vector< Attic< TObject > > >& items,
       uint32 FLUSH_SIZE, Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* current_meta_file_prop, const File_Properties* attic_meta_file_prop,
       uint32& element_count);
};

#endif
