#include "item.h"

using namespace std;

class Item_Constraint : public Query_Constraint
{
  public:
    Item_Constraint(Item_Statement& item_) : item(&item_) {}
    bool collect(Resource_Manager& rman, Set& into, int type, const vector< uint32 >& ids);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Item_Constraint() {}
    
  private:
    Item_Statement* item;
};

template< typename TIndex, typename TObject >
void collect_elements(const map< TIndex, vector< TObject > >& from,
		      map< TIndex, vector< TObject > >& into,
		      const vector< uint32 >& ids)
{
  into.clear();
  for (typename map< TIndex, vector< TObject > >::const_iterator iit = from.begin();
      iit != from.end(); ++iit)
  {
    for (typename vector< TObject >::const_iterator cit = iit->second.begin();
        cit != iit->second.end(); ++cit)
    {
      if ((binary_search(ids.begin(), ids.end(), cit->id)) || (ids.empty()))
	into[iit->first].push_back(*cit);
    }
  }
}

bool Item_Constraint::collect(Resource_Manager& rman, Set& into,
			      int type, const vector< uint32 >& ids)
{
  if (type == QUERY_NODE)
    collect_elements(rman.sets()[item->get_result_name()].nodes, into.nodes, ids);
  if (type == QUERY_WAY)
    collect_elements(rman.sets()[item->get_result_name()].ways, into.ways, ids);
  if (type == QUERY_RELATION)
    collect_elements(rman.sets()[item->get_result_name()].relations, into.relations, ids);
  return true;
}

template< typename TIndex, typename TObject >
void item_filter_map
    (map< TIndex, vector< TObject > >& modify,
     const map< TIndex, vector< TObject > >& read)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    sort(it->second.begin(), it->second.end());
    typename map< TIndex, vector< TObject > >::const_iterator
        from_it = read.find(it->first);
    if (from_it == read.end())
    {
      it->second.clear();
      continue;
    }
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = from_it->second.begin();
        iit != from_it->second.end(); ++iit)
    {
      if (binary_search(it->second.begin(), it->second.end(), *iit))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

void Item_Constraint::filter(Resource_Manager& rman, Set& into)
{
  item_filter_map(into.nodes, rman.sets()[item->get_result_name()].nodes);
  item_filter_map(into.ways, rman.sets()[item->get_result_name()].ways);
  item_filter_map(into.relations, rman.sets()[item->get_result_name()].relations);
}

//-----------------------------------------------------------------------------

Item_Statement::Item_Statement(int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["set"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["set"];
}

Item_Statement::~Item_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

void Item_Statement::forecast()
{
/*  declare_union_set(output);
  
  finish_statement_forecast();
  
  display_full();
  display_state();*/
}

Query_Constraint* Item_Statement::get_query_constraint()
{
  constraints.push_back(new Item_Constraint(*this));
  return constraints.back();
}
