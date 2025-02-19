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


Generic_Statement_Maker_2< Print_Statement > Print_Statement::statement_maker("print");


Print_Statement::Print_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes)
    : Statement(line_number_),
      mode(0), order(order_by_id), limit(std::numeric_limits< unsigned int >::max()),
      collection_print_target(0), diff_valid(true),
      south(1.0), north(0.0), west(0.0), east(0.0)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["ids"] = "yes";
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
    mode = Output_Mode::ID | Output_Mode::COUNT;
  else
  {
    mode = Output_Mode::ID;
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

  if (attributes["ids"] == "yes")
    ;
  else if (attributes["ids"] == "no")
    mode = mode & ~Output_Mode::ID;
  else
  {
    std::ostringstream temp;
    temp<<"For the attribute \"ids\" of the element \"print\""
        <<" the only allowed values are \"yes\" or \"no\".";
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
      Resource_Manager& rman, const Statement& stmt, const Set& to_print,
      unsigned int mode, Feature_Action action, unsigned int limit,
      double south, double north, double west, double east);
  ~Extra_Data();

  const std::map< uint32, std::string >* get_users() const;

  unsigned int mode;
  Feature_Action action;
  const unsigned int limit;
  uint32_t element_count;
  Way_Bbox_Geometry_Store* way_geometry_store;
  Way_Bbox_Geometry_Store* attic_way_geometry_store;
  Relation_Geometry_Store* relation_geometry_store;
  Relation_Geometry_Store* attic_relation_geometry_store;
  const std::map< uint32, std::string >* roles;
  const std::map< uint32, std::string >* users;
};


Extra_Data::Extra_Data(
    Resource_Manager& rman, const Statement& stmt, const Set& to_print,
    unsigned int mode_, Feature_Action action_, unsigned int limit_,
    double south, double north, double west, double east)
    : mode(mode_), action(action_), limit(limit_), element_count(0),
    way_geometry_store(0), attic_way_geometry_store(0),
    relation_geometry_store(0), attic_relation_geometry_store(0), roles(0), users(0)
{
  Request_Context context(&stmt, rman);

  if (mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
  {
    way_geometry_store = new Way_Bbox_Geometry_Store(to_print.ways, context, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_way_geometry_store = new Way_Bbox_Geometry_Store(
          to_print.attic_ways, context, south, north, west, east);
    }

    relation_geometry_store = new Relation_Geometry_Store(
        to_print.relations, context, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_relation_geometry_store = new Relation_Geometry_Store(
          to_print.attic_relations, context, south, north, west, east);
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


void print_item(Extra_Data& extra_data, OSM_Data_Printer& output, uint32 ll_upper, const Node_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta = 0)
{
  output.print_item(skel, Point_Geometry(::lat(ll_upper, skel.ll_lower), ::lon(ll_upper, skel.ll_lower)),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode), extra_data.action);
}


void print_item(Extra_Data& extra_data, OSM_Data_Printer& output, uint32 ll_upper, const Way_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_way_geom(skel, extra_data.mode, extra_data.way_geometry_store),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode), extra_data.action);
}


void print_item(Extra_Data& extra_data, OSM_Data_Printer& output, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_way_geom(skel, extra_data.mode, extra_data.attic_way_geometry_store),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode), extra_data.action);
}


void print_item(Extra_Data& extra_data, OSM_Data_Printer& output, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_relation_geom(skel, extra_data.mode, extra_data.relation_geometry_store),
      tags, meta, extra_data.roles, extra_data.get_users(), Output_Mode(extra_data.mode), extra_data.action);
}


void print_item(Extra_Data& extra_data, OSM_Data_Printer& output, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0)
{
  Geometry_From_Quad_Coords broker;
  output.print_item(skel,
      broker.make_relation_geom(skel, extra_data.mode, extra_data.attic_relation_geometry_store),
      tags, meta, extra_data.roles, extra_data.get_users(), Output_Mode(extra_data.mode), extra_data.action);
}


void print_item(
    Extra_Data& extra_data, OSM_Data_Printer& output, const Area_Skeleton& skel,
    const std::vector< std::pair< std::string, std::string > >* tags = 0)
{
  Derived_Skeleton derived("area", Uint64(skel.id.val()));
  output.print_item(derived, Null_Geometry(), tags, Output_Mode(extra_data.mode), extra_data.action);
}


