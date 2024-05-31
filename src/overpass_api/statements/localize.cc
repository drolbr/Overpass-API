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
#include "localize.h"
#include "set_prop.h"


Generic_Statement_Maker< Localize_Statement > Localize_Statement::statement_maker("localize");


Localize_Statement::Localize_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), type(data)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "l";
  attributes["s"] = "";
  attributes["n"] = "";
  attributes["w"] = "";
  attributes["e"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);
  input = attributes["from"];

  if (attributes["type"] == "ll")
    type = also_loose;
  else if (attributes["type"] == "llb")
    type = all;
  else if (attributes["type"] != "l")
    add_static_error("Localize must have one of the values \"l\", \"ll\", or \"llb\". \"l\" is the default value.");

  south = 100.;
  if (!attributes["s"].empty() || !attributes["n"].empty()
      || !attributes["w"].empty() || !attributes["e"].empty())
  {
    south = atof(attributes["s"].c_str());
    if ((south < -90.0) || (south > 90.0) || (attributes["s"] == ""))
      add_static_error("For the attribute \"s\" of the element \"bbox-query\""
          " the only allowed values are floats between -90.0 and 90.0.");

    north = atof(attributes["n"].c_str());
    if ((north < -90.0) || (north > 90.0) || (attributes["n"] == ""))
      add_static_error("For the attribute \"n\" of the element \"bbox-query\""
          " the only allowed values are floats between -90.0 and 90.0.");
    if (north < south)
      add_static_error("The value of attribute \"n\" of the element \"bbox-query\""
          " must always be greater or equal than the value of attribute \"s\".");

    west = atof(attributes["w"].c_str());
    if ((west < -180.0) || (west > 180.0) || (attributes["w"] == ""))
      add_static_error("For the attribute \"w\" of the element \"bbox-query\""
          " the only allowed values are floats between -180.0 and 180.0.");

    east = atof(attributes["e"].c_str());
    if ((east < -180.0) || (east > 180.0) || (attributes["e"] == ""))
      add_static_error("For the attribute \"e\" of the element \"bbox-query\""
          " the only allowed values are floats between -180.0 and 180.0.");
  }
}


class NWR_Context
{
public:
  struct Node_Context
  {
    Node_Context() : local_id(0ull), count(0), lat(100.), lon(0.) {}
    Derived_Skeleton::Id_Type local_id;
    uint count;
    double lat;
    double lon;
    std::map< Node_Skeleton::Id_Type, std::vector< Way_Skeleton::Id_Type > > link_by_origin;
  };

  struct Way_Section_Context
  {
    Way_Section_Context(uint pos_) : pos(pos_), local_id(0ull), local_from(0ull), local_to(0ull) {}
    uint pos;
    Derived_Skeleton::Id_Type local_id;
    Derived_Skeleton::Id_Type local_from;
    Derived_Skeleton::Id_Type local_to;
    std::vector< Point_Double > points;
  };

  NWR_Context(Resource_Manager& rman, const Statement& stmt, const Set& input_set,
      const Owner< Bbox_Double >& bbox);
  void prepare_links(Resource_Manager& rman, const Statement& stmt, const Set& input_set,
      const Owner< Bbox_Double >& bbox);
  Derived_Skeleton::Id_Type local_id_by_node_id(Node_Skeleton::Id_Type id) const;
  Node_Context& context_by_node_id(Node_Skeleton::Id_Type id) { return context_by_node_id_[id]; }
  std::vector< Way_Section_Context >& context_by_way_id(Way_Skeleton::Id_Type id) { return context_by_way_id_[id]; }

private:
  Set relevant_nwrs;
  std::map< Node_Skeleton::Id_Type, Node_Context > context_by_node_id_;
  std::map< Way_Skeleton::Id_Type, std::vector< Way_Section_Context > > context_by_way_id_;
};


