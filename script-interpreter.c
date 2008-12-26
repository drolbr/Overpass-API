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
    set< pair< int, int > > node_members;
    set< pair< int, int > > way_members;
    set< pair< int, int > > relation_members;
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

vector< Error > parsing_errors;
vector< Error > static_errors;
MYSQL* mysql(NULL);
vector< string > role_cache;

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
      temp<<"In Line "<<current_line_number()
	  <<": Unknown attribute \""<<attr[i]<<"\" in element \""<<element<<"\".";
      static_errors.push_back(Error(temp.str(), current_line_number()));
    }
  }
}

void substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"In Line "<<current_line_number()
      <<": Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
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

void prepare_caches()
{
  role_cache.push_back("");
  
  MYSQL_RES* result(mysql_query_wrapper("select id, role from member_roles"));
  if (!result)
    return;
  
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]))
  {
    unsigned int id((unsigned int)atol(row[0]));
    if (role_cache.size() <= id)
      role_cache.resize(id+64);
    role_cache[id] = row[1];
    row = mysql_fetch_row(result);
  }
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

class Has_Key_Value_Statement : public Statement
{
  public:
    Has_Key_Value_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "has-kv"; }
    virtual void execute(map< string, Set >& maps);
    virtual ~Has_Key_Value_Statement() {}
    
    string get_key() { return key; }
    string get_value() { return value; }
    
  private:
    string key, value;
};

void Has_Key_Value_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["v"] = "";
  
  eval_cstr_array("has-kv", attributes, attr);
  
  ostringstream temp;
  escape_xml(temp, attributes["k"]);
  key = temp.str();
  temp.str("");
  escape_xml(temp, attributes["v"]);
  value = temp.str();
  if (key == "")
  {
    temp.str("");
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    static_errors.push_back(Error(temp.str(), current_line_number()));
  }
}

void Has_Key_Value_Statement::add_statement(Statement* statement)
{
  substatement_error("has-kv", statement);
}

void Has_Key_Value_Statement::execute(map< string, Set >& maps)
{
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
    vector< pair< string, string > > key_values;
};

const unsigned int QUERY_NODE = 1;
const unsigned int QUERY_WAY = 2;
const unsigned int QUERY_RELATION = 3;

void Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array("query", attributes, attr);
  
  output = attributes["into"];
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
  if (statement->get_name() == "has-kv")
    key_values.push_back(make_pair< string, string >
	(((Has_Key_Value_Statement*)statement)->get_key(),
	  ((Has_Key_Value_Statement*)statement)->get_value()));
  else
    substatement_error("query", statement);
}

