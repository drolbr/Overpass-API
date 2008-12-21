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

class Set
{
  public:
    Set(const set< unsigned int >& nodes_,
	const set< unsigned int >& ways_,
        const set< unsigned int >& relations_)
  : nodes(nodes_), ways(ways_), relations(relations_) {}
    const set< unsigned int >& get_nodes() const { return nodes; }
    const set< unsigned int >& get_ways() const { return ways; }
    const set< unsigned int >& get_relations() const { return relations; }
  
  private:
    set< unsigned int > nodes;
    set< unsigned int > ways;
    set< unsigned int > relations;
};

class Statement
{
  public:
    virtual void execute(map< string, Set >& maps) = 0;
    virtual ~Statement() {}
};

MYSQL* mysql(NULL);
  
class Return_Statement : public Statement
{
  public:
    Return_Statement(map< string, string > attributes)
  : arg(attributes["set"]) {}
    
    virtual void execute(map< string, Set >& maps);
    virtual ~Return_Statement() {}
    
    static map< string, string > possible_attributes() {
      map< string, string > attributes;
      attributes["set"] = "_";
      return attributes;
    }
    
  private:
    string arg;
};

void Return_Statement_0execute(map< string, Set >& maps)
{
  string arg;
  map< string, Set >::const_iterator mit(maps.find(arg));
  if (mit != maps.end())
  {
    //With prepared statements: ~180s per Million
    MYSQL_STMT* mysql_stmt(mysql_stmt_init(mysql));
    if (mysql_stmt_prepare(mysql_stmt, "select id, lat, lon from nodes where id = ?", 43))
    {
      cout<<"Error during statement preparation.\n";
      mysql_stmt_close(mysql_stmt);
      return;
    }
    
    unsigned int id(0);
    MYSQL_BIND bind_in[1];
    memset(bind_in, 0, sizeof(bind_in));
    bind_in[0].buffer_type = MYSQL_TYPE_LONG;
    bind_in[0].buffer = &id;
    bind_in[0].is_unsigned = true;
    if (mysql_stmt_bind_param(mysql_stmt, bind_in))
    {
      cout<<"Error during prepared statement parameter binding.\n";
      mysql_stmt_close(mysql_stmt);
      return;
    }
    
    unsigned int res_id;
    int lat, lon;
    
    my_bool is_null[3];
    unsigned long length[3];
    my_bool error[3];
    MYSQL_BIND bind_out[3];
    memset(bind_out, 0, sizeof(bind_out));
    for (unsigned int i(0); i < 3; ++i)
    {
      bind_out[i].buffer_type = MYSQL_TYPE_LONG;
      bind_out[i].is_null = &(is_null[i]);
      bind_out[i].length = &(length[i]);
      bind_out[i].error = &(error[i]);
    }
    bind_out[0].buffer = &res_id;
    bind_out[0].is_unsigned = true;
    bind_out[1].buffer = &lat;
    bind_out[1].is_unsigned = false;
    bind_out[2].buffer = &lon;
    bind_out[2].is_unsigned = false;
    if (mysql_stmt_bind_result(mysql_stmt, bind_out))
    {
      cout<<"Error during prepared statement output buffer binding.\n";
      mysql_stmt_close(mysql_stmt);
      return;
    }
    
    for (set< unsigned int >::const_iterator it(mit->second.get_nodes().begin());
	 it != mit->second.get_nodes().end(); ++it)
    {
      id = *it;
      
      if (mysql_stmt_execute(mysql_stmt))
      {
	cout<<"Error during prepared statement execution.\n";
	break;
      }
      int fetch_status(mysql_stmt_fetch(mysql_stmt));
      if (fetch_status)
      {
	if (fetch_status != MYSQL_NO_DATA)
	{
	  cout<<"Error during prepared statement data fetching.\n";
	  break;
	}
      }
      else
      {
	cout<<"<node id=\""<<res_id
	    <<"\" lat=\""<<((double)lat)/10000000
	    <<"\" lon=\""<<((double)lon)/10000000<<"\"/>\n";
      }
    }
    
    mysql_stmt_close(mysql_stmt);
    
    for (set< unsigned int >::const_iterator it(mit->second.get_ways().begin());
	 it != mit->second.get_ways().end(); ++it)
      cout<<"<way id=\""<<*it<<"\"/>\n";
    for (set< unsigned int >::const_iterator it(mit->second.get_relations().begin());
	 it != mit->second.get_relations().end(); ++it)
      cout<<"<relation id=\""<<*it<<"\"/>\n";
  }
}

