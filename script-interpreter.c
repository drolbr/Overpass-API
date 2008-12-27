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

set< int >& multiint_query(string query, set< int >& result_set)
{
  MYSQL_RES* result(mysql_query_wrapper(query));
  if (!result)
    return result_set;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]))
  {
    result_set.insert(atoi(row[0]));
    row = mysql_fetch_row(result);
  }
  delete result;
  return result_set;
}

set< Node >& multiNode_query(string query, set< Node >& result_set)
{
  MYSQL_RES* result(mysql_query_wrapper(query));
  if (!result)
    return result_set;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]))
  {
    result_set.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
    row = mysql_fetch_row(result);
  }
  delete result;
  return result_set;
}

set< int >& multiint_to_multiint_query
    (string prefix, string suffix, const set< int >& source, set< int >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    delete result;
  }
  return result_set;
}

set< Node >& multiint_to_multiNode_query
    (string prefix, string suffix, const set< int >& source, set< Node >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]) && (row[1]) && (row[2]))
    {
      result_set.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
      row = mysql_fetch_row(result);
    }
    delete result;
  }
  return result_set;
}

set< Way >& multiint_to_multiWay_query
    (string prefix, string suffix, const set< int >& source, set< Way >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      Way way(atoi(row[0]));
      way.members.reserve(10);
      while ((row) && (row[0]) && (way.id == atoi(row[0])))
      {
	if ((row[1]) && (row[2]))
	{
	  unsigned int count((unsigned int)atol(row[1]));
	  if (way.members.capacity() < count)
	    way.members.reserve(count+10);
	  if (way.members.size() < count)
	    way.members.resize(count);
	  way.members[count-1] = atoi(row[2]);
	}
	row = mysql_fetch_row(result);
      }
      result_set.insert(way);
    }
    delete result;
  }
  return result_set;
}

set< Relation >& multiint_to_multiRelation_query
    (string prefix1, string suffix1, string prefix2, string suffix2, string prefix3, string suffix3,
     const set< int >& source, set< Relation >& result_set)
{
  map< int, set< pair< int, int > > > node_members;
  map< int, set< pair< int, int > > > way_members;
  map< int, set< pair< int, int > > > relation_members;
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix1;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix1;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > nodes;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    nodes.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    nodes.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      node_members[id] = nodes;
    }
    delete result;
  }
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix2;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix2;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > ways;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    ways.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    ways.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      way_members[id] = ways;
    }
    delete result;
  }
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix3;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix3;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > relations;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    relations.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    relations.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      relation_members[id] = relations;
    }
    delete result;
  }
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    Relation relation(*it);
    relation.node_members = node_members[*it];
    relation.way_members = way_members[*it];
    relation.relation_members = relation_members[*it];
    result_set.insert(relation);
  }
  
  return result_set;
}

set< int >& multiWay_to_multiint_query
    (string prefix, string suffix, const set< Way >& source, set< int >& result_set)
{
  for (set< Way >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    delete result;
  }
  return result_set;
}

set< int >& multiRelation_to_multiint_query
    (string prefix, string suffix, const set< Relation >& source, set< int >& result_set)
{
  for (set< Relation >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_wrapper(temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    delete result;
  }
  return result_set;
}

//-----------------------------------------------------------------------------

rusage resources;

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
  
  eval_cstr_array(get_name(), attributes, attr);
}

void Root_Statement::add_statement(Statement* statement)
{
  if ((statement->get_name() == "id-query") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "print") ||
       (statement->get_name() == "recurse"))
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Root_Statement::execute(map< string, Set >& maps)
{
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    cout<<"+++ "<<(uintmax_t)time(NULL)<<'\n';
    (*it)->execute(maps);
  }
  cout<<"+++ "<<(uintmax_t)time(NULL)<<'\n';
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
  
  eval_cstr_array(get_name(), attributes, attr);
  
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
  substatement_error(get_name(), statement);
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
  
  eval_cstr_array(get_name(), attributes, attr);
  
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
    substatement_error(get_name(), statement);
}

