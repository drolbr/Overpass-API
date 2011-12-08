#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "meta_collector.h"
#include "newer.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

class Newer_Constraint : public Query_Constraint
{
  public:
    Newer_Constraint(Newer_Statement& newer) : timestamp(newer.get_timestamp()) {}
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Newer_Constraint() {}
    
  private:
    uint64 timestamp;
};

template< typename TIndex, typename TObject >
void newer_filter_map
    (map< TIndex, vector< TObject > >& modify,
     Resource_Manager& rman, uint64 timestamp, File_Properties* file_properties)
{
  if (modify.empty())
    return;
  Meta_Collector< TIndex, TObject > meta_collector
      (modify, *rman.get_transaction(), file_properties, false);
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      const OSM_Element_Metadata_Skeleton* meta_skel
	  = meta_collector.get(it->first, iit->id);
      if ((meta_skel) && (meta_skel->timestamp >= timestamp))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

void Newer_Constraint::filter(Resource_Manager& rman, Set& into)
{
  newer_filter_map(into.nodes, rman, timestamp, meta_settings().NODES_META);
  newer_filter_map(into.ways, rman, timestamp, meta_settings().WAYS_META);
  newer_filter_map(into.relations, rman, timestamp, meta_settings().RELATIONS_META);
}

//-----------------------------------------------------------------------------

Newer_Statement::Newer_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["than"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  string timestamp = attributes["than"];
  
  than_timestamp = 0;
  than_timestamp |= (atoll(timestamp.c_str())<<26); //year
  than_timestamp |= (atoi(timestamp.c_str()+5)<<22); //month
  than_timestamp |= (atoi(timestamp.c_str()+8)<<17); //day
  than_timestamp |= (atoi(timestamp.c_str()+11)<<12); //hour
  than_timestamp |= (atoi(timestamp.c_str()+14)<<6); //minute
  than_timestamp |= atoi(timestamp.c_str()+17); //second
  
  if (than_timestamp == 0)
  {
    ostringstream temp;
    temp<<"The attribute than must contain a timestamp exactly in the form yyyy-mm-ddThh:mm:ssZ.";
    add_static_error(temp.str());
  }
}

Newer_Statement::~Newer_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

void Newer_Statement::forecast() {}

void Newer_Statement::execute(Resource_Manager& rman) {}

Query_Constraint* Newer_Statement::get_query_constraint()
{
  constraints.push_back(new Newer_Constraint(*this));
  return constraints.back();
}