void Return_Statement_1execute(map< string, Set >& maps)
{
  string arg;
  const unsigned int BLOCKSIZE = 10000;
  
  map< string, Set >::const_iterator mit(maps.find(arg));
  if (mit != maps.end())
  {
    //With prepared blockwise statements: ~99s per Million at BLOCKSIZE = 10000
    MYSQL_STMT* mysql_stmt(mysql_stmt_init(mysql));
    ostringstream temp;
    temp<<"select id, lat, lon from nodes where id in (?";
    for (unsigned int i(1); i < BLOCKSIZE; ++i)
      temp<<", ?";
    temp<<')';
    if (mysql_stmt_prepare(mysql_stmt, temp.str().c_str(), temp.str().length()))
    {
      cout<<"Error during statement preparation.\n";
      mysql_stmt_close(mysql_stmt);
      return;
    }
    
    unsigned int id[BLOCKSIZE];
    MYSQL_BIND bind_in[BLOCKSIZE];
    memset(bind_in, 0, sizeof(bind_in));
    for (unsigned int i(0); i < BLOCKSIZE; ++i)
    {
      bind_in[i].buffer_type = MYSQL_TYPE_LONG;
      bind_in[i].buffer = &(id[i]);
      bind_in[i].is_unsigned = true;
    }
    if (mysql_stmt_bind_param(mysql_stmt, bind_in))
    {
      cout<<"Error during prepared statement parameter binding.\n";
      mysql_stmt_close(mysql_stmt);
      return;
    }
    
    unsigned int res_id;
    int lat, lon;
    
    my_bool is_null[3];
    unsigned long length[3];
    my_bool error[3];
    MYSQL_BIND bind_out[3];
    memset(bind_out, 0, sizeof(bind_out));
    for (unsigned int i(0); i < 3; ++i)
    {
      bind_out[i].buffer_type = MYSQL_TYPE_LONG;
      bind_out[i].is_null = &(is_null[i]);
      bind_out[i].length = &(length[i]);
      bind_out[i].error = &(error[i]);
    }
    bind_out[0].buffer = &res_id;
    bind_out[0].is_unsigned = true;
    bind_out[1].buffer = &lat;
    bind_out[1].is_unsigned = false;
    bind_out[2].buffer = &lon;
    bind_out[2].is_unsigned = false;
    if (mysql_stmt_bind_result(mysql_stmt, bind_out))
    {
      cout<<"Error during prepared statement output buffer binding.\n";
      mysql_stmt_close(mysql_stmt);
      return;
    }
    
    for (set< unsigned int >::const_iterator it(mit->second.get_nodes().begin());
	 it != mit->second.get_nodes().end(); )
    {
      unsigned int i(0);
      while ((it != mit->second.get_nodes().end()) && (i < BLOCKSIZE))
      {
	id[i] = *it;
	++it;
	++i;
      }
      while (i < BLOCKSIZE)
      {
	id[i] = 0;
	++i;
      }
      
      if (mysql_stmt_execute(mysql_stmt))
      {
	cout<<"Error during prepared statement execution.\n";
	break;
      }
      if (mysql_stmt_store_result(mysql_stmt))
      {
	cout<<"Error during prepared statement data caching.\n";
	break;
      }
      int fetch_status(mysql_stmt_fetch(mysql_stmt));
      while (!fetch_status)
      {
	cout<<"<node id=\""<<res_id
	    <<"\" lat=\""<<((double)lat)/10000000
	    <<"\" lon=\""<<((double)lon)/10000000<<"\"/>\n";
	fetch_status = mysql_stmt_fetch(mysql_stmt);
      }
      if (fetch_status != MYSQL_NO_DATA)
      {
	cout<<"Error during prepared statement data fetching.\n";
	break;
      }
    }
    
    mysql_stmt_close(mysql_stmt);
    
    for (set< unsigned int >::const_iterator it(mit->second.get_ways().begin());
	 it != mit->second.get_ways().end(); ++it)
      cout<<"<way id=\""<<*it<<"\"/>\n";
    for (set< unsigned int >::const_iterator it(mit->second.get_relations().begin());
	 it != mit->second.get_relations().end(); ++it)
      cout<<"<relation id=\""<<*it<<"\"/>\n";
  }
}

