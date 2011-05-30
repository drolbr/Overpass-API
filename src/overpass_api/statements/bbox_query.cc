#include <algorithm>
#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
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
    add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0) || (attributes["n"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0) || (attributes["w"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0) || (attributes["e"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"bbox-query\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
}

void Bbox_Query_Statement::forecast()
{
}

void Bbox_Query_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();
  
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

  vector< pair< uint32, uint32 > >* uint_ranges
    (Node::calc_ranges(south, north, west, east));
    
  set< pair< Uint32_Index, Uint32_Index > > req;
  for (vector< pair< uint32, uint32 > >::const_iterator
      it(uint_ranges->begin()); it != uint_ranges->end(); ++it)
  {
    pair< Uint32_Index, Uint32_Index > range
      (make_pair(Uint32_Index(it->first), Uint32_Index(it->second)));
    req.insert(range);
  }
  delete(uint_ranges);
  
  stopwatch.stop(Stopwatch::NO_DISK);
  uint nodes_count = 0;
  
  uint32 isouth((south + 91.0)*10000000+0.5);
  uint32 inorth((north + 91.0)*10000000+0.5);
  int32 iwest(west*10000000 + (west > 0 ? 0.5 : -0.5));
  int32 ieast(east*10000000 + (east > 0 ? 0.5 : -0.5));
  
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
    (rman.get_transaction().data_index(de_osm3s_file_ids::NODES));
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
    
    uint32 ilat(Node::ilat(it.index().val(), it.object().ll_lower));
    int32 ilon(Node::ilon(it.index().val(), it.object().ll_lower));
    if ((ilat >= isouth) && (ilat <= inorth) &&
        (((ilon >= iwest) && (ilon <= ieast))
	  || ((ieast < iwest) && ((ilon >= iwest) || (ilon <= ieast)))))
      nodes[it.index()].push_back(it.object());    
  }
  stopwatch.add(Stopwatch::NODES, nodes_db.read_count());
  stopwatch.stop(Stopwatch::NODES);
  
  stopwatch.report(get_name());
  rman.health_check(*this);
}
