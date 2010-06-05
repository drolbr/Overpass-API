#include <algorithm>
#include <sstream>

#include "../backend/block_backend.h"
#include "../backend/random_file.h"
#include "../core/settings.h"
#include "bbox_query.h"
// #include "area_query.h"

using namespace std;

//-----------------------------------------------------------------------------

const unsigned int QUERY_NODE = 1;
const unsigned int QUERY_WAY = 2;
const unsigned int QUERY_RELATION = 3;
// const unsigned int QUERY_AREA = 4;

void Bbox_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["s"] = "";
  attributes["n"] = "";
  attributes["w"] = "";
  attributes["e"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  south = atof(attributes["s"].c_str());
  if ((south < -90.0) || (south > 90.0) || (attributes["s"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    //add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0) || (attributes["n"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    //add_static_error(temp.str());
  }
  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0) || (attributes["w"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    //add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0) || (attributes["e"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    //add_static_error(temp.str());
  }
}

void Bbox_Query_Statement::forecast()
{
}

vector< pair< uint32, uint32 > >* Bbox_Query_Statement::calc_ranges
    (double south, double north, double west, double east)
{
  vector< pair< uint32, uint32 > >* ranges;
  if (west <= east)
    ranges = new vector< pair< uint32, uint32 > >();
  else
  {
    ranges = calc_ranges(south, north, west, 180.0);
    west = -180.0;
  }
  for (int i(0); 65536.0/10000000.0*i <= north - south; ++i)
  {
    for (int j(0); 65536.0/10000000.0*j <= east - west; ++j)
    {
      pair< uint32, uint32 > range;
      range.first = Node::ll_upper
        (south + 65536.0/10000000.0*i, west + 65536.0/10000000.0*j);
      range.second = range.first + 1;
      ranges->push_back(range);
    }
  }
  return ranges;
}

void Bbox_Query_Statement::execute(map< string, Set >& maps)
{
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(maps[output].nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways(maps[output].ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(maps[output].relations);
  //set< Area >& areas(maps[output].areas);
  
  nodes.clear();
  ways.clear();
  relations.clear();
  //areas.clear();

  vector< pair< uint32, uint32 > >* uint_ranges
    (calc_ranges(south, north, west, east));
    
  set< pair< Uint32_Index, Uint32_Index > > req;
  for (vector< pair< uint32, uint32 > >::const_iterator
      it(uint_ranges->begin()); it != uint_ranges->end(); ++it)
  {
    pair< Uint32_Index, Uint32_Index > range
      (make_pair(Uint32_Index(it->first), Uint32_Index(it->second)));
    req.insert(range);
  }
  delete(uint_ranges);
  
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
    (*de_osm3s_file_ids::NODES, false);
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Range_Iterator
    it(nodes_db.range_begin
      (Default_Range_Iterator< Uint32_Index >(req.begin()),
       Default_Range_Iterator< Uint32_Index >(req.end())));
    !(it == nodes_db.range_end()); ++it)
  {
    double lat(Node::lat(it.index().val(), it.object().ll_lower));
    double lon(Node::lon(it.index().val(), it.object().ll_lower));
    if ((lat >= south) && (lat <= north) &&
        (((lon >= west) && (lon <= east))
	  || ((east < west) && ((lon >= west) || (lon <= east)))))
      nodes[it.index()].push_back(it.object());
  }
}