void Query_Statement::execute(map< string, Set >& maps)
{
  if (key_values.size() == 0)
    return;
  
  ostringstream temp;
  if (type == QUERY_NODE)
  {
    if (key_values.size() == 1)
      temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	  <<"left join node_tags on nodes.id = node_tags.id ";
    else
      temp<<"select node_tags.id from node_tags ";
    if (key_values.front().second == "")
      temp<<"left join key_s on key_s.id = node_tags.key_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"order by node_tags.id";
    else
      temp<<"left join key_s on key_s.id = node_tags.key_ "
	  <<"left join value_s on value_s.id = node_tags.value_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	  <<"order by node_tags.id";
  }
  else if (type == QUERY_WAY)
  {
    if (key_values.front().second == "")
      temp<<"select ways.id from ways "
	  <<"left join way_tags on ways.id = way_tags.id "
	  <<"left join key_s on key_s.id = way_tags.key_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"order by ways.id";
    else
      temp<<"select ways.id from ways "
	  <<"left join way_tags on ways.id = way_tags.id "
	  <<"left join key_s on key_s.id = way_tags.key_ "
	  <<"left join value_s on value_s.id = way_tags.value_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	  <<"order by ways.id";
  }
  else if (type == QUERY_RELATION)
  {
    if (key_values.front().second == "")
      temp<<"select relations.id from relations "
	  <<"left join relation_tags on relations.id = relation_tags.id "
	  <<"left join key_s on key_s.id = relation_tags.key_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"order by relations.id";
    else
      temp<<"select relations.id from relations "
	  <<"left join relation_tags on relations.id = relation_tags.id "
	  <<"left join key_s on key_s.id = relation_tags.key_ "
	  <<"left join value_s on value_s.id = relation_tags.value_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	  <<"order by relations.id";
  }
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
    set< int > tnodes;
    if (key_values.size() > 1)
    {
      while ((row) && (row[0]))
      {
	tnodes.insert(atoi(row[0]));
	row = mysql_fetch_row(result);
      }
    }
    else
    {
      while ((row) && (row[0]) && (row[1]) && (row[2]))
      {
	nodes.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
	row = mysql_fetch_row(result);
      }
    }
    
    unsigned int key_count(1);
    while (key_count < key_values.size()-1)
    {
      set< int > new_nodes;
      for (set< int >::const_iterator it(tnodes.begin()); it != tnodes.end(); )
      {
	temp.str("");
	if (key_values[key_count].second == "")
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and node_tags.id in ("<<*it;
	else
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"left join value_s on value_s.id = node_tags.value_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	      <<"and node_tags.id in ("<<*it;
	unsigned int i(0);
	while (((++it) != tnodes.end()) && (i++ < 10000))
	  temp<<", "<<*it;
	temp<<") order by node_tags.id";
	
	delete result;
	result = mysql_query_wrapper(temp.str());
	
	if (!result)
	  return;
	
	row = mysql_fetch_row(result);
	while ((row) && (row[0]))
	{
	  new_nodes.insert(atoi(row[0]));
	  row = mysql_fetch_row(result);
	}
      }
      tnodes = new_nodes;
      
      ++key_count;
    }
    
    if (key_values.size() > 1)
    {
      for (set< int >::const_iterator it(tnodes.begin()); it != tnodes.end(); )
      {
	temp.str("");
	if (key_values[key_count].second == "")
	  temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	      <<"left join node_tags on nodes.id = node_tags.id "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and node_tags.id in ("<<*it;
	else
	  temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	      <<"left join node_tags on nodes.id = node_tags.id "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"left join value_s on value_s.id = node_tags.value_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	      <<"and node_tags.id in ("<<*it;
	unsigned int i(0);
	while (((++it) != tnodes.end()) && (i++ < 10000))
	  temp<<", "<<*it;
	temp<<") order by nodes.id";
	
	delete result;
	result = mysql_query_wrapper(temp.str());
	
	if (!result)
	  return;
	
	row = mysql_fetch_row(result);
	while ((row) && (row[0]) && (row[1]) && (row[2]))
	{
	  nodes.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
	  row = mysql_fetch_row(result);
	}
      }
    }
  }
  else if (type == QUERY_WAY)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    set< int > tways;
    while ((row) && (row[0]))
    {
      tways.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      set< int > new_ways;
      for (set< int >::const_iterator it(tways.begin()); it != tways.end(); )
      {
	temp.str("");
	if (key_values[key_count].second == "")
	  temp<<"select way_tags.id from way_tags "
	      <<"left join key_s on key_s.id = way_tags.key_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and way_tags.id in ("<<*it;
	else
	  temp<<"select way_tags.id from way_tags "
	      <<"left join key_s on key_s.id = way_tags.key_ "
	      <<"left join value_s on value_s.id = way_tags.value_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	      <<"and way_tags.id in ("<<*it;
	unsigned int i(0);
	while (((++it) != tways.end()) && (i++ < 10000))
	  temp<<", "<<*it;
	temp<<") order by way_tags.id";
	
	delete result;
	result = mysql_query_wrapper(temp.str());
	
	if (!result)
	  return;
	
	row = mysql_fetch_row(result);
	while ((row) && (row[0]))
	{
	  new_ways.insert(atoi(row[0]));
	  row = mysql_fetch_row(result);
	}
      }
      tways = new_ways;
      
      ++key_count;
    }
    
    map< int, vector< int > > member_map;
    for (set< int >::const_iterator it(tways.begin()); it != tways.end(); ++it)
      member_map[*it] = vector< int >();

    for (map< int, vector< int > >::iterator it(member_map.begin());
	 it != member_map.end(); )
    {
      map< int, vector< int > >::iterator it2(it);
      temp.str("");
      temp<<"select id, count, ref from way_members "
	  <<"where id in ("<<it->first;
      unsigned int i(0);
      while (((++it) != member_map.end()) && (i++ < 10000))
	temp<<", "<<it->first;
      temp<<") order by id";
	
      delete result;
      result = mysql_query_wrapper(temp.str());
	
      if (!result)
	return;
	
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	int id(atoi(row[0]));
	while (it2->first < id)
	  ++it2;
	it2->second.reserve(100);
	while ((row) && (row[0]) && (it2->first == atoi(row[0])))
	{
	  if ((row[1]) && (row[2]))
	  {
	    unsigned int count((unsigned int)atol(row[1]));
	    if (it2->second.capacity() < count)
	      it2->second.reserve(count+100);
	    if (it2->second.size() < count)
	      it2->second.resize(count);
	    it2->second[count-1] = atoi(row[2]);
	  }
	  row = mysql_fetch_row(result);
	}
	++it2;
      }
    }

    for (set< int >::const_iterator it(tways.begin()); it != tways.end(); ++it)
    {
      Way way(*it);
      way.members = member_map[*it];
      ways.insert(way);
    }
  }
  else if (type == QUERY_RELATION)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    set< int > rels;
    while ((row) && (row[0]))
    {
      rels.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      set< int > new_rels;
      for (set< int >::const_iterator it(rels.begin()); it != rels.end(); )
      {
	temp.str("");
	if (key_values[key_count].second == "")
	  temp<<"select relation_tags.id from relation_tags "
	      <<"left join key_s on key_s.id = relation_tags.key_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and relation_tags.id in ("<<*it;
	else
	  temp<<"select relation_tags.id from relation_tags "
	      <<"left join key_s on key_s.id = relation_tags.key_ "
	      <<"left join value_s on value_s.id = relation_tags.value_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	      <<"and relation_tags.id in ("<<*it;
	unsigned int i(0);
	while (((++it) != rels.end()) && (i++ < 10000))
	  temp<<", "<<*it;
	temp<<") order by relation_tags.id";
	
	delete result;
	result = mysql_query_wrapper(temp.str());
	
	if (!result)
	  return;
	
	row = mysql_fetch_row(result);
	while ((row) && (row[0]))
	{
	  new_rels.insert(atoi(row[0]));
	  row = mysql_fetch_row(result);
	}
      }
      rels = new_rels;
      
      ++key_count;
    }
    
    map< int, set< pair< int, int > > > node_member_map;
    for (set< int >::const_iterator it(rels.begin()); it != rels.end(); ++it)
      node_member_map[*it] = set< pair< int, int > >();

    for (map< int, set< pair< int, int > > >::iterator it(node_member_map.begin());
	 it != node_member_map.end(); )
    {
      map< int, set< pair< int, int > > >::iterator it2(it);
      temp.str("");
      temp<<"select id, ref, role from relation_node_members "
	  <<"where id in ("<<it->first;
      unsigned int i(0);
      while (((++it) != node_member_map.end()) && (i++ < 10000))
	temp<<", "<<it->first;
      temp<<") order by id";
	
      delete result;
      result = mysql_query_wrapper(temp.str());
	
      if (!result)
	return;
	
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	int id(atoi(row[0]));
	while (it2->first < id)
	  ++it2;
	while ((row) && (row[0]) && (it2->first == atoi(row[0])))
	{
	  if (row[1])
	  {
	    if (row[2])
	      it2->second.insert
		  (make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	    else
	      it2->second.insert
		  (make_pair< int, int >(atoi(row[1]), 0));
	  }
	  row = mysql_fetch_row(result);
	}
	++it2;
      }
    }

    map< int, set< pair< int, int > > > way_member_map;
    for (set< int >::const_iterator it(rels.begin()); it != rels.end(); ++it)
      way_member_map[*it] = set< pair< int, int > >();

    for (map< int, set< pair< int, int > > >::iterator it(way_member_map.begin());
	 it != way_member_map.end(); )
    {
      map< int, set< pair< int, int > > >::iterator it2(it);
      temp.str("");
      temp<<"select id, ref, role from relation_way_members "
	  <<"where id in ("<<it->first;
      unsigned int i(0);
      while (((++it) != way_member_map.end()) && (i++ < 10000))
	temp<<", "<<it->first;
      temp<<") order by id";
	
      delete result;
      result = mysql_query_wrapper(temp.str());
	
      if (!result)
	return;
	
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	int id(atoi(row[0]));
	while (it2->first < id)
	  ++it2;
	while ((row) && (row[0]) && (it2->first == atoi(row[0])))
	{
	  if (row[1])
	  {
	    if (row[2])
	      it2->second.insert
		  (make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	    else
	      it2->second.insert
		  (make_pair< int, int >(atoi(row[1]), 0));
	  }
	  row = mysql_fetch_row(result);
	}
	++it2;
      }
    }

    map< int, set< pair< int, int > > > relation_member_map;
    for (set< int >::const_iterator it(rels.begin()); it != rels.end(); ++it)
      relation_member_map[*it] = set< pair< int, int > >();

    for (map< int, set< pair< int, int > > >::iterator it(relation_member_map.begin());
	 it != relation_member_map.end(); )
    {
      map< int, set< pair< int, int > > >::iterator it2(it);
      temp.str("");
      temp<<"select id, ref, role from relation_relation_members "
	  <<"where id in ("<<it->first;
      unsigned int i(0);
      while (((++it) != relation_member_map.end()) && (i++ < 10000))
	temp<<", "<<it->first;
      temp<<") order by id";
	
      delete result;
      result = mysql_query_wrapper(temp.str());
	
      if (!result)
	return;
	
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	int id(atoi(row[0]));
	while (it2->first < id)
	  ++it2;
	while ((row) && (row[0]) && (it2->first == atoi(row[0])))
	{
	  if (row[1])
	  {
	    if (row[2])
	      it2->second.insert
		  (make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	    else
	      it2->second.insert
		  (make_pair< int, int >(atoi(row[1]), 0));
	  }
	  row = mysql_fetch_row(result);
	}
	++it2;
      }
    }

    for (set< int >::const_iterator it(rels.begin()); it != rels.end(); ++it)
    {
      Relation relation(*it);
      relation.node_members = node_member_map[*it];
      relation.way_members = way_member_map[*it];
      relation.relation_members = relation_member_map[*it];
      relations.insert(relation);
    }
  }
  delete result;
  
  maps[output] = Set(nodes, ways, relations);
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
  
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  
  eval_cstr_array("query", attributes, attr);
  
  output = attributes["into"];
  
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
    temp<<"select relations.id, relation_node_members.ref, relation_node_members.role from relations "
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
      way.members.reserve(16);
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
	if (row[2])
	  relation.node_members.insert
	      (make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	else
	  relation.node_members.insert
	      (make_pair< int, int >(atoi(row[1]), 0));
	row = mysql_fetch_row(result);
      }
      
      temp.str("");
      temp<<"select ref, role from relation_way_members "
	  <<"where relation_way_members.id = "<<ref;
      delete result;
      result = mysql_query_wrapper(temp.str());
      if (!result)
	return;
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	if (row[1])
	  relation.way_members.insert
	      (make_pair< int, int >(atoi(row[0]), atoi(row[1])));
	else
	  relation.way_members.insert
	      (make_pair< int, int >(atoi(row[0]), 0));
	row = mysql_fetch_row(result);
      }
      
      temp.str("");
      temp<<"select ref, role from relation_relation_members "
	  <<"where relation_relation_members.id = "<<ref;
      delete result;
      result = mysql_query_wrapper(temp.str());
      if (!result)
	return;
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	if (row[1])
	  relation.relation_members.insert
	      (make_pair< int, int >(atoi(row[0]), atoi(row[1])));
	else
	  relation.relation_members.insert
	      (make_pair< int, int >(atoi(row[0]), 0));
	row = mysql_fetch_row(result);
      }
      
      relations.insert(relation);
    }
  }
  delete result;
  
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
    unsigned int mode;
};