void Return_Statement::execute(map< string, Set >& maps)
{
  //string arg;
  map< string, Set >::const_iterator mit(maps.find(arg));
  if (mit != maps.end())
  {
    
    //With direct execution: Million nodes query aborted
    for (set< unsigned int >::const_iterator it(mit->second.get_nodes().begin());
	 it != mit->second.get_nodes().end(); ++it)
    {
      ostringstream temp;
      temp<<"select id, lat, lon from nodes where id = "<<*it;
      int query_status(mysql_query(mysql, temp.str().c_str()));
      if (query_status)
      {
	cout<<"Error during SQL query: "<<*it<<' ';
	cout<<'('<<query_status<<')';
	cout<<'\n';
	cout<<mysql_error(mysql)<<'\n';
	break;
      }
  
      MYSQL_RES* result(mysql_store_result(mysql));
      if (result)
      {
	MYSQL_ROW row(mysql_fetch_row(result));
	while (row)
	{
	  if ((row[0]) && (row[1]) && (row[2]))
	    cout<<"<node id=\""<<row[0]
		<<"\" lat=\""<<atof(row[1])/10000000
		<<"\" lon=\""<<atof(row[2])/10000000<<"\"/>\n";
	  row = mysql_fetch_row(result);
	}
      }
      
      //cout<<"<node id=\""<<*it<<"\"/>\n";
    }
    for (set< unsigned int >::const_iterator it(mit->second.get_ways().begin());
	 it != mit->second.get_ways().end(); ++it)
      cout<<"<way id=\""<<*it<<"\"/>\n";
    for (set< unsigned int >::const_iterator it(mit->second.get_relations().begin());
	 it != mit->second.get_relations().end(); ++it)
      cout<<"<relation id=\""<<*it<<"\"/>\n";
  }
}

void Return_Statement_3execute(map< string, Set >& maps)
{
  string arg;
  map< string, Set >::const_iterator mit(maps.find(arg));
  if (mit != maps.end())
  {
    
    //With blockwise direct execution: ~100s per million rows at BLOCKSIZE = 10000
    for (set< unsigned int >::const_iterator it(mit->second.get_nodes().begin());
	 it != mit->second.get_nodes().end(); )
    {
      ostringstream temp;
      temp<<"select id, lat, lon from nodes where id in ("<<*it;
      unsigned int i(0);
      while (((++it) != mit->second.get_nodes().end()) && (i++ < 10000))
	temp<<", "<<*it;
      temp<<')';
      mysql_query(mysql, temp.str().c_str());
  
      MYSQL_RES* result(mysql_store_result(mysql));
      if (result)
      {
	MYSQL_ROW row(mysql_fetch_row(result));
	while (row)
	{
	  if ((row[0]) && (row[1]) && (row[2]))
	    cout<<"<node id=\""<<row[0]
		<<"\" lat=\""<<atof(row[1])/10000000
		<<"\" lon=\""<<atof(row[2])/10000000<<"\"/>\n";
	  row = mysql_fetch_row(result);
	}
      }
    }
    for (set< unsigned int >::const_iterator it(mit->second.get_ways().begin());
	 it != mit->second.get_ways().end(); ++it)
      cout<<"<way id=\""<<*it<<"\"/>\n";
    for (set< unsigned int >::const_iterator it(mit->second.get_relations().begin());
	 it != mit->second.get_relations().end(); ++it)
      cout<<"<relation id=\""<<*it<<"\"/>\n";
  }
}

