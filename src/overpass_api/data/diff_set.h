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

#ifndef DE__OSM3S___OVERPASS_API__DATA__DIFF_SET_H
#define DE__OSM3S___OVERPASS_API__DATA__DIFF_SET_H


#include "../core/datatypes.h"

#include <map>
#include <string>
#include <utility>
#include <vector>


typedef std::vector< std::pair< std::string, std::string > > Tag_Container;


struct Node_With_Context
{
  Uint31_Index idx;
  Node_Skeleton elem;
  uint64 expiration_date;
  OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta;
  Tag_Container tags;

  Node_With_Context(Uint31_Index idx_, Node_Skeleton elem_,
      uint64 expiration_date_, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta_
          = OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
      Tag_Container tags_ = Tag_Container())
  : idx(idx_), elem(elem_), expiration_date(expiration_date_), meta(meta_), tags(tags_) {}

  bool operator<(const Node_With_Context& e) const
  {
    if (this->elem.id < e.elem.id)
      return true;
    if (e.elem.id < this->elem.id)
      return false;
    return (this->meta.version < e.meta.version);
  }
};


struct Way_With_Context
{
  Uint31_Index idx;
  Way_Skeleton elem;
  uint64 expiration_date;
  OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
  Tag_Container tags;
  std::vector< Quad_Coord > geometry;

  Way_With_Context(Uint31_Index idx_, Way_Skeleton elem_, const std::vector< Quad_Coord >& geometry_,
      uint64 expiration_date_, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta_
          = OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
      Tag_Container tags_ = Tag_Container())
  : idx(idx_), elem(elem_), expiration_date(expiration_date_), meta(meta_), tags(tags_), geometry(geometry_) {}

  bool operator<(const Way_With_Context& e) const
  {
    if (this->elem.id < e.elem.id)
      return true;
    if (e.elem.id < this->elem.id)
      return false;
    return (this->meta.version < e.meta.version);
  }
};


struct Relation_With_Context
{
  Uint31_Index idx;
  Relation_Skeleton elem;
  uint64 expiration_date;
  OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > meta;
  Tag_Container tags;
  std::vector< std::vector< Quad_Coord > > geometry;

  Relation_With_Context(Uint31_Index idx_, Relation_Skeleton elem_,
      const std::vector< std::vector< Quad_Coord > >& geometry_,
      uint64 expiration_date_, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > meta_
          = OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
      Tag_Container tags_ = Tag_Container())
  : idx(idx_), elem(elem_), expiration_date(expiration_date_), meta(meta_), tags(tags_), geometry(geometry_) {}

  bool operator<(const Relation_With_Context& e) const
  {
    if (this->elem.id < e.elem.id)
      return true;
    if (e.elem.id < this->elem.id)
      return false;
    return (this->meta.version < e.meta.version);
  }
};


struct Diff_Set
{
  std::vector< std::pair< Node_With_Context, Node_With_Context > > different_nodes;
  std::vector< std::pair< Way_With_Context, Way_With_Context > > different_ways;
  std::vector< std::pair< Relation_With_Context, Relation_With_Context > > different_relations;
  std::vector< Derived_Structure > lhs_deriveds;
  std::vector< Derived_Structure > rhs_deriveds;

  void clear()
  {
    different_nodes.clear();
    different_ways.clear();
    different_relations.clear();
  }

  const Diff_Set& swap(Diff_Set& rhs)
  {
    different_nodes.swap(rhs.different_nodes);
    different_ways.swap(rhs.different_ways);
    different_relations.swap(rhs.different_relations);
    lhs_deriveds.swap(rhs.lhs_deriveds);
    rhs_deriveds.swap(rhs.rhs_deriveds);

    return *this;
  }

  Set make_from_set() const;
  Set make_to_set() const;
};


class Output_Handler;


void print_diff_set(const Diff_Set& result,
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, const std::map< uint32, std::string >& roles,
    bool add_deletion_information);


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


const std::pair< Quad_Coord, Quad_Coord* >* bound_variant(Double_Coords& double_coords, unsigned int mode);


#endif
