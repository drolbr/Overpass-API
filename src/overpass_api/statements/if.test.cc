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

#include <iostream>
#include <sstream>

#include "aggregators.h"
#include "binary_operators.h"
#include "id_query.h"
#include "if.h"
#include "print.h"
#include "tag_value.h"
#include "testing_tools.h"
#include "unary_operators.h"


template< typename T >
std::string to_string_(T val)
{
  std::ostringstream buf;
  buf<<val;
  return buf.str();
}


void execute_base_test_case(const std::map< std::string, std::string >& id_attributes, Statement& condition,
    Resource_Manager& rman, Parsed_Query& global_settings, bool add_else)
{
  Statement_Container stmt_cont(global_settings);

  Id_Query_Statement(0, id_attributes, global_settings).execute(rman);
  {
    If_Statement stmt(0, Attr().kvs(), global_settings);
    stmt.add_statement(&condition, "");
    stmt_cont.add_stmt(new Id_Query_Statement(0, Attr()("type", "way")("ref", "2").kvs(), global_settings), &stmt);

    if (add_else)
    {
      stmt_cont.add_stmt(new Else_Statement(0, Attr().kvs(), global_settings), &stmt);
      stmt_cont.add_stmt(new Id_Query_Statement(0, Attr()("type", "way")("ref", "4").kvs(), global_settings),
          &stmt);
    }
    stmt.execute(rman);
  }
  Print_Statement(0, Attr().kvs(), global_settings).execute(rman);
}


int main(int argc, char* args[])
{
  if (argc < 4)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute db_dir node_id_offset\n";
    return 0;
  }
  std::string test_to_execute = args[1];
  Parsed_Query global_settings;
  global_settings.set_output_handler(Output_Handler_Parser::get_format_parser("xml"), 0, 0);

  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction, &global_settings);
      Statement_Container stmt_cont(global_settings);

      Evaluator_Min_Value stmt0(0, Attr().kvs(), global_settings);
      stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), &stmt0);

      execute_base_test_case(Attr()("type", "way")("ref", "1").kvs(), stmt0, rman, global_settings, false);
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction, &global_settings);
      Statement_Container stmt_cont(global_settings);

      Evaluator_Not_Equal stmt0(0, Attr().kvs(), global_settings);

      Statement* stmt01 =
          stmt_cont.add_stmt(new Evaluator_Min_Value(0, Attr().kvs(), global_settings), &stmt0);
      stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), stmt01);
      stmt_cont.add_stmt(new Evaluator_Fixed(0, Attr()("v", "1").kvs(), global_settings), &stmt0);

      execute_base_test_case(Attr()("type", "way")("ref", "1").kvs(), stmt0, rman, global_settings, false);
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction, &global_settings);
      Statement_Container stmt_cont(global_settings);

      Evaluator_Min_Value stmt0(0, Attr()("from", "a").kvs(), global_settings);
      stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), &stmt0);

      execute_base_test_case(Attr()("type", "way")("ref", "1")("into", "a").kvs(), stmt0,
          rman, global_settings, false);
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction, &global_settings);
      Statement_Container stmt_cont(global_settings);

      Id_Query_Statement(0, Attr()("type", "way")("ref", "3").kvs(), global_settings).execute(rman);

      Evaluator_Not_Equal stmt0(0, Attr().kvs(), global_settings);
      Statement* stmt01 =
          stmt_cont.add_stmt(new Evaluator_Min_Value(0, Attr()("from", "a").kvs(), global_settings), &stmt0);
      stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), stmt01);
      stmt_cont.add_stmt(new Evaluator_Fixed(0, Attr()("v", "1").kvs(), global_settings), &stmt0);

      execute_base_test_case(Attr()("type", "way")("ref", "1")("into", "a").kvs(), stmt0,
          rman, global_settings, false);
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction, &global_settings);
      Statement_Container stmt_cont(global_settings);

      Evaluator_Min_Value stmt0(0, Attr().kvs(), global_settings);
      stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), &stmt0);

      execute_base_test_case(Attr()("type", "way")("ref", "1").kvs(), stmt0, rman, global_settings, true);
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    try
    {
      Nonsynced_Transaction transaction(false, false, args[2], "");
      Resource_Manager rman(transaction, &global_settings);
      Statement_Container stmt_cont(global_settings);

      Evaluator_Not_Equal stmt0(0, Attr().kvs(), global_settings);

      Statement* stmt01 =
          stmt_cont.add_stmt(new Evaluator_Min_Value(0, Attr().kvs(), global_settings), &stmt0);
      stmt_cont.add_stmt(new Evaluator_Id(0, Attr().kvs(), global_settings), stmt01);
      stmt_cont.add_stmt(new Evaluator_Fixed(0, Attr()("v", "1").kvs(), global_settings), &stmt0);

      execute_base_test_case(Attr()("type", "way")("ref", "1").kvs(), stmt0, rman, global_settings, true);
    }
    catch (File_Error e)
    {
      std::cerr<<"File error caught: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  std::cout<<"</osm>\n";
  return 0;
}
