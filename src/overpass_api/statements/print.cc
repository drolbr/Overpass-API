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

#include "../core/index_computations.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "../data/geometry_from_quad_coords.h"
#include "../data/meta_collector.h"
#include "../data/relation_geometry_store.h"
#include "../data/set_comparison.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "../data/way_geometry_store.h"
#include "print.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>


const unsigned int NODE_FLUSH_SIZE = 1024*1024;
const unsigned int WAY_FLUSH_SIZE = 512*1024;
const unsigned int RELATION_FLUSH_SIZE = 512*1024;
const unsigned int AREA_FLUSH_SIZE = 64*1024;


Generic_Statement_Maker< Print_Statement > Print_Statement::statement_maker("print");


Print_Statement::Print_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_),
      mode(0), order(order_by_id), limit(std::numeric_limits< unsigned int >::max()),
      collection_print_target(0), collection_mode(dont_collect), add_deletion_information(false),
      south(1.0), north(0.0), west(0.0), east(0.0)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["mode"] = "body";
  attributes["order"] = "id";
  attributes["limit"] = "";
  attributes["geometry"] = "skeleton";
  attributes["s"] = "";
  attributes["n"] = "";
  attributes["w"] = "";
  attributes["e"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];

  if (attributes["mode"] == "ids_only")
    mode = Output_Mode::ID;
  else if (attributes["mode"] == "skeleton")
    mode = Output_Mode::ID
        | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS;
  else if (attributes["mode"] == "tags")
    mode = Output_Mode::ID | Output_Mode::TAGS;
  else if (attributes["mode"] == "body")
    mode = Output_Mode::ID
        | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
	| Output_Mode::TAGS;
  else if (attributes["mode"] == "meta")
    mode = Output_Mode::ID
        | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
	| Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META;
  else if (attributes["mode"] == "count")
    mode = Output_Mode::COUNT;
  else
  {
    mode = 0;
    std::ostringstream temp;
    temp<<"For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\", \"body\", \"tags\",  \"count\", or \"meta\".";
    add_static_error(temp.str());
  }

  if (attributes["order"] == "id")
    order = order_by_id;
  else if (attributes["order"] == "quadtile")
    order = order_by_quadtile;
  else
  {
    std::ostringstream temp;
    temp<<"For the attribute \"order\" of the element \"print\""
        <<" the only allowed values are \"id\" or \"quadtile\".";
    add_static_error(temp.str());
  }

  if (attributes["limit"] != "")
    limit = atoll(attributes["limit"].c_str());

  if (attributes["geometry"] == "skeleton")
    ;
  else if (attributes["geometry"] == "full")
    mode = mode | Output_Mode::GEOMETRY | Output_Mode::BOUNDS;
  else if (attributes["geometry"] == "bounds")
    mode = mode | Output_Mode::BOUNDS;
  else if (attributes["geometry"] == "center")
    mode = mode | Output_Mode::CENTER;
  else
  {
    std::ostringstream temp;
    temp<<"For the attribute \"geometry\" of the element \"print\""
        <<" the only allowed values are \"skeleton\", \"full\", \"bounds\", or \"center\".";
    add_static_error(temp.str());
  }

  south = atof(attributes["s"].c_str());
  if ((south < -90.0) || (south > 90.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"print\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"print\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  if (north < south)
  {
    std::ostringstream temp;
    temp<<"The value of attribute \"n\" of the element \"print\""
    <<" must always be greater or equal than the value of attribute \"s\".";
    add_static_error(temp.str());
  }

  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"print\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"print\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  if ((attributes["n"] == "") && (attributes["s"] == "") &&
      (attributes["w"] == "") && (attributes["e"] == ""))
  {
    south = 1.0;
    north = 0.0;
  }
}


struct Extra_Data
{
  Extra_Data(
      Resource_Manager& rman, const Statement& stmt, const Set& to_print, unsigned int mode_,
      double south, double north, double west, double east);
  ~Extra_Data();

  const std::map< uint32, std::string >* get_users() const;

  unsigned int mode;
  Way_Bbox_Geometry_Store* way_geometry_store;
  Way_Bbox_Geometry_Store* attic_way_geometry_store;
  Relation_Geometry_Store* relation_geometry_store;
  Relation_Geometry_Store* attic_relation_geometry_store;
  const std::map< uint32, std::string >* roles;
  const std::map< uint32, std::string >* users;
};


Extra_Data::Extra_Data(
    Resource_Manager& rman, const Statement& stmt, const Set& to_print, unsigned int mode_,
    double south, double north, double west, double east)
    : mode(mode_), way_geometry_store(0), attic_way_geometry_store(0),
    relation_geometry_store(0), attic_relation_geometry_store(0), roles(0), users(0)
{
  if (mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
  {
    way_geometry_store = new Way_Bbox_Geometry_Store(to_print.ways, stmt, rman, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_way_geometry_store = new Way_Bbox_Geometry_Store(
          to_print.attic_ways, stmt, rman,
          south, north, west, east);
    }

    relation_geometry_store = new Relation_Geometry_Store(
        to_print.relations, stmt, rman, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_relation_geometry_store = new Relation_Geometry_Store(
          to_print.attic_relations, stmt, rman,
          south, north, west, east);
    }
  }

  roles = &relation_member_roles(*rman.get_transaction());

  if (mode & Output_Mode::META)
    users = &rman.users();
}


const std::map< uint32, std::string >* Extra_Data::get_users() const
{
  return users;
}


Extra_Data::~Extra_Data()
{
  delete way_geometry_store;
  delete attic_way_geometry_store;
  delete relation_geometry_store;
  delete attic_relation_geometry_store;
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Node_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta = 0)
{
  output.print_item(skel, Point_Geometry(::lat(ll_upper, skel.ll_lower), ::lon(ll_upper, skel.ll_lower)),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Way_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_way_geom(skel, extra_data.mode, extra_data.way_geometry_store),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_way_geom(skel, extra_data.mode, extra_data.attic_way_geometry_store),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_relation_geom(skel, extra_data.mode, extra_data.relation_geometry_store),
      tags, meta, extra_data.roles, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_relation_geom(skel, extra_data.mode, extra_data.attic_relation_geometry_store),
      tags, meta, extra_data.roles, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Area_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Area_Skeleton::Id_Type >* meta = 0)
{
  Derived_Skeleton derived("area", Uint64(skel.id.val()));
  output.print_item(derived, Null_Geometry(), tags, Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Derived_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Derived_Skeleton::Id_Type >* meta = 0)
{
  output.print_item(skel, Null_Geometry(), tags, Output_Mode(extra_data.mode));
}


template< class TIndex, class TObject >
void quadtile_
    (const std::map< TIndex, std::vector< TObject > >& items, Output_Handler& output,
     Transaction& transaction, Extra_Data& extra_data, uint32 limit, uint32& element_count)
{
  typename std::map< TIndex, std::vector< TObject > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename std::vector< TObject >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++element_count > limit)
	return;
      print_item(extra_data, output, item_it->first.val(), *it2);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void tags_quadtile_
    (Extra_Data& extra_data, const std::map< Index, std::vector< Object > >& items,
     Output_Handler& output,
     Resource_Manager& rman, Transaction& transaction, uint32 limit, uint32& element_count)
{
  Tag_Store< Index, Object > tag_store(*rman.get_transaction());
  tag_store.prefetch_all(items);

  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, transaction,
      (extra_data.mode & Output_Mode::META) ? current_meta_file_properties< Object >() : 0);

  typename std::map< Index, std::vector< Object > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename std::vector< Object >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++element_count > limit)
        return;
      print_item(extra_data, output, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
          meta_printer.get(item_it->first, it2->id));
    }
    ++item_it;
  }
}


