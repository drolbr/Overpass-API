#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "around.h"
#include "recurse.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

class Around_Constraint : public Query_Constraint
{
  public:
    Around_Constraint(Around_Statement& around_) : around(&around_), ranges_used(false) {}
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into);
    void filter(const Statement& query, Resource_Manager& rman, Set& into);
    virtual ~Around_Constraint() {}
    
  private:
    Around_Statement* around;
    bool ranges_used;
};

bool Around_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges_used = true;
  
  ranges = around->calc_ranges(rman.sets()[around->get_source_name()].nodes);
  return true;
}

bool Around_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  set< pair< Uint32_Index, Uint32_Index > > node_ranges;
  this->get_ranges(rman, node_ranges);
  ranges = calc_parents(node_ranges);
  return true;
}

void Around_Constraint::filter(Resource_Manager& rman, Set& into)
{
  around->calc_ranges(rman.sets()[around->get_source_name()].nodes);
  
  // process nodes
  for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = into.nodes.begin();
      it != into.nodes.end(); ++it)
  {
    vector< Node_Skeleton > local_into;
    for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      double lat(Node::lat(it->first.val(), iit->ll_lower));
      double lon(Node::lon(it->first.val(), iit->ll_lower));
      if (around->is_inside(lat, lon))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
  
  // pre-process ways to reduce the load of the expensive filter
  if (ranges_used == false)
  {
    set< pair< Uint31_Index, Uint31_Index > > ranges;
    get_ranges(rman, ranges);

    // pre-filter ways
    {
      set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
      map< Uint31_Index, vector< Way_Skeleton > >::iterator it = into.ways.begin();
      for (; it != into.ways.end() && ranges_it != ranges.end(); )
      {
        if (!(it->first < ranges_it->second))
	  ++ranges_it;
        else if (!(it->first < ranges_it->first))
	  ++it;
        else
        {
	  it->second.clear();
	  ++it;
        }
      }
      for (; it != into.ways.end(); ++it)
        it->second.clear();
    }

    // pre-filter relations
    {
      set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
      map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = into.relations.begin();
      for (; it != into.relations.end() && ranges_it != ranges.end(); )
      {
        if (!(it->first < ranges_it->second))
	  ++ranges_it;
        else if (!(it->first < ranges_it->first))
	  ++it;
        else
        {
	  it->second.clear();
	  ++it;
        }
      }
      for (; it != into.relations.end(); ++it)
        it->second.clear();
    }
  }
  ranges_used = false;  
}

