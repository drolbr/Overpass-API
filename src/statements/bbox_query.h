#ifndef BBOX_QUERY_STATEMENT_DEFINED
#define BBOX_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Bbox_Query_Statement : public Statement
{
  public:
    Bbox_Query_Statement(int line_number_, int stmt_id_)
      : Statement(line_number_, stmt_id_)/*, area_restriction(0),
        bbox_restriction(0)*/ {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "bbox-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Bbox_Query_Statement() {}
    
    vector< pair< uint32, uint32 > >* calc_ranges()
    {
      return calc_ranges(south, north, west, east);
    }
    
    double get_south() const { return south; }
    double get_north() const { return north; }
    double get_west() const { return west; }
    double get_east() const { return east; }
    
  private:
    string output;
    unsigned int type;
    double south, north, west, east;
    
    static vector< pair< uint32, uint32 > >* calc_ranges
        (double south, double north, double west, double east);
};

#endif
