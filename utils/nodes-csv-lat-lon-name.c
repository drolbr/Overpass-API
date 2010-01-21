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

#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../expat_justparse_interface.h"

using namespace std;

struct NamedNode
{
  public:
    NamedNode() : lat(100.0), lon(200.0), name("") {}
    
    double lat, lon;
    string name;
};

map< unsigned int, NamedNode > nodes;
list< unsigned int > stops;
NamedNode nnode;
unsigned int id;
int direction(0);

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
  {
    string key, value;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	key = attr[i+1];
      else if (!strcmp(attr[i], "v"))
	value = attr[i+1];
    }
    if (key == "name")
      nnode.name = value;
  }
  else if (!strcmp(el, "node"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atol(attr[i+1]);
      else if (!strcmp(attr[i], "lat"))
	nnode.lat = atof(attr[i+1]);
      else if (!strcmp(attr[i], "lon"))
	nnode.lon = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "member"))
  {
    unsigned int ref(0);
    string type, role;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "ref"))
	ref = atol(attr[i+1]);
      else if (!strcmp(attr[i], "type"))
	type = attr[i+1];
      else if (!strcmp(attr[i], "role"))
	role = attr[i+1];
    }
    if (type == "node")
    {
      if (direction == 0)
	stops.push_back(ref);
      else if (direction == 1)
      {
	if ((role == "forward_stop") || (role == "forward_stop_0"))
	  stops.push_back(ref);
      }
      else if (direction == -1)
      {
	if ((role == "backward_stop") || (role == "backward_stop_0"))
	  stops.push_front(ref);
      }
    }
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    nodes[id] = nnode;
  }
}

int main(int argc, char *argv[])
{
  if (argc == 2)
  {
    if (!strcmp(argv[1], "--forward"))
    {
      direction = 1;
    }
    if (!strcmp(argv[1], "--backward"))
    {
      direction = -1;
    }
  }
  
  //reading the main document
  parse(stdin, start, end);
  
  for (list< unsigned int >::const_iterator it(stops.begin()); it != stops.end(); ++it)
  {
    NamedNode nnode(nodes[*it]);
    if (nnode.lat <= 90.0)
    {
      cout<<setprecision(14)<<nnode.lat<<"\t"<<nnode.lon<<"\t"<<nnode.name<<"\n";
    }
  }
  
  return 0;
}
