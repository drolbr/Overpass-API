#ifndef SCRIPT_DATATYPES
#define SCRIPT_DATATYPES

#include <map>
#include <set>
#include <string>
#include <vector>

#include <mysql.h>

using namespace std;

struct Node
{
  public:
    Node(int id_, int lat_, int lon_) : id(id_), lat(lat_), lon(lon_) {}
    
    const int id;
    int lat, lon;
};

inline bool operator<(const Node& node_1, const Node& node_2)
{
  return (node_1.id < node_2.id);
}

struct Way
{
  public:
    Way(int id_) : id(id_) {}
    
    const int id;
    vector< int > members;
};

inline bool operator<(const Way& way_1, const Way& way_2)
{
  return (way_1.id < way_2.id);
}

struct Relation
{
  public:
    Relation(int id_) : id(id_) {}
    
    const int id;
    set< pair< int, int > > node_members;
    set< pair< int, int > > way_members;
    set< pair< int, int > > relation_members;
};

inline bool operator<(const Relation& relation_1, const Relation& relation_2)
{
  return (relation_1.id < relation_2.id);
}

class Set
{
  public:
    Set() {}
    Set(const set< Node >& nodes_,
	const set< Way >& ways_,
        const set< Relation >& relations_)
  : nodes(nodes_), ways(ways_), relations(relations_) {}
    
    const set< Node >& get_nodes() const { return nodes; }
    const set< Way >& get_ways() const { return ways; }
    const set< Relation >& get_relations() const { return relations; }
  
  private:
    set< Node > nodes;
    set< Way > ways;
    set< Relation > relations;
};

class Statement
{
  public:
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement) = 0;
    virtual string get_name() = 0;
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) = 0;
    virtual ~Statement() {}
};

#endif