const unsigned int PRINT_IDS_ONLY = 1;
const unsigned int PRINT_SKELETON = 2;
const unsigned int PRINT_BODY = 3;

void Print_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["mode"] = "skeleton";
  
  eval_cstr_array("print", attributes, attr);
  
  input = attributes["from"];
  
  if (attributes["mode"] == "ids_only")
    mode = PRINT_IDS_ONLY;
  else if (attributes["mode"] == "skeleton")
    mode = PRINT_SKELETON;
  else if (attributes["mode"] == "body")
    mode = PRINT_BODY;
  else
  {
    mode = 0;
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\" or \"body\".";
    static_errors.push_back(Error(temp.str(), current_line_number()));
  }
}

void Print_Statement::add_statement(Statement* statement)
{
  substatement_error("print", statement);
}

void out_node(const Node& node, bool complete = true)
{
  cout<<"<node id=\""<<node.id
      <<"\" lat=\""<<((double)(node.lat))/10000000
      <<"\" lon=\""<<((double)(node.lon))/10000000<<'\"'
      <<(complete ? "/>" : ">")<<'\n';
}

void out_way(const Way& way, bool complete = true)
{
  if (way.members.size() == 0)
    cout<<"<way id=\""<<way.id<<'\"'
	<<(complete ? "/>" : ">")<<'\n';
  else
  {
    cout<<"<way id=\""<<way.id<<"\">\n";
    for (vector< int >::const_iterator it2(way.members.begin());
	 it2 != way.members.end(); ++it2)
      cout<<"  <nd ref=\""<<*it2<<"\"/>\n";
    if (complete)
      cout<<"</way>\n";
  }
}

