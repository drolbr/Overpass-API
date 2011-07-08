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

double great_circle_dist(double lat1, double lon1, double lat2, double lon2)
{
  double scalar_prod =
      sin(lat1/90.0*acos(0))*sin(lat2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*sin(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*sin(lon2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*cos(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*cos(lon2/90.0*acos(0));
  return acos(scalar_prod)*(20*1000*1000/acos(0));
}

void Around_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["radius"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
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
      double west = lon - radius*(360.0/(40000.0*1000.0))/cos(lat/90.0*acos(0));
      double east = lon + radius*(360.0/(40000.0*1000.0))/cos(lat/90.0*acos(0));
      
      lat_lons.push_back(make_pair(lat, lon));
      
      vector< pair< uint32, uint32 > >* uint_ranges
      (Node::calc_ranges(south, north, west, east));
      for (vector< pair< uint32, uint32 > >::const_iterator
	it(uint_ranges->begin()); it != uint_ranges->end(); ++it)
      {
	pair< Uint32_Index, Uint32_Index > range
	(make_pair(Uint32_Index(it->first), Uint32_Index(it->second)));
	req.insert(range);
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
  for (vector< pair< double, double > >::const_iterator cit = lat_lons.begin();
      cit != lat_lons.end(); ++cit)
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