template< class Index, class Object >
void tags_quadtile_attic_
    (Extra_Data& extra_data, const std::map< Index, std::vector< Attic< Object > > >& items,
     Output_Handler& output,
     Resource_Manager& rman, Transaction& transaction, uint32 limit, uint32& element_count)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);

  Attic_Meta_Collector< Index, Object > meta_printer(items, transaction, extra_data.mode & Output_Mode::META);

  typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      item_it(items.begin());
  while (item_it != items.end())
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++element_count > limit)
        return;
      print_item(extra_data, output, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
                 meta_printer.get(item_it->first, it2->id, it2->timestamp));
    }
    ++item_it;
  }
}


template< class TComp >
struct Skeleton_Comparator_By_Id {
  bool operator() (const std::pair< const TComp*, uint32 >& a,
		   const std::pair< const TComp*, uint32 >& b)
  {
    return (a.first->id < b.first->id);
  }
};


template< typename Index, typename Object >
struct Maybe_Attic_Ref
{
public:
  Maybe_Attic_Ref(Index idx_, const Object* obj_, uint64 timestamp_)
  : idx(idx_), obj(obj_), timestamp(timestamp_) {}

  Index idx;
  const Object* obj;
  uint64 timestamp;

  bool operator<(const Maybe_Attic_Ref& rhs) const { return obj->id < rhs.obj->id; }
};


