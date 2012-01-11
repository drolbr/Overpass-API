#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "around.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

class Around_Constraint : public Query_Constraint
{
  public:
    Around_Constraint(Around_Statement& around_) : around(&around_) {}
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Around_Constraint() {}
    
  private:
    Around_Statement* around;
};

bool Around_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges = around->calc_ranges(rman.sets()[around->get_source_name()].nodes);
  return true;
}

void Around_Constraint::filter(Resource_Manager& rman, Set& into)
{
  around->calc_ranges(rman.sets()[around->get_source_name()].nodes);
  
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
  return acos(scalar_prod)*(20*1000*1000/acos(0));
}

set< pair< Uint32_Index, Uint32_Index > > Around_Statement::calc_ranges
    (const map< Uint32_Index, vector< Node_Skeleton > >& input_nodes)
{
  set< pair< Uint32_Index, Uint32_Index > > req;
  lat_lons.clear();
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
	  lat_lons[idx].push_back(make_pair(lat, lon));
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
      = lat_lons.find(Node::ll_upper_(lat, lon));
  if (mit == lat_lons.end())
    return false;
  for (vector< pair< double, double > >::const_iterator cit = mit->second.begin();
      cit != mit->second.end(); ++cit)
  {
    if (great_circle_dist(cit->first, cit->second, lat, lon) <= radius)
      return true;
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