void print_item(
    Extra_Data& extra_data, OSM_Data_Printer& output, const Derived_Structure& skel,
    const std::vector< std::pair< std::string, std::string > >* tags = 0)
{
  if (skel.get_geometry())
    output.print_item(skel, *skel.get_geometry(), tags, Output_Mode(extra_data.mode), extra_data.action);
  else
    output.print_item(skel, Null_Geometry(), tags, Output_Mode(extra_data.mode), extra_data.action);
}


template< class Index, class Object >
void tags_quadtile(
    Extra_Data& extra_data, const std::map< Index, std::vector< Object > >& current_items,
    OSM_Data_Printer& output, Request_Context& context)
{
  Tag_Store< Index, Object > current_tag_store(context);
  if (extra_data.mode & Output_Mode::TAGS)
    current_tag_store.prefetch_all(current_items);

  typename std::map< Index, std::vector< Object > >::const_iterator
      item_it(current_items.begin());
  // print the result
  while (item_it != current_items.end())
  {
    for (typename std::vector< Object >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++extra_data.element_count > extra_data.limit)
        return;
      print_item(
          extra_data, output, *it2,
          (extra_data.mode & Output_Mode::TAGS) ? current_tag_store.get(item_it->first, *it2) : nullptr);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void tags_quadtile_attic
    (Extra_Data& extra_data,
     const std::map< Index, std::vector< Object > >& current_items,
     const std::map< Index, std::vector< Attic< Object > > >& attic_items,
     OSM_Data_Printer& output,
     Resource_Manager& rman, Transaction& transaction)
{
  Request_Context context(nullptr, rman);
  Tag_Store< Index, Object > current_tag_store(context);
  Tag_Store< Index, Object > attic_tag_store(context);
  if (extra_data.mode & Output_Mode::TAGS)
  {
    current_tag_store.prefetch_all(current_items);
    if (!attic_items.empty())
      attic_tag_store.prefetch_all(attic_items);
  }

  Idx_Cached_Meta_Collector< Index, Object > meta_collector(
      current_items, attic_items, context.get_desired_timestamp(), context);

  auto current_it = current_items.begin();
  auto attic_it = attic_items.begin();
  while (current_it != current_items.end() || attic_it != attic_items.end())
  {
    while (current_it != current_items.end() &&
        (attic_it == attic_items.end() || !(attic_it->first < current_it->first)))
    {
      for (auto it2 = current_it->second.begin(); it2 != current_it->second.end(); ++it2)
      {
        if (++extra_data.element_count > extra_data.limit)
          return;

        const Full_Monotype_Meta* meta = nullptr;
        if (extra_data.mode & Output_Mode::META)
          meta = meta_collector.get(current_it->first, it2->id.val());
        OSM_Element_Metadata_Skeleton< typename Object::Id_Type > temp_meta(it2->id, meta ? meta->timestamp : 0);
        if (meta)
        {
          temp_meta.version = meta->version;
          temp_meta.changeset = meta->changeset;
          temp_meta.user_id = meta->uid;
        }

        print_item(
            extra_data, output, current_it->first.val(), *it2,
            (extra_data.mode & Output_Mode::TAGS) ? current_tag_store.get(current_it->first, *it2) : nullptr,
            ((extra_data.mode & Output_Mode::META) && meta) ? &temp_meta : nullptr);
      }
      ++current_it;
    }

    while (attic_it != attic_items.end() &&
        (current_it == current_items.end() || attic_it->first < current_it->first))
    {
      for (auto it2 = attic_it->second.begin(); it2 != attic_it->second.end(); ++it2)
      {
        if (++extra_data.element_count > extra_data.limit)
          return;

        const Full_Monotype_Meta* meta = nullptr;
        meta = meta_collector.get(attic_it->first, it2->id.val());
        OSM_Element_Metadata_Skeleton< typename Object::Id_Type > temp_meta(it2->id, meta ? meta->timestamp : 0);
        if (meta)
        {
          temp_meta.version = meta->version;
          temp_meta.changeset = meta->changeset;
          temp_meta.user_id = meta->uid;
        }

        print_item(
            extra_data, output, attic_it->first.val(), *it2,
            (extra_data.mode & Output_Mode::TAGS) ? attic_tag_store.get(attic_it->first, *it2) : nullptr,
            ((extra_data.mode & Output_Mode::META) && meta) ? &temp_meta : nullptr);
      }
      ++attic_it;
    }
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
  std::sort(items_by_id.begin(), items_by_id.end(), Skeleton_Comparator_By_Id< Object >());

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
  std::sort(items_by_id.begin(), items_by_id.end());

  return items_by_id;
}


template< class Index, class Object >
void tags_by_id(
    const std::map< Index, std::vector< Object > >& items,
    Extra_Data& extra_data, uint32 FLUSH_SIZE, OSM_Data_Printer& output,
    Tag_Store< Index, Object >&& tag_store)
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

    if (extra_data.mode & Output_Mode::TAGS)
      tag_store.prefetch_chunk(items, lower_id_bound, upper_id_bound);

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      if (++extra_data.element_count > extra_data.limit)
        return;
      print_item(
          extra_data, output, *(items_by_id[i.val()].first),
          (extra_data.mode & Output_Mode::TAGS)
              ? tag_store.get(Index(items_by_id[i.val()].second), *items_by_id[i.val()].first) : nullptr);
    }
  }
}