template< class Index, class Object >
std::vector< std::pair< const Object*, uint32 > > collect_items_by_id(
    const std::map< Index, std::vector< Object > >& items)
{
  std::vector< std::pair< const Object*, uint32 > > items_by_id;

  for (typename std::map< Index, std::vector< Object > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(std::make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< Object >());

  return items_by_id;
}


template< class Index, class Object >
std::vector< Maybe_Attic_Ref< Index, Object > > collect_items_by_id(
    const std::map< Index, std::vector< Object > >& items,
    const std::map< Index, std::vector< Attic< Object > > >& attic_items)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id;
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(Maybe_Attic_Ref< Index, Object >(it->first, &(*it2), NOW));
  }
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      it(attic_items.begin()); it != attic_items.end(); ++it)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(Maybe_Attic_Ref< Index, Object >(it->first, &(*it2), it2->timestamp));
  }
  sort(items_by_id.begin(), items_by_id.end());

  return items_by_id;
}


template< class Index, class Object >
void by_id
  (const std::map< Index, std::vector< Object > >& items, Output_Handler& output,
   Transaction& transaction, Extra_Data& extra_data, uint32 limit, uint32& element_count)
{
  std::vector< std::pair< const Object*, uint32 > > items_by_id = collect_items_by_id(items);

  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    print_item(extra_data, output, items_by_id[i].second, *(items_by_id[i].first));
  }
}


template< class Index, class Object >
void by_id
  (const std::map< Index, std::vector< Object > >& items,
   const std::map< Index, std::vector< Attic< Object > > >& attic_items,
   Output_Handler& output,
   Transaction& transaction, Extra_Data& extra_data, uint32 limit, uint32& element_count)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id = collect_items_by_id(items, attic_items);

  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    if (items_by_id[i].timestamp == NOW)
      print_item(extra_data, output, items_by_id[i].idx.val(), *items_by_id[i].obj);
    else
      print_item(extra_data, output, items_by_id[i].idx.val(),
		      Attic< Object >(*items_by_id[i].obj, items_by_id[i].timestamp));
  }
}


template< class Index, class Object >
void collect_metadata(std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >& metadata,
		      const std::map< Index, std::vector< Object > >& items,
		      typename Object::Id_Type lower_id_bound, typename Object::Id_Type upper_id_bound,
		      Meta_Collector< Index, typename Object::Id_Type >& meta_printer)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (!(it2->id < lower_id_bound) && (it2->id < upper_id_bound))
      {
	const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
	    = meta_printer.get(it->first, it2->id);
	if (meta)
	  metadata.insert(*meta);
      }
    }
  }
}


template< class Index, class Object >
void collect_metadata(std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >& metadata,
                      const std::map< Index, std::vector< Attic< Object > > >& items,
                      typename Object::Id_Type lower_id_bound, typename Object::Id_Type upper_id_bound,
                      Attic_Meta_Collector< Index, Object >& meta_printer)
{
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (!(it2->id < lower_id_bound) && (it2->id < upper_id_bound))
      {
        const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
            = meta_printer.get(it->first, it2->id, it2->timestamp);
        if (meta)
          metadata.insert(*meta);
      }
    }
  }
}


template< typename Id_Type >
typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::const_iterator
    find_matching_metadata
    (const std::set< OSM_Element_Metadata_Skeleton< Id_Type > >& metadata,
     Id_Type ref, uint64 timestamp)
{
  typename std::set< OSM_Element_Metadata_Skeleton< Id_Type > >::iterator it
      = metadata.lower_bound(OSM_Element_Metadata_Skeleton< Id_Type >(ref, timestamp));
  if (it == metadata.begin())
    return metadata.end();
  --it;
  if (it->ref == ref)
    return it;
  else
    return metadata.end();
}


