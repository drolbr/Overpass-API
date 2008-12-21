#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "cgi-helper.h"

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
  if (node_1.id < node_2.id)
    return true;
  return false;
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
  if (way_1.id < way_2.id)
    return true;
  return false;
}

struct Relation
{
  public:
    Relation(int id_) : id(id_) {}
    
    const int id;
    set< int > node_members;
    set< int > way_members;
    set< int > relation_members;
};

inline bool operator<(const Relation& relation_1, const Relation& relation_2)
{
  if (relation_1.id < relation_2.id)
    return true;
  return false;
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

struct Error
{
  public:
    Error(string text_, int line_number_)
  : text(text_), line_number(line_number_) {}
    
    string text;
    int line_number;
};

class Statement
{
  public:
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement) = 0;
    virtual string get_name() = 0;
    virtual void execute(map< string, Set >& maps) = 0;
    virtual ~Statement() {}
};

MYSQL* mysql(NULL);
vector< Error > parsing_errors;
vector< Error > static_errors;

void eval_cstr_array(string element, map< string, string >& attributes, const char **attr)
{
  for (unsigned int i(0); attr[i]; i += 2)
  {
    map< string, string >::iterator it(attributes.find(attr[i]));
    if (it != attributes.end())
      it->second = attr[i+1];
    else
    {
      ostringstream temp;
      temp<<"Unknown attribute \""<<attr[i]<<"\" in element \""<<element<<'"'
	  <<" in line "<<current_line_number()<<'!';
      static_errors.push_back(Error(temp.str(), current_line_number()));
    }
  }
}

void substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<'"'
      <<". Line: "<<current_line_number();
  static_errors.push_back(Error(temp.str(), current_line_number()));
  
  delete child;
}

void display_static_errors()
{
  cout<<"Content-type: text/html\n\n";
  
  cout<<"<html>\n<head>\n<title>Static Error(s)!</title>\n</head>\n<body>\n";
  for(vector< Error >::const_iterator it(static_errors.begin());
      it != static_errors.end(); ++it)
  {
    cout<<"<p>"<<it->text<<"</p>\n";
  }
  cout<<"\n</body>\n</html>\n";
}

//-----------------------------------------------------------------------------

class Root_Statement : public Statement
{
  public:
    Root_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "osm-script"; }
    virtual void execute(map< string, Set >& maps);
    virtual ~Root_Statement() {}
    
  private:
    vector< Statement* > substatements;
};

void Root_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  eval_cstr_array("osm-script", attributes, attr);
}

void Root_Statement::add_statement(Statement* statement)
{
  if ((statement->get_name() == "id-query") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "print"))
    substatements.push_back(statement);
  else
    substatement_error("osm-script", statement);
}

void Root_Statement::execute(map< string, Set >& maps)
{
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
    (*it)->execute(maps);
}

//-----------------------------------------------------------------------------

class Query_Statement : public Statement
{
  public:
    Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "query"; }
    virtual void execute(map< string, Set >& maps);
    virtual ~Query_Statement() {}
    
  private:
    string output;
    unsigned int type;
};

const unsigned int QUERY_NODE = 1;
const unsigned int QUERY_WAY = 2;
const unsigned int QUERY_RELATION = 3;

void Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["set"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array("query", attributes, attr);
  
  output = attributes["set"];
  if (attributes["type"] == "node")
    type = QUERY_NODE;
  else if (attributes["type"] == "way")
    type = QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = QUERY_RELATION;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
    static_errors.push_back(Error(temp.str(), current_line_number()));
  }
}

void Query_Statement::add_statement(Statement* statement)
{
  substatement_error("query", statement);
}

void Query_Statement::execute(map< string, Set >& maps)
{
}

//-----------------------------------------------------------------------------

class Id_Query_Statement : public Statement
{
  public:
    Id_Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "id-query"; }
    virtual void execute(map< string, Set >& maps);
    virtual ~Id_Query_Statement() {}
    
  private:
    string output;
    unsigned int type;
    unsigned int ref;
};