struct Node_Skeleton_By_Coord_Then_Id
{
  bool operator()(const Node_Skeleton* lhs, const Node_Skeleton* rhs)
  {
    if (lhs->ll_lower != rhs->ll_lower)
      return lhs->ll_lower < rhs->ll_lower;
    return lhs->id < rhs->id;
  }
};


bool admissible_by_bbox(const Owner< Bbox_Double >& bbox, const NWR_Context::Node_Context& node_context)
{
  return (!bbox || bbox->contains(Point_Double(node_context.lat, node_context.lon)));
}


bool admissible_by_bbox(const Owner< Bbox_Double >& bbox, const std::vector< Point_Double >& points)
{
  if (!bbox)
    return true;

  for (std::vector< Point_Double >::const_iterator it = points.begin(); it != points.end(); ++it)
  {
    if (bbox->contains(*it))
      return true;
  }
  for (uint j = 1; j < points.size(); ++j)
  {
    if (bbox->intersects(points[j-1], points[j]))
      return true;
  }
  return false;
}


template< typename Index, typename Object, typename Task >
void for_each_elem(const std::map< Index, std::vector< Object > >& container, Task task)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it_idx = container.begin(); it_idx != container.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
      task(it_idx->first, *it_elem);
  }
}


struct Set_Coord_In_Context
{
  void operator()(Uint32_Index idx, const Node_Skeleton& node)
  {
    NWR_Context::Node_Context& context = (*context_by_node_id)[node.id];
    context.lat = ::lat(idx.val(), node.ll_lower);
    context.lon = ::lon(idx.val(), node.ll_lower);
  }

  Set_Coord_In_Context(std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_)
    : context_by_node_id(&context_by_node_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
};


struct Count_Node_Use
{
  void operator()(Uint31_Index idx, const Way_Skeleton& way)
  {
    if (!way.nds.empty())
    {
      (*context_by_node_id)[way.nds.front()].count += 2;
      for (uint i = 1; i < way.nds.size()-1; ++i)
        ++(*context_by_node_id)[way.nds[i]].count;
      (*context_by_node_id)[way.nds.back()].count += 2;
    }
  }

  Count_Node_Use(std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_)
    : context_by_node_id(&context_by_node_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
};


struct Partition_Into_Links
{
  void operator()(Uint31_Index idx, const Way_Skeleton& way)
  {
    if (!way.nds.empty())
    {
      std::vector< NWR_Context::Way_Section_Context >& way_context = (*context_by_way_id)[way.id];

      std::vector< NWR_Context::Node_Context* > nds_contexts;
      nds_contexts.reserve(way.nds.size());
      for (std::vector< Node::Id_Type >::const_iterator it_nds = way.nds.begin(); it_nds != way.nds.end(); ++it_nds)
      {
        std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >::iterator it_ctx
            = context_by_node_id->find(*it_nds);
        if (it_ctx == context_by_node_id->end() || it_ctx->second.lat == 100.)
          return;
        nds_contexts.push_back(&it_ctx->second);
      }

      uint start = 0;
      const NWR_Context::Node_Context& start_context = *nds_contexts[start];
      way_context.push_back(NWR_Context::Way_Section_Context(start));
      way_context.back().local_from = start_context.local_id;
      way_context.back().points.push_back(Point_Double(start_context.lat, start_context.lon));

      for (uint i = 1; i < way.nds.size()-1; ++i)
      {
        NWR_Context::Node_Context& context = *nds_contexts[i];
        way_context.back().points.push_back(Point_Double(context.lat, context.lon));

        if (context.local_id.val() || context.count > 1)
        {
          context.link_by_origin[way.nds[start]].push_back(way.id);
          start = i;
          way_context.back().local_to = context.local_id;
          if (!admissible_by_bbox(*bbox, way_context.back().points))
            way_context.back().points.clear();
          way_context.push_back(NWR_Context::Way_Section_Context(start));
          way_context.back().local_from = context.local_id;
          way_context.back().points.push_back(Point_Double(context.lat, context.lon));
        }
      }

      NWR_Context::Node_Context& context = *nds_contexts.back();
      way_context.back().points.push_back(Point_Double(context.lat, context.lon));
      way_context.back().local_to = context.local_id;
      context.link_by_origin[way.nds[start]].push_back(way.id);
      if (!admissible_by_bbox(*bbox, way_context.back().points))
        way_context.back().points.clear();
    }
  }