void Around_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  {
    //Process ways

    // Retrieve all nodes referred by the ways.
    map< Uint32_Index, vector< Node_Skeleton > > way_members;
    collect_nodes(query, rman, into.ways.begin(), into.ways.end(), way_members);
  
    // Order node ids by id.
    vector< pair< Uint32_Index, const Node_Skeleton* > > way_members_by_id;
    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = way_members.begin();
        it != way_members.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        way_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    Order_By_Node_Id order_by_node_id;
    sort(way_members_by_id.begin(), way_members_by_id.end(), order_by_node_id);
  
    for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = into.ways.begin();
        it != into.ways.end(); ++it)
    {
      vector< Way_Skeleton > local_into;
      for (vector< Way_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
	if (around->is_inside(*iit, way_members_by_id))
	  local_into.push_back(*iit);
      }
      it->second.swap(local_into);
    }
  }
  {
    //Process relations
    
    // Retrieve all nodes referred by the relations.
    set< pair< Uint32_Index, Uint32_Index > > node_ranges;
    get_ranges(rman, node_ranges);
    
    map< Uint32_Index, vector< Node_Skeleton > > node_members;
    collect_nodes(query, rman, into.relations.begin(), into.relations.end(), node_members,
		  node_ranges);
  
    // Order node ids by id.
    vector< pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id;
    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = node_members.begin();
        it != node_members.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        node_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    Order_By_Node_Id order_by_node_id;
    sort(node_members_by_id.begin(), node_members_by_id.end(), order_by_node_id);
    
    // Retrieve all ways referred by the relations.
    set< pair< Uint31_Index, Uint31_Index > > way_ranges;
    get_ranges(rman, way_ranges);
    
    map< Uint31_Index, vector< Way_Skeleton > > way_members;
    collect_ways(query, rman, into.relations.begin(), into.relations.end(), way_members,
		 way_ranges);
    
    // Order way ids by id.
    vector< pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id;
    for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = way_members.begin();
        it != way_members.end(); ++it)
    {
      for (vector< Way_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        way_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    Order_By_Way_Id order_by_way_id;
    sort(way_members_by_id.begin(), way_members_by_id.end(), order_by_way_id);
    
    // Retrieve all nodes referred by the ways.
    map< Uint32_Index, vector< Node_Skeleton > > way_nds;
    collect_nodes(query, rman, way_members.begin(), way_members.end(), way_nds);
    
    // Order node ids by id.
    vector< pair< Uint32_Index, const Node_Skeleton* > > way_nds_by_id;
    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = way_nds.begin();
        it != way_nds.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        way_nds_by_id.push_back(make_pair(it->first, &*iit));
    }
    sort(way_nds_by_id.begin(), way_nds_by_id.end(), order_by_node_id);
    
    for (map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = into.relations.begin();
        it != into.relations.end(); ++it)
    {
      vector< Relation_Skeleton > local_into;
      for (vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
	for (vector< Relation_Entry >::const_iterator nit = iit->members.begin();
	    nit != iit->members.end(); ++nit)
        {
	  if (nit->type == Relation_Entry::NODE)
	  {
	    const pair< Uint32_Index, const Node_Skeleton* >* second_nd =
	        binary_search_for_pair_id(node_members_by_id, nit->ref);
	    if (!second_nd)
	      continue;
	    double lat(Node::lat(second_nd->first.val(), second_nd->second->ll_lower));
	    double lon(Node::lon(second_nd->first.val(), second_nd->second->ll_lower));
	  
	    if (around->is_inside(lat, lon))
	    {
	      local_into.push_back(*iit);
	      break;
	    }
	  }
	  else if (nit->type == Relation_Entry::WAY)
	  {
	    const pair< Uint31_Index, const Way_Skeleton* >* second_nd =
	        binary_search_for_pair_id(way_members_by_id, nit->ref);
	    if (!second_nd)
	      continue;
	    if (around->is_inside(*second_nd->second, way_nds_by_id))
	    {
	      local_into.push_back(*iit);
	      break;
	    }
	  }
        }
      }
      it->second.swap(local_into);
    }
  }  
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Around_Statement > Around_Statement::statement_maker("around");

