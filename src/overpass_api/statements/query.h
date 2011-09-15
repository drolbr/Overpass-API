#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"
#include "area_query.h"
#include "around.h"
#include "bbox_query.h"
#include "item.h"
#include "newer.h"
#include "user.h"

using namespace std;

class Query_Constraint
{
  public:
    virtual void filter(Resource_Manager& rman, Set& into) = 0;
    virtual ~Query_Constraint() {}
};

//-----------------------------------------------------------------------------

class Query_Statement : public Statement
{
  public:
    Query_Statement(int line_number_)
      : Statement(line_number_),
        area_restriction(0), around_restriction(0), bbox_restriction(0), item_restriction(0) {}
    virtual ~Query_Statement();
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    
  private:
    string output;
    unsigned int type;
    vector< pair< string, string > > key_values;
    Area_Query_Statement* area_restriction;
    Around_Statement* around_restriction;
    Bbox_Query_Statement* bbox_restriction;
    Item_Statement* item_restriction;
    
    vector< Query_Constraint* > constraints;
    
    vector< uint32 >* collect_ids
        (const vector< pair< string, string > >& key_values,
	 const File_Properties& file_prop, uint32 stopwatch_account,
	 Resource_Manager& rman);
	 
    template < typename TIndex, typename TObject >
    void get_elements_by_id_from_db
        (map< TIndex, vector< TObject > >& elements,
	 const vector< uint32 >& ids, const set< pair< TIndex, TIndex > >& range_req,
         Resource_Manager& rman, File_Properties& file_prop);
};

class Has_Kv_Statement : public Statement
{
  public:
    Has_Kv_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "has-kv"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman) {}
    virtual ~Has_Kv_Statement() {}
    
    string get_key() { return key; }
    string get_value() { return value; }
    
  private:
    string key, value;
};

#endif