  Partition_Into_Links(
      std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_,
      std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >& context_by_way_id_,
      const Owner< Bbox_Double >& bbox_)
    : context_by_node_id(&context_by_node_id_), context_by_way_id(&context_by_way_id_), bbox(&bbox_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
  std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >* context_by_way_id;
  const Owner< Bbox_Double >* bbox;
};


struct Figure_Out_Local_Ids_Of_Links
{
  void operator()(Uint31_Index idx, const Way_Skeleton& way)
  {
    if (!way.nds.empty())
    {
      std::vector< NWR_Context::Way_Section_Context >& way_context = (*context_by_way_id)[way.id];
      std::vector< std::pair< Node_Skeleton::Id_Type, Node_Skeleton::Id_Type > > segment_count;

      for (uint i = 0; i < way_context.size(); ++i)
      {
        NWR_Context::Node_Context& context = (*context_by_node_id)[
            i+1 < way_context.size() ? way.nds[way_context[i+1].pos] : way.nds.back()];
        const std::vector< Way_Skeleton::Id_Type >& parallel_links
            = context.link_by_origin.find(way.nds[way_context[i].pos])->second;
        if (parallel_links.size() > 1)
        {
          uint parallel_links_pos = 0;
          while (parallel_links_pos < parallel_links.size()
              && !(parallel_links[parallel_links_pos] == way.id))
            ++parallel_links_pos;
          if (parallel_links_pos+1 == parallel_links.size() || !(parallel_links[parallel_links_pos+1] == way.id))
            way_context[i].local_id = parallel_links_pos + 1;
          else
          {
            Node_Skeleton::Id_Type target_id =
                (i+1 < way_context.size() ? way.nds[way_context[i+1].pos] : way.nds.back());
            for (std::vector< std::pair< Node_Skeleton::Id_Type, Node_Skeleton::Id_Type > >::const_iterator
                it_pair = segment_count.begin(); it_pair != segment_count.end(); ++it_pair)
            {
              if (it_pair->first == way.nds[way_context[i].pos] && it_pair->second == target_id)
                ++parallel_links_pos;
            }
            way_context[i].local_id = parallel_links_pos + 1;
            segment_count.push_back(std::make_pair(way.nds[way_context[i].pos], target_id));
          }
        }
      }
    }
  }

