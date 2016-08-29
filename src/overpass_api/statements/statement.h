/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_H

#include <map>
#include <set>
#include <vector>

#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../dispatch/resource_manager.h"
#include "../osm-backend/area_updater.h"

using namespace std;

class Query_Constraint
{
  public:
    virtual bool delivers_data(Resource_Manager& rman) = 0;
    
    virtual bool collect_nodes(Resource_Manager& rman, Set& into,
			 const vector< Uint64 >& ids, bool invert_ids) { return false; }
    virtual bool collect(Resource_Manager& rman, Set& into,
			 int type, const vector< Uint32_Index >& ids, bool invert_ids) { return false; }
			 
    virtual bool get_ranges
        (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
      { return false; }
    virtual bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
      { return false; }
      
    virtual bool get_node_ids
        (Resource_Manager& rman, vector< Node_Skeleton::Id_Type >& ids)
      { return false; }
    virtual bool get_way_ids
        (Resource_Manager& rman, vector< Way_Skeleton::Id_Type >& ids)
      { return false; }
    virtual bool get_relation_ids
        (Resource_Manager& rman, vector< Relation_Skeleton::Id_Type >& ids)
      { return false; }
      
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
			  const set< pair< Uint32_Index, Uint32_Index > >& ranges,
			  const vector< Node::Id_Type >& ids,
                          bool invert_ids, uint64 timestamp)
      { return false; }
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
			  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
			  int type,
                          const vector< Uint32_Index >& ids,
                          bool invert_ids, uint64 timestamp)
      { return false; }
    
    // Cheap filter. No health_check in between needed and should be called first.
    virtual void filter(Resource_Manager& rman, Set& into, uint64 timestamp) {}

    // Expensive filter. Health_check may be needed in between. These are called last
    // to minimize the number of elements that need to be processed.
    virtual void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp) {}
    
    virtual ~Query_Constraint() {}
};

/**
 * The base class for all statements
 */
class Statement
{
  public:
    struct Factory
    {
      Factory() : error_output_(error_output), bbox_limitation(0) {}
      ~Factory();
      
      Statement* create_statement(string element, int line_number,
				  const map< string, string >& attributes);
      
      vector< Statement* > created_statements;
      Error_Output* error_output_;
      Query_Constraint* bbox_limitation;
    };
    
    class Statement_Maker
    {
      public:
	virtual Statement* create_statement
	    (int line_number, const map< string, string >& attributes, Query_Constraint* bbox_limitation) = 0;
	virtual ~Statement_Maker() {}
    };
    
    static map< string, Statement_Maker* >& maker_by_name();
    
    Statement(int line_number_) : line_number(line_number_), progress(0) {}
    
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() const = 0;
    virtual string get_result_name() const = 0;
    virtual void execute(Resource_Manager& rman) = 0;

    // May return 0. The ownership of the Query_Constraint remains at the called
    // object.
    virtual Query_Constraint* get_query_constraint() { return 0; }
    
    virtual ~Statement() {}

    int get_progress() const { return progress; }
    int get_line_number() const { return line_number; }
    int get_startpos() const { return startpos; }
    void set_startpos(int pos) { startpos = pos; }
    int get_endpos() const { return endpos; }
    void set_endpos(int pos) { endpos = pos; }
    int get_tagendpos() const { return tagendpos; }
    void set_tagendpos(int pos) { tagendpos = pos; }
    
    void display_full();
    void display_starttag();
        
    static void set_error_output(Error_Output* error_output_)
    {
      error_output = error_output_;
    }

    void runtime_remark(string error) const;

    const static int NODE = 1;
    const static int WAY = 2;
    const static int RELATION = 3;
    const static int AREA = 4;
    
  private:
    static Error_Output* error_output;
    
    int line_number;
    int startpos, endpos, tagendpos;
    int progress;
    
  protected:
    void eval_attributes_array
        (string element, map< string, string >& attributes,
	 const map< string, string >& input);
    void assure_no_text(string text, string name);
    void substatement_error(string parent, Statement* child);
    
    void add_static_error(string error);
    void add_static_remark(string remark);

    void set_progress(int progress_) { progress = progress_; }
};


map< string, string > convert_c_pairs(const char** attr);


template< class TStatement >
class Generic_Statement_Maker : public Statement::Statement_Maker
{
  public:
    virtual Statement* create_statement
        (int line_number, const map< string, string >& attributes, Query_Constraint* bbox_limitation)
    {
      return new TStatement(line_number, attributes, bbox_limitation);
    }
    
    Generic_Statement_Maker(const string& name) { Statement::maker_by_name()[name] = this; }
    virtual ~Generic_Statement_Maker() {}
};


class Output_Statement : public Statement
{
  public:
    Output_Statement(int line_number) : Statement(line_number), output("_") {}
    
    virtual string get_result_name() const { return output; }
    
  protected:
    void set_output(std::string output_) { output = output_; }

    void transfer_output(Resource_Manager& rman, Set& into) const;
  
  private:
    string output;
};


#endif
