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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__TESTING_TOOLS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__TESTING_TOOLS_H


#include "../core/parsed_query.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


struct Attr
{
  Attr& operator()(const std::string& k, const std::string& v)
  {
      kvs_[k] = v;
      return *this;
  }

  const std::map< std::string, std::string >& kvs() { return kvs_; }

private:
  std::map< std::string, std::string > kvs_;
};


struct Statement_Container
{
  Statement_Container(Parsed_Query& global_settings) : global_settings_(&global_settings) {}
  ~Statement_Container()
  {
    for (std::vector< Statement* >::iterator it = cont.begin(); it != cont.end(); ++it)
      delete *it;
  }

  Statement* add_stmt(Statement* stmt, Statement* parent);
  Parsed_Query& global_settings() { return *global_settings_; }

  template< typename NewStatement >
  Statement* create_stmt(const std::map< std::string, std::string >& attributes, Statement* parent)
  {
    return add_stmt(new NewStatement(0, attributes, *global_settings_), parent);
  }

private:
  Parsed_Query* global_settings_;
  std::vector< Statement* > cont;
};


#endif