template< class Index, class Object >
void tags_by_id_attic
  (const std::map< Index, std::vector< Object > >& current_items,
   const std::map< Index, std::vector< Attic< Object > > >& attic_items,
   Extra_Data& extra_data, uint32 FLUSH_SIZE, OSM_Data_Printer& output,
   Request_Context& context)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id = collect_items_by_id(current_items, attic_items);
  
  Tag_Store< Index, Object > current_tag_store(context);
  Tag_Store< Index, Object > attic_tag_store(context);

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

    Chunked_Meta_Collector< Index > meta_collector;
    meta_collector.prefetch_chunk(
        (extra_data.mode & Output_Mode::META) ?
            current_items : std::map< Index, std::vector< Object > >{}, // fetch meta for current items only if requested
        attic_items, // fetch always meta for attic items to enforce redactions
        lower_id_bound.val(), upper_id_bound.val(), context.get_desired_timestamp(), context);
    
    if (extra_data.mode & Output_Mode::TAGS)
    {
      current_tag_store.prefetch_chunk(current_items, lower_id_bound, upper_id_bound);
      attic_tag_store.prefetch_chunk(attic_items, lower_id_bound, upper_id_bound);
    }

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      if (++extra_data.element_count > extra_data.limit)
        return;

      const Full_Monotype_Meta* meta = nullptr;
      if ((extra_data.mode & Output_Mode::META) || items_by_id[i.val()].timestamp != NOW)
        meta = meta_collector.get(items_by_id[i.val()].idx, items_by_id[i.val()].obj->id.val());
      OSM_Element_Metadata_Skeleton< typename Object::Id_Type > temp_meta(
          items_by_id[i.val()].obj->id, meta ? meta->timestamp : 0);
      if (meta)
      {
        temp_meta.version = meta->version;
        temp_meta.changeset = meta->changeset;
        temp_meta.user_id = meta->uid;
      }

      if (items_by_id[i.val()].timestamp == NOW)
        print_item(
            extra_data, output, items_by_id[i.val()].idx.val(), *items_by_id[i.val()].obj,
            (extra_data.mode & Output_Mode::TAGS)
                ? current_tag_store.get(items_by_id[i.val()].idx, *items_by_id[i.val()].obj) : nullptr,
            ((extra_data.mode & Output_Mode::META) && meta) ? &temp_meta : nullptr);
      else
        print_item(
            extra_data, output, items_by_id[i.val()].idx.val(),
            Attic< Object >(*items_by_id[i.val()].obj, items_by_id[i.val()].timestamp),
            (extra_data.mode & Output_Mode::TAGS)
                ? attic_tag_store.get(items_by_id[i.val()].idx, *items_by_id[i.val()].obj) : nullptr,
            ((extra_data.mode & Output_Mode::META) && meta) ? &temp_meta : nullptr);
    }
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
  Cpu_Timer cpu(rman, 2);

  Diff_Action::_ action = rman.get_desired_action();
  if (action == Diff_Action::collect_lhs
      || action == Diff_Action::collect_rhs_no_del || action == Diff_Action::collect_rhs_with_del)
  {
    execute_comparison(rman);
    return;
  }

  if (action == Diff_Action::positive && rman.area_updater())
    rman.area_updater()->flush();

  const Set* input_set = rman.get_set(input);

  Set count_set;
  const Set* output_items = 0;
  if (mode & Output_Mode::COUNT)
  {
    count_set.deriveds[Uint31_Index(0u)].push_back(Derived_Structure("count", Uint64(0ull),
        make_count_tags(input_set ? *input_set : Set(), rman.get_area_transaction()), 0));
    output_items = &count_set;
    mode = mode | Output_Mode::TAGS;
  }
  else
  {
    if (!input_set)
      return;

    output_items = input_set;
  }

  Feature_Action feature_action = Feature_Action::keep;
  if (action == Diff_Action::show_old)
    feature_action = Feature_Action::show_from;
  else if (action == Diff_Action::show_new)
    feature_action = Feature_Action::show_to;

  Extra_Data extra_data(rman, *this, *output_items, mode, feature_action, limit, south, north, west, east);
  Output_Handler& output_handler = *dynamic_cast< Output_Handler* >(rman.get_global_settings().get_output_handler());
  auto& data_printer = *output_handler.get_data_printer();

  Request_Context context(this, rman);
  if (order == order_by_id)
  {
    tags_by_id_attic(
        output_items->nodes, output_items->attic_nodes, extra_data, NODE_FLUSH_SIZE, data_printer, context);
    tags_by_id_attic(
        output_items->ways, output_items->attic_ways, extra_data, WAY_FLUSH_SIZE, data_printer, context);
    tags_by_id_attic(
        output_items->relations, output_items->attic_relations, extra_data,
        RELATION_FLUSH_SIZE, data_printer, context);

    if (rman.get_area_transaction())
      tags_by_id(
          output_items->areas, extra_data, AREA_FLUSH_SIZE, data_printer,
          Tag_Store< Uint31_Index, Area_Skeleton >(context));

    tags_by_id(
        output_items->deriveds, extra_data, std::numeric_limits< uint32 >::max(), data_printer,
        Tag_Store< Uint31_Index, Derived_Structure >());
  }
  else
  {
    tags_quadtile_attic(
        extra_data, output_items->nodes, output_items->attic_nodes,
        data_printer, rman, *rman.get_transaction());
    tags_quadtile_attic(
        extra_data, output_items->ways, output_items->attic_ways,
        data_printer, rman, *rman.get_transaction());
    tags_quadtile_attic(
        extra_data, output_items->relations, output_items->attic_relations,
        data_printer, rman, *rman.get_transaction());

    if (rman.get_area_transaction())
      tags_quadtile(extra_data, output_items->areas, data_printer, context);

    tags_quadtile(extra_data, output_items->deriveds, data_printer, context);
  }

  rman.health_check(*this);
}


