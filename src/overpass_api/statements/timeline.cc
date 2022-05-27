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

#include "evaluator.h"
#include "timeline.h"


Generic_Statement_Maker< Timeline_Statement > Timeline_Statement::statement_maker("timeline");


Timeline_Statement::Timeline_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  attributes["version"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);

  if (attributes["type"] == "node")
    type = Statement::NODE;
  else if (attributes["type"] == "way")
    type = Statement::WAY;
  else if (attributes["type"] == "relation")
    type = Statement::RELATION;
  else
  {
    type = 0;
    add_static_error("For the attribute \"type\" of the element \"timeline\""
	" the only allowed values are \"node\", \"way\", or \"relation\".");
  }

  ref = atoll(attributes["ref"].c_str());

  if (!ref)
    add_static_error("For the attribute \"ref\" of the element \"timeline\""
        " the only allowed values are positive integers.");

  version = atoll(attributes["version"].c_str());

  if (version == 0 && attributes["version"] != "")
    add_static_error("For the attribute \"version\" of the element \"timeline\""
        " the only allowed values are positive integers.");
}


template< typename Skeleton >
struct Ref_Ver_Equal
{
  bool operator()(const OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >& lhs,
                  const OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >& rhs)
  {
    return (lhs.ref == rhs.ref && lhs.version == rhs.version);
  }
};


template< typename Index, typename Skeleton >
void create_timeline_entries(uint64 ref, uint32 version, Statement* stmt, Resource_Manager& rman,
    const std::string& reftype, std::map< Uint31_Index, std::vector< Derived_Structure > >& deriveds)
{
  std::vector< typename Skeleton::Id_Type > ids;
  ids.push_back(ref);
  std::vector< Index > req = get_indexes_< Index, Skeleton >(ids, rman, true);

  std::vector< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > metas;
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
        typename std::vector< Index >::const_iterator > current_meta_db
        (rman.get_transaction()->data_index(current_meta_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
        typename std::vector< Index >::const_iterator >::Discrete_Iterator
        it = current_meta_db.discrete_begin(req.begin(), req.end());
        !(it == current_meta_db.discrete_end()); ++it)
    {
      if (it.object().ref == ref)
        metas.push_back(it.object());
    }
  }
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
        typename std::vector< Index >::const_iterator > attic_meta_db
        (rman.get_transaction()->data_index(attic_meta_file_properties< Skeleton >()));
    for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
        typename std::vector< Index >::const_iterator >::Discrete_Iterator
        it = attic_meta_db.discrete_begin(req.begin(), req.end());
        !(it == attic_meta_db.discrete_end()); ++it)
    {
      if (it.object().ref == ref)
        metas.push_back(it.object());
    }
  }

  std::sort(metas.begin(), metas.end());
  metas.erase(std::unique(metas.begin(), metas.end(), Ref_Ver_Equal< Skeleton >()), metas.end());

  for (uint i = 0; i < metas.size(); ++i)
  {
    if (version == 0 || version == metas[i].version)
    {
      Derived_Structure result("timeline", rman.get_global_settings().dispense_derived_id());
      result.tags.push_back(std::make_pair(std::string("reftype"), to_string(reftype)));
      result.tags.push_back(std::make_pair(std::string("ref"), to_string(metas[i].ref.val())));
      result.tags.push_back(std::make_pair(std::string("refversion"), to_string(metas[i].version)));
      result.tags.push_back(std::make_pair(std::string("created"), Timestamp(metas[i].timestamp).str()));
      if (i+1 < metas.size())
        result.tags.push_back(std::make_pair(std::string("expired"), Timestamp(metas[i+1].timestamp).str()));
      deriveds[Uint31_Index(0u)].push_back(result);
    }
  }
}



void Timeline_Statement::execute(Resource_Manager& rman)
{
  Set into;

  if (type == NODE)
    create_timeline_entries< Node::Index, Node_Skeleton >(ref, version, this, rman, "node", into.deriveds);
  else if (type == WAY)
    create_timeline_entries< Way::Index, Way_Skeleton >(ref, version, this, rman, "way", into.deriveds);
  else if (type == RELATION)
    create_timeline_entries< Relation::Index, Relation_Skeleton >(ref, version, this, rman, "relation", into.deriveds);

  transfer_output(rman, into);
  rman.health_check(*this);
}