  Figure_Out_Local_Ids_Of_Links(
      std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >& context_by_node_id_,
      std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >& context_by_way_id_)
    : context_by_node_id(&context_by_node_id_), context_by_way_id(&context_by_way_id_) {}
private:
  std::map< Node_Skeleton::Id_Type, NWR_Context::Node_Context >* context_by_node_id;
  std::map< Way_Skeleton::Id_Type, std::vector< NWR_Context::Way_Section_Context > >* context_by_way_id;
};


NWR_Context::NWR_Context(Resource_Manager& rman, const Statement& stmt, const Set& input_set,
    const Owner< Bbox_Double >& bbox)
{
  // Collect all objects required for context

  Request_Context context(&stmt, rman);
  if (bbox)
  {
    Bbox_Filter filter(*bbox);
    add_nw_member_objects(context, input_set, relevant_nwrs, &filter.get_ranges_32(), &filter.get_ranges_31());
  }
  else
    add_nw_member_objects(context, input_set, relevant_nwrs);

  if (rman.get_desired_timestamp() == NOW)
  {
    Set extra_ways;
    collect_ways(context, relevant_nwrs.nodes, {}, 0, {}, true).swap(extra_ways.ways, extra_ways.attic_ways);
    sort_second(extra_ways.ways);
    indexed_set_union(relevant_nwrs.ways, extra_ways.ways);
  }
  else
  {
    auto extra_ways = collect_ways(context, relevant_nwrs.nodes, relevant_nwrs.attic_nodes, 0, {}, true);
    extra_ways.sort();
    extra_ways.set_union(Timeless< Uint31_Index, Way_Skeleton >{ relevant_nwrs.ways, relevant_nwrs.attic_ways }.sort());
    extra_ways.keep_matching_skeletons(rman.get_desired_timestamp());
    extra_ways.swap(relevant_nwrs.ways, relevant_nwrs.attic_ways);
  }

  // Build local ids for node segments
  Timeless< Uint32_Index, Node_Skeleton > relevant_nodes;
  relevant_nodes.swap(relevant_nwrs.nodes, relevant_nwrs.attic_nodes);
  relevant_nodes.set_union(Timeless< Uint32_Index, Node_Skeleton >{ input_set.nodes, input_set.attic_nodes }.sort());
  relevant_nodes.keep_matching_skeletons(rman.get_desired_timestamp());
  relevant_nodes.swap(relevant_nwrs.nodes, relevant_nwrs.attic_nodes);

  std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator it_now_idx
      = relevant_nwrs.nodes.begin();
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator it_attic_idx
      = relevant_nwrs.attic_nodes.begin();

  while (it_now_idx != relevant_nwrs.nodes.end() || it_attic_idx != relevant_nwrs.attic_nodes.end())
  {
    std::vector< const Node_Skeleton* > elem_refs;

    bool now_relevant = false;
    bool attic_relevant = false;
    if (it_attic_idx == relevant_nwrs.attic_nodes.end())
      now_relevant = true;
    else if (it_now_idx == relevant_nwrs.nodes.end())
      attic_relevant = true;
    else if (it_now_idx->first < it_attic_idx->first)
      now_relevant = true;
    else if (it_attic_idx->first < it_now_idx->first)
      attic_relevant = true;
    else
    {
      now_relevant = true;
      attic_relevant = true;
    }

    if (now_relevant)
    {
      for (std::vector< Node_Skeleton >::const_iterator it_elem = it_now_idx->second.begin();
          it_elem != it_now_idx->second.end(); ++it_elem)
        elem_refs.push_back(&*it_elem);

      ++it_now_idx;
    }

    if (attic_relevant)
    {
      for (std::vector< Attic< Node_Skeleton > >::const_iterator it_elem = it_attic_idx->second.begin();
          it_elem != it_attic_idx->second.end(); ++it_elem)
        elem_refs.push_back(&*it_elem);

      ++it_attic_idx;
    }

    std::sort(elem_refs.begin(), elem_refs.end(), Node_Skeleton_By_Coord_Then_Id());

    uint equal_since = 0;
    for (uint i = 1; i < elem_refs.size(); ++i)
    {
      if (elem_refs[equal_since]->ll_lower == elem_refs[i]->ll_lower)
      {
        if (i - equal_since == 1)
          context_by_node_id_[elem_refs[equal_since]->id].local_id = 1;
        context_by_node_id_[elem_refs[i]->id].local_id = i + 1 - equal_since;
      }
      else
        equal_since = i;
    }
  }

  // Store node coordinates for use by links
  for_each_elem(relevant_nwrs.nodes, Set_Coord_In_Context(context_by_node_id_));
  for_each_elem(relevant_nwrs.attic_nodes, Set_Coord_In_Context(context_by_node_id_));

  // Count node use by ways
  for_each_elem(relevant_nwrs.ways, Count_Node_Use(context_by_node_id_));
  for_each_elem(relevant_nwrs.attic_ways, Count_Node_Use(context_by_node_id_));
}


void NWR_Context::prepare_links(Resource_Manager& rman, const Statement& stmt, const Set& input_set,
    const Owner< Bbox_Double >& bbox)
{
  // Figure out links
  for_each_elem(relevant_nwrs.ways, Partition_Into_Links(context_by_node_id_, context_by_way_id_, bbox));
  for_each_elem(relevant_nwrs.attic_ways, Partition_Into_Links(context_by_node_id_, context_by_way_id_, bbox));

  for (std::map< Node_Skeleton::Id_Type, Node_Context >::iterator it_ctx = context_by_node_id_.begin();
      it_ctx != context_by_node_id_.end(); ++it_ctx)
  {
    for (std::map< Node_Skeleton::Id_Type, std::vector< Way_Skeleton::Id_Type > >::iterator
        it_orig = it_ctx->second.link_by_origin.begin(); it_orig != it_ctx->second.link_by_origin.end(); ++it_orig)
      std::sort(it_orig->second.begin(), it_orig->second.end());
  }

  // Figure out local ids of links
  for_each_elem(relevant_nwrs.ways, Figure_Out_Local_Ids_Of_Links(context_by_node_id_, context_by_way_id_));
  for_each_elem(relevant_nwrs.attic_ways, Figure_Out_Local_Ids_Of_Links(context_by_node_id_, context_by_way_id_));
}


Derived_Skeleton::Id_Type NWR_Context::local_id_by_node_id(Node_Skeleton::Id_Type id) const
{
  std::map< Node_Skeleton::Id_Type, Node_Context >::const_iterator it
      = context_by_node_id_.find(id);
  if (it != context_by_node_id_.end())
    return it->second.local_id;
  return Derived_Skeleton::Id_Type(0ull);
}


struct Derived_Structure_Builder
{
  Derived_Structure_Builder(Resource_Manager& rman,
      const std::string& name, Opaque_Geometry* geometry,
      const std::vector< std::pair< std::string, std::string > >* tags)
      : target(name, 0ull)
  {
    target.id = rman.get_global_settings().dispense_derived_id();
    target.acquire_geometry(geometry);

    if (tags)
    {
      for (std::vector< std::pair< std::string, std::string > >::const_iterator it_tag = tags->begin();
          it_tag != tags->end(); ++it_tag)
        target.tags.push_back(std::make_pair(
            it_tag->first.empty() ? "" : it_tag->first[0] == '_' ? "_" + it_tag->first : it_tag->first,
            it_tag->second));
    }
  }

