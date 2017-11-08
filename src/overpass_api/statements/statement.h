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
#include "../core/parsed_query.h"
#include "../core/settings.h"
#include "../data/diff_set.h"
#include "../dispatch/resource_manager.h"
#include "../frontend/tokenizer_utils.h"
#include "../osm-backend/area_updater.h"


class Query_Constraint
{
  public:
    virtual bool delivers_data(Resource_Manager& rman) = 0;

    virtual bool collect_nodes(Resource_Manager& rman, Set& into,
			 const std::vector< Uint64 >& ids, bool invert_ids) { return false; }
    virtual bool collect(Resource_Manager& rman, Set& into,
			 int type, const std::vector< Uint32_Index >& ids, bool invert_ids) { return false; }
			
    virtual bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
      { return false; }
    virtual bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges)
      { return false; }

    virtual bool get_node_ids
        (Resource_Manager& rman, std::vector< Node_Skeleton::Id_Type >& ids)
      { return false; }
    virtual bool get_way_ids
        (Resource_Manager& rman, std::vector< Way_Skeleton::Id_Type >& ids)
      { return false; }
    virtual bool get_relation_ids
        (Resource_Manager& rman, std::vector< Relation_Skeleton::Id_Type >& ids)
      { return false; }

    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
			  const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
			  const std::vector< Node::Id_Type >& ids,
                          bool invert_ids)
      { return false; }
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
			  const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
			  int type,
                          const std::vector< Uint32_Index >& ids,
                          bool invert_ids)
      { return false; }

    // Cheap filter. No health_check in between needed and should be called first.
    virtual void filter(Resource_Manager& rman, Set& into) {}

    // Expensive filter. Health_check may be needed in between. These are called last
    // to minimize the number of elements that need to be processed.
    virtual void filter(const Statement& query, Resource_Manager& rman, Set& into) {}

    virtual ~Query_Constraint() {}
};

/**
 * The base class for all statements
 */
class Statement
{
  public:
    enum QL_Context { generic, in_convert, evaluator_expected, elem_eval_possible };

    struct Factory
    {
      Factory(Parsed_Query& global_settings_) : error_output_(error_output), global_settings(global_settings_) {}
      ~Factory();

      Statement* create_statement(std::string element, int line_number,
				  const std::map< std::string, std::string >& attributes);
      Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context);
      Statement* create_criterion(const Token_Node_Ptr& tree_it,
                                  const std::string& type, bool& can_standalone, const std::string& into);

      std::vector< Statement* > created_statements;
      Error_Output* error_output_;
      Parsed_Query& global_settings;
    };

    struct Statement_Maker
    {
      virtual Statement* create_statement
          (int line_number, const std::map< std::string, std::string >& attributes, Parsed_Query& global_settings) = 0;
      virtual ~Statement_Maker() {}
    };

    struct Criterion_Maker
    {
      virtual bool can_standalone(const std::string& type) = 0;
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output) = 0;
      virtual ~Criterion_Maker() {}
    };

    struct Evaluator_Maker
    {
      virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output) = 0;
      virtual ~Evaluator_Maker() {}
    };

    static std::map< std::string, Statement_Maker* >& maker_by_name();
    static std::map< std::string, Criterion_Maker* >& maker_by_ql_criterion();
    static std::map< std::string, std::vector< Evaluator_Maker* > >& maker_by_token();
    static std::map< std::string, std::vector< Evaluator_Maker* > >& maker_by_func_name();

    Statement(int line_number_) : line_number(line_number_), progress(0) {}

    virtual void add_statement(Statement* statement, std::string text);
    virtual void add_final_text(std::string text);
    virtual std::string get_name() const = 0;
    virtual std::string get_result_name() const = 0;
    virtual void execute(Resource_Manager& rman) = 0;

    // May return 0. The ownership of the Query_Constraint remains at the called
    // object.
    virtual Query_Constraint* get_query_constraint() { return 0; }

    virtual void set_collect_lhs() {}
    virtual void set_collect_rhs(bool add_deletion_information) {}

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

    virtual std::string dump_xml(const std::string&) const { return ""; }
    virtual std::string dump_compact_ql(const std::string&) const { return ""; }
    virtual std::string dump_pretty_ql(const std::string&) const { return ""; }
    virtual std::string dump_ql_in_query(const std::string& indent) const { return dump_compact_ql(indent); }

    static void set_error_output(Error_Output* error_output_)
    {
      error_output = error_output_;
    }

    void runtime_error(std::string error) const;
    void runtime_remark(std::string error) const;

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
        (std::string element, std::map< std::string, std::string >& attributes,
	 const std::map< std::string, std::string >& input);
    void assure_no_text(std::string text, std::string name);
    void substatement_error(std::string parent, Statement* child);

    void add_static_error(std::string error);
    void add_static_remark(std::string remark);

    void set_progress(int progress_) { progress = progress_; }
};


std::map< std::string, std::string > convert_c_pairs(const char** attr);


template< class TStatement >
class Generic_Statement_Maker : public Statement::Statement_Maker
{
  public:
    virtual Statement* create_statement
        (int line_number, const std::map< std::string, std::string >& attributes, Parsed_Query& global_settings)
    {
      return new TStatement(line_number, attributes, global_settings);
    }

    Generic_Statement_Maker(const std::string& name) { Statement::maker_by_name()[name] = this; }
    virtual ~Generic_Statement_Maker() {}
};


class Output_Statement : public Statement
{
  public:
    Output_Statement(int line_number) : Statement(line_number), output("_") {}

    virtual std::string get_result_name() const { return output; }

    std::string dump_ql_result_name() const { return output != "_" ? std::string("->.") + output : ""; }
    std::string dump_xml_result_name() const { return output != "_" ? std::string(" into=\"") + output + "\"" : ""; }

  protected:
    void set_output(std::string output_) { output = output_; }

    void transfer_output(Resource_Manager& rman, Set& into) const;
    void transfer_output(Resource_Manager& rman, Diff_Set& into) const;

  private:
    std::string output;
};


#endif
