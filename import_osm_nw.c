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
#include <map>
#include <string>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "script_datatypes.h"
#include "expat_justparse_interface.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <mysql.h>

using namespace std;

typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

const unsigned int BLOCKSIZE = 512*1024;
const char* NODE_DATA = "/opt/osm_why_api/nodes.dat";
const char* NODE_IDX = "/opt/osm_why_api/nodes.b.idx";
const char* NODE_IDXA = "/opt/osm_why_api/nodes.1.idx";

//-----------------------------------------------------------------------------

int nodes_dat_fd;
multimap< int, unsigned int > block_index;
int max_node_id(0);
int* wr_buf;
int* rd_buf;

void prepare_nodes()
{
  wr_buf = (int*) malloc(sizeof(int) + BLOCKSIZE*sizeof(Node));
  rd_buf = (int*) malloc(sizeof(int) + BLOCKSIZE*sizeof(Node));
  if ((!wr_buf) || (!rd_buf))
  {
    cerr<<"malloc: "<<errno<<'\n';
    exit(0);
  }
  
  nodes_dat_fd = open64(NODE_DATA, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (nodes_dat_fd < 0)
  {
    cerr<<"open64: "<<errno<<'\n';
    exit(0);
  }
}

void flush_nodes(const multimap< int, Node >& nodes)
{
  static int next_block_id(0);
  
  if (block_index.empty())
  {
    unsigned int i(0), total(0);
    Node* nodes_buf((Node*)(&wr_buf[1]));
    
    if (nodes.size() > BLOCKSIZE)
      wr_buf[0] = BLOCKSIZE;
    else
      wr_buf[0] = nodes.size();
    block_index.insert(make_pair< int, unsigned int >(nodes.begin()->first, next_block_id++));
    
    multimap< int, Node >::const_iterator it(nodes.begin());
    while (it != nodes.end())
    {
      if (i == BLOCKSIZE)
      {
	write(nodes_dat_fd, wr_buf, sizeof(int) + BLOCKSIZE*sizeof(Node));
	
	total += i;
	if (nodes.size() - total > BLOCKSIZE)
	  wr_buf[0] = BLOCKSIZE;
	else
	  wr_buf[0] = nodes.size() - total;
	block_index.insert(make_pair< int, unsigned int >(it->first, next_block_id++));
	i = 0;
      }
      
      new (&nodes_buf[i]) Node(it->second);
      if (it->second.id > max_node_id)
	max_node_id = it->second.id;
      ++it;
      ++i;
    }
    if (i > 0)
      write(nodes_dat_fd, wr_buf, sizeof(int) + BLOCKSIZE*sizeof(Node));
  }
  else
  {
    multimap< int, Node >::const_iterator nodes_it(nodes.begin());
    multimap< int, unsigned int >::const_iterator block_it(block_index.begin());
    unsigned int cur_block((block_it++)->second);
    while (nodes_it != nodes.end())
    {
      while ((block_it != block_index.end()) && (block_it->first <= nodes_it->first))
	cur_block = (block_it++)->second;
      
      unsigned int new_element_count(0);
      multimap< int, Node >::const_iterator nodes_it2(nodes_it);
      if (block_it != block_index.end())
      {
	while ((nodes_it2 != nodes.end()) && (block_it->first > nodes_it2->first))
	{
	  if (nodes_it2->second.id > max_node_id)
	    max_node_id = nodes_it2->second.id;
	  ++new_element_count;
	  ++nodes_it2;
	}
      }
      else
      {
	while (nodes_it2 != nodes.end())
	{
	  if (nodes_it2->second.id > max_node_id)
	    max_node_id = nodes_it2->second.id;
	  ++new_element_count;
	  ++nodes_it2;
	}
      }
      
      lseek64(nodes_dat_fd, (int64)cur_block*(sizeof(int) + BLOCKSIZE*sizeof(Node)), SEEK_SET);
      read(nodes_dat_fd, rd_buf, sizeof(int) + BLOCKSIZE*sizeof(Node));
      new_element_count += rd_buf[0];
      
      int i(0);
      while (new_element_count > BLOCKSIZE)
      {
	unsigned int blocksize(new_element_count/(new_element_count/BLOCKSIZE + 1));
	
	unsigned int j(0);
	wr_buf[0] = blocksize;
	Node* nodes_rd_buf((Node*)(&rd_buf[1]));
	Node* nodes_wr_buf((Node*)(&wr_buf[1]));
	while ((j < blocksize) && (nodes_it != nodes_it2) && (i < rd_buf[0]))
	{
	  if (nodes_it->first < ll_idx(nodes_rd_buf[i].lat, nodes_rd_buf[i].lon))
	    new (&nodes_wr_buf[j++]) Node((nodes_it++)->second);
	  else
	    new (&nodes_wr_buf[j++]) Node(nodes_rd_buf[i++]);
	}
	while ((j < blocksize) && (nodes_it != nodes_it2))
	  new (&nodes_wr_buf[j++]) Node((nodes_it++)->second);
	while ((j < blocksize) && (i < rd_buf[0]))
	  new (&nodes_wr_buf[j++]) Node(nodes_rd_buf[i++]);
	
	lseek64(nodes_dat_fd, (int64)cur_block*(sizeof(int) + BLOCKSIZE*sizeof(Node)), SEEK_SET);
	write(nodes_dat_fd, wr_buf, sizeof(int) + BLOCKSIZE*sizeof(Node));
	
	cur_block = next_block_id;
	if ((i >= rd_buf[0]) || (nodes_it->first < ll_idx(nodes_rd_buf[i].lat, nodes_rd_buf[i].lon)))
	  block_index.insert(make_pair< int, unsigned int >(nodes_it->first, next_block_id++));
	else
	  block_index.insert(make_pair< int, unsigned int >
	      (ll_idx(nodes_rd_buf[i].lat, nodes_rd_buf[i].lon), next_block_id++));
	new_element_count -= blocksize;	
      }
      
      unsigned int j(0);
      wr_buf[0] = new_element_count;
      Node* nodes_rd_buf((Node*)(&rd_buf[1]));
      Node* nodes_wr_buf((Node*)(&wr_buf[1]));
      while ((nodes_it != nodes_it2) && (i < rd_buf[0]))
      {
	if (nodes_it->first < ll_idx(nodes_rd_buf[i].lat, nodes_rd_buf[i].lon))
	  new (&nodes_wr_buf[j++]) Node((nodes_it++)->second);
	else
	  new (&nodes_wr_buf[j++]) Node(nodes_rd_buf[i++]);
      }
      while (nodes_it != nodes_it2)
	new (&nodes_wr_buf[j++]) Node((nodes_it++)->second);
      while (i < rd_buf[0])
	new (&nodes_wr_buf[j++]) Node(nodes_rd_buf[i++]);
	
      lseek64(nodes_dat_fd, (int64)cur_block*(sizeof(int) + BLOCKSIZE*sizeof(Node)), SEEK_SET);
      write(nodes_dat_fd, wr_buf, sizeof(int) + BLOCKSIZE*sizeof(Node));
      
      nodes_it = nodes_it2;
    }
  }
}

void postprocess_nodes()
{
  free(wr_buf);
  wr_buf = 0;
  
  pair< int, unsigned int >* buf = (pair< int, unsigned int >*)
      malloc(sizeof(int)*2*block_index.size());
  unsigned int i(0);
  for (multimap< int, unsigned int >::const_iterator it(block_index.begin());
       it != block_index.end(); ++it)
    buf[i++] = *it;
  
  int nodes_idx_fd = open64(NODE_IDX, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  write(nodes_idx_fd, buf, sizeof(int)*2*block_index.size());
  close(nodes_idx_fd);
  
  free(buf);
  
  uint16* idx_buf = (uint16*) malloc(sizeof(uint16)*max_node_id);
  
  lseek64(nodes_dat_fd, 0, SEEK_SET);
  for (unsigned int i(0); i < block_index.size(); ++i)
  {
    read(nodes_dat_fd, rd_buf, sizeof(int) + BLOCKSIZE*sizeof(Node));
    
    Node* nodes_rd_buf((Node*)(&rd_buf[1]));
    for (int j(0); j < rd_buf[0]; ++j)
      idx_buf[nodes_rd_buf[j].id] = i;
  }
  nodes_idx_fd = open64(NODE_IDXA, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  write(nodes_idx_fd, idx_buf, sizeof(uint16)*max_node_id);
  close(nodes_idx_fd);
  
  free(idx_buf);
  free(rd_buf);
  rd_buf = 0;
  
  close(nodes_dat_fd);
}

//-----------------------------------------------------------------------------

// const int NODE = 1;
// const int WAY = 2;
// const int RELATION = 3;
// int tag_type(0);
// unsigned int current_id(0);

const unsigned int FLUSH_INTERVAL = 32*1024*1024;
const unsigned int DOT_INTERVAL = 512*1024;
unsigned int structure_count(0);

// unsigned int way_member_count(0);

multimap< int, Node > nodes;

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "nd"))
  {
  }
  else if (!strcmp(el, "node"))
  {
    unsigned int id(0);
    int lat(100*10000000), lon(200*10000000);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (!strcmp(attr[i], "lat"))
	lat = (int)in_lat_lon(attr[i+1]);
      if (!strcmp(attr[i], "lon"))
	lon = (int)in_lat_lon(attr[i+1]);
    }
    nodes.insert(make_pair< int, Node >
	(ll_idx(lat, lon), Node(id, lat, lon)));
  }
  else if (!strcmp(el, "way"))
  {
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    ++structure_count;
    if (structure_count % DOT_INTERVAL == 0)
      cerr<<'.';
  }
  else if (!strcmp(el, "way"))
  {
  }
  else if (!strcmp(el, "relation"))
  {
  }
  if (structure_count >= FLUSH_INTERVAL)
  {
    flush_nodes(nodes);
    nodes.clear();
    
    structure_count = 0;
  }
}

int main(int argc, char *argv[])
{
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  prepare_nodes();
  
  //reading the main document
  parse(stdin, start, end);
  
  flush_nodes(nodes);
  nodes.clear();
  postprocess_nodes();
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