  void set_tag(const std::string& key, const std::string& value)
  { target.tags.push_back(std::make_pair(key, value)); }

  const Derived_Structure& get()
  {
    std::sort(target.tags.begin(), target.tags.end());
    return target;
  }

private:
  Derived_Structure target;
};


template< typename Index, typename Maybe_Attic >
void process_vertices(const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Set& into, Resource_Manager& rman, NWR_Context& nwr_context,
    Localize_Statement::Mode mode, const Owner< Bbox_Double >& bbox)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const Element_With_Context< Maybe_Attic >& data = context_from.get_context(it_idx->first, *it_elem);

      if (mode == Localize_Statement::all || (data.tags && !data.tags->empty()))
      {
        NWR_Context::Node_Context& node_context = nwr_context.context_by_node_id(it_elem->id);
        if (data.tags && !data.tags->empty())
          ++node_context.count;

        if (admissible_by_bbox(bbox, node_context))
        {
          Derived_Structure_Builder result(rman, "vertex",
              new Point_Geometry(
                  ::lat(it_idx->first.val(), it_elem->ll_lower), ::lon(it_idx->first.val(), it_elem->ll_lower)),
              data.tags);

          Derived_Skeleton::Id_Type local_id = nwr_context.local_id_by_node_id(it_elem->id);
          if (local_id.val())
            result.set_tag("_local_id", to_string(local_id.val()));

          if (mode == Localize_Statement::all)
            result.set_tag("_node_ref", to_string(it_elem->id.val()));

          into.deriveds[Uint31_Index(0u)].push_back(result.get());
        }
      }
    }
  }
}


