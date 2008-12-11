/*****************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the license contained in the
 * COPYING file that comes with the expat distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "expat_justparse_interface.h"

#include <mysql.h>

using namespace std;

const int NODE = 1;
const int WAY = 2;
const int RELATION = 3;

//const unsigned int FLUSH_INTERVAL = 100000000;
unsigned int FLUSH_INTERVAL(0);
unsigned int strucutre_count(0);

ofstream nodes_out("/tmp/db_area_nodes.tsv");

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
  {
  }
  else if (!strcmp(el, "node"))
  {
    unsigned int id(0);
    int lat(0), lon(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (!strcmp(attr[i], "lat"))
	lat = (int)(atof(attr[i+1])*10000000);
      if (!strcmp(attr[i], "lon"))
	lon = (int)(atof(attr[i+1])*10000000);
    }
    nodes_out<<id<<'\t'<<lat<<'\t'<<lon<<'\n';
  }
  else if (!strcmp(el, "way"))
  {
  }
  else if (!strcmp(el, "relation"))
  {
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    ++structure_count;
  }
  else if (!strcmp(el, "way"))
  {
  }
  else if (!strcmp(el, "relation"))
  {
  }
  if (structure_count == FLUSH_INTERVAL)
  {
    flush_to_db();
    nodes_out.open("/tmp/db_area_nodes.tsv");
    structure_count = 0;
  }
}

void prepare_db()
{
  mysql_query(mysql, "create database if not exists osm");
  mysql_query(mysql, "drop database osm");
  mysql_query(mysql, "create database osm");
  mysql_query(mysql, "use osm");

  mysql_query(mysql, "create table nodes (id int unsigned, lat int, lon int, primary key(id))");
}

void flush_to_db()
{
  nodes_out.close();
  mysql_query(mysql, "load data local infile '/tmp/db_area_nodes.tsv' into table nodes");
}

int main(int argc, char *argv[])
{
  FLUSH_INTERVAL = atoi(argv[1]);
  
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  MYSQL* mysql(mysql_init(NULL));
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    cerr<<"Connection to database failed.\n";
    return 1;
  }
  
  prepare_db();
  
  //reading the main document
  parse(stdin, start, end);
  
  flush_to_db();
  
  //test whether the database is successfully populated
  mysql_query(mysql, "select * from nodes where id >= 200000000 and id < 200100000");
  
  MYSQL_RES* result(mysql_store_result(mysql));
  if (result)
  {
    unsigned int num_fields(mysql_num_fields(result));
    MYSQL_ROW row(mysql_fetch_row(result));
    while (row)
    {
      for (unsigned int i(0); i < num_fields; ++i)
	if (row[i])
	  cout<<row[i]<<'\t';
      cout<<'\n';
      row = mysql_fetch_row(result);
    }
  }
  //end of test sequence
  
  mysql_close(mysql);
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
