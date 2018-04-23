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

#include "output_handler_parser.h"


Output_Handler_Parser::Output_Handler_Parser(const std::string& format_name)
{
  registry()[format_name] = this;
}


Output_Handler_Parser* Output_Handler_Parser::get_format_parser(const std::string& format_name)
{
  std::map< std::string, Output_Handler_Parser* >::iterator
      it = registry().find(format_name);
  if (it == registry().end())
    return 0;
  else
    return it->second;
}


std::map< std::string, Output_Handler_Parser* >& Output_Handler_Parser::registry()
{
  static std::map< std::string, Output_Handler_Parser* > singleton;
  return singleton;
}