template< typename Index, typename Maybe_Attic >
void generate_links(const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Set& into, Resource_Manager& rman, NWR_Context& nwr_context,
    Localize_Statement::Mode mode, const Owner< Bbox_Double >& bbox)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const Element_With_Context< Maybe_Attic >& data = context_from.get_context(it_idx->first, *it_elem);

      if (mode == Localize_Statement::all || (data.tags && !data.tags->empty()))
      {
        std::vector< NWR_Context::Way_Section_Context >& way_context = nwr_context.context_by_way_id(it_elem->id);

        for (std::vector< NWR_Context::Way_Section_Context >::const_iterator it_way_ctx = way_context.begin();
            it_way_ctx != way_context.end(); ++it_way_ctx)
        {
          if (it_way_ctx->points.empty())
            continue;

          Derived_Structure_Builder result(rman, "link",
              new Linestring_Geometry(it_way_ctx->points), data.tags);

          if (it_way_ctx->local_from.val())
            result.set_tag("_local_from", to_string(it_way_ctx->local_from.val()));
          if (it_way_ctx->local_to.val())
            result.set_tag("_local_to", to_string(it_way_ctx->local_to.val()));
          if (it_way_ctx->local_id.val())
            result.set_tag("_local_id", to_string(it_way_ctx->local_id.val()));

          if (mode == Localize_Statement::all)
            result.set_tag("_way_ref", to_string(it_elem->id.val()));

          into.deriveds[Uint31_Index(0u)].push_back(result.get());
        }
      }
    }
  }
}


template< typename Index, typename Maybe_Attic >
void generate_loose_links(const std::map< Index, std::vector< Maybe_Attic > >& items,
    Set_With_Context& context_from, Set& into, Resource_Manager& rman, Localize_Statement::Mode mode)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const Element_With_Context< Maybe_Attic >& data = context_from.get_context(it_idx->first, *it_elem);

      if ((mode == Localize_Statement::all || (data.tags && !data.tags->empty()))
          && it_elem->nds.empty())
      {
        Derived_Structure_Builder result(rman, "link", new Null_Geometry(), data.tags);

        if (mode == Localize_Statement::all)
          result.set_tag("_way_ref", to_string(it_elem->id.val()));

        into.deriveds[Uint31_Index(0u)].push_back(result.get());
      }
    }
  }
}


