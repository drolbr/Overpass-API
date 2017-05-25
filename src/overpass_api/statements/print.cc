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
#include "../data/meta_collector.h"
#include "../data/relation_geometry_store.h"
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
      Resource_Manager& rman, const Print_Statement& stmt, const Set& to_print, unsigned int mode_,
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
    Resource_Manager& rman, const Print_Statement& stmt, const Set& to_print, unsigned int mode_,
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
          to_print.attic_ways, rman.get_desired_timestamp(), stmt, rman,
          south, north, west, east);
    }

    relation_geometry_store = new Relation_Geometry_Store(
        to_print.relations, stmt, rman, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_relation_geometry_store = new Relation_Geometry_Store(
          to_print.attic_relations, rman.get_desired_timestamp(), stmt, rman,
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


struct Geometry_Broker
{
  Geometry_Broker() : geom(0) {}
  ~Geometry_Broker() { delete geom; }

  const Opaque_Geometry& make_way_geom(const Way_Skeleton& skel, unsigned int mode, Way_Bbox_Geometry_Store* store);
  const Opaque_Geometry& make_relation_geom(
      const Relation_Skeleton& skel, unsigned int mode, Relation_Geometry_Store* store);

  const Opaque_Geometry& make_way_geom(
      const std::vector< Quad_Coord >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds);
  const Opaque_Geometry& make_relation_geom(
      const std::vector< std::vector< Quad_Coord > >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds);

private:
  Opaque_Geometry* geom;
};


const Opaque_Geometry& Geometry_Broker::make_way_geom(
    const Way_Skeleton& skel, unsigned int mode, Way_Bbox_Geometry_Store* store)
{
  delete geom;
  geom = 0;

  if (store && (mode & Output_Mode::GEOMETRY))
  {
    //geom = new_opaque_geometry(geometry);
    
    std::vector< Quad_Coord > geometry = store->get_geometry(skel);

    bool is_complete = true;
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
      is_complete &= (it->ll_upper != 0 || it->ll_lower != 0);

    if (is_complete)
    {
      std::vector< Point_Double > coords;
      for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
        coords.push_back(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
      geom = new Linestring_Geometry(coords);
    }
    else
    {
      Partial_Way_Geometry* pw_geom = new Partial_Way_Geometry();
      geom = pw_geom;
      for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
      {
        if (it->ll_upper != 0 || it->ll_lower != 0)
          pw_geom->add_point(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
        else
          pw_geom->add_point(Point_Double(100., 200.));
      }
    }
  }
  else if (store && ((mode & Output_Mode::BOUNDS) || (mode & Output_Mode::CENTER)))
  {
    std::vector< Quad_Coord > geometry = store->get_geometry(skel);

    double min_lat = 100.;
    double max_lat = -100.;
    double min_lon = 200.;
    double max_lon = -200.;

    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    {
      double lat = ::lat(it->ll_upper, it->ll_lower);
      min_lat = std::min(min_lat, lat);
      max_lat = std::max(max_lat, lat);
      double lon = ::lon(it->ll_upper, it->ll_lower);
      min_lon = std::min(min_lon, lon);
      max_lon = std::max(max_lon, lon);
    }

    if (mode & Output_Mode::BOUNDS)
      geom = new Bbox_Geometry(min_lat, min_lon, max_lat, max_lon);
    else
      geom = new Point_Geometry((min_lat + max_lat) / 2., (min_lon + max_lon) / 2.);
  }
  else
    geom = new Null_Geometry();

  return *geom;
}


const Opaque_Geometry& Geometry_Broker::make_relation_geom(
    const Relation_Skeleton& skel, unsigned int mode, Relation_Geometry_Store* store)
{
  delete geom;
  geom = 0;

  if (store && (mode & Output_Mode::GEOMETRY))
  {
    std::vector< std::vector< Quad_Coord > > geometry = store->get_geometry(skel);

    bool is_complete = true;
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->empty())
        is_complete = false;
      else if (it->size() == 1)
        is_complete &= ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0);
      else
      {
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          is_complete &= (it2->ll_upper != 0 || it2->ll_lower != 0);
      }
    }

    if (is_complete)
    {
      Compound_Geometry* cp_geom = new Compound_Geometry();
      geom = cp_geom;
      for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
          it != geometry.end(); ++it)
      {
        if (it->empty())
          cp_geom->add_component(new Null_Geometry());
        else if (it->size() == 1)
          cp_geom->add_component(new Point_Geometry(
              ::lat(it->front().ll_upper, it->front().ll_lower),
              ::lon(it->front().ll_upper, it->front().ll_lower)));
        else
        {
          std::vector< Point_Double > coords;
          for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
            coords.push_back(Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
          cp_geom->add_component(new Linestring_Geometry(coords));
        }
      }
    }
    else if (geometry.empty())
      geom = new Null_Geometry();
    else
    {
      Partial_Relation_Geometry* pr_geom = new Partial_Relation_Geometry();
      geom = pr_geom;
      for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
          it != geometry.end(); ++it)
      {
        if (it->empty())
          pr_geom->add_placeholder();
        else if (it->size() == 1 && ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0))
          pr_geom->add_point(Point_Double(
                ::lat(it->front().ll_upper, it->front().ll_lower),
                ::lon(it->front().ll_upper, it->front().ll_lower)));
        else
        {
          pr_geom->start_way();
          for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          {
            if (it2->ll_upper != 0 || it2->ll_lower != 0)
              pr_geom->add_way_point(
                  Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
            else
              pr_geom->add_way_placeholder();
          }
        }
      }
    }
  }
  else if (store && ((mode & Output_Mode::BOUNDS) || (mode & Output_Mode::CENTER)))
  {
    std::vector< std::vector< Quad_Coord > > geometry = store->get_geometry(skel);

    double min_lat = 100.;
    double max_lat = -100.;
    double min_lon = 200.;
    double max_lon = -200.;

    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->size() == 1)
      {
        double lat = ::lat((*it)[0].ll_upper, (*it)[0].ll_lower);
        min_lat = std::min(min_lat, lat);
        max_lat = std::max(max_lat, lat);
        double lon = ::lon((*it)[0].ll_upper, (*it)[0].ll_lower);
        min_lon = std::min(min_lon, lon);
        max_lon = std::max(max_lon, lon);
      }
      else if (!it->empty())
      {
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        {
          double lat = ::lat(it2->ll_upper, it2->ll_lower);
          min_lat = std::min(min_lat, lat);
          max_lat = std::max(max_lat, lat);
          double lon = ::lon(it2->ll_upper, it2->ll_lower);
          min_lon = std::min(min_lon, lon);
          max_lon = std::max(max_lon, lon);
        }
      }
    }

    if (mode & Output_Mode::BOUNDS)
      geom = new Bbox_Geometry(min_lat, min_lon, max_lat, max_lon);
    else
      geom = new Point_Geometry((min_lat + max_lat) / 2., (min_lon + max_lon) / 2.);
  }
  else
    geom = new Null_Geometry();

  return *geom;
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
  Geometry_Broker broker;
  output.print_item(skel,
      broker.make_way_geom(skel, extra_data.mode, extra_data.way_geometry_store),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0)
{
  Geometry_Broker broker;
  output.print_item(skel,
      broker.make_way_geom(skel, extra_data.mode, extra_data.attic_way_geometry_store),
      tags, meta, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0)
{
  Geometry_Broker broker;
  output.print_item(skel,
      broker.make_relation_geom(skel, extra_data.mode, extra_data.relation_geometry_store),
      tags, meta, extra_data.roles, extra_data.get_users(), Output_Mode(extra_data.mode));
}


void print_item(Extra_Data& extra_data, Output_Handler& output, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0)
{
  Geometry_Broker broker;
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

  std::map< std::string, Set >::const_iterator mit(rman.sets().find(input));

  Set count_set;
  const Set* output_items = 0;
  if (mode & Output_Mode::COUNT)
  {
    count_set.deriveds[Uint31_Index(0u)].push_back(Derived_Structure("count", Uint64(0ull),
        make_count_tags(mit == rman.sets().end() ? Set() : mit->second, rman.get_area_transaction())));
    output_items = &count_set;
    mode = mode | Output_Mode::TAGS;
  }
  else
  {
    if (mit == rman.sets().end())
      return;

    output_items = &mit->second;
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


const Opaque_Geometry& Geometry_Broker::make_way_geom(
    const std::vector< Quad_Coord >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds)
{
  delete geom;
  geom = 0;

  if (geometry)
  {
    bool is_complete = true;
    for (std::vector< Quad_Coord >::const_iterator it = geometry->begin(); it != geometry->end(); ++it)
      is_complete &= (it->ll_upper != 0 || it->ll_lower != 0);

    if (is_complete)
    {
      std::vector< Point_Double > coords;
      for (std::vector< Quad_Coord >::const_iterator it = geometry->begin(); it != geometry->end(); ++it)
        coords.push_back(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
      geom = new Linestring_Geometry(coords);
    }
    else
    {
      Partial_Way_Geometry* pw_geom = new Partial_Way_Geometry();
      geom = pw_geom;
      for (std::vector< Quad_Coord >::const_iterator it = geometry->begin(); it != geometry->end(); ++it)
      {
        if (it->ll_upper != 0 || it->ll_lower != 0)
          pw_geom->add_point(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
        else
          pw_geom->add_point(Point_Double(100., 200.));
      }
    }
  }
  else if (bounds)
  {
    if (bounds->second)
      geom = new Bbox_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
                           ::lon(bounds->first.ll_upper, bounds->first.ll_lower),
                           ::lat(bounds->second->ll_upper, bounds->second->ll_lower),
                           ::lon(bounds->second->ll_upper, bounds->second->ll_lower));
    else
      geom = new Point_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
                            ::lon(bounds->first.ll_upper, bounds->first.ll_lower));
  }
  else
    geom = new Null_Geometry();

  return *geom;
}


const Opaque_Geometry& Geometry_Broker::make_relation_geom(
    const std::vector< std::vector< Quad_Coord > >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds)
{
  delete geom;
  geom = 0;

  if (geometry)
  {
    bool is_complete = true;
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry->begin();
        it != geometry->end(); ++it)
    {
      if (it->empty())
        is_complete = false;
      else if (it->size() == 1)
        is_complete &= ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0);
      else
      {
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          is_complete &= (it2->ll_upper != 0 || it2->ll_lower != 0);
      }
    }

    if (is_complete)
    {
      Compound_Geometry* cp_geom = new Compound_Geometry();
      geom = cp_geom;
      for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry->begin();
          it != geometry->end(); ++it)
      {
        if (it->empty())
          cp_geom->add_component(new Null_Geometry());
        else if (it->size() == 1)
          cp_geom->add_component(new Point_Geometry(
              ::lat(it->front().ll_upper, it->front().ll_lower),
              ::lon(it->front().ll_upper, it->front().ll_lower)));
        else
        {
          std::vector< Point_Double > coords;
          for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
            coords.push_back(Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
          cp_geom->add_component(new Linestring_Geometry(coords));
        }
      }
    }
    else if (geometry->empty())
      geom = new Null_Geometry();
    else
    {
      Partial_Relation_Geometry* pr_geom = new Partial_Relation_Geometry();
      geom = pr_geom;
      for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry->begin();
          it != geometry->end(); ++it)
      {
        if (it->empty())
          pr_geom->add_placeholder();
        else if (it->size() == 1 && ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0))
          pr_geom->add_point(Point_Double(
                ::lat(it->front().ll_upper, it->front().ll_lower),
                ::lon(it->front().ll_upper, it->front().ll_lower)));
        else
        {
          pr_geom->start_way();
          for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          {
            if (it2->ll_upper != 0 || it2->ll_lower != 0)
              pr_geom->add_way_point(
                  Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
            else
              pr_geom->add_way_placeholder();
          }
        }
      }
    }
  }
  else if (bounds)
  {
    if (bounds->second)
      geom = new Bbox_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
          ::lon(bounds->first.ll_upper, bounds->first.ll_lower),
          ::lat(bounds->second->ll_upper, bounds->second->ll_lower),
          ::lon(bounds->second->ll_upper, bounds->second->ll_lower));
    else
      geom = new Point_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
          ::lon(bounds->first.ll_upper, bounds->first.ll_lower));
  }
  else
    geom = new Null_Geometry();

  return *geom;
}


