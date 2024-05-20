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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PREPARE_EDIT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PREPARE_EDIT_H

#include "statement.h"

#include <map>
#include <string>


/* === The statement <em>prepare-edit</em> ===
*/

class Prepare_Edit : public Output_Statement
{
public:
  Prepare_Edit(int line_number_, const std::map< std::string, std::string >& attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "prepare-edit"; }
  virtual void execute(Resource_Manager& rman);
  virtual ~Prepare_Edit();
  static Generic_Statement_Maker< Prepare_Edit > statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<prepare-edit" + dump_xml_result_name() + " type=\"" + type + "\"/>\n"; }

  virtual std::string dump_compact_ql(const std::string& indent) const
  { return indent + "prepare_edit " + type + dump_ql_result_name() + ";"; }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  { return dump_compact_ql(indent); }

private:
  std::string type;
};


#endif
