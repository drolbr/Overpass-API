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

#include "../data/bbox_filter.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "dijkstra.h"
#include "set_prop.h"


Generic_Statement_Maker< Dijkstra_Statement > Dijkstra_Statement::statement_maker("dijkstra");


Dijkstra_Statement::Dijkstra_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);
  input = attributes["from"];
}


typedef Point_Double Dijkstra_Vertex;

struct Dijkstra_Link_Info
{
  Dijkstra_Link_Info(double cost_, Point_Double& neighbour_, Uint31_Index idx_, const Derived_Structure* link_)
      : cost(cost_), neighbour(neighbour_), idx(idx_), link(link_) {}

  double cost;
  Point_Double neighbour;
  Uint31_Index idx;
  const Derived_Structure* link;
};

struct Dijkstra_Vertex_Info
{
  Dijkstra_Vertex_Info() : best_dist(DIST_UNKNOWN) {}

  std::vector< Dijkstra_Link_Info > neighbours;
  double best_dist;

  static const double DIST_UNKNOWN = -1;
};


std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info > create_open_list(
    const std::map< Uint31_Index, std::vector< Derived_Structure > >& deriveds)
{
  std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info > result;

  for (std::map< Uint31_Index, std::vector< Derived_Structure > >::const_iterator it_idx = deriveds.begin();
      it_idx != deriveds.end(); ++it_idx)
  {
    for (std::vector< Derived_Structure >::const_iterator it_obj = it_idx->second.begin();
        it_obj != it_idx->second.end(); ++it_obj)
    {
      if (it_obj->type_name == "link")
      {
        const Opaque_Geometry* geom = it_obj->get_geometry();
        if (!geom || !geom->has_line_geometry() || geom->get_line_geometry()->empty())
          continue;

        Dijkstra_Vertex source = geom->get_line_geometry()->front();
        Dijkstra_Vertex sink = geom->get_line_geometry()->back();
        double cost = length(*geom);

        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_tag = it_obj->tags.begin();
            it_tag != it_obj->tags.end(); ++it_tag)
        {
          double speed = 1.;
          if (it_tag->first == "calcspeed" && try_double(it_tag->second, speed))
            cost /= speed;
        }

        result[source].neighbours.push_back(Dijkstra_Link_Info(cost, sink, it_idx->first, &*it_obj));
        result[sink].neighbours.push_back(Dijkstra_Link_Info(cost, source, it_idx->first, &*it_obj));
      }
      else if (it_obj->type_name == "vertex")
      {
        const Opaque_Geometry* geom = it_obj->get_geometry();
        if (!geom || !geom->has_center())
          continue;

        Dijkstra_Vertex source(geom->center_lat(), geom->center_lon());
        result[source].best_dist = 0;
      }
    }
  }

  return result;
}


void dump_open_list(const std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info >& open_list)
{
  for (std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info >::const_iterator it = open_list.begin();
      it != open_list.end(); ++it)
    std::cout<<"A\t"<<it->first.lat<<" "<<it->first.lon<<'\t'<<it->second.best_dist<<'\t'<<it->second.neighbours.size()<<'\n';
}


void Dijkstra_Statement::execute(Resource_Manager& rman)
{
  Set into;

  const Set* input_set = rman.get_set(input);
  if (!input_set)
  {
    transfer_output(rman, into);
    return;
  }

  Requested_Context requested_context;
  requested_context.add_usage(input, Set_Usage::TAGS);
  Prepare_Task_Context context(requested_context, *this, rman);
  Set_With_Context* context_from = context.get_set(input);

  if (context_from && context_from->base)
  {
    std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info > open_list = create_open_list(context_from->base->deriveds);
    //dump_open_list(open_list);

    double closure_dist = -1;
    while (true)
    {
      double next_dist = 1e12;
      for (std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info >::iterator it = open_list.begin();
          it != open_list.end(); ++it)
      {
        if (it->second.best_dist != Dijkstra_Vertex_Info::DIST_UNKNOWN
            && it->second.best_dist > closure_dist && it->second.best_dist < next_dist)
          next_dist = it->second.best_dist;
      }
      if (next_dist == 1e12)
        break;

      //std::cout<<"B\t"<<next_dist<<'\n';
      for (std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info >::iterator it = open_list.begin();
          it != open_list.end(); ++it)
      {
        if (it->second.best_dist == next_dist)
        {
          for (std::vector< Dijkstra_Link_Info >::const_iterator
              it_nb = it->second.neighbours.begin(); it_nb != it->second.neighbours.end(); ++it_nb)
          {
            std::map< Dijkstra_Vertex, Dijkstra_Vertex_Info >::iterator it_nbv = open_list.find(it_nb->neighbour);
            //std::cout<<"C\t"<<it->first.lat<<' '<<it->first.lon<<'\t'
            //    <<it_nbv->first.lat<<' '<<it_nbv->first.lon<<'\n';
            if (it_nbv->second.best_dist == Dijkstra_Vertex_Info::DIST_UNKNOWN
                || it_nbv->second.best_dist > it->second.best_dist + it_nb->cost)
              it_nbv->second.best_dist = it->second.best_dist + it_nb->cost;
            if (it->second.best_dist < it_nbv->second.best_dist)
            {
              Derived_Structure new_link(*it_nb->link);
              new_link.tags.push_back(std::make_pair("distance", to_string(it->second.best_dist)));
              into.deriveds[it_nb->idx].push_back(new_link);
            }
          }
        }
      }

      closure_dist = next_dist;
    }
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}