template< typename Index, typename Maybe_Attic >
void generate_trigraphs(const std::map< Index, std::vector< Maybe_Attic > >& items, Set_With_Context& context_from,
    Set& into, Resource_Manager& rman, NWR_Context& nwr_context,
    Localize_Statement::Mode mode, const Owner< Bbox_Double >& bbox)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const Element_With_Context< Maybe_Attic >& data = context_from.get_context(it_idx->first, *it_elem);

      if (mode == Localize_Statement::all || (data.tags && !data.tags->empty()))
      {
        Derived_Structure* last = 0;
        Point_Double last_point(100., 0.);

        for (std::vector< Relation_Entry >::const_iterator it_memb = data.object->members.begin();
            it_memb != data.object->members.end(); ++it_memb)
        {
          if (it_memb->type == Relation_Entry::NODE)
          {
            NWR_Context::Node_Context& node_context = nwr_context.context_by_node_id(it_memb->ref);

            if (admissible_by_bbox(bbox, node_context))
            {
              Derived_Structure_Builder result(rman, "trigraph",
                  new Point_Geometry(node_context.lat, node_context.lon), data.tags);

              if (node_context.local_id.val())
                result.set_tag("_local_ref", to_string(node_context.local_id.val()));

              if (mode == Localize_Statement::all)
              {
                result.set_tag("_child_node_ref", to_string(it_memb->ref.val()));
                result.set_tag("_relation_ref", to_string(it_elem->id.val()));
              }

              if (last)
              {
                last->tags.push_back(std::make_pair("_next", to_string(result.get().id.val())));
                result.set_tag("_previous", to_string(last->id.val()));
              }
              into.deriveds[Uint31_Index(0u)].push_back(result.get());
              last = &into.deriveds[Uint31_Index(0u)].back();
            }
            else
              last = 0;
          }
          else if (it_memb->type == Relation_Entry::WAY)
          {
            std::vector< NWR_Context::Way_Section_Context >& way_context
                = nwr_context.context_by_way_id(Way_Skeleton::Id_Type(it_memb->ref.val()));

            if (way_context.empty())
            {
              last = 0;
              continue;
            }

            enum { both_possible, must_forwards, must_backwards } elem_orientation = both_possible;
            if (last && last_point.lat <= 90.)
            {
              if (!way_context.front().points.empty() && last_point == way_context.front().points.front())
                elem_orientation = must_forwards;
              else if (!way_context.back().points.empty() && last_point == way_context.back().points.back())
                elem_orientation = must_backwards;
            }
            if (elem_orientation == both_possible)
            {
              std::vector< Relation_Entry >::const_iterator it_next = it_memb+1;
              if (it_next != data.object->members.end() && it_next->type == Relation_Entry::WAY)
              {
                std::vector< NWR_Context::Way_Section_Context >& next_way_context
                    = nwr_context.context_by_way_id(Way_Skeleton::Id_Type(it_next->ref.val()));
                if (!next_way_context.empty())
                {
                  if (!way_context.back().points.empty()
                    && ((!next_way_context.front().points.empty()
                        && next_way_context.front().points.front() == way_context.back().points.back())
                      || (!next_way_context.back().points.empty()
                        && next_way_context.back().points.back() == way_context.back().points.back())))
                    elem_orientation = must_forwards;
                  else if (!way_context.front().points.empty()
                    && ((!next_way_context.front().points.empty()
                        && next_way_context.front().points.front() == way_context.front().points.front())
                      || (!next_way_context.back().points.empty()
                        && next_way_context.back().points.back() == way_context.front().points.front())))
                    elem_orientation = must_backwards;
                }
              }
            }

            if (elem_orientation != must_backwards)
            {
              for (uint i = 0; i < way_context.size(); ++i)
              {
                if (way_context[i].points.empty())
                {
                  last = 0;
                  continue;
                }

                Derived_Structure_Builder result(rman, "trigraph",
                    new Linestring_Geometry(way_context[i].points), data.tags);

                if (way_context[i].local_from.val())
                  result.set_tag("_vertex_local_from", to_string(way_context[i].local_from.val()));
                if (way_context[i].local_to.val())
                  result.set_tag("_vertex_local_to", to_string(way_context[i].local_to.val()));
                if (way_context[i].local_id.val())
                  result.set_tag("_local_ref", to_string(way_context[i].local_id.val()));

                if (mode == Localize_Statement::all)
                {
                  result.set_tag("_child_way_ref", to_string(it_memb->ref.val()));
                  result.set_tag("_relation_ref", to_string(it_elem->id.val()));
                }

                if (last)
                {
                  last->tags.push_back(std::make_pair("_next", to_string(result.get().id.val())));
                  result.set_tag("_previous", to_string(last->id.val()));
                }
                into.deriveds[Uint31_Index(0u)].push_back(result.get());
                last = &into.deriveds[Uint31_Index(0u)].back();
              }
              last_point = (way_context.empty() || way_context.back().points.empty()) ? Point_Double(100., 0.)
                  : way_context.back().points.back();
            }
            else
            {
              for (int i = way_context.size()-1; i >= 0; --i)
              {
                if (way_context[i].points.empty())
                {
                  last = 0;
                  continue;
                }

                std::vector< Point_Double > points = way_context[i].points;
                std::reverse(points.begin(), points.end());
                Derived_Structure_Builder result(rman, "trigraph",
                    new Linestring_Geometry(points), data.tags);

                if (way_context[i].local_to.val())
                  result.set_tag("_vertex_local_from", to_string(way_context[i].local_to.val()));
                if (way_context[i].local_from.val())
                  result.set_tag("_vertex_local_to", to_string(way_context[i].local_from.val()));
                if (way_context[i].local_id.val())
                  result.set_tag("_local_ref", to_string(way_context[i].local_id.val()));

                if (mode == Localize_Statement::all)
                {
                  result.set_tag("_child_way_ref", to_string(it_memb->ref.val()));
                  result.set_tag("_relation_ref", to_string(it_elem->id.val()));
                }

                if (last)
                {
                  last->tags.push_back(std::make_pair("_next", to_string(result.get().id.val())));
                  result.set_tag("_previous", to_string(last->id.val()));
                }
                into.deriveds[Uint31_Index(0u)].push_back(result.get());
                last = &into.deriveds[Uint31_Index(0u)].back();
              }
              last_point = (way_context.empty() || way_context.front().points.empty()) ? Point_Double(100., 0.)
                  : way_context.front().points.front();
            }
          }
          else if (mode >= Localize_Statement::also_loose)
          {
            Derived_Structure_Builder result(rman, "trigraph", new Null_Geometry(), data.tags);

            if (mode == Localize_Statement::all)
            {
              result.set_tag("_child_relation_ref", to_string(it_memb->ref.val()));
              result.set_tag("_relation_ref", to_string(it_elem->id.val()));
            }

            if (last)
            {
              last->tags.push_back(std::make_pair("_next", to_string(result.get().id.val())));
              result.set_tag("_previous", to_string(last->id.val()));
            }
            into.deriveds[Uint31_Index(0u)].push_back(result.get());
            last = &into.deriveds[Uint31_Index(0u)].back();
          }
          else
            // Relation members do not have any effect without loose mode
            last = 0;
        }
      }
    }
  }
}


