/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#include "resource_manager.h"
#include "scripting_core.h"
#include "../core/parsed_query.h"
#include "../core/settings.h"
#include "../frontend/console_output.h"
#include "../frontend/map_ql_parser.h"
#include "../frontend/user_interface.h"
#include "../statements/area_query.h"
#include "../statements/coord_query.h"
#include "../statements/id_query.h"
#include "../statements/make_area.h"
#include "../statements/map_to_area.h"
#include "../statements/osm_script.h"
#include "../statements/query.h"
#include "../statements/statement.h"
#include "../statements/statement_dump.h"
#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../../template_db/types.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


namespace
{
  std::vector< Statement* > statement_stack_;
  std::vector< Statement_Dump* > statement_dump_stack_;
  std::vector< std::string > text_stack;
  Script_Parser xml_parser;

  template< class TStatement >
  std::vector< TStatement* >& statement_stack();

  template< >
  std::vector< Statement* >& statement_stack< Statement >() { return statement_stack_; }

  template< >
  std::vector< Statement_Dump* >& statement_stack< Statement_Dump >() { return statement_dump_stack_; }
}


int determine_area_level(Error_Output* error_output, int area_level)
{
  if ((area_level < 2) && (Make_Area_Statement::is_used()))
  {
    if (error_output)
    {
      error_output->runtime_error
          ("Specify --rules to execute a rule. make-area can only appear in rules.");
      throw Exit_Error();
    }
  }
  if ((area_level == 0) &&
      (Coord_Query_Statement::is_used() || Area_Query_Statement::is_used() ||
       Map_To_Area_Statement::is_used() ||
       Query_Statement::area_query_exists() || Id_Query_Statement::area_query_exists()))
    area_level = 1;

  return area_level;
}


Statement::Factory* stmt_factory_global = 0;
Statement_Dump::Factory* stmt_dump_factory_global = 0;

Statement::Factory* get_factory(Statement::Factory*)
{
  return stmt_factory_global;
}

Statement_Dump::Factory* get_factory(Statement_Dump::Factory*)
{
  return stmt_dump_factory_global;
}

template< class TStatement >
void start(const char *el, const char **attr)
{
  TStatement* statement(get_factory((typename TStatement::Factory*)(0))->create_statement
      (el, xml_parser.current_line_number(), convert_c_pairs(attr)));

  statement_stack< TStatement >().push_back(statement);
  text_stack.push_back(xml_parser.get_parsed_text());
  xml_parser.reset_parsed_text();
}

template< class TStatement >
void end(const char *el)
{
  if (statement_stack< TStatement >().size() > 1)
  {
    TStatement* statement(statement_stack< TStatement >().back());

    if (statement)
    {
      statement->add_final_text(xml_parser.get_parsed_text());
      xml_parser.reset_parsed_text();
/*      statement->set_endpos(get_tag_end());*/
    }

    statement_stack< TStatement >().pop_back();
    if (statement_stack< TStatement >().back() && statement)
      statement_stack< TStatement >().back()->add_statement(statement, text_stack.back());
    text_stack.pop_back();
  }
  else if ((statement_stack< TStatement >().size() == 1) &&
      (statement_stack< TStatement >().front()))
    statement_stack< TStatement >().front()->add_final_text(xml_parser.get_parsed_text());
}

bool parse_and_validate
    (Statement::Factory& stmt_factory, Parsed_Query& parsed_query,
     const std::string& xml_raw, Error_Output* error_output, Debug_Level debug_level)
{
  if (error_output && error_output->display_encoding_errors())
    return false;

  unsigned int pos(0);
  while (pos < xml_raw.size() && isspace(xml_raw[pos]))
    ++pos;

  if (pos < xml_raw.size() && xml_raw[pos] == '<')
  {
    try
    {
      if (debug_level == parser_dump_xml || debug_level == parser_dump_compact_map_ql
	  || debug_level == parser_dump_pretty_map_ql || debug_level == parser_dump_bbox_map_ql)
      {
	Statement_Dump::Factory stmt_dump_factory(stmt_factory);
	stmt_dump_factory_global = &stmt_dump_factory;
	xml_parser.parse(xml_raw, start< Statement_Dump >, end< Statement_Dump >);

        for (std::vector< Statement_Dump* >::const_iterator it =
	    statement_stack< Statement_Dump >().begin();
            it != statement_stack< Statement_Dump >().end(); ++it)
	{
	  if (debug_level == parser_dump_xml)
            std::cout<<(*it)->dump_xml();
	  else if (debug_level == parser_dump_compact_map_ql)
	    std::cout<<(*it)->dump_compact_map_ql(stmt_factory);
	  else if (debug_level == parser_dump_bbox_map_ql)
	    std::cout<<(*it)->dump_bbox_map_ql(stmt_factory);
	  else if (debug_level == parser_dump_pretty_map_ql)
	    std::cout<<(*it)->dump_pretty_map_ql(stmt_factory);
	}
        for (std::vector< Statement_Dump* >::iterator it = statement_stack< Statement_Dump >().begin();
            it != statement_stack< Statement_Dump >().end(); ++it)
          delete *it;
      }
      else
      {
	stmt_factory_global = &stmt_factory;
        xml_parser.parse(xml_raw, start< Statement >, end< Statement >);
	Osm_Script_Statement* root =
	    get_statement_stack()->empty() ? 0 :
	        dynamic_cast< Osm_Script_Statement* >(get_statement_stack()->front());
	if (root)
	  root->set_factory(&stmt_factory);
	stmt_factory_global = 0;
      }
    }
    catch(Parse_Error parse_error)
    {
      if (error_output)
        error_output->add_parse_error(parse_error.message,
				      xml_parser.current_line_number());
    }
    catch(File_Error e)
    {
      std::ostringstream temp;
      temp<<"open: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
      if (error_output)
        error_output->runtime_error(temp.str());

      return false;
    }
  }
  else
  {
    if (debug_level == parser_execute)
    {
      parse_and_validate_map_ql(stmt_factory, xml_raw, error_output, parsed_query);
      Osm_Script_Statement* root =
          dynamic_cast< Osm_Script_Statement* >(get_statement_stack()->front());
      if (root)
	root->set_factory(&stmt_factory);
    }
    else if (debug_level == parser_dump_xml)
      parse_and_dump_xml_from_map_ql(stmt_factory, xml_raw, error_output, parsed_query);
    else if (debug_level == parser_dump_compact_map_ql)
      parse_and_dump_compact_from_map_ql(stmt_factory, xml_raw, error_output, parsed_query);
    else if (debug_level == parser_dump_bbox_map_ql)
      parse_and_dump_bbox_from_map_ql(stmt_factory, xml_raw, error_output, parsed_query);
    else if (debug_level == parser_dump_pretty_map_ql)
      parse_and_dump_pretty_from_map_ql(stmt_factory, xml_raw, error_output, parsed_query);
  }

  if ((error_output) && (error_output->display_parse_errors()))
  {
    return false;
  }
  if ((error_output) && (error_output->display_static_errors()))
  {
    return false;
  }

  return true;
}

std::vector< Statement* >* get_statement_stack()
{
  return &statement_stack_;
}
