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

#include "diff_set.h"
#include "geometry_from_quad_coords.h"
#include "../frontend/output_handler.h"


Set Diff_Set::make_from_set() const
{
  Set result;
  
  for (std::vector< std::pair< Node_With_Context, Node_With_Context > >::const_iterator
      it = different_nodes.begin(); it != different_nodes.end(); ++it)
  {
    if (it->first.idx.val() != 0xffu)
    {
      if (it->first.expiration_date == NOW)
        result.nodes[it->first.idx].push_back(it->first.elem);
      else
        result.attic_nodes[it->first.idx].push_back(
            Attic< Node_Skeleton >(it->first.elem, it->first.expiration_date));
    }
  }
  
  for (std::vector< std::pair< Way_With_Context, Way_With_Context > >::const_iterator it = different_ways.begin();
      it != different_ways.end(); ++it)
  {
    if (it->first.idx.val() != 0xffu)
    {
      if (it->first.expiration_date == NOW)
        result.ways[it->first.idx].push_back(it->first.elem);
      else
        result.attic_ways[it->first.idx].push_back(
            Attic< Way_Skeleton >(it->first.elem, it->first.expiration_date));
    }
  }
  
  for (std::vector< std::pair< Relation_With_Context, Relation_With_Context > >::const_iterator it = different_relations.begin();
      it != different_relations.end(); ++it)
  {
    if (it->first.idx.val() != 0xffu)
    {
      if (it->first.expiration_date == NOW)
        result.relations[it->first.idx].push_back(it->first.elem);
      else
        result.attic_relations[it->first.idx].push_back(
            Attic< Relation_Skeleton >(it->first.elem, it->first.expiration_date));
    }
  }
  
  return result;
}


Set Diff_Set::make_to_set() const
{
  Set result;
  
  for (std::vector< std::pair< Node_With_Context, Node_With_Context > >::const_iterator
      it = different_nodes.begin(); it != different_nodes.end(); ++it)
  {
    if ((it->second.idx.val() | 2) != 0xffu)
    {
      if (it->second.expiration_date == NOW)
        result.nodes[it->second.idx].push_back(it->second.elem);
      else
        result.attic_nodes[it->second.idx].push_back(
            Attic< Node_Skeleton >(it->second.elem, it->second.expiration_date));
    }
  }
  
  for (std::vector< std::pair< Way_With_Context, Way_With_Context > >::const_iterator it = different_ways.begin();
      it != different_ways.end(); ++it)
  {
    if ((it->second.idx.val() | 2) != 0xffu)
    {
      if (it->second.expiration_date == NOW)
        result.ways[it->second.idx].push_back(it->second.elem);
      else
        result.attic_ways[it->second.idx].push_back(
            Attic< Way_Skeleton >(it->second.elem, it->second.expiration_date));
    }
  }
  
  for (std::vector< std::pair< Relation_With_Context, Relation_With_Context > >::const_iterator it = different_relations.begin();
      it != different_relations.end(); ++it)
  {
    if ((it->second.idx.val() | 2) != 0xffu)
    {
      if (it->second.expiration_date == NOW)
        result.relations[it->second.idx].push_back(it->second.elem);
      else
        result.attic_relations[it->second.idx].push_back(
            Attic< Relation_Skeleton >(it->second.elem, it->second.expiration_date));
    }
  }
  
  return result;
}


const std::pair< Quad_Coord, Quad_Coord* >* bound_variant(Double_Coords& double_coords, unsigned int mode)
{
  if (mode & Output_Mode::BOUNDS)
    return &double_coords.bounds();
  else if (mode & Output_Mode::CENTER)
    return &double_coords.center();

  return 0;
}


void print_nodes(const std::vector< std::pair< Node_With_Context, Node_With_Context > >& different_nodes,
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, bool add_deletion_information)
{
  for (std::vector< std::pair< Node_With_Context, Node_With_Context > >::const_iterator
      it = different_nodes.begin(); it != different_nodes.end(); ++it)
  {
    if ((it->second.idx.val() | 2) == 0xffu)
    {
      if (add_deletion_information)
      {
        Node_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
                ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode,
            it->second.idx.val() == 0xfdu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
                ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Null_Geometry null_geom;
      Point_Geometry old_geom(::lat(it->first.idx.val(), it->first.elem.ll_lower),
          ::lon(it->first.idx.val(), it->first.elem.ll_lower));
      Point_Geometry new_geom(::lat(it->second.idx.val(), it->second.elem.ll_lower),
          ::lon(it->second.idx.val(), it->second.elem.ll_lower));
      Opaque_Geometry* old_opaque = &null_geom;
      if (it->first.idx.val() != 0xfdu)
        old_opaque = &old_geom;
      output->print_item(it->first.elem, *old_opaque,
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          &users, output_mode, Output_Handler::modify,
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
          &users, output_mode, Output_Handler::create);
  }
}


void print_ways(const std::vector< std::pair< Way_With_Context, Way_With_Context > >& different_ways,
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, bool add_deletion_information)
{
  for (std::vector< std::pair< Way_With_Context, Way_With_Context > >::const_iterator it = different_ways.begin();
      it != different_ways.end(); ++it)
  {
    if ((it->second.idx.val() | 2) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      Geometry_From_Quad_Coords broker;
      if (add_deletion_information)
      {
        Way_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode,
            it->second.idx.val() == 0xfdu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      Geometry_From_Quad_Coords broker;
      Geometry_From_Quad_Coords new_broker;
      output->print_item(it->first.elem,
          broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          &users, output_mode, Output_Handler::modify,
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
      Geometry_From_Quad_Coords broker;
      output->print_item(it->second.elem,
          broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          &users, output_mode, Output_Handler::create);
    }
  }
}


void print_relations(
    const std::vector< std::pair< Relation_With_Context, Relation_With_Context > >& different_relations,
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, const std::map< uint32, std::string >& roles,
    bool add_deletion_information)
{
  for (std::vector< std::pair< Relation_With_Context, Relation_With_Context > >::const_iterator it = different_relations.begin();
      it != different_relations.end(); ++it)
  {
    if ((it->second.idx.val() | 2) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      Geometry_From_Quad_Coords broker;
      if (add_deletion_information)
      {
        Relation_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &roles, &users, output_mode,
            it->second.idx.val() == 0xfdu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &roles, &users, output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      Geometry_From_Quad_Coords broker;
      Geometry_From_Quad_Coords new_broker;
      output->print_item(it->first.elem,
          broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          &roles, &users, output_mode, Output_Handler::modify,
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
      Geometry_From_Quad_Coords broker;
      output->print_item(it->second.elem,
          broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          &roles, &users, output_mode, Output_Handler::create);
    }
  }
}


void print_diff_set(const Diff_Set& result,
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, const std::map< uint32, std::string >& roles,
    bool add_deletion_information)
{
  print_nodes(result.different_nodes, output_mode, output, users, add_deletion_information);
  print_ways(result.different_ways, output_mode, output, users, add_deletion_information);
  print_relations(result.different_relations, output_mode, output, users, roles, add_deletion_information);
}