void Localize_Statement::execute(Resource_Manager& rman)
{
  //TODO: diff

  Set into;

  const Set* input_set = rman.get_set(input);
  if (!input_set)
  {
    transfer_output(rman, into);
    return;
  }

  Owner< Bbox_Double > bbox(south <= 90. ? new Bbox_Double(south, west, north, east) : 0);

  Requested_Context requested_context;
  requested_context.add_usage(input, Set_Usage::TAGS);
  Prepare_Task_Context context(requested_context, *this, rman);
  Set_With_Context* context_from = context.get_set(input);

  if (context_from && context_from->base)
  {
    NWR_Context nwr_context(rman, *this, *input_set, bbox);
    process_vertices(context_from->base->nodes, *context_from, into, rman, nwr_context, type, bbox);
    if (!context_from->base->attic_nodes.empty())
      process_vertices(context_from->base->attic_nodes, *context_from, into, rman, nwr_context, type, bbox);

    nwr_context.prepare_links(rman, *this, *input_set, bbox);
    generate_links(context_from->base->ways, *context_from, into, rman, nwr_context, type, bbox);
    if (!context_from->base->attic_ways.empty())
      generate_links(context_from->base->attic_ways, *context_from, into, rman, nwr_context, type, bbox);

    if (type >= also_loose)
    {
      generate_loose_links(context_from->base->ways, *context_from, into, rman, type);
      if (!context_from->base->attic_ways.empty())
        generate_loose_links(context_from->base->attic_ways, *context_from, into, rman, type);
    }

    generate_trigraphs(context_from->base->relations, *context_from, into, rman, nwr_context, type, bbox);
    if (!context_from->base->attic_ways.empty())
      generate_trigraphs(context_from->base->attic_relations, *context_from, into, rman, nwr_context, type, bbox);
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}