void Print_Statement::execute_comparison(Resource_Manager& rman)
{
  Diff_Action::_ action = rman.get_desired_action();

  const Diff_Set* input_diff_set = rman.get_diff_set(input);
  if (input_diff_set)
  {
    print_diff_set(
        *input_diff_set, mode,
        dynamic_cast< Output_Handler* >(rman.get_global_settings().get_output_handler()),
        rman.users(), relation_member_roles(*rman.get_transaction()), action == Diff_Action::collect_rhs_with_del);
    return;
  }

  const Set* input_set = rman.get_set(input);
  if (!input_set)
    return;

  if (action == Diff_Action::collect_lhs)
  {
    if (collection_print_target && diff_valid)
    {
      runtime_error("A print statement cannot be executed in a loop in a diff setting.");
      diff_valid = false;
      delete collection_print_target;
      collection_print_target = 0;
      return;
    }
    delete collection_print_target;
    collection_print_target = new Set_Comparison(
        *rman.get_transaction(), *input_set, rman.get_desired_timestamp());
  }
  else
  {
    if (!diff_valid || !collection_print_target)
      return;

    Diff_Set result = collection_print_target->compare_to_lhs(rman, *this, *input_set,
        south, north, west, east, action == Diff_Action::collect_rhs_with_del);

    print_diff_set(
        result, mode,
        dynamic_cast< Output_Handler* >(rman.get_global_settings().get_output_handler()),
        rman.users(), relation_member_roles(*rman.get_transaction()), action == Diff_Action::collect_rhs_with_del);
  }

  rman.health_check(*this);
}


Print_Statement::~Print_Statement()
{
  delete collection_print_target;
}
