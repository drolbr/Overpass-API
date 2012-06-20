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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Print_Target
{
  public:
    Print_Target(uint32 mode_, Transaction& transaction);
    virtual ~Print_Target() {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) = 0;
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) = 0;
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) = 0;
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) = 0;

    static const unsigned int PRINT_IDS = 1;
    static const unsigned int PRINT_COORDS = 2;
    static const unsigned int PRINT_NDS = 4;
    static const unsigned int PRINT_MEMBERS = 8;
    static const unsigned int PRINT_TAGS = 16;
    static const unsigned int PRINT_VERSION = 32;
    static const unsigned int PRINT_META = 64;
			    
  protected:
    uint32 mode;
    map< uint32, string > roles;
};


class Output_Handle;

class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Print_Statement();

    static Generic_Statement_Maker< Print_Statement > statement_maker;
    
    void set_output_handle(Output_Handle* output_handle_) { output_handle = output_handle_; }
    
  private:
    string input;
    unsigned int mode;
    enum { order_by_id, order_by_quadtile } order;
    unsigned int limit;
    Output_Handle* output_handle;

    template< class TIndex, class TObject >
    void tags_quadtile
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop, Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* meta_file_prop = 0, uint32& element_count = 0);
    
    template< class TIndex, class TObject >
    void tags_by_id
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop,
       uint32 FLUSH_SIZE, Print_Target& target,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* meta_file_prop = 0, uint32& element_count = 0);
};

#endif