void Query_Statement::execute(map< string, Set >& maps)
{
  if (key_values.size() == 0)
    return;
  
  if (type == QUERY_NODE)
  {
    if (key_values.size() == 1)
    {
      ostringstream temp;
      temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	  <<"left join node_tags on nodes.id = node_tags.id ";
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
      
      set< Node > nodes;
      maps[output] = Set(multiNode_query(temp.str(), nodes),
			 set< Way >(), set< Relation >());
    }
    else
    {
      ostringstream temp;
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
    
      set< int > tnodes;
      tnodes = multiint_query(temp.str(), tnodes);
    
      unsigned int key_count(1);
      while (key_count < key_values.size()-1)
      {
	temp.str("");
	if (key_values[key_count].second == "")
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and node_tags.id in";
	else
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"left join value_s on value_s.id = node_tags.value_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	      <<"and node_tags.id in";
      
	set< int > new_nodes;
	tnodes = multiint_to_multiint_query(temp.str(), "order by node_tags.id", tnodes, new_nodes);
      
	++key_count;
      }
    
      temp.str("");
      if (key_values[key_count].second == "")
	temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	    <<"left join node_tags on nodes.id = node_tags.id "
	    <<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and node_tags.id in";
      else
	temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	    <<"left join node_tags on nodes.id = node_tags.id "
	    <<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	    <<"and node_tags.id in";
      
      set< Node > nodes;
      maps[output] = Set(multiint_to_multiNode_query(temp.str(), "order by nodes.id", tnodes, nodes),
	  set< Way >(), set< Relation >());
    }
  }
  else if (type == QUERY_WAY)
  {
    ostringstream temp;
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
  
    set< int > tways;
    tways = multiint_query(temp.str(), tways);
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      temp.str("");
      if (key_values[key_count].second == "")
	temp<<"select way_tags.id from way_tags "
	    <<"left join key_s on key_s.id = way_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and way_tags.id in";
      else
	temp<<"select way_tags.id from way_tags "
	    <<"left join key_s on key_s.id = way_tags.key_ "
	    <<"left join value_s on value_s.id = way_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	    <<"and way_tags.id in";
      
      set< int > new_ways;
      tways = multiint_to_multiint_query(temp.str(), "order by way_tags.id", tways, new_ways);
      
      ++key_count;
    }
    
    set< Way > ways;
    ways = multiint_to_multiWay_query
	("select id, count, ref from way_members "
	"where id in", "order by id", tways, ways);
    maps[output] = Set(set< Node >(), ways, set< Relation >());
  }
  else if (type == QUERY_RELATION)
  {
    ostringstream temp;
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
  
    set< int > rels;
    rels = multiint_query(temp.str(), rels);
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      temp.str("");
      if (key_values[key_count].second == "")
	temp<<"select relation_tags.id from relation_tags "
	    <<"left join key_s on key_s.id = relation_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and relation_tags.id in";
      else
	temp<<"select relation_tags.id from relation_tags "
	    <<"left join key_s on key_s.id = relation_tags.key_ "
	    <<"left join value_s on value_s.id = relation_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	    <<"and relation_tags.id in";
      
      set< int > new_rels;
      rels = multiint_to_multiint_query(temp.str(), "order by relation_tags.id", rels, new_rels);
      
      ++key_count;
    }
  
    set< Relation > relations;
    relations = multiint_to_multiRelation_query
	("select id, ref, role from relation_node_members "
	"where id in", "order by id",
	"select id, ref, role from relation_way_members "
	"where id in", "order by id",
	"select id, ref, role from relation_relation_members "
	"where id in", "order by id", rels, relations);
    maps[output] = Set(set< Node >(), set< Way >(), relations);
  }
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
  
  eval_cstr_array(get_name(), attributes, attr);
  
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
  substatement_error(get_name(), statement);
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

