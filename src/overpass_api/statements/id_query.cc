#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "id_query.h"

using namespace std;

Generic_Statement_Maker< Id_Query_Statement > Id_Query_Statement::statement_maker("id-query");

template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
		   uint32 lower, uint32 upper,
		   map< TIndex, vector< TObject > >& elems)
{
  set< TIndex > req;
  {
    Random_File< TIndex > random(rman.get_transaction()->random_index(&prop));
    for (uint32 i = lower; i <= upper; ++i)
      req.insert(random.get(i));
  }    
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if (it.object().id >= lower && it.object().id <= upper)
      elems[it.index()].push_back(it.object());
  }
}

template< class TIndex, class TObject >
void collect_elems(Resource_Manager& rman, const File_Properties& prop,
		   uint32 lower, uint32 upper, const vector< uint32 >& ids,
		   map< TIndex, vector< TObject > >& elems)
{
  set< TIndex > req;
  {
    Random_File< TIndex > random(rman.get_transaction()->random_index(&prop));
    for (uint32 i = lower; i <= upper; ++i)
    {
      if (binary_search(ids.begin(), ids.end(), i))
        req.insert(random.get(i));
    }
  }    
  Block_Backend< TIndex, TObject > elems_db(rman.get_transaction()->data_index(&prop));
  for (typename Block_Backend< TIndex, TObject >::Discrete_Iterator
      it(elems_db.discrete_begin(req.begin(), req.end()));
      !(it == elems_db.discrete_end()); ++it)
  {
    if (it.object().id >= lower && it.object().id <= upper
        && binary_search(ids.begin(), ids.end(), it.object().id))
      elems[it.index()].push_back(it.object());
  }
}

template< class TIndex, class TObject >
void filter_elems(uint32 lower, uint32 upper,
		  map< TIndex, vector< TObject > >& elems)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (iit->id >= lower && iit->id <= upper)
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

//-----------------------------------------------------------------------------

class Id_Query_Constraint : public Query_Constraint
{
  public:
    Id_Query_Constraint(Id_Query_Statement& stmt_) : stmt(&stmt_) {}
    bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
		  const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		  const vector< uint32 >& ids);
    bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  int type, const vector< uint32 >& ids);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Id_Query_Constraint() {}
    
  private:
    Id_Query_Statement* stmt;
};

bool Id_Query_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint32_Index, Uint32_Index > >& ranges,
     const vector< uint32 >& ids)
{
  if (stmt->get_type() == Statement::NODE)
  {
    if (ids.empty())
      collect_elems(rman, *osm_base_settings().NODES, stmt->get_lower(), stmt->get_upper(),
		    into.nodes);
    else
      collect_elems(rman, *osm_base_settings().NODES, stmt->get_lower(), stmt->get_upper(),
		    ids, into.nodes);
  }
		    
  return true;
}

bool Id_Query_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint31_Index, Uint31_Index > >& ranges,
     int type, const vector< uint32 >& ids)
{
  if (stmt->get_type() == Statement::WAY)
  {
    if (ids.empty())
      collect_elems(rman, *osm_base_settings().WAYS, stmt->get_lower(), stmt->get_upper(),
		    into.ways);
    else
      collect_elems(rman, *osm_base_settings().WAYS, stmt->get_lower(), stmt->get_upper(),
		    ids, into.ways);
  }
  else if (stmt->get_type() == Statement::RELATION)
  {
    if (ids.empty())
      collect_elems(rman, *osm_base_settings().RELATIONS, stmt->get_lower(), stmt->get_upper(),
		    into.relations);
    else
      collect_elems(rman, *osm_base_settings().RELATIONS, stmt->get_lower(), stmt->get_upper(),
		    ids, into.relations);
  }

  return true;
}

void Id_Query_Constraint::filter(Resource_Manager& rman, Set& into)
{
  if (stmt->get_type() == Statement::NODE)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.nodes);
  else
    into.nodes.clear();

  if (stmt->get_type() == Statement::WAY)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.ways);
  else
    into.ways.clear();
  
  if (stmt->get_type() == Statement::RELATION)
    filter_elems(stmt->get_lower(), stmt->get_upper(), into.relations);
  else
    into.relations.clear();
}

//-----------------------------------------------------------------------------

Id_Query_Statement::Id_Query_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  attributes["lower"] = "";
  attributes["upper"] = "";
  
  Statement::eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["into"];
  
  if (attributes["type"] == "node")
    type = Statement::NODE;
  else if (attributes["type"] == "way")
    type = Statement::WAY;
  else if (attributes["type"] == "relation")
    type = Statement::RELATION;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"id-query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
    add_static_error(temp.str());
  }
  
  ref = (unsigned int)atol(attributes["ref"].c_str());
  lower = (unsigned int)atol(attributes["lower"].c_str());
  upper = (unsigned int)atol(attributes["upper"].c_str());
  if (ref <= 0)
  {
    if (lower == 0 || upper == 0)
    {
      ostringstream temp;
      temp<<"For the attribute \"ref\" of the element \"id-query\""
	  <<" the only allowed values are positive integers.";
      add_static_error(temp.str());
    }
  }
  else
  {
    lower = ref;
    upper = ref;
  }
}

void Id_Query_Statement::forecast()
{
}

void Id_Query_Statement::execute(Resource_Manager& rman)
{
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

  if (type == NODE)
    collect_elems(rman, *osm_base_settings().NODES, lower, upper, nodes);
  else if (type == WAY)
    collect_elems(rman, *osm_base_settings().WAYS, lower, upper, ways);
  else if (type == RELATION)
    collect_elems(rman, *osm_base_settings().RELATIONS, lower, upper, relations);

  rman.health_check(*this);
}

Id_Query_Statement::~Id_Query_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

Query_Constraint* Id_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Id_Query_Constraint(*this));
  return constraints.back();
}