void out_relation(const Relation& rel, bool complete = true)
{
  if ((rel.node_members.size() + rel.way_members.size() + rel.relation_members.size() == 0) && (complete))
    cout<<"<relation id=\""<<rel.id<<"\"/>\n";
  else
  {
    cout<<"<relation id=\""<<rel.id<<"\">\n";
    for (set< pair< int, int > >::const_iterator it2(rel.node_members.begin());
	 it2 != rel.node_members.end(); ++it2)
      cout<<"  <member type=\"node\" ref=\""<<it2->first
	  <<"\" role=\""<<role_cache[it2->second]<<"\"/>\n";
    for (set< pair< int, int > >::const_iterator it2(rel.way_members.begin());
	 it2 != rel.way_members.end(); ++it2)
      cout<<"  <member type=\"way\" ref=\""<<it2->first
	  <<"\" role=\""<<role_cache[it2->second]<<"\"/>\n";
    for (set< pair< int, int > >::const_iterator it2(rel.relation_members.begin());
	 it2 != rel.relation_members.end(); ++it2)
      cout<<"  <member type=\"relation\" ref=\""<<it2->first
	  <<"\" role=\""<<role_cache[it2->second]<<"\"/>\n";
    if (complete)
      cout<<"</relation>\n";
  }
}

