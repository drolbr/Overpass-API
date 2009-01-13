#ifndef MAKE_AREA_STATEMENT_DEFINED
#define MAKE_AREA_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

// Tries to make an area from the ways in the "from"-set.
// It assumes that
// - all nodes referenced in the ways exist also in the nodes
// - the ways do intersect each other only at nodes that are members of all involved ways
// - the ways can be concatenated such that they form only closed ways.
// It produces the datastructure Area described in script_datatypes.h
// - due to the size restrictions, the algorithm might split up line segments and produce
//   addtional vertices. It does not add nodes for these datastructures to the "into"-set,
//   they are contained only in the Area dataset.
class Make_Area_Statement : public Statement
{
  public:
    Make_Area_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() { return "make-area"; }
    virtual string get_result_name() { return output; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Make_Area_Statement() {}
    
  private:
    string input, output, tags;
    unsigned int type;
};

#endif