template< class Index, class Object >
void tags_by_id
  (Extra_Data& extra_data, const std::map< Index, std::vector< Object > >& items,
   uint32 FLUSH_SIZE, Output_Handler& output,
   Resource_Manager& rman, Meta_Collector< Index, typename Object::Id_Type >* meta_printer,
   Tag_Store< Index, Object >& tag_store, uint32 limit, uint32& element_count)
{
  std::vector< std::pair< const Object*, uint32 > > items_by_id = collect_items_by_id(items);

  // iterate over the result
  for (typename Object::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    // Disable health_check: This ensures that a result will be always printed completely
    //rman.health_check(*this);

    typename Object::Id_Type lower_id_bound(items_by_id[id_pos.val()].first->id);
    typename Object::Id_Type upper_id_bound;
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[(id_pos + FLUSH_SIZE).val()].first->id;
    else
    {
      upper_id_bound = items_by_id[items_by_id.size()-1].first->id;
      ++upper_id_bound;
    }

    tag_store.prefetch_chunk(items, lower_id_bound, upper_id_bound);

    std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > metadata;
    if (meta_printer)
    {
      // collect metadata if required
      collect_metadata(metadata, items, lower_id_bound, upper_id_bound, *meta_printer);
      meta_printer->reset();
    }

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      if (++element_count > limit)
	return;
      typename std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
          = metadata.lower_bound(OSM_Element_Metadata_Skeleton< typename Object::Id_Type >
              (items_by_id[i.val()].first->id));
      print_item(extra_data, output, items_by_id[i.val()].second, *(items_by_id[i.val()].first),
		 tag_store.get(Index(items_by_id[i.val()].second), *items_by_id[i.val()].first),
		 (meta_it != metadata.end() && meta_it->ref == items_by_id[i.val()].first->id) ?
		     &*meta_it : 0);
    }
  }
}



template< class Index, class Object >
void tags_by_id_attic
  (const std::map< Index, std::vector< Object > >& current_items,
   const std::map< Index, std::vector< Attic< Object > > >& attic_items,
   Extra_Data& extra_data, uint32 FLUSH_SIZE, Output_Handler& output,
   Resource_Manager& rman, Transaction& transaction, uint32 limit, uint32& element_count)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id = collect_items_by_id(current_items, attic_items);

  Tag_Store< Index, Object > current_tag_store(transaction);
  Tag_Store< Index, Object > attic_tag_store(transaction);

  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > only_current_meta_printer
      (current_items, transaction,
      (extra_data.mode & Output_Mode::META) ? current_meta_file_properties< Object >() : 0);

  for (typename Object::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    // Disable health_check: This ensures that a result will be always printed completely
    //rman.health_check(*this);

    typename Object::Id_Type lower_id_bound(items_by_id[id_pos.val()].obj->id);
    typename Object::Id_Type upper_id_bound;
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[(id_pos + FLUSH_SIZE).val()].obj->id;
    else
    {
      upper_id_bound = items_by_id[items_by_id.size()-1].obj->id;
      ++upper_id_bound;
    }

    current_tag_store.prefetch_chunk(current_items, lower_id_bound, upper_id_bound);
    attic_tag_store.prefetch_chunk(attic_items, lower_id_bound, upper_id_bound);

    // collect metadata if required
    std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > only_current_metadata;
    collect_metadata(only_current_metadata, current_items, lower_id_bound, upper_id_bound,
		     only_current_meta_printer);
    only_current_meta_printer.reset();

    Attic_Meta_Collector< Index, Object > meta_printer(attic_items, transaction, extra_data.mode & Output_Mode::META);
    std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > attic_metadata;
    collect_metadata(attic_metadata, attic_items, lower_id_bound, upper_id_bound, meta_printer);

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      if (++element_count > limit)
	return;
      if (items_by_id[i.val()].timestamp == NOW)
      {
        typename std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
            = only_current_metadata.lower_bound(OSM_Element_Metadata_Skeleton< typename Object::Id_Type >
                (items_by_id[i.val()].obj->id));
        print_item(extra_data, output, items_by_id[i.val()].idx.val(), *items_by_id[i.val()].obj,
		 current_tag_store.get(items_by_id[i.val()].idx, *items_by_id[i.val()].obj),
		 (meta_it != only_current_metadata.end() && meta_it->ref == items_by_id[i.val()].obj->id) ?
		     &*meta_it : 0);
      }
      else
      {
        typename std::set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
            = find_matching_metadata(attic_metadata,
                  items_by_id[i.val()].obj->id, items_by_id[i.val()].timestamp);
        print_item(extra_data, output, items_by_id[i.val()].idx.val(),
		   Attic< Object >(*items_by_id[i.val()].obj, items_by_id[i.val()].timestamp),
		 attic_tag_store.get(items_by_id[i.val()].idx, *items_by_id[i.val()].obj),
                 meta_it != attic_metadata.end() ? &*meta_it : 0);
      }
    }
  }
}