Around_Statement::Around_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["radius"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  output = attributes["into"];
  radius = atof(attributes["radius"].c_str());
  if ((radius < 0.0) || (attributes["radius"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"radius\" of the element \"around\""
    <<" the only allowed values are nonnegative floats.";
    add_static_error(temp.str());
  }
}

Around_Statement::~Around_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

double great_circle_dist(double lat1, double lon1, double lat2, double lon2)
{
  double scalar_prod =
      sin(lat1/90.0*acos(0))*sin(lat2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*sin(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*sin(lon2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*cos(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*cos(lon2/90.0*acos(0));
  if (scalar_prod > 1)
    scalar_prod = 1;
  return acos(scalar_prod)*(10*1000*1000/acos(0));
}


double great_circle_line_dist(double llat1, double llon1, double llat2, double llon2,
			      double plat, double plon)
{
  double l1_x = sin(llat1/90.0*acos(0));
  double l1_y = cos(llat1/90.0*acos(0))*sin(llon1/90.0*acos(0));
  double l1_z = cos(llat1/90.0*acos(0))*cos(llon1/90.0*acos(0));
  double l2_x = sin(llat2/90.0*acos(0));
  double l2_y = cos(llat2/90.0*acos(0))*sin(llon2/90.0*acos(0));
  double l2_z = cos(llat2/90.0*acos(0))*cos(llon2/90.0*acos(0));
  double laenge_normale = sqrt(
      (l1_y*l2_z - l1_z*l2_y)*(l1_y*l2_z - l1_z*l2_y)
      + (l1_z*l2_x - l1_x*l2_z)*(l1_z*l2_x - l1_x*l2_z)
      + (l1_x*l2_y - l1_y*l2_x)*(l1_x*l2_y - l1_y*l2_x));
  double scalar_prod = abs(
      sin(plat/90.0*acos(0))*(l1_y*l2_z - l1_z*l2_y) +
      cos(plat/90.0*acos(0))*sin(plon/90.0*acos(0))*(l1_z*l2_x - l1_x*l2_z) +
      cos(plat/90.0*acos(0))*cos(plon/90.0*acos(0))*(l1_x*l2_y - l1_y*l2_x))/laenge_normale;
  if (scalar_prod > 1)
    scalar_prod = 1;
  
  return asin(scalar_prod)*(10*1000*1000/acos(0));
}


set< pair< Uint32_Index, Uint32_Index > > Around_Statement::calc_ranges
    (const map< Uint32_Index, vector< Node_Skeleton > >& input_nodes)
{
  set< pair< Uint32_Index, Uint32_Index > > req;
  radius_lat_lons.clear();
  simple_lat_lons.clear();
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator iit(input_nodes.begin());
      iit != input_nodes.end(); ++iit)
  {
    for (vector< Node_Skeleton >::const_iterator nit(iit->second.begin());
        nit != iit->second.end(); ++nit)
    {
      double lat = Node::lat(iit->first.val(), nit->ll_lower);
      double lon = Node::lon(iit->first.val(), nit->ll_lower);
      double south = lat - radius*(360.0/(40000.0*1000.0));
      double north = lat + radius*(360.0/(40000.0*1000.0));
      double scale_lat = lat > 0.0 ? north : south;
      if (abs(scale_lat) >= 89.9)
	scale_lat = 89.9;
      double west = lon - radius*(360.0/(40000.0*1000.0))/cos(scale_lat/90.0*acos(0));
      double east = lon + radius*(360.0/(40000.0*1000.0))/cos(scale_lat/90.0*acos(0));
      
      simple_lat_lons.push_back(make_pair(lat, lon));
      
      vector< pair< uint32, uint32 > >* uint_ranges
          (Node::calc_ranges(south, north, west, east));
      for (vector< pair< uint32, uint32 > >::const_iterator
	it(uint_ranges->begin()); it != uint_ranges->end(); ++it)
      {
	pair< Uint32_Index, Uint32_Index > range
	    (make_pair(Uint32_Index(it->first), Uint32_Index(it->second)));
	req.insert(range);
	
	for (uint32 idx = Uint32_Index(it->first).val();
	    idx < Uint32_Index(it->second).val(); ++idx)
	  radius_lat_lons[idx].push_back(make_pair(lat, lon));
      }
      delete(uint_ranges);
    }
  }
  return req;
}

void Around_Statement::forecast()
{
}

bool Around_Statement::is_inside(double lat, double lon) const
{
  map< Uint32_Index, vector< pair< double, double > > >::const_iterator mit
      = radius_lat_lons.find(Node::ll_upper_(lat, lon));
  if (mit == radius_lat_lons.end())
    return false;
  for (vector< pair< double, double > >::const_iterator cit = mit->second.begin();
      cit != mit->second.end(); ++cit)
  {
    if (great_circle_dist(cit->first, cit->second, lat, lon) <= radius)
      return true;
  }
  return false;
}

bool Around_Statement::is_inside
    (double first_lat, double first_lon, double second_lat, double second_lon) const
{
  for (vector< pair< double, double > >::const_iterator cit = simple_lat_lons.begin();
      cit != simple_lat_lons.end(); ++cit)
  {
    if (great_circle_line_dist(first_lat, first_lon, second_lat, second_lon,
                               cit->first, cit->second) <= radius)
    {
      double gcdist = great_circle_dist(first_lat, first_lon, second_lat, second_lon);
      double limit = sqrt(gcdist*gcdist + radius*radius);
      return (great_circle_dist(cit->first, cit->second, first_lat, first_lon) <= limit &&
          great_circle_dist(cit->first, cit->second, second_lat, second_lon) <= limit);
    }
  }
  return false;
}

bool Around_Statement::is_inside
    (const Way_Skeleton& way,
     const vector< pair< Uint32_Index, const Node_Skeleton* > >& nodes_by_id) const
{
  vector< uint32 >::const_iterator nit = way.nds.begin();
  if (nit == way.nds.end())
    return false;
  const pair< Uint32_Index, const Node_Skeleton* >* first_nd =
      binary_search_for_pair_id(nodes_by_id, *nit);
  if (!first_nd)
  {
    ostringstream out;
    out<<"Node "<<*nit<<" not found in the database. This is a serious fault of the database.";
    this->runtime_remark(out.str());
    return true;
  }
  double first_lat(Node::lat(first_nd->first.val(), first_nd->second->ll_lower));
  double first_lon(Node::lon(first_nd->first.val(), first_nd->second->ll_lower));
  
  // Pre-check if node is inside
  if (is_inside(first_lat, first_lon))
    return true;
  for (vector< uint32 >::const_iterator it = nit; it != way.nds.end(); ++it)
  {
    const pair< Uint32_Index, const Node_Skeleton* >* second_nd =
        binary_search_for_pair_id(nodes_by_id, *it);
    if (!second_nd)
    {
      ostringstream out;
      out<<"Node "<<*nit<<" not found in the database. This is a serious fault of the database.";
      this->runtime_remark(out.str());
      return true;
    }
    double second_lat(Node::lat(second_nd->first.val(), second_nd->second->ll_lower));
    double second_lon(Node::lon(second_nd->first.val(), second_nd->second->ll_lower));
    if (is_inside(second_lat, second_lon))
      return true;
  }
  
  for (++nit; nit != way.nds.end(); ++nit)
  {
    const pair< Uint32_Index, const Node_Skeleton* >* second_nd =
        binary_search_for_pair_id(nodes_by_id, *nit);
    if (!second_nd)
    {
      ostringstream out;
      out<<"Node "<<*nit<<" not found in the database. This is a serious fault of the database.";
      this->runtime_remark(out.str());
      return true;
    }
    double second_lat(Node::lat(second_nd->first.val(), second_nd->second->ll_lower));
    double second_lon(Node::lon(second_nd->first.val(), second_nd->second->ll_lower));
    
    if (is_inside(first_lat, first_lon, second_lat, second_lon))
      return true;
    
    first_lat = second_lat;
    first_lon = second_lon;
  }
  return false;
}

void Around_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();

  map< Uint32_Index, vector< Node_Skeleton > >& input_nodes
      (rman.sets()[input].nodes);

  set< pair< Uint32_Index, Uint32_Index > > req = calc_ranges(input_nodes);

  map< Uint32_Index, vector< Node_Skeleton > >& nodes
      (rman.sets()[output].nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways
      (rman.sets()[output].ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations
      (rman.sets()[output].relations);
  map< Uint31_Index, vector< Area_Skeleton > >& areas
      (rman.sets()[output].areas);
  
  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();

  stopwatch.stop(Stopwatch::NO_DISK);
  uint nodes_count = 0;
  
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
    (rman.get_transaction()->data_index(osm_base_settings().NODES));
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Range_Iterator
    it(nodes_db.range_begin
      (Default_Range_Iterator< Uint32_Index >(req.begin()),
       Default_Range_Iterator< Uint32_Index >(req.end())));
    !(it == nodes_db.range_end()); ++it)
  {
    if (++nodes_count >= 64*1024)
    {
      nodes_count = 0;
      rman.health_check(*this);
    }
    
    double lat(Node::lat(it.index().val(), it.object().ll_lower));
    double lon(Node::lon(it.index().val(), it.object().ll_lower));
    if (is_inside(lat, lon))
      nodes[it.index()].push_back(it.object());
  }
  stopwatch.add(Stopwatch::NODES, nodes_db.read_count());
  stopwatch.stop(Stopwatch::NODES);
  
  stopwatch.report(get_name());
  rman.health_check(*this);
}

Query_Constraint* Around_Statement::get_query_constraint()
{
  constraints.push_back(new Around_Constraint(*this));
  return constraints.back();
}