void Return_Statement_4execute(map< string, Set >& maps)
{
  string arg;
  map< string, Set >::const_iterator mit(maps.find(arg));
  if (mit != maps.end())
  {
    
    //With blockwise direct execution: ~100s per million rows at BLOCKSIZE = 10000
    //and keys and values - it doesn't affect the speed
    for (set< unsigned int >::const_iterator it(mit->second.get_nodes().begin());
	 it != mit->second.get_nodes().end(); )
    {
      ostringstream temp;
      temp<<"select nodes.id, lat, lon, key_s.key_, value_s.value_ "
	  <<"from nodes left outer join node_tags on node_tags.id = nodes.id "
	  <<"left outer join key_s on node_tags.key_ = key_s.id "
	  <<"left outer join value_s on node_tags.value_ = value_s.id "
	  <<"where nodes.id in ("<<*it;
      unsigned int i(0);
      while (((++it) != mit->second.get_nodes().end()) && (i++ < 10000))
	temp<<", "<<*it;
      temp<<") order by nodes.id";
      mysql_query(mysql, temp.str().c_str());
  
      MYSQL_RES* result(mysql_store_result(mysql));
      if (result)
      {
	MYSQL_ROW row(mysql_fetch_row(result));
	while (row)
	{
	  if ((row[3]) && (row[4]))
	  {
	    cout<<"<node id=\""<<row[0]
		<<"\" lat=\""<<atof(row[1])/10000000
		<<"\" lon=\""<<atof(row[2])/10000000<<"\">\n";
	    unsigned int current_id = atoi(row[0]);
	    while ((row) && ((unsigned int)(atol(row[0])) == current_id))
	    {
	      if ((row[3]) && (row[4]))
		cout<<"  <tag k=\""<<row[3]<<"\" v=\""<<row[4]<<"\"/>\n";
	      row = mysql_fetch_row(result);
	    }
	    cout<<"</node>\n";
	  }
	  else
	  {
	    cout<<"<node id=\""<<row[0]
		<<"\" lat=\""<<atof(row[1])/10000000
		<<"\" lon=\""<<atof(row[2])/10000000<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	}
      }
    }
    for (set< unsigned int >::const_iterator it(mit->second.get_ways().begin());
	 it != mit->second.get_ways().end(); ++it)
      cout<<"<way id=\""<<*it<<"\"/>\n";
    for (set< unsigned int >::const_iterator it(mit->second.get_relations().begin());
	 it != mit->second.get_relations().end(); ++it)
      cout<<"<relation id=\""<<*it<<"\"/>\n";
  }
}

void assign_attributes(map< string, string >& attributes, const char **attr)
{
  for (unsigned int i(0); attr[i]; i += 2)
  {
    map< string, string >::iterator it(attributes.find(attr[i]));
    if (it != attributes.end())
      it->second = attr[i+1];
  }
}

vector< Statement* > statements;

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "query"))
  {
  }
  else if (!strcmp(el, "return"))
  {
    map< string, string > attributes(Return_Statement::possible_attributes());
    assign_attributes(attributes, attr);
    statements.push_back(new Return_Statement(attributes));
  }
  else if (!strcmp(el, "osm-script"))
    ;
  else
  {
    return_error((string)"Unknown tag: " + el);
    exit(0);
  }
}

void end(const char *el)
{
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
  
  cout<<"Content-type: application/xml\n\n"
      <<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not-osm>\n\n";
  
  set< unsigned int > nodes;
  for (unsigned int i(0); i < 1000000; ++i)
    nodes.insert(i);
  set< unsigned int > ways;
  ways.insert(1);
  ways.insert(4);
  set< unsigned int > relations;
  relations.insert(28);
  map< string, Set > maps;
  maps.insert(make_pair< string, Set >("_", Set(nodes, ways, relations)));
  
  cout<<(uintmax_t)time(NULL)<<'\n';
  for (vector< Statement* >::const_iterator it(statements.begin());
       it != statements.end(); ++it)
    (*it)->execute(maps);
  cout<<(uintmax_t)time(NULL)<<'\n';

  cout<<"\n</not-osm>"<<'\n';
  
  mysql_close(mysql);
  
  return 0;
}
