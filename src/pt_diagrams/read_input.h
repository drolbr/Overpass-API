#ifndef DE__OSM3S__READ_INPUT
#define DE__OSM3S__READ_INPUT

/*****************************************************************
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <map>
#include <string>
#include <vector>

using namespace std;

typedef unsigned int uint32;

class OSMElement
{
  public:
    virtual void operator()() const {}
};

struct Node : public OSMElement
{
  map< string, string > tags;
  double lat;
  double lon;
};

struct Way : public OSMElement
{
  map< string, string > tags;
  vector< Node* > nds;
};

struct Relation : public OSMElement
{
  map< string, string > tags;
  vector< pair< OSMElement*, string > > members;
};

struct RelMember
{
  int type;
  uint32 ref;
  string role;
  
  static const int NODE = 1;
  static const int WAY = 2;
  static const int RELATION = 3;
};

struct OSMData
{
  ~OSMData();
  
  map< uint32, Node* > nodes;
  map< uint32, Way* > ways;
  map< uint32, Relation* > relations;
};

const OSMData& read_osm();

#endif