template < typename T >
struct Optional
{
  Optional(T* obj_) : obj(obj_) {}
  ~Optional() { delete obj; }

  T* obj;
};


class Collection_Print_Target
{
  public:
    Collection_Print_Target(uint32 mode_, Transaction& transaction,
        Extra_Data* extra_data_, Output_Handler* output_)
        : final_target(0), extra_data(extra_data_), output(output_),
        output_mode(mode_) {}

    virtual ~Collection_Print_Target() {}

    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0);

    void set_target(bool target);

    void clear_nodes(Resource_Manager& rman, bool add_deletion_information = false);
    void clear_ways(Resource_Manager& rman, bool add_deletion_information = false);
    void clear_relations(Resource_Manager& rman, bool add_deletion_information = false);

  private:
    
    typedef std::vector< std::pair< std::string, std::string > > Tag_Container;

    struct Node_Entry
    {
      Uint31_Index idx;
      Node_Skeleton elem;
      OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta;
      Tag_Container tags;

      Node_Entry(Uint31_Index idx_, Node_Skeleton elem_,
            OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta_
                = OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
            Tag_Container tags_
                = Tag_Container())
          : idx(idx_), elem(elem_), meta(meta_), tags(tags_) {}

      bool operator<(const Node_Entry& e) const
      {
        if (this->elem.id < e.elem.id)
          return true;
        if (e.elem.id < this->elem.id)
          return false;
        return (this->meta.version < e.meta.version);
      }
    };

    struct Way_Entry
    {
      Uint31_Index idx;
      Way_Skeleton elem;
      OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
      Tag_Container tags;
      std::vector< Quad_Coord > geometry;

      Way_Entry(Uint31_Index idx_, Way_Skeleton elem_,
            const std::vector< Quad_Coord >& geometry_,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta_
                = OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
            Tag_Container tags_
                = Tag_Container())
          : idx(idx_), elem(elem_), meta(meta_), tags(tags_), geometry(geometry_) {}

      bool operator<(const Way_Entry& e) const
      {
        if (this->elem.id < e.elem.id)
          return true;
        if (e.elem.id < this->elem.id)
          return false;
        return (this->meta.version < e.meta.version);
      }
    };

    struct Relation_Entry
    {
      Uint31_Index idx;
      Relation_Skeleton elem;
      OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > meta;
      Tag_Container tags;
      std::vector< std::vector< Quad_Coord > > geometry;

      Relation_Entry(Uint31_Index idx_, Relation_Skeleton elem_,
            const std::vector< std::vector< Quad_Coord > >& geometry_,
            OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > meta_
                = OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
            Tag_Container tags_
                = Tag_Container())
          : idx(idx_), elem(elem_), meta(meta_), tags(tags_), geometry(geometry_) {}

      bool operator<(const Relation_Entry& e) const
      {
        if (this->elem.id < e.elem.id)
          return true;
        if (e.elem.id < this->elem.id)
          return false;
        return (this->meta.version < e.meta.version);
      }
    };

    bool final_target;
    std::vector< Node_Entry > nodes;
    std::vector< Way_Entry > ways;
    std::vector< Relation_Entry > relations;
    std::vector< std::pair< Node_Entry, Node_Entry > > different_nodes;
    std::vector< std::pair< Way_Entry, Way_Entry > > different_ways;
    std::vector< std::pair< Relation_Entry, Relation_Entry > > different_relations;
    Extra_Data* extra_data;
    Output_Handler* output;
    Output_Mode output_mode;
};


