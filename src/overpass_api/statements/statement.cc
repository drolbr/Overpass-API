/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "statement.h"

using namespace std;

map< string, Statement::Statement_Maker* >& Statement::maker_by_name()
{
  static map< string, Statement::Statement_Maker* > makers;
  return makers;
}

void Statement::eval_attributes_array(string element, map< string, string >& attributes,
				      const map< string, string >& input)
{
  for (map< string, string >::const_iterator it = input.begin(); it != input.end(); ++it)
  {
    map< string, string >::iterator ait(attributes.find(it->first));
    if (ait != attributes.end())
      ait->second = it->second;
    else
    {
      ostringstream temp;
      temp<<"Unknown attribute \""<<it->first<<"\" in element \""<<element<<"\".";
      add_static_error(temp.str());
    }
  }
}

void Statement::assure_no_text(string text, string name)
{
  for (unsigned int i(0); i < text.size(); ++i)
  {
    if (!isspace(text[i]))
    {
      ostringstream temp;
      temp<<"Element \""<<name<<"\" must not contain text.";
      add_static_error(temp.str());
      break;
    }
  }
}

void Statement::substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  add_static_error(temp.str());
  
  delete child;
}

void Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  substatement_error(get_name(), statement);
}

void Statement::add_final_text(string text)
{
  assure_no_text(text, this->get_name());
}

void Statement::display_full()
{
  //display_verbatim(get_source(startpos, endpos - startpos));
}

void Statement::display_starttag()
{
  //display_verbatim(get_source(startpos, tagendpos - startpos));
}

Statement::Factory::~Factory()
{
  for (vector< Statement* >::const_iterator it = created_statements.begin();
      it != created_statements.end(); ++it)
    delete *it;
}

Statement* Statement::Factory::create_statement
    (string element, int line_number, const map< string, string >& attributes)
{
  Statement* statement = 0;

  map< string, Statement::Statement_Maker* >::iterator maker_it =
      Statement::maker_by_name().find(element);
  
  if (maker_it != Statement::maker_by_name().end())
    statement = maker_it->second->create_statement(line_number, attributes);
  
  if (statement)
    created_statements.push_back(statement);
  else
  {
    ostringstream temp;
    temp<<"Unknown tag \""<<element<<"\" in line "<<line_number<<'.';
    if (error_output_)
      error_output_->add_static_error(temp.str(), line_number);
  }
  
  return statement;
}

Error_Output* Statement::error_output = 0;

void Statement::add_static_error(string error)
{
  if (error_output)
    error_output->add_static_error(error, line_number);
}

void Statement::add_static_remark(string remark)
{
  if (error_output)
    error_output->add_static_remark(remark, line_number);
}

void Statement::runtime_remark(string error) const
{
  if (error_output)
    error_output->runtime_remark(error);
}

map< string, string > convert_c_pairs(const char** attr)
{
  map< string, string > result;
  for (int i = 0; attr[i]; i+=2)
    result[attr[i]] = attr[i+1];
  return result;
}