void Id_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["set"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  
  eval_cstr_array("query", attributes, attr);
  
  output = attributes["set"];
  
  if (attributes["type"] == "node")
    type = QUERY_NODE;
  else if (attributes["type"] == "way")
    type = QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = QUERY_RELATION;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"type\" of the element \"id-query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
    static_errors.push_back(Error(temp.str(), current_line_number()));
  }
  
  ref = (unsigned int)atol(attributes["ref"].c_str());
  if (ref == 0)
  {
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"ref\" of the element \"id-query\""
	<<" the only allowed values are positive integers.";
    static_errors.push_back(Error(temp.str(), current_line_number()));
  }
}

void Id_Query_Statement::add_statement(Statement* statement)
{
  substatement_error("id-query", statement);
}

MYSQL_RES* mysql_query_wrapper(string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    cout<<"Error during SQL query ";
    cout<<'('<<query_status<<"):\n";
    cout<<"Query: "<<query<<'\n';
    cout<<"Error: "<<mysql_error(mysql)<<'\n';
  }

  MYSQL_RES* result(mysql_store_result(mysql));
  if (!result)
  {
    cout<<"Error during SQL query (result is null pointer)\n";
    cout<<mysql_error(mysql)<<'\n';
  }
  
  return result;
}

void Id_Query_Statement::execute(map< string, Set >& maps)
{
  if (ref == 0)
    return;
  
  ostringstream temp;
  if (type == QUERY_NODE)
    temp<<"select id, lat, lon from nodes where id = "<<ref;
  else if (type == QUERY_WAY)
    temp<<"select ways.id, way_members.count, way_members.ref "
	<<"from ways left join way_members on ways.id = way_members.id "
	<<"where ways.id = "<<ref;
  else if (type == QUERY_RELATION)
    temp<<"select relations.id, relation_node_members.ref from relations "
	<<"left join relation_node_members on relations.id = relation_node_members.id "
	<<"where relations.id = "<<ref;
  else
    return;
  
  set< Node > nodes;
  set< Way > ways;
  set< Relation > relations;
  
  MYSQL_RES* result(mysql_query_wrapper(temp.str()));
  
  if (!result)
    return;
  
  if (type == QUERY_NODE)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    if ((row) && (row[0]))
    {
      if ((row[1]) && (row[2]))
	nodes.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
    }
  }
  else if (type == QUERY_WAY)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    if ((row) && (row[0]))
    {
      Way way(atoi(row[0]));
      while ((row) && (row[1]) && (row[2]))
      {
	unsigned int count((unsigned int)atol(row[1]));
	if (way.members.size() < count)
	  way.members.resize(count);
	way.members[count-1] = atoi(row[2]);
	row = mysql_fetch_row(result);
      }
      ways.insert(way);
    }
  }
  else if (type == QUERY_RELATION)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    if ((row) && (row[0]))
    {
      Relation relation(atoi(row[0]));
      while ((row) && (row[1]))
      {
	relation.node_members.insert(atoi(row[1]));
	row = mysql_fetch_row(result);
      }
      
      temp.str("");
      temp<<"select relation_way_members.ref from relation_way_members "
	  <<"where relation_way_members.id = "<<ref;
      result = mysql_query_wrapper(temp.str());
      if (!result)
	return;
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	relation.way_members.insert(atoi(row[0]));
	row = mysql_fetch_row(result);
      }
      
      temp.str("");
      temp<<"select relation_relation_members.ref from relation_relation_members "
	  <<"where relation_relation_members.id = "<<ref;
      result = mysql_query_wrapper(temp.str());
      if (!result)
	return;
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	relation.relation_members.insert(atoi(row[0]));
	row = mysql_fetch_row(result);
      }
      
      relations.insert(relation);
    }
  }
  
  maps[output] = Set(nodes, ways, relations);
}

//-----------------------------------------------------------------------------

class Print_Statement : public Statement
{
  public:
    Print_Statement() {}
    
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "print"; }
    virtual void execute(map< string, Set >& maps);
    virtual ~Print_Statement() {}
    
  private:
    string input;
};

void Print_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["set"] = "_";
  
  eval_cstr_array("print", attributes, attr);
  
  input = attributes["set"];
}