void Collection_Print_Target::set_target(bool target)
{
  final_target = target;
  std::sort(nodes.begin(), nodes.end());
  std::sort(ways.begin(), ways.end());
  std::sort(relations.begin(), relations.end());
}


void print_item(Extra_Data& extra_data, Collection_Print_Target& target, uint32 ll_upper, const Node_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta = 0,
                    const std::map< uint32, std::string >* users = 0)
{
  target.print_item(ll_upper, skel, tags, meta, users);
}


struct Double_Coords
{
public:
  explicit Double_Coords(const std::vector< Quad_Coord >& geometry)
    : min_lat(100.0), max_lat(-100.0), min_lon(200.0), max_lon(-200.0)
  {
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    {
      if (it->ll_upper != 0 || it->ll_lower != 0)
      {
        double lat = ::lat(it->ll_upper, it->ll_lower);
        double lon = ::lon(it->ll_upper, it->ll_lower);
        min_lat = std::min(min_lat, lat);
        max_lat = std::max(max_lat, lat);
        min_lon = std::min(min_lon, lon);
        max_lon = std::max(max_lon, lon);
      }
    }
  }

  explicit Double_Coords(const std::vector< std::vector< Quad_Coord > >& geometry)
    : min_lat(100.0), max_lat(-100.0), min_lon(200.0), max_lon(-200.0)
  {
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
         it != geometry.end(); ++it)
    {
      for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
      {
        if (it2->ll_upper != 0 || it2->ll_lower != 0)
        {
          double lat = ::lat(it2->ll_upper, it2->ll_lower);
          double lon = ::lon(it2->ll_upper, it2->ll_lower);
          min_lat = std::min(min_lat, lat);
          max_lat = std::max(max_lat, lat);
          min_lon = std::min(min_lon, lon);
          max_lon = std::max(max_lon, lon);
        }
      }
    }
  }

  const std::pair< Quad_Coord, Quad_Coord* >& bounds()
  {
    if (max_lat > -100.0)
    {
      max = Quad_Coord(::ll_upper_(max_lat, max_lon), ::ll_lower(max_lat, max_lon));
      bounds_ = std::make_pair(Quad_Coord(::ll_upper_(min_lat, min_lon), ::ll_lower(min_lat, min_lon)), &max);
    }
    else
    {
      max = Quad_Coord(0u, 0u);
      bounds_ = std::make_pair(Quad_Coord(0u, 0u), &max);
    }
    return bounds_;
  }

  const std::pair< Quad_Coord, Quad_Coord* >& center()
  {
    if (max_lat > -100.0)
      center_ = std::make_pair(Quad_Coord(
          ::ll_upper_((min_lat + max_lat) / 2, (min_lon + max_lon) / 2),
          ::ll_lower((min_lat + max_lat) / 2, (min_lon + max_lon) / 2)
          ), (Quad_Coord*)0);
    else
      center_ = std::make_pair(Quad_Coord(0u, 0u), (Quad_Coord*)0);
    return center_;
  }