class Recurse_Statement : public Statement
{
  public:
    Recurse_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "recurse"; }
    virtual void execute(map< string, Set >& maps);
    virtual ~Recurse_Statement() {}
    
  private:
    string input, output;
    unsigned int type;
};

const unsigned int RECURSE_WAY_NODE = 1;
const unsigned int RECURSE_RELATION_RELATION = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;

void Recurse_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
  
  if (attributes["type"] == "way-node")
    type = RECURSE_WAY_NODE;
  else if (attributes["type"] == "relation-relation")
    type = RECURSE_RELATION_RELATION;
  else if (attributes["type"] == "relation-way")
    type = RECURSE_RELATION_WAY;
  else if (attributes["type"] == "relation-node")
    type = RECURSE_RELATION_NODE;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"way-node\", \"relation-relation\","
	<<"\"relation-way\" or \"relation-node\".";
    static_errors.push_back(Error(temp.str(), current_line_number()));
  }
}

void Recurse_Statement::add_statement(Statement* statement)
{
  substatement_error(get_name(), statement);
}

void Recurse_Statement::execute(map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation > relations;
  if (input == output)
  {
    nodes = maps[output].get_nodes();
    ways = maps[output].get_ways();
    relations = maps[output].get_relations();
  }
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
  {
    maps[output] = Set(nodes, ways, relations);
    return;
  }
  
  if (type == RECURSE_WAY_NODE)
  {
    set< int > tnodes;
    tnodes = multiWay_to_multiint_query
	("select way_members.ref from ways "
	"left join way_members on way_members.id = ways.id "
	"where ways.id in", "order by ways.id", mit->second.get_ways(), tnodes);
    
    nodes = multiint_to_multiNode_query
	("select id, lat, lon from nodes where id in", "order by id", tnodes, nodes);
  }
  else if (type == RECURSE_RELATION_RELATION)
  {
    set< int > rels;
    rels = multiRelation_to_multiint_query
	("select relation_relation_members.ref from relations "
	"left join relation_relation_members on relation_relation_members.id = relations.id "
	"where relations.id in", "order by relations.id", mit->second.get_relations(), rels);
    
    relations = multiint_to_multiRelation_query
	("select id, ref, role from relation_node_members "
	"where id in", "order by id",
	"select id, ref, role from relation_way_members "
	"where id in", "order by id",
	"select id, ref, role from relation_relation_members "
	"where id in", "order by id", rels, relations);
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    set< int > tways;
    tways = multiRelation_to_multiint_query
	("select relation_way_members.ref from relations "
	"left join relation_way_members on relation_way_members.id = relations.id "
	"where relations.id in", "order by relations.id", mit->second.get_relations(), tways);
    
    ways = multiint_to_multiWay_query
	("select ways.id, way_members.count, way_members.ref from ways "
	"left join way_members on way_members.id = ways.id "
	"where ways.id in", "order by ways.id", tways, ways);
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    set< int > tnodes;
    tnodes = multiRelation_to_multiint_query
	("select relation_node_members.ref from relations "
	"left join relation_node_members on relation_node_members.id = relations.id "
	"where relations.id in", "order by relations.id", mit->second.get_relations(), tnodes);
    
    nodes = multiint_to_multiNode_query
	("select id, lat, lon from nodes where id in", "order by id", tnodes, nodes);
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
  
  eval_cstr_array(get_name(), attributes, attr);
  
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
  substatement_error(get_name(), statement);
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
  else if (element == "recurse")
    return new Recurse_Statement();
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
       (element == "has-kv") ||
       (element == "recurse") ||
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
  
  prepare_caches();
  
  map< string, Set > maps;
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
       it != statement_stack.end(); ++it)
    (*it)->execute(maps);
  
  cout<<"\n</not-osm>"<<'\n';
  
  mysql_close(mysql);
  
  return 0;
}
