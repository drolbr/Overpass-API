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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H


#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Set_Prop_Statement;


/* == The statement <em>make</em> ==

The statement <em>make</em> produces per execution one derived element.
The content of this output element is controlled by the parameters of the statement.

It is necessary to set a fixed type as type for the generated element.
After that, an arbitrary number of tags can be set.
As opposed to the <em>convert</em> statement, a generic key cannot be set.

Finally, it is possible to explicitly set the id of the generated object.
If you do not set an id then an unique id from a global ascending counter is assigned.

The base syntax is

  make <Type> <List of Tags>
  
where <List of Tags> is a comma separated list of items,
each of which must be one the following

  <Key> = <Evaluator>
  ::id = <Evaluator>
*/

class Make_Statement : public Output_Statement
{
public:
  Make_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "make"; }
  virtual void add_statement(Statement* statement, string text);
  virtual void execute(Resource_Manager& rman);
  virtual ~Make_Statement();
  static Generic_Statement_Maker< Make_Statement > statement_maker;

private:
  std::string type;
  std::vector< Set_Prop_Statement* > evaluators;
  Set_Prop_Statement* id_evaluator;
  Set_Prop_Statement* multi_evaluator;
};


#endif
