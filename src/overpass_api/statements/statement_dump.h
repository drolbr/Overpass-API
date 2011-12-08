#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_DUMP_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_DUMP_H

#include <map>
#include <string>
#include <vector>

using namespace std;

/**
 * The base class for all statements
 */
class Statement_Dump
{
  public:
    Statement_Dump(string name_, const map< string, string >& attributes_)
        : name(name_), attributes(attributes_) {}
    ~Statement_Dump();
    
    void add_statement(Statement_Dump* statement, string text);
    string dump_xml() const;
    string dump_pretty_map_ql() const;
    string dump_compact_map_ql() const;
    
    static Statement_Dump* create_statement(string element, int line_number,
				       const map< string, string >& attributes);
  private:
    string name;
    map< string, string > attributes;
    vector< Statement_Dump* > substatements;
};

#endif
