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

#include "output_handler.h"


const std::string& member_type_name(uint32 type)
{
  static std::vector< std::string > MEMBER_TYPE;
  if (MEMBER_TYPE.empty())
  {
    MEMBER_TYPE.push_back("invalid");
    MEMBER_TYPE.push_back("node");
    MEMBER_TYPE.push_back("way");
    MEMBER_TYPE.push_back("relation");
  }

  if (type < MEMBER_TYPE.size())
    return MEMBER_TYPE[type];
  else
    return MEMBER_TYPE[0];
}