void Print_Statement::execute(map< string, Set >& maps)
{
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit != maps.end())
  {
    if (mode == PRINT_IDS_ONLY)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); ++it)
	cout<<"<node id=\""<<it->id<<"\"/>\n";
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); ++it)
	cout<<"<way id=\""<<it->id<<"\"/>\n";
      for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
	cout<<"<relation id=\""<<it->id<<"\"/>\n";
    }
    else if (mode == PRINT_SKELETON)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); ++it)
	out_node(*it);
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); ++it)
	out_way(*it);
      for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
	out_relation(*it);
    }
    else if (mode == PRINT_BODY)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); )
      {
	set< Node >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select node_tags.id, key_s.key_, value_s.value_ from node_tags "
	    <<"left join key_s on node_tags.key_ = key_s.id "
	    <<"left join value_s on node_tags.value_ = value_s.id "
	    <<"where node_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_nodes().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by node_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_node(*it2);
	    ++it2;
	  }
	  out_node(*it2, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</node>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_node(*it2);
	  ++it2;
	}
	delete result;
      }
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); )
      {
	set< Way >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select way_tags.id, key_s.key_, value_s.value_ from way_tags "
	    <<"left join key_s on way_tags.key_ = key_s.id "
	    <<"left join value_s on way_tags.value_ = value_s.id "
	    <<"where way_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_ways().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by way_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_way(*it2);
	    ++it2;
	  }
	  out_way(*it2, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</way>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_way(*it2);
	  ++it2;
	}
	delete result;
      }
      for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); )
      {
	set< Relation >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select relation_tags.id, key_s.key_, value_s.value_ from relation_tags "
	    <<"left join key_s on relation_tags.key_ = key_s.id "
	    <<"left join value_s on relation_tags.value_ = value_s.id "
	    <<"where relation_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_relations().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by relation_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_relation(*it2);
	    ++it2;
	  }
	  out_relation(*it2, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</relation>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_relation(*it2);
	  ++it2;
	}
	delete result;
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
  else if (element == "has-kv")
    return new Has_Key_Value_Statement();
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
       (element == "query") ||
       (element == "has-kv") ||
       (element == "id-query") ||
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
    //Include an end-control to catch e.g. empty query-statements?
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
  
  //Sanity-Check
  
  cout<<"Content-type: application/xml\n\n"
      <<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not-osm>\n\n";
  
  cout<<(uintmax_t)time(NULL)<<'\n';
  
  prepare_caches();
  
  map< string, Set > maps;
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
       it != statement_stack.end(); ++it)
    (*it)->execute(maps);
  
  cout<<(uintmax_t)time(NULL)<<'\n';

  cout<<"\n</not-osm>"<<'\n';
  
  mysql_close(mysql);
  
  return 0;
}