template< class Index, class Object >
void tags_by_id
  (Extra_Data& extra_data, const std::map< Index, std::vector< Object > >& items,
   const std::map< Index, std::vector< Attic< Object > > >& attic_items,
   unsigned int mode, uint32 FLUSH_SIZE, Output_Handler& output, Resource_Manager& rman,
   uint32 limit, uint32& element_count)
{
  if (mode & Output_Mode::META)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      Tag_Store< Index, Object > tag_store(*rman.get_transaction());
      Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, *rman.get_transaction(),
          current_meta_file_properties< Object >());
      tags_by_id(extra_data, items, FLUSH_SIZE, output, rman, &meta_printer, tag_store, limit, element_count);
    }
    else
      tags_by_id_attic(items, attic_items, extra_data, FLUSH_SIZE, output, rman, *rman.get_transaction(),
                            limit, element_count);
  }
  else
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      Tag_Store< Index, Object > tag_store(*rman.get_transaction());
      tags_by_id(extra_data, items, FLUSH_SIZE, output, rman,
          (Meta_Collector< Index, typename Object::Id_Type >*)0, tag_store, limit, element_count);
    }
    else
      tags_by_id_attic(items, attic_items, extra_data, FLUSH_SIZE, output, rman, *rman.get_transaction(),
                            limit, element_count);
  }
}


std::vector< std::pair< std::string, std::string > > make_count_tags(const Set& set, bool include_areas)
{
  unsigned int num_nodes = count(set.nodes) + count(set.attic_nodes);
  unsigned int num_ways = count(set.ways) + count(set.attic_ways);
  unsigned int num_relations = count(set.relations) + count(set.attic_relations);
  unsigned int num_areas = include_areas ? count(set.areas) : 0;

  std::vector< std::pair< std::string, std::string > > count_tags;
  count_tags.push_back(std::make_pair("nodes", to_string(num_nodes)));
  count_tags.push_back(std::make_pair("ways", to_string(num_ways)));
  count_tags.push_back(std::make_pair("relations", to_string(num_relations)));
  if (include_areas)
    count_tags.push_back(std::make_pair("areas", to_string(num_areas)));
  count_tags.push_back(std::make_pair("total", to_string(num_nodes + num_ways + num_relations + num_areas)));

  return count_tags;
}