private:
  double min_lat;
  double max_lat;
  double min_lon;
  double max_lon;
  std::pair< Quad_Coord, Quad_Coord* > bounds_;
  std::pair< Quad_Coord, Quad_Coord* > center_;
  Quad_Coord max;
};


const std::pair< Quad_Coord, Quad_Coord* >* bound_variant(Double_Coords& double_coords, unsigned int mode)
{
  if (mode & Output_Mode::BOUNDS)
    return &double_coords.bounds();
  else if (mode & Output_Mode::CENTER)
    return &double_coords.center();

  return 0;
}


void print_item(Extra_Data& extra_data, Collection_Print_Target& target, uint32 ll_upper, const Way_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0,
                    const std::map< uint32, std::string >* users = 0)
{
  if (extra_data.way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = extra_data.way_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void print_item(Extra_Data& extra_data, Collection_Print_Target& target, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta = 0,
                    const std::map< uint32, std::string >* users = 0)
{
  if (extra_data.attic_way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = extra_data.attic_way_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void print_item(Extra_Data& extra_data, Collection_Print_Target& target, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0,
                    const std::map< uint32, std::string >* users = 0)
{
  if (extra_data.relation_geometry_store)
  {
    std::vector< std::vector< Quad_Coord > > geometry = extra_data.relation_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void print_item(Extra_Data& extra_data, Collection_Print_Target& target, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags = 0,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta = 0,
                    const std::map< uint32, std::string >* users = 0)
{
  if (extra_data.attic_relation_geometry_store)
  {
    std::vector< std::vector< Quad_Coord > > geometry = extra_data.attic_relation_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


template< class TIndex, class TObject >
void quadtile
    (const std::map< TIndex, std::vector< TObject > >& items, Collection_Print_Target& target,
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
      print_item(extra_data, target, item_it->first.val(), *it2);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void tags_quadtile
    (Extra_Data& extra_data, const std::map< Index, std::vector< Object > >& items,
     Collection_Print_Target& target,
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
      print_item(extra_data, target, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
          meta_printer.get(item_it->first, it2->id), extra_data.users);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void tags_quadtile_attic
    (Extra_Data& extra_data, const std::map< Index, std::vector< Attic< Object > > >& items,
     Collection_Print_Target& target,
     Resource_Manager& rman, Transaction& transaction, uint32 limit, uint32& element_count)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (items, transaction,
      (extra_data.mode & Output_Mode::META) ? current_meta_file_properties< Object >() : 0);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (items, transaction,
      (extra_data.mode & Output_Mode::META) ? attic_meta_file_properties< Object >() : 0);

  typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      item_it(items.begin());
  while (item_it != items.end())
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++element_count > limit)
        return;
      const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
          = attic_meta_printer.get(item_it->first, it2->id, it2->timestamp);
      if (!meta)
        meta = current_meta_printer.get(item_it->first, it2->id, it2->timestamp);
      print_item(extra_data, target, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
                 meta, extra_data.users);
    }
    ++item_it;
  }
}


template< typename Index, typename Skeleton >
std::vector< typename Skeleton::Id_Type > find_still_existing_skeletons
    (Resource_Manager& rman, const std::vector< Index >& req,
     const std::vector< typename Skeleton::Id_Type >& searched_ids)
{
  std::vector< typename Skeleton::Id_Type > found_ids;
  std::map< Index, std::vector< Skeleton > > current_result;
  std::map< Index, std::vector< Attic< Skeleton > > > attic_result;
  if (rman.get_desired_timestamp() == NOW)
    collect_items_discrete(0, rman, *current_skeleton_file_properties< Skeleton >(), req,
        Id_Predicate< Skeleton >(searched_ids), current_result);
  else
  {
    collect_items_discrete_by_timestamp(0, rman, req,
        Id_Predicate< Skeleton >(searched_ids), current_result, attic_result);
    filter_attic_elements(rman, rman.get_desired_timestamp(), current_result, attic_result);
  }
  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current_result.begin();
       it != current_result.end(); ++it)
  {
    for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      found_ids.push_back(it2->id);
  }
  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic_result.begin();
       it != attic_result.end(); ++it)
  {
    for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
	 it2 != it->second.end(); ++it2)
      found_ids.push_back(it2->id);
  }
  std::sort(found_ids.begin(), found_ids.end());
  found_ids.erase(std::unique(found_ids.begin(), found_ids.end()), found_ids.end());

  return found_ids;
}


template< typename Index, typename Skeleton >
std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
    find_meta_elements
    (Resource_Manager& rman, const std::vector< Index >& idx_set,
     const std::vector< typename Skeleton::Id_Type >& searched_ids)
{
  std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > result;
  uint64 timestamp = rman.get_desired_timestamp();

  Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
        typename std::vector< Index >::const_iterator >
      attic_meta_db(rman.get_transaction()->data_index(attic_meta_file_properties< Skeleton >()));
  for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
          typename std::vector< Index >::const_iterator >::Discrete_Iterator
      it = attic_meta_db.discrete_begin(idx_set.begin(), idx_set.end());
      !(it == attic_meta_db.discrete_end()); ++it)
  {
    if (!(timestamp < it.object().timestamp)
        && std::binary_search(searched_ids.begin(), searched_ids.end(), it.object().ref))
    {
      typename std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
          ::iterator meta_it = result.find(it.object().ref);
      if (meta_it == result.end())
	result.insert(std::make_pair(it.object().ref, it.object()));
      else if (meta_it->second.timestamp < it.object().timestamp)
	meta_it->second = it.object();
    }
  }

  // Same thing with current meta data
  Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
          typename std::vector< Index >::const_iterator >
      meta_db(rman.get_transaction()->data_index(current_meta_file_properties< Skeleton >()));

  for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
          typename std::vector< Index >::const_iterator >::Discrete_Iterator
      it = meta_db.discrete_begin(idx_set.begin(), idx_set.end());
      !(it == meta_db.discrete_end()); ++it)
  {
    if (!(timestamp < it.object().timestamp)
        && std::binary_search(searched_ids.begin(), searched_ids.end(), it.object().ref))
    {
      typename std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
          ::iterator meta_it = result.find(it.object().ref);
      if (meta_it == result.end())
	result.insert(std::make_pair(it.object().ref, it.object()));
      else if (meta_it->second.timestamp < it.object().timestamp)
	meta_it->second = it.object();
    }
  }

  return result;
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
{
  if (final_target)
  {
    std::vector< Node_Entry >::iterator nodes_it
        = std::lower_bound(nodes.begin(), nodes.end(), Node_Entry(ll_upper, skel));

    if (nodes_it == nodes.end() || skel.id < nodes_it->elem.id)
      different_nodes.push_back(std::make_pair(
	  Node_Entry(0xffu, Node_Skeleton(skel.id),
	      OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Node_Entry(ll_upper, skel,
              meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
    else
    {
      if (!(nodes_it->idx.val() == ll_upper) || !(nodes_it->elem.ll_lower == skel.ll_lower) ||
          (tags && !(nodes_it->tags == *tags)) || (meta && !(nodes_it->meta.timestamp == meta->timestamp)))
	different_nodes.push_back(std::make_pair(*nodes_it, Node_Entry(ll_upper, skel,
                  meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
                  tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
      nodes_it->idx = 0xffu;
    }
  }
  else
    nodes.push_back(Node_Entry(ll_upper, skel,
        meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Collection_Print_Target::clear_nodes(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Node_Skeleton::Id_Type > searched_ids;
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	searched_ids.push_back(it->elem.id);
    }

    std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(searched_ids, rman, true);
    std::vector< Node_Skeleton::Id_Type > found_ids
        = find_still_existing_skeletons< Uint32_Index, Node_Skeleton >(rman, req, searched_ids);
    std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > > found_meta
        = find_meta_elements< Uint32_Index, Node_Skeleton >(rman, req, searched_ids);

    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_nodes.push_back(std::make_pair(*it,
	    Node_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfeu : 0xffu,
		Node_Skeleton(it->elem.id),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
  }
  else
  {
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	different_nodes.push_back(std::make_pair(*it,
	    Node_Entry(0xffu, Node_Skeleton(it->elem.id),
	        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }

  std::sort(different_nodes.begin(), different_nodes.end());
  for (std::vector< std::pair< Node_Entry, Node_Entry > >::const_iterator it = different_nodes.begin();
      it != different_nodes.end(); ++it)
  {
    if ((it->second.idx.val() | 1) == 0xffu)
    {
      if (add_deletion_information)
      {
        Node_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
                ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            extra_data->get_users(), output_mode,
            it->second.idx.val() == 0xfeu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
                ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            extra_data->get_users(), output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Point_Geometry new_geom(::lat(it->second.idx.val(), it->second.elem.ll_lower),
              ::lon(it->second.idx.val(), it->second.elem.ll_lower));
      output->print_item(it->first.elem,
          Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
              ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          extra_data->get_users(), output_mode, Output_Handler::modify,
          &it->second.elem, &new_geom,
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0);
    }
    else
      // No old element exists
      output->print_item(it->second.elem,
          Point_Geometry(::lat(it->second.idx.val(), it->second.elem.ll_lower),
              ::lon(it->second.idx.val(), it->second.elem.ll_lower)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          extra_data->get_users(), output_mode, Output_Handler::create);
  }
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta)
{
  if (final_target)
  {
    std::vector< Way_Entry >::iterator ways_it
        = std::lower_bound(ways.begin(), ways.end(), Way_Entry(ll_upper, skel, std::vector< Quad_Coord >()));

    if (ways_it == ways.end() || skel.id < ways_it->elem.id)
      different_ways.push_back(std::make_pair(
	  Way_Entry(0xffu, Way_Skeleton(skel.id),
	      std::vector< Quad_Coord >(),
	      OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Way_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< Quad_Coord >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
    else
    {
      if (!(ways_it->idx.val() == ll_upper) || !(ways_it->elem.nds == skel.nds) ||
          (geometry && !(ways_it->geometry == *geometry)) ||
          (tags && !(ways_it->tags == *tags)) || (meta && !(ways_it->meta.timestamp == meta->timestamp)))
	different_ways.push_back(std::make_pair(*ways_it, Way_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< Quad_Coord >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
      ways_it->idx = 0xffu;
    }
  }
  else
    ways.push_back(Way_Entry(ll_upper, skel,
        geometry ? *geometry : std::vector< Quad_Coord >(),
        meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Collection_Print_Target::clear_ways(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Way_Skeleton::Id_Type > searched_ids;
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	searched_ids.push_back(it->elem.id);
    }

    std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(searched_ids, rman, true);
    std::vector< Way_Skeleton::Id_Type > found_ids
        = find_still_existing_skeletons< Uint31_Index, Way_Skeleton >(rman, req, searched_ids);
    std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > > found_meta
        = find_meta_elements< Uint31_Index, Way_Skeleton >(rman, req, searched_ids);

    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_ways.push_back(std::make_pair(*it,
	    Way_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfeu : 0xffu,
		Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
  }
  else
  {
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	different_ways.push_back(std::make_pair(*it,
	    Way_Entry(0xffu, Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }

  std::sort(different_ways.begin(), different_ways.end());
  for (std::vector< std::pair< Way_Entry, Way_Entry > >::const_iterator it = different_ways.begin();
      it != different_ways.end(); ++it)
  {
    if ((it->second.idx.val() | 1) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      Geometry_Broker broker;
      if (add_deletion_information)
      {
        Way_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            extra_data->get_users(), output_mode,
            it->second.idx.val() == 0xfeu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            extra_data->get_users(), output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      Geometry_Broker broker;
      Geometry_Broker new_broker;
      output->print_item(it->first.elem,
          broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          extra_data->get_users(), output_mode, Output_Handler::modify,
          &it->second.elem,
          &new_broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords_new, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0);
    }
    else
    {
      // No old element exists
      Double_Coords double_coords(it->second.geometry);
      Geometry_Broker broker;
      output->print_item(it->second.elem,
          broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          extra_data->get_users(), output_mode, Output_Handler::create);
    }
  }
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  if (final_target)
  {
    std::vector< Relation_Entry >::iterator relations_it
        = std::lower_bound(relations.begin(), relations.end(),
	    Relation_Entry(ll_upper, skel, std::vector< std::vector< Quad_Coord > >()));

    if (relations_it == relations.end() || skel.id < relations_it->elem.id)
      different_relations.push_back(std::make_pair(
	  Relation_Entry(0xffu, Relation_Skeleton(skel.id),
	      std::vector< std::vector< Quad_Coord > >(),
	      OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Relation_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
    else
    {
      if (!(relations_it->idx.val() == ll_upper) || !(relations_it->elem.members == skel.members) ||
	  (geometry && !(relations_it->geometry == *geometry)) ||
	  (tags && !(relations_it->tags == *tags)) || (meta && !(relations_it->meta.timestamp == meta->timestamp)))
	different_relations.push_back(std::make_pair(*relations_it, Relation_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
      relations_it->idx = 0xffu;
    }
  }
  else
    relations.push_back(Relation_Entry(ll_upper, skel,
        geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
        meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Collection_Print_Target::clear_relations(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Relation_Skeleton::Id_Type > searched_ids;
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	searched_ids.push_back(it->elem.id);
    }

    std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true);
    std::vector< Relation_Skeleton::Id_Type > found_ids
        = find_still_existing_skeletons< Uint31_Index, Relation_Skeleton >(rman, req, searched_ids);
    std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > > found_meta
        = find_meta_elements< Uint31_Index, Relation_Skeleton >(rman, req, searched_ids);

    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_relations.push_back(std::make_pair(*it,
	    Relation_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfeu : 0xffu,
		Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
  }
  else
  {
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	different_relations.push_back(std::make_pair(*it,
	    Relation_Entry(0xffu, Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }

  std::sort(different_relations.begin(), different_relations.end());
  for (std::vector< std::pair< Relation_Entry, Relation_Entry > >::const_iterator it = different_relations.begin();
      it != different_relations.end(); ++it)
  {
    if ((it->second.idx.val() | 1) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      Geometry_Broker broker;
      if (add_deletion_information)
      {
        Relation_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            extra_data->roles, extra_data->get_users(), output_mode,
            it->second.idx.val() == 0xfeu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            extra_data->roles, extra_data->get_users(), output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      Geometry_Broker broker;
      Geometry_Broker new_broker;
      output->print_item(it->first.elem,
          broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          extra_data->roles, extra_data->get_users(), output_mode, Output_Handler::modify,
          &it->second.elem,
          &new_broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords_new, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0);
    }
    else
    {
      // No old element exists
      Double_Coords double_coords(it->second.geometry);
      Geometry_Broker broker;
      output->print_item(it->second.elem,
          broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          extra_data->roles, extra_data->get_users(), output_mode, Output_Handler::create);
    }
  }
}


void Print_Statement::execute_comparison(Resource_Manager& rman)
{
  std::map< std::string, Set >::const_iterator mit(rman.sets().find(input));
  uint32 element_count = 0;
  if (mit == rman.sets().end())
    return;

  Extra_Data extra_data(rman, *this, mit->second, Output_Mode::ID
        | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
        | Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META
        | Output_Mode::GEOMETRY, south, north, west, east);

  if (collection_mode == collect_lhs)
    collection_print_target = new Collection_Print_Target(
        mode, *rman.get_transaction(), &extra_data, rman.get_global_settings().get_output_handler());
  else
    collection_print_target->set_target(true);

  uint32 outer_mode = mode;
  mode = Output_Mode::ID
      | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
      | Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META
      | Output_Mode::GEOMETRY;

  {
    tags_quadtile(extra_data, mit->second.nodes,
		    *collection_print_target, rman, *rman.get_transaction(), limit, element_count);

    if (rman.get_desired_timestamp() != NOW)
      tags_quadtile_attic(extra_data, mit->second.attic_nodes,
                      *collection_print_target, rman, *rman.get_transaction(), limit, element_count);

    if (collection_mode == collect_rhs)
      collection_print_target->clear_nodes(rman, add_deletion_information);

    tags_quadtile(extra_data, mit->second.ways,
		    *collection_print_target, rman, *rman.get_transaction(), limit, element_count);

    if (rman.get_desired_timestamp() != NOW)
      tags_quadtile_attic(extra_data, mit->second.attic_ways,
                      *collection_print_target, rman, *rman.get_transaction(), limit, element_count);

    if (collection_mode == collect_rhs)
      collection_print_target->clear_ways(rman, add_deletion_information);

    tags_quadtile(extra_data, mit->second.relations,
		    *collection_print_target, rman, *rman.get_transaction(), limit, element_count);

    if (rman.get_desired_timestamp() != NOW)
      tags_quadtile_attic(extra_data, mit->second.attic_relations,
                      *collection_print_target, rman, *rman.get_transaction(), limit, element_count);

    if (collection_mode == collect_rhs)
      collection_print_target->clear_relations(rman, add_deletion_information);
  }

  if (collection_mode == collect_lhs)
    mode = outer_mode;

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