void Print_Statement::add_statement(Statement* statement)
{
  substatement_error("print", statement);
}

void Print_Statement::execute(map< string, Set >& maps)
{
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit != maps.end())
  {
    
    for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	 it != mit->second.get_nodes().end(); ++it)
      cout<<"<node id=\""<<it->id
	  <<"\" lat=\""<<((double)(it->lat))/10000000
	  <<"\" lon=\""<<((double)(it->lon))/10000000<<"\"/>\n";
    for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	 it != mit->second.get_ways().end(); ++it)
    {
      if (it->members.size() == 0)
	cout<<"<way id=\""<<it->id<<"\"/>\n";
      else
      {
	cout<<"<way id=\""<<it->id<<"\">\n";
	for (vector< int >::const_iterator it2(it->members.begin());
		    it2 != it->members.end(); ++it2)
	  cout<<"  <nd ref=\""<<*it2<<"\"/>\n";
	cout<<"</way>\n";
      }
    }
    for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	 it != mit->second.get_relations().end(); ++it)
    {
      if (it->node_members.size() + it->way_members.size() + it->relation_members.size() == 0)
	cout<<"<relation id=\""<<it->id<<"\"/>\n";
      else
      {
	cout<<"<relation id=\""<<it->id<<"\">\n";
	for (set< int >::const_iterator it2(it->node_members.begin());
		    it2 != it->node_members.end(); ++it2)
	  cout<<"  <member type=\"node\" ref=\""<<*it2<<"\"/>\n";
	for (set< int >::const_iterator it2(it->way_members.begin());
		    it2 != it->way_members.end(); ++it2)
	  cout<<"  <member type=\"way\" ref=\""<<*it2<<"\"/>\n";
	for (set< int >::const_iterator it2(it->relation_members.begin());
		    it2 != it->relation_members.end(); ++it2)
	  cout<<"  <member type=\"relation\" ref=\""<<*it2<<"\"/>\n";
	cout<<"</relation>\n";
      }
    }
  }
}

//-----------------------------------------------------------------------------

Statement* generate_statement(string element)
{
  if (element == "osm-script")
    return new Root_Statement();
  else if (element == "id-query")
    return new Id_Query_Statement();
  else if (element == "query")
    return new Query_Statement();
  else if (element == "print")
    return new Print_Statement();
  
  ostringstream temp;
  temp<<"Unknown tag \""<<element<<"\" in line "<<current_line_number()<<'!';
  static_errors.push_back(Error(temp.str(), current_line_number()));
  
  return 0;
}

bool is_known_element(string element)
{
  if ((element == "osm-script") ||
       (element == "id-query") ||
       (element == "query") ||
       (element == "print"))
    return true;
  
  return false;
}

//-----------------------------------------------------------------------------

vector< Statement* > statement_stack;

void start(const char *el, const char **attr)
{
  Statement* statement(generate_statement(el));
  if (statement)
  {
    statement->set_attributes(attr);
    statement_stack.push_back(statement);
  }
}

void end(const char *el)
{
  if ((is_known_element(el)) && (statement_stack.size() > 1))
  {
    Statement* statement(statement_stack.back());
    statement_stack.pop_back();
    statement_stack.back()->add_statement(statement);
  }
}

int main(int argc, char *argv[])
{
  string xml_raw(get_xml_raw());
  
  string parse_status(parse(xml_raw, start, end));
  if (parse_status != "")
  {
    return_error(parse_status);
    return 0;
  }
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    return_error("Connection to database failed.\n");
    return 0;
  }
  
  if (static_errors.size() != 0)
  {
    display_static_errors();
    return 0;
  }
  
  cout<<"Content-type: application/xml\n\n"
      <<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not-osm>\n\n";
  
  cout<<(uintmax_t)time(NULL)<<'\n';
  
  map< string, Set > maps;
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
       it != statement_stack.end(); ++it)
    (*it)->execute(maps);
  
  cout<<(uintmax_t)time(NULL)<<'\n';

  cout<<"\n</not-osm>"<<'\n';
  
  mysql_close(mysql);
  
  return 0;
}
