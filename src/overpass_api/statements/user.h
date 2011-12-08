#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__USER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__USER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class User_Statement : public Statement
{
  public:
    User_Statement(int line_number_, const map< string, string >& input_attributes);
    virtual string get_name() const { return "user"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~User_Statement();

    virtual Query_Constraint* get_query_constraint();
    
    void calc_ranges
        (set< pair< Uint32_Index, Uint32_Index > >& node_req,
         set< pair< Uint31_Index, Uint31_Index > >& other_req,
         Transaction& transaction);
	 
    // Reads the user id from the database.
    uint32 get_id(Transaction& transaction);
    
    // Works only if get_id(Transaction&) has been called before.
    uint32 get_id() const { return user_id; }
    
  private:
    string input, output;
    uint32 user_id;
    string user_name;
    string result_type;
    vector< Query_Constraint* > constraints;
};

#endif