void Print_Statement::execute(Resource_Manager& rman)
{
  if (collection_mode != dont_collect)
  {
    execute_comparison(rman);
    return;
  }

  if (rman.area_updater())
    rman.area_updater()->flush();

  const Set* input_set = rman.get_set(input);

  Set count_set;
  const Set* output_items = 0;
  if (mode & Output_Mode::COUNT)
  {
    count_set.deriveds[Uint31_Index(0u)].push_back(Derived_Structure("count", Uint64(0ull),
        make_count_tags(input_set ? *input_set : Set(), rman.get_area_transaction())));
    output_items = &count_set;
    mode = mode | Output_Mode::TAGS;
  }
  else
  {
    if (!input_set)
      return;

    output_items = input_set;
  }

  Extra_Data extra_data(rman, *this, *output_items, mode, south, north, west, east);
  Output_Handler& output_handler = *rman.get_global_settings().get_output_handler();
  uint32 element_count = 0;

  if (order == order_by_id)
  {
    if (mode & Output_Mode::TAGS)
    {
      tags_by_id(extra_data, output_items->nodes, output_items->attic_nodes, mode, NODE_FLUSH_SIZE,
		 output_handler, rman, limit, element_count);
      tags_by_id(extra_data, output_items->ways, output_items->attic_ways, mode, WAY_FLUSH_SIZE,
		 output_handler, rman, limit, element_count);
      tags_by_id(extra_data, output_items->relations, output_items->attic_relations, mode, RELATION_FLUSH_SIZE,
		 output_handler, rman, limit, element_count);

      if (rman.get_area_transaction())
      {
	Tag_Store< Uint31_Index, Area_Skeleton > tag_store(*rman.get_transaction());
	tags_by_id(extra_data, output_items->areas, AREA_FLUSH_SIZE, output_handler, rman,
		   (Meta_Collector< Uint31_Index, Area_Skeleton::Id_Type >*)0,
		   tag_store, limit, element_count);
      }

      Tag_Store< Uint31_Index, Derived_Structure > tag_store;
      tags_by_id(extra_data, output_items->deriveds, std::numeric_limits< uint32 >::max(), output_handler, rman,
          (Meta_Collector< Uint31_Index, Derived_Structure::Id_Type >*)0, tag_store, limit, element_count);
    }
    else
    {
      by_id(output_items->nodes, output_items->attic_nodes,
            output_handler, *rman.get_transaction(), extra_data, limit, element_count);
      by_id(output_items->ways, output_items->attic_ways,
            output_handler, *rman.get_transaction(), extra_data, limit, element_count);
      by_id(output_items->relations, output_items->attic_relations,
            output_handler, *rman.get_transaction(), extra_data, limit, element_count);
      if (rman.get_area_transaction())
        by_id(output_items->areas, output_handler, *rman.get_area_transaction(), extra_data, limit, element_count);
      by_id(output_items->deriveds, output_handler, *rman.get_transaction(), extra_data, limit, element_count);
    }
  }
  else
  {
    if (mode & Output_Mode::TAGS)
    {
      tags_quadtile_(extra_data, output_items->nodes,
		    output_handler, rman, *rman.get_transaction(), limit, element_count);

      if (rman.get_desired_timestamp() != NOW)
        tags_quadtile_attic_(extra_data, output_items->attic_nodes,
                      output_handler, rman, *rman.get_transaction(), limit, element_count);

      tags_quadtile_(extra_data, output_items->ways,
		    output_handler, rman, *rman.get_transaction(), limit, element_count);

      if (rman.get_desired_timestamp() != NOW)
        tags_quadtile_attic_(extra_data, output_items->attic_ways,
                      output_handler, rman, *rman.get_transaction(), limit, element_count);

      tags_quadtile_(extra_data, output_items->relations,
		    output_handler, rman, *rman.get_transaction(), limit, element_count);

      if (rman.get_desired_timestamp() != NOW)
        tags_quadtile_attic_(extra_data, output_items->attic_relations,
                      output_handler, rman, *rman.get_transaction(), limit, element_count);

      if (rman.get_area_transaction())
        tags_quadtile_(extra_data, output_items->areas,
		      output_handler, rman, *rman.get_area_transaction(), limit, element_count);

      tags_quadtile_(extra_data, output_items->deriveds,
                    output_handler, rman, *rman.get_transaction(), limit, element_count);
    }
    else
    {
      quadtile_(output_items->nodes, output_handler, *rman.get_transaction(), extra_data, limit, element_count);
      quadtile_(output_items->attic_nodes, output_handler, *rman.get_transaction(), extra_data, limit, element_count);

      quadtile_(output_items->ways, output_handler, *rman.get_transaction(), extra_data, limit, element_count);
      quadtile_(output_items->attic_ways, output_handler, *rman.get_transaction(), extra_data, limit, element_count);

      quadtile_(output_items->relations, output_handler, *rman.get_transaction(), extra_data, limit, element_count);
      quadtile_(output_items->attic_relations, output_handler, *rman.get_transaction(), extra_data, limit, element_count);

      if (rman.get_area_transaction())
        quadtile_(output_items->areas, output_handler, *rman.get_area_transaction(), extra_data, limit, element_count);

      quadtile_(output_items->deriveds, output_handler, *rman.get_transaction(), extra_data, limit, element_count);
    }
  }

  rman.health_check(*this);
}


void Print_Statement::execute_comparison(Resource_Manager& rman)
{
  const Set* input_set = rman.get_set(input);
  if (!input_set)
    return;

  if (collection_mode == collect_lhs)
  {
    delete collection_print_target;
    collection_print_target = new Set_Comparison(
        *rman.get_transaction(), *input_set, rman.get_desired_timestamp());
  }
  else
  {
    Diff_Set result = collection_print_target->compare_to_lhs(rman, *this, *input_set,
        south, north, west, east, add_deletion_information);
    
    print_diff_set(result, mode, rman.get_global_settings().get_output_handler(),
        rman.users(), relation_member_roles(*rman.get_transaction()), add_deletion_information);
  }

  rman.health_check(*this);
}


Print_Statement::~Print_Statement()
{
  delete collection_print_target;
}


void Print_Statement::set_collect_lhs()
{
  collection_mode = collect_lhs;
}


void Print_Statement::set_collect_rhs(bool add_deletion_information_)
{
  collection_mode = collect_rhs;
  add_deletion_information = add_deletion_information_;
}
