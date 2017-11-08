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


#include "../data/set_comparison.h"
#include "evaluator.h"
#include "compare.h"


Generic_Statement_Maker< Compare_Statement > Compare_Statement::statement_maker("compare");


Compare_Statement::Compare_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), set_comparison(0), collection_mode(dont_collect)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);
}


void Compare_Statement::execute(Resource_Manager& rman)
{
  const Set* input_set = rman.get_set(input);
  if (!input_set)
    return;

  if (collection_mode == collect_lhs)
  {
    delete set_comparison;
    set_comparison = new Set_Comparison(
        *rman.get_transaction(), *input_set, rman.get_desired_timestamp());
    
    Set into;
    transfer_output(rman, into);
    rman.health_check(*this);
  }
  else if (collection_mode == collect_rhs)
  {
    Diff_Set into = set_comparison->compare_to_lhs(rman, *this, *input_set,
        1., 0., 0., 0., add_deletion_information);
    
    transfer_output(rman, into);
    rman.health_check(*this);
  }
  else
  {
    Set into;
    transfer_output(rman, into);
    rman.health_check(*this);
  }
}


Compare_Statement::~Compare_Statement()
{
  delete set_comparison;
}


void Compare_Statement::set_collect_lhs()
{
  collection_mode = collect_lhs;
}


void Compare_Statement::set_collect_rhs(bool add_deletion_information_)
{
  collection_mode = collect_rhs;
  add_deletion_information = add_deletion_information_;
}
