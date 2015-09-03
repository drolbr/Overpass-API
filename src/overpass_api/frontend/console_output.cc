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
#include "../../template_db/transaction.h"


void Console_Output::add_encoding_error(const std::string& error)
{
  if (log_level != Error_Output::QUIET)
    std::cerr<<"encoding error: "<<error<<'\n';
  encoding_errors = true;
}


void Console_Output::add_parse_error(const std::string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
    std::cerr<<"line "<<line_number<<": parse error: "<<error<<'\n';
  parse_errors = true;
}


void Console_Output::add_static_error(const std::string& error, int line_number)
{
  if (log_level != Error_Output::QUIET)
    std::cerr<<"line "<<line_number<<": static error: "<<error<<'\n';
  static_errors = true;
}


void Console_Output::add_encoding_remark(const std::string& error)
{
  if (log_level >= Error_Output::ASSISTING)
    std::cerr<<"encoding remark: "<<error<<'\n';
}


void Console_Output::add_parse_remark(const std::string& error, int line_number)
{
  if (log_level >= Error_Output::ASSISTING)
    std::cerr<<"line "<<line_number<<": parse remark: "<<error<<'\n';
}


void Console_Output::add_static_remark(const std::string& error, int line_number)
{
  if (log_level >= Error_Output::ASSISTING)
    std::cerr<<"line "<<line_number<<": static remark: "<<error<<'\n';
}


void Console_Output::runtime_error(const std::string& error)
{
  if (log_level != Error_Output::QUIET)
    std::cerr<<"runtime error: "<<error<<'\n';
}


void Console_Output::runtime_remark(const std::string& error)
{
  if (log_level >= Error_Output::ASSISTING)
    std::cerr<<"runtime remark: "<<error<<'\n';
}


void Console_Output::display_statement_progress
    (uint timer, const std::string& name, int progress, int line_number,
     const std::vector< std::pair< uint, uint > >& stack)
{
  if (log_level < Error_Output::PROGRESS)
    return;
  std::cerr<<"After "<<timer/3600<<"h"<<timer/60%60<<"m"<<timer%60<<"s: "
      <<"in \""<<name<<"\", part "<<progress<<", on line "<<line_number<<".";
  if (!stack.empty())
  {
    std::cerr<<" Stack:";
    for (std::vector< std::pair< uint, uint > >::const_iterator it(stack.begin());
        it != stack.end(); ++it)
      std::cerr<<" "<<it->first<<" of "<<it->second;
  }
  std::cerr<<'\n';
}


void Console_Output::dump_cache_statistics(Transaction& transaction)
{
  if (log_level < Error_Output::PROGRESS)
    return;
  
  std::cerr<<"Total read: "<<transaction.size_total_requested()<<" bytes in "
      <<transaction.num_total_requested()<<" blocks.\n";
  std::cerr<<"Read from disk: "<<transaction.size_read_from_disk()<<" bytes in "
      <<transaction.num_read_from_disk()<<" blocks.\n";
  std::cerr<<"Cached: "<<transaction.size_cached()<<" bytes in "<<transaction.num_cached()<<" blocks.\n";
}
