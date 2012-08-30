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

#include <iomanip>
#include <iostream>
#include <string>

#include "console_output.h"

using namespace std;

void Console_Output::add_encoding_error(const string& error)
{
  if (log_level != Error_Output::QUIET)
    cerr<<"encoding error: "<<error<<'\n';
  encoding_errors = true;
}

void Console_Output::add_parse_error(const string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
    cerr<<"line "<<line_number<<": parse error: "<<error<<'\n';
  parse_errors = true;
}

void Console_Output::add_static_error(const string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
    cerr<<"line "<<line_number<<": static error: "<<error<<'\n';
  static_errors = true;
}

void Console_Output::add_encoding_remark(const string& error)
{
  if (log_level >= Error_Output::ASSISTING)
    cerr<<"encoding remark: "<<error<<'\n';
}

void Console_Output::add_parse_remark(const string& error, int line_number)
{
  if (log_level >= Error_Output::ASSISTING)
    cerr<<"line "<<line_number<<": parse remark: "<<error<<'\n';
}

void Console_Output::add_static_remark(const string& error, int line_number)
{
  if (log_level >= Error_Output::ASSISTING)
    cerr<<"line "<<line_number<<": static remark: "<<error<<'\n';
}

void Console_Output::runtime_error(const string& error)
{
  if (log_level != Error_Output::QUIET)
    cerr<<"runtime error: "<<error<<'\n';
}

void Console_Output::runtime_remark(const string& error)
{
  if (log_level >= Error_Output::ASSISTING)
    cerr<<"runtime remark: "<<error<<'\n';
}

void Console_Output::display_statement_progress
    (uint timer, const string& name, int progress, int line_number,
     const vector< pair< uint, uint > >& stack)
{
  if (log_level < Error_Output::PROGRESS)
    return;
  cerr<<"After "<<timer/3600<<"h"<<timer/60%60<<"m"<<timer%60<<"s: "
      <<"in \""<<name<<"\", part "<<progress<<", on line "<<line_number<<".";
  if (!stack.empty())
  {
    cerr<<" Stack:";
    for (vector< pair< uint, uint > >::const_iterator it(stack.begin());
        it != stack.end(); ++it)
      cerr<<" "<<it->first<<" of "<<it->second;
  }
  cerr<<'\n';
}
