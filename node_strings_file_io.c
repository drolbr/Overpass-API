#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "file_types.h"
#include "node_strings_file_io.h"
#include "raw_file_db.h"
#include "script_datatypes.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

const char* NODE_STRING_DATA = ((string)DATADIR + "node_strings.dat").c_str();
const char* NODE_STRING_IDX = ((string)DATADIR + "node_strings.idx").c_str();

const char* NODE_TAG_TMPAPREFIX = "/tmp/node_strings_";
const char* NODE_TAG_TMPB = "/tmp/node_tag_ids";

//-----------------------------------------------------------------------------

uint32 NodeCollection::next_node_tag_id(0);

void flush_node_tags(uint& current_run, map< KeyValue, NodeCollection >& node_tags)
{
  ostringstream temp;
  temp<<NODE_TAG_TMPAPREFIX<<current_run;
  int dest_fd = open64(temp.str().c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(dest_fd);
  
  dest_fd = open64(temp.str().c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno);
  
  if (current_run == 0)
  {
    int dest_id_fd = open64(NODE_TAG_TMPB, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (dest_id_fd < 0)
      throw File_Error(errno);
    
    for (map< KeyValue, NodeCollection >::iterator it(node_tags.begin());
         it != node_tags.end(); ++it)
    {
      uint16 key_size(it->first.key.size());
      uint16 value_size(it->first.value.size());
      if (it->second.bitmask>>8)
        it->second.position = 0xffffffff;
      else
        it->second.position &= 0xffffff00;
      write(dest_fd, &(it->second.id), sizeof(uint32));
      write(dest_fd, &(it->second.position), sizeof(uint32));
      write(dest_fd, &(key_size), sizeof(uint16));
      write(dest_fd, &(value_size), sizeof(uint16));
      write(dest_fd, &(it->first.key[0]), key_size);
      write(dest_fd, &(it->first.value[0]), value_size);
      
      uint32 nc_size(it->second.nodes.size());
      write(dest_id_fd, &(it->second.id), sizeof(uint32));
      write(dest_id_fd, &(nc_size), sizeof(uint32));
      for (vector< int32 >::const_iterator it2(it->second.nodes.begin());
	   it2 != it->second.nodes.end(); ++it2)
	write(dest_id_fd, &(*it2), sizeof(uint32));	
    }
    
    close(dest_id_fd);
  }
  else
  {
    uint32* cnt_rd_buf = (uint32*) malloc(2*sizeof(uint32) + 2*sizeof(uint16));
    uint16* size_rd_buf = (uint16*) &(cnt_rd_buf[2]);
    char* key_rd_buf = (char*) malloc(64*1024);
    char* value_rd_buf = (char*) malloc(64*1024);
    
    temp.str("");
    temp<<NODE_TAG_TMPAPREFIX<<(current_run-1);
    int source_fd = open64(temp.str().c_str(), O_RDONLY);
    if (source_fd < 0)
      throw File_Error(errno);
    
    map< KeyValue, NodeCollection >::iterator it(node_tags.begin());
    while (read(source_fd, cnt_rd_buf, 2*sizeof(uint32) + 2*sizeof(uint16)))
    {
      read(source_fd, key_rd_buf, size_rd_buf[0]);
      key_rd_buf[size_rd_buf[0]] = 0;
      read(source_fd, value_rd_buf, size_rd_buf[1]);
      value_rd_buf[size_rd_buf[1]] = 0;
      while ((it != node_tags.end()) &&
             (strcmp(key_rd_buf, it->first.key.c_str()) > 0))
      {
        uint16 key_size(it->first.key.size());
        uint16 value_size(it->first.value.size());
        if (it->second.bitmask>>8)
          it->second.position = 0xffffffff;
        else
          it->second.position &= 0xffffff00;
        write(dest_fd, &(it->second.id), sizeof(uint32));
        write(dest_fd, &(it->second.position), sizeof(uint32));
        write(dest_fd, &(key_size), sizeof(uint16));
        write(dest_fd, &(value_size), sizeof(uint16));
        write(dest_fd, &(it->first.key[0]), key_size);
        write(dest_fd, &(it->first.value[0]), value_size);
        ++it;
      }
      while ((it != node_tags.end()) &&
             (strcmp(key_rd_buf, it->first.key.c_str()) == 0) &&
             (strcmp(value_rd_buf, it->first.value.c_str()) > 0))
      {
        uint16 key_size(it->first.key.size());
        uint16 value_size(it->first.value.size());
        if (it->second.bitmask>>8)
          it->second.position = 0xffffffff;
        else
          it->second.position &= 0xffffff00;
        write(dest_fd, &(it->second.id), sizeof(uint32));
        write(dest_fd, &(it->second.position), sizeof(uint32));
        write(dest_fd, &(key_size), sizeof(uint16));
        write(dest_fd, &(value_size), sizeof(uint16));
        write(dest_fd, &(it->first.key[0]), key_size);
        write(dest_fd, &(it->first.value[0]), value_size);
        ++it;
      }
      if ((it != node_tags.end()) &&
          (strcmp(key_rd_buf, it->first.key.c_str()) == 0) &&
          (strcmp(value_rd_buf, it->first.value.c_str()) == 0))
      {
        it->second.merge(cnt_rd_buf[0], cnt_rd_buf[1]);
        if ((cnt_rd_buf[1] == 0xffffffff) || (it->second.bitmask>>8))
          cnt_rd_buf[1] = 0xffffffff;
        else
          cnt_rd_buf[1] &= 0xffffff00;
        ++it;
      }
      write(dest_fd, cnt_rd_buf, 2*sizeof(uint32) + 2*sizeof(uint16));
      write(dest_fd, key_rd_buf, size_rd_buf[0]);
      write(dest_fd, value_rd_buf, size_rd_buf[1]);
    }
    
    free(cnt_rd_buf);
    free(key_rd_buf);
    free(value_rd_buf);
    
    close(source_fd);
    temp.str("");
    temp<<NODE_TAG_TMPAPREFIX<<(current_run-1);
    remove(temp.str().c_str());
    
    int dest_id_fd = open64(NODE_TAG_TMPB, O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (dest_id_fd < 0)
      throw File_Error(errno);
    
    for (map< KeyValue, NodeCollection >::iterator it(node_tags.begin());
	 it != node_tags.end(); ++it)
    {
      uint32 nc_size(it->second.nodes.size());
      write(dest_id_fd, &(it->second.id), sizeof(uint32));
      write(dest_id_fd, &(nc_size), sizeof(uint32));
      for (vector< int32 >::const_iterator it2(it->second.nodes.begin());
	   it2 != it->second.nodes.end(); ++it2)
	write(dest_id_fd, &(*it2), sizeof(uint32));	
    }
    
    close(dest_id_fd);
  }
  
  close(dest_fd);
  ++current_run;
}

void node_tag_statistics(uint& current_run, vector< uint32 >& split_idx)
{
  cerr<<'s';
  
  if (current_run == 0)
  {
    cerr<<"No node tags.\n";
    return;
  }
  
  ostringstream temp;
  temp<<NODE_TAG_TMPAPREFIX<<(current_run-1);
  int source_fd = open64(temp.str().c_str(), O_RDONLY);
  if (source_fd < 0)
    throw File_Error(errno);
  
  uint32 global_count(0);
  uint32* spatial_count = (uint32*) calloc(16*1024*1024, sizeof(uint32));
  uint32* cnt_rd_buf = (uint32*) malloc(2*sizeof(uint32) + 2*sizeof(uint16));
  uint16* size_rd_buf = (uint16*) &(cnt_rd_buf[2]);
  
  while (read(source_fd, cnt_rd_buf, 2*sizeof(uint32) + 2*sizeof(uint16)))
  {
    lseek64(source_fd, size_rd_buf[0] + size_rd_buf[1], SEEK_CUR);
    ++(spatial_count[cnt_rd_buf[1]>>8]);
    ++global_count;
  }
  global_count -= spatial_count[0x00ffffff];
  
  uint32 split_count(0);
  for (unsigned int i(0); i < 16*1024*1024; ++i)
  {
    split_count += spatial_count[i];
    if (split_count >= global_count / NODE_TAG_SPATIAL_PARTS)
    {
      split_idx.push_back(i<<8);
      split_count = 0;
    }
  }
  
  free(cnt_rd_buf);
  free(spatial_count);
  
  close(source_fd);

  cerr<<'s';
}

void node_tag_split_and_index(uint& current_run, vector< uint32 >& split_idx, uint32*& block_of_id)
{
  cerr<<'p';

  ostringstream temp;
  temp<<NODE_TAG_TMPAPREFIX<<(current_run-1);
  int source_fd = open64(temp.str().c_str(), O_RDONLY);
  if (source_fd < 0)
    throw File_Error(errno);
  
  int dest_fd = open64(NODE_STRING_DATA, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(dest_fd);
  
  dest_fd = open64(NODE_STRING_DATA, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno);
  
  int dest_idx_fd = open64(NODE_STRING_IDX, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(dest_idx_fd);
  
  dest_idx_fd = open64(NODE_STRING_IDX, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno);
  
  for (uint32 i(0); i < NODE_TAG_SPATIAL_PARTS; ++i)
    write(dest_idx_fd, &(split_idx[i]), sizeof(uint32));
  
  uint8* cur_block_count = (uint8*) calloc(NODE_TAG_SPATIAL_PARTS+1, sizeof(uint8));
  block_of_id = (uint32*) calloc((NodeCollection::next_node_tag_id+1), sizeof(uint32));
  
  uint32* write_pos = (uint32*) malloc((NODE_TAG_SPATIAL_PARTS+1)*sizeof(uint32));
  for (uint32 i(0); i < NODE_TAG_SPATIAL_PARTS+1; ++i)
    write_pos[i] = sizeof(uint32);
  char* write_blocks = (char*) malloc((NODE_TAG_SPATIAL_PARTS+1) * NODE_STRING_BLOCK_SIZE);
  uint32* cnt_rd_buf = (uint32*) malloc(2*sizeof(uint32) + 2*sizeof(uint16));
  uint16* size_rd_buf = (uint16*) &(cnt_rd_buf[2]);
  
  while (read(source_fd, cnt_rd_buf, 2*sizeof(uint32) + 2*sizeof(uint16)))
  {
    uint16 block(0);
    if (cnt_rd_buf[1] != 0xffffffff)
    {
      while (split_idx[block] <= cnt_rd_buf[1])
	++block;
      ++block;
    }
    
    if (write_pos[block] + 2*sizeof(uint32) + 2*sizeof(uint16) + size_rd_buf[0] + size_rd_buf[1]
	>= NODE_STRING_BLOCK_SIZE*4/5)
    {
      *((uint32*)&(write_blocks[block * NODE_STRING_BLOCK_SIZE])) = write_pos[block] - sizeof(uint32);
      write(dest_fd, &(write_blocks[block * NODE_STRING_BLOCK_SIZE]), NODE_STRING_BLOCK_SIZE);
      write(dest_idx_fd, &(block), sizeof(uint16));
      write(dest_idx_fd, &(write_blocks[block * NODE_STRING_BLOCK_SIZE
	  + 3*sizeof(uint32)]), *(uint16*)&(write_blocks[block * NODE_STRING_BLOCK_SIZE
	      + 3*sizeof(uint32)]) + *(uint16*)&(write_blocks[block * NODE_STRING_BLOCK_SIZE
		  + 3*sizeof(uint32) + sizeof(uint16)]) + 2*sizeof(uint16));
    
      write_pos[block] = sizeof(uint32);

      memcpy(&(write_blocks[block * NODE_STRING_BLOCK_SIZE + write_pos[block]]), cnt_rd_buf,
	       2*sizeof(uint32) + 2*sizeof(uint16));
      write_pos[block] += 2*sizeof(uint32) + 2*sizeof(uint16);
      read(source_fd, &(write_blocks[block * NODE_STRING_BLOCK_SIZE + write_pos[block]]),
	   size_rd_buf[0] + size_rd_buf[1]);
      write_pos[block] += size_rd_buf[0] + size_rd_buf[1];
      
      block_of_id[cnt_rd_buf[0]] = (++cur_block_count[block]) | ((cnt_rd_buf[1]) & (0xffffff00));
    }
    else
    {
      memcpy(&(write_blocks[block * NODE_STRING_BLOCK_SIZE + write_pos[block]]), cnt_rd_buf,
	       2*sizeof(uint32) + 2*sizeof(uint16));
      write_pos[block] += 2*sizeof(uint32) + 2*sizeof(uint16);
      read(source_fd, &(write_blocks[block * NODE_STRING_BLOCK_SIZE + write_pos[block]]),
	   size_rd_buf[0] + size_rd_buf[1]);
      write_pos[block] += size_rd_buf[0] + size_rd_buf[1];
    
      block_of_id[cnt_rd_buf[0]] = (cur_block_count[block]) | ((cnt_rd_buf[1]) & (0xffffff00));
    }
  }
  
  for (unsigned int i(0); i < NODE_TAG_SPATIAL_PARTS+1; ++i)
  {
    *((uint32*)&(write_blocks[i * NODE_STRING_BLOCK_SIZE])) = write_pos[i] - sizeof(uint32);
    write(dest_fd, &(write_blocks[i * NODE_STRING_BLOCK_SIZE]), NODE_STRING_BLOCK_SIZE);
    write(dest_idx_fd, &(i), sizeof(uint16));
    write(dest_idx_fd, &(write_blocks[i * NODE_STRING_BLOCK_SIZE
	+ 3*sizeof(uint32)]), *(uint16*)&(write_blocks[i * NODE_STRING_BLOCK_SIZE
	    + 3*sizeof(uint32)]) + *(uint16*)&(write_blocks[i * NODE_STRING_BLOCK_SIZE
		+ 3*sizeof(uint32) + sizeof(uint16)]) + 2*sizeof(uint16));
  }
  
  free(cur_block_count);
  free(write_pos);
  free(write_blocks);
  free(cnt_rd_buf);
  
  close(source_fd);
  close(dest_fd);

  cerr<<'p';
}

struct tag_id_local_less : public binary_function< uint32*, uint32*, bool >
{
  tag_id_local_less(uint32* block_of_id) : block_of_id_(block_of_id) {}

  bool operator() (uint32* const& a, uint32* const& b)
  {
    if ((block_of_id_[*a] & 0xffffff00) < (block_of_id_[*b] & 0xffffff00))
      return true;
    else if ((block_of_id_[*a] & 0xffffff00) > (block_of_id_[*b] & 0xffffff00))
      return false;
    return (*a < *b);
  }
private:
  uint32* block_of_id_;
};

struct tag_id_global_less : public binary_function< uint32*, uint32*, bool >
{
  bool operator() (uint32* const& a, uint32* const& b)
  {
    return (*a < *b);
  }
};

void node_tag_create_id_node_idx(uint32* block_of_id)
{
  uint32 rd_buf_pos(0);
  Tag_Id_Node_Local_Writer env_local(block_of_id);
  Tag_Id_Node_Global_Writer env_global;
  
  int source_fd = open64(NODE_TAG_TMPB, O_RDONLY);
  if (source_fd < 0)
    throw File_Error(errno);
  
  uint32* tag_rd_buf = (uint32*) malloc(TAG_SORT_BUFFER_SIZE);
  uint32* tag_alt_buf = (uint32*) malloc(TAG_SORT_BUFFER_SIZE);
  uint32 max_pos(0);
  
  while ((max_pos =
	 read(source_fd, &(tag_rd_buf[rd_buf_pos]), TAG_SORT_BUFFER_SIZE - rd_buf_pos*sizeof(uint32))))
  {
    vector< uint32* > tag_id_local, tag_id_global;
    uint32 alt_buf_pos(0);
    max_pos += rd_buf_pos*sizeof(uint32);
    rd_buf_pos = 0;
  
    while ((rd_buf_pos + 1 < max_pos / sizeof(uint32)) &&
	    (rd_buf_pos + tag_rd_buf[rd_buf_pos+1] + 1 < max_pos / sizeof(uint32)))
    {
      uint32 next_pos(tag_rd_buf[rd_buf_pos+1] + 2);
      
      if ((~(block_of_id[tag_rd_buf[rd_buf_pos]])) & 0xffffff00)
      {
	tag_id_local.push_back(&(tag_rd_buf[rd_buf_pos]));
      
	while (tag_rd_buf[rd_buf_pos+1] > 255)
	{
	  if (tag_rd_buf[rd_buf_pos+1] > 510)
	  {
	    tag_id_local.push_back(&(tag_alt_buf[alt_buf_pos]));
	    tag_id_local.push_back(&(tag_rd_buf[rd_buf_pos + 510]));
	  
	    tag_alt_buf[alt_buf_pos++] = tag_rd_buf[rd_buf_pos];
	    tag_alt_buf[alt_buf_pos++] = 255;
	    memcpy(&(tag_alt_buf[alt_buf_pos]), &(tag_rd_buf[rd_buf_pos+257]), 255*sizeof(uint32));
	    alt_buf_pos += 255;
	    tag_rd_buf[rd_buf_pos+1] = 255;
	  
	    next_pos -= 510;
	    rd_buf_pos += 510;
	  
	    tag_rd_buf[rd_buf_pos] = tag_rd_buf[rd_buf_pos - 510];
	    tag_rd_buf[rd_buf_pos+1] = next_pos - 2;
	  }
	  else
	  {
	    tag_id_local.push_back(&(tag_alt_buf[alt_buf_pos]));
	  
	    tag_alt_buf[alt_buf_pos++] = tag_rd_buf[rd_buf_pos];
	    tag_alt_buf[alt_buf_pos++] = tag_rd_buf[rd_buf_pos+1] - 255;
	    memcpy(&(tag_alt_buf[alt_buf_pos]), &(tag_rd_buf[rd_buf_pos+257]),
		     (tag_rd_buf[rd_buf_pos+1] - 255)*sizeof(uint32));
	    alt_buf_pos += (tag_rd_buf[rd_buf_pos+1] - 255);
	    tag_rd_buf[rd_buf_pos+1] = 255;
	  }
	}
      }
      else
      {
	tag_id_global.push_back(&(tag_rd_buf[rd_buf_pos]));
      
	while (tag_rd_buf[rd_buf_pos+1] > 255)
	{
	  if (tag_rd_buf[rd_buf_pos+1] > 510)
	  {
	    tag_id_global.push_back(&(tag_alt_buf[alt_buf_pos]));
	    tag_id_global.push_back(&(tag_rd_buf[rd_buf_pos + 510]));
	  
	    tag_alt_buf[alt_buf_pos++] = tag_rd_buf[rd_buf_pos];
	    tag_alt_buf[alt_buf_pos++] = 255;
	    memcpy(&(tag_alt_buf[alt_buf_pos]), &(tag_rd_buf[rd_buf_pos+257]), 255*sizeof(uint32));
	    alt_buf_pos += 255;
	    tag_rd_buf[rd_buf_pos+1] = 255;
	  
	    next_pos -= 510;
	    rd_buf_pos += 510;
	  
	    tag_rd_buf[rd_buf_pos] = tag_rd_buf[rd_buf_pos - 510];
	    tag_rd_buf[rd_buf_pos+1] = next_pos - 2;
	  }
	  else
	  {
	    tag_id_global.push_back(&(tag_alt_buf[alt_buf_pos]));
	  
	    tag_alt_buf[alt_buf_pos++] = tag_rd_buf[rd_buf_pos];
	    tag_alt_buf[alt_buf_pos++] = tag_rd_buf[rd_buf_pos+1] - 255;
	    memcpy(&(tag_alt_buf[alt_buf_pos]), &(tag_rd_buf[rd_buf_pos+257]),
		     (tag_rd_buf[rd_buf_pos+1] - 255)*sizeof(uint32));
	    alt_buf_pos += (tag_rd_buf[rd_buf_pos+1] - 255);
	    tag_rd_buf[rd_buf_pos+1] = 255;
	  }
	}
      }
      
      rd_buf_pos += next_pos;
    }
    
    sort(tag_id_local.begin(), tag_id_local.end(), tag_id_local_less(block_of_id));
    sort(tag_id_global.begin(), tag_id_global.end(), tag_id_global_less());
    
    flush_data< Tag_Id_Node_Local_Writer >
	(env_local, tag_id_local.begin(), tag_id_local.end());
    flush_data< Tag_Id_Node_Global_Writer >
        (env_global, tag_id_global.begin(), tag_id_global.end());
    
    cerr<<'.';
    
    memmove(tag_rd_buf, &(tag_rd_buf[rd_buf_pos]), TAG_SORT_BUFFER_SIZE - rd_buf_pos*sizeof(uint32));
    rd_buf_pos = TAG_SORT_BUFFER_SIZE / sizeof(uint32) - rd_buf_pos;
  }
  
  make_block_index< Tag_Id_Node_Local_Writer >(env_local);
  make_block_index< Tag_Id_Node_Global_Writer >(env_global);
  
  free(tag_rd_buf);
  free(tag_alt_buf);
  
  close(source_fd);
}

struct node_idx_less : public binary_function< uint32, uint32, bool >
{
  node_idx_less(uint32* ll_idx__) : ll_idx_(ll_idx__) {}
  
  bool operator() (const uint32& a, const uint32& b)
  {
    return (ll_idx_[a] < ll_idx_[b]);
  }
  
  private:
    uint32* ll_idx_;
};

static void prepare_nodes_chunk(uint32 offset, uint32 count, uint32* ll_idx_buf)
{
  int nodes_dat_fd = open64(NODE_DATA, O_RDONLY);
  
  int* rd_buf = (int*) malloc(NODE_FILE_BLOCK_SIZE);
  Node* nodes_rd_buf((Node*)(&rd_buf[1]));
  
  while (read(nodes_dat_fd, rd_buf, NODE_FILE_BLOCK_SIZE))
  {
    for (int j(0); j < rd_buf[0]; ++j)
    {
      if (((uint32)(nodes_rd_buf[j].id) >= offset) && (nodes_rd_buf[j].id - offset < count))
	ll_idx_buf[nodes_rd_buf[j].id - offset] = ll_idx(nodes_rd_buf[j].lat, nodes_rd_buf[j].lon);
    }
  }
  
  free(rd_buf);
  rd_buf = 0;
  
  close(nodes_dat_fd);
}

void node_tag_create_node_id_idx(uint32* block_of_id, uint32 max_node_id)
{
  const uint32 max_nodes_ram = 32*1024*1024;
  
  int source_fd = open64(NODE_TAG_TMPB, O_RDONLY);
  if (source_fd < 0)
    throw File_Error(errno);
  
  uint8* blocklet_of_id = (uint8*) malloc((NodeCollection::next_node_tag_id+1)*sizeof(uint8));
  for (uint32 i(0); i < (NodeCollection::next_node_tag_id+1); ++i)
  {
    if ((block_of_id[i] & 0xffffff00) == 0xffffff00)
      blocklet_of_id[i] = (block_of_id[i] & 0xff);
    else
      blocklet_of_id[i] = 0xff;
  }
  free(block_of_id);
  
  uint32 count(max_nodes_ram);
  uint32* ll_idx_ = (uint32*) malloc(sizeof(uint32)*count);
  
  Tag_Node_Id_Writer env(ll_idx_, blocklet_of_id);
  env.offset = 1;
  
  while (env.offset < max_node_id)
  {
    env.ids_of_node.clear();
    env.ids_of_node.resize(count);
  
    cerr<<'n';
    prepare_nodes_chunk(env.offset, count, ll_idx_);
    cerr<<'n';
    lseek64(source_fd, 0, SEEK_SET);
    
    uint32* tag_rd_buf = (uint32*) malloc(TAG_SORT_BUFFER_SIZE);
    
    uint32 rd_buf_pos(0), max_pos(0);
    while ((max_pos =
	    read(source_fd, &(tag_rd_buf[rd_buf_pos]), TAG_SORT_BUFFER_SIZE - rd_buf_pos*sizeof(uint32))))
    {
      max_pos += rd_buf_pos*sizeof(uint32);
      rd_buf_pos = 0;
  
      while ((rd_buf_pos + 1 < max_pos / sizeof(uint32)) &&
	      (rd_buf_pos + tag_rd_buf[rd_buf_pos+1] + 1 < max_pos / sizeof(uint32)))
      {
	if (blocklet_of_id[tag_rd_buf[rd_buf_pos]] != 0xff)
	{
	  for (uint32 i(0); i < tag_rd_buf[rd_buf_pos+1]; ++i)
	  {
	    if ((tag_rd_buf[rd_buf_pos+2+i] >= env.offset) &&
			(tag_rd_buf[rd_buf_pos+2+i] - env.offset < count))
	    {
	      if (env.ids_of_node[tag_rd_buf[rd_buf_pos+2+i] - env.offset].size() < 65535)
		env.ids_of_node[tag_rd_buf[rd_buf_pos+2+i] - env.offset].push_back(tag_rd_buf[rd_buf_pos]);
	      else
	      {
		cerr<<"Node "<<dec<<tag_rd_buf[rd_buf_pos+2+i]<<" has more than 2^16 tags.\n";
		exit(0);
	      }
	    }
	  }
	}
	rd_buf_pos += tag_rd_buf[rd_buf_pos+1] + 2;
      }
      cerr<<'t';
    
      memmove(tag_rd_buf, &(tag_rd_buf[rd_buf_pos]), TAG_SORT_BUFFER_SIZE - rd_buf_pos*sizeof(uint32));
      rd_buf_pos = TAG_SORT_BUFFER_SIZE / sizeof(uint32) - rd_buf_pos;
    }
    env.read_order.clear();
    for (uint32 i(0); i < count; ++i)
    {
      if (env.ids_of_node[i].size() > 0)
	env.read_order.push_back(i);
    }
    sort(env.read_order.begin(), env.read_order.end(), node_idx_less(ll_idx_));
    
    free(tag_rd_buf);
    
    flush_data< Tag_Node_Id_Writer >(env, env.begin(), env.end());
    
    env.offset += count;
  }
  
  cerr<<3;
  make_block_index< Tag_Node_Id_Writer >(env);
  cerr<<4;
    
  free(ll_idx_);
  free(blocklet_of_id);
  
  close(source_fd);
}

void node_tag_id_statistics()
{
  int dest_fd = open64(NODE_TAG_ID_STATS, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(dest_fd);
  
  dest_fd = open64(NODE_TAG_ID_STATS, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno);
  
  vector< uint32 > id_count(NodeCollection::next_node_tag_id);
  Tag_Id_Count_Local_Reader local_stats(id_count);
  select_all< Tag_Id_Count_Local_Reader >(local_stats);
  Tag_Id_Count_Global_Reader global_stats(id_count);
  select_all< Tag_Id_Count_Global_Reader >(global_stats);
  
  for (vector< uint32 >::const_iterator it(id_count.begin()); it != id_count.end(); ++it)
    write(dest_fd, &(*it), sizeof(uint32));

  close(dest_fd);
}

//-----------------------------------------------------------------------------

struct Node_String_Cache
{
  static const vector< uint32 >& get_spatial_boundaries()
  {
    if (spatial_boundaries.empty())
      init();
    return spatial_boundaries;
  }
  
  static const vector< vector< pair< string, string > > >& get_kv_to_id_idx()
  {
    if (kv_to_id_idx.empty())
      init();
    return kv_to_id_idx;
  }
  
  static const vector< vector< uint16 > >& get_kv_to_id_block_idx()
  {
    if (kv_to_id_block_idx.empty())
      init();
    return kv_to_id_block_idx;
  }
  
  private:
    static void init()
    {
      spatial_boundaries.clear();
    
      int string_idx_fd = open64(NODE_STRING_IDX, O_RDONLY);
      if (string_idx_fd < 0)
	throw File_Error(errno);
  
      uint32* string_spat_idx_buf = (uint32*) malloc(NODE_TAG_SPATIAL_PARTS*sizeof(uint32));
      read(string_idx_fd, string_spat_idx_buf, NODE_TAG_SPATIAL_PARTS*sizeof(uint32));
      for (uint32 i(0); i < NODE_TAG_SPATIAL_PARTS; ++i)
	spatial_boundaries.push_back(string_spat_idx_buf[i]);
      free(string_spat_idx_buf);
    
      kv_to_id_idx.clear();
      kv_to_id_idx.resize(NODE_TAG_SPATIAL_PARTS+1);
      kv_to_id_block_idx.clear();
      kv_to_id_block_idx.resize(NODE_TAG_SPATIAL_PARTS+1);
    
      uint16* kv_to_id_idx_buf_1 = (uint16*) malloc(3*sizeof(uint16));
      char* kv_to_id_idx_buf_2 = (char*) malloc(2*64*1024);
      uint32 block_id(0);
      while (read(string_idx_fd, kv_to_id_idx_buf_1, 3*sizeof(uint16)))
      {
	read(string_idx_fd, kv_to_id_idx_buf_2, kv_to_id_idx_buf_1[1] + kv_to_id_idx_buf_1[2]);
	kv_to_id_idx[kv_to_id_idx_buf_1[0]].push_back(make_pair< string, string >
	    (string(kv_to_id_idx_buf_2, kv_to_id_idx_buf_1[1]),
	     string(&(kv_to_id_idx_buf_2[kv_to_id_idx_buf_1[1]]), kv_to_id_idx_buf_1[2])));
	kv_to_id_block_idx[kv_to_id_idx_buf_1[0]].push_back(block_id++);
      }
      free(kv_to_id_idx_buf_2);
      free(kv_to_id_idx_buf_1);
  
      close(string_idx_fd);
    }
  
    static vector< uint32 > spatial_boundaries;
    static vector< vector< pair< string, string > > > kv_to_id_idx;
    static vector< vector< uint16 > > kv_to_id_block_idx;
};

vector< uint32 > Node_String_Cache::spatial_boundaries;
vector< vector< pair< string, string > > > Node_String_Cache::kv_to_id_idx;
vector< vector< uint16 > > Node_String_Cache::kv_to_id_block_idx;

void select_kv_to_ids
    (string key, string value, set< uint32 >& string_ids_global,
     set< uint32 >& string_ids_local, set< uint32 >& string_idxs_local)
{
  const vector< vector< pair< string, string > > >& kv_to_id_idx
      (Node_String_Cache::get_kv_to_id_idx());
  const vector< vector< uint16 > >& kv_to_id_block_idx
      (Node_String_Cache::get_kv_to_id_block_idx());
  
  set< uint16 > kv_to_idx_block_ids;
  for (uint32 i(0); i < NODE_TAG_SPATIAL_PARTS+1; ++i)
  {
    uint32 j(1);
    if (value == "")
    {
      while ((j < kv_to_id_idx[i].size()) && (kv_to_id_idx[i][j].first < key))
	++j;
      kv_to_idx_block_ids.insert(kv_to_id_block_idx[i][j-1]);
      while ((j < kv_to_id_idx[i].size()) && (kv_to_id_idx[i][j].first <= key))
      {
	kv_to_idx_block_ids.insert(kv_to_id_block_idx[i][j]);
	++j;
      }
    }
    else
    {
      while ((j < kv_to_id_idx[i].size()) && ((kv_to_id_idx[i][j].first < key) ||
	      ((kv_to_id_idx[i][j].first == key) && (kv_to_id_idx[i][j].second < value))))
	++j;
      kv_to_idx_block_ids.insert(kv_to_id_block_idx[i][j-1]);
      while ((j < kv_to_id_idx[i].size()) && ((kv_to_id_idx[i][j].first < key) ||
	      ((kv_to_id_idx[i][j].first == key) && (kv_to_id_idx[i][j].second <= value))))
      {
	kv_to_idx_block_ids.insert(kv_to_id_block_idx[i][j]);
	++j;
      }
    }
  }

  int string_fd = open64(NODE_STRING_DATA, O_RDONLY);
  if (string_fd < 0)
    throw File_Error(errno);
  
  char* string_idxs_buf = (char*) malloc(NODE_STRING_BLOCK_SIZE);
  if (value == "")
  {
    for (set< uint16 >::const_iterator it(kv_to_idx_block_ids.begin());
	 it != kv_to_idx_block_ids.end(); ++it)
    {
      lseek64(string_fd, ((uint64)(*it))*NODE_STRING_BLOCK_SIZE, SEEK_SET);
      read(string_fd, string_idxs_buf, NODE_STRING_BLOCK_SIZE);
      uint32 pos(sizeof(uint32));
      while (pos < *((uint32*)string_idxs_buf) + sizeof(uint32))
      {
	pos += 2*sizeof(uint32);
	uint16& key_len(*((uint16*)&(string_idxs_buf[pos])));
	pos += sizeof(uint16);
	uint16& value_len(*((uint16*)&(string_idxs_buf[pos])));
	pos += sizeof(uint16);
	if ((key_len >= key.size()) && (!(strncmp(key.c_str(), &(string_idxs_buf[pos]), key_len))))
	{
	  if (*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - sizeof(uint32)]))
			== 0xffffffff)
	    string_ids_global.insert
		(*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - 2*sizeof(uint32)])));
	  else
	  {
	    string_idxs_local.insert
		(*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - sizeof(uint32)])));
	    string_ids_local.insert
		(*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - 2*sizeof(uint32)])));
	  }
	}
	pos += key_len + value_len;
      }
    }
  }
  else
  {
    for (set< uint16 >::const_iterator it(kv_to_idx_block_ids.begin());
	 it != kv_to_idx_block_ids.end(); ++it)
    {
      lseek64(string_fd, ((uint64)(*it))*NODE_STRING_BLOCK_SIZE, SEEK_SET);
      read(string_fd, string_idxs_buf, NODE_STRING_BLOCK_SIZE);
      uint32 pos(sizeof(uint32));
      while (pos < *((uint32*)string_idxs_buf) + sizeof(uint32))
      {
	pos += 2*sizeof(uint32);
	uint16& key_len(*((uint16*)&(string_idxs_buf[pos])));
	pos += sizeof(uint16);
	uint16& value_len(*((uint16*)&(string_idxs_buf[pos])));
	pos += sizeof(uint16);
	if ((key_len >= key.size()) && (value_len >= value.size()) &&
		    (!(strncmp(key.c_str(), &(string_idxs_buf[pos]), key_len))) &&
		    (!(strncmp(value.c_str(), &(string_idxs_buf[pos + key_len]), value_len))))
	{
	  if (*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - sizeof(uint32)]))
			== 0xffffffff)
	    string_ids_global.insert
		(*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - 2*sizeof(uint32)])));
	  else
	  {
	    string_idxs_local.insert
		(*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - sizeof(uint32)])));
	    string_ids_local.insert
		(*((uint32*)&(string_idxs_buf[pos - 2*sizeof(uint16) - 2*sizeof(uint32)])));
	  }
	}
	pos += key_len + value_len;
      }
    }
  }
  free(string_idxs_buf);
  
  close(string_fd);
}

void select_ids_to_kvs
    (const map< uint32, uint32 > ids_local,
     const set< uint32 > ids_global,
     map< uint32, pair< string, string > >& kvs)
{
  const vector< uint32 >& spatial_boundaries(Node_String_Cache::get_spatial_boundaries());
  const vector< vector< uint16 > >& kv_to_id_block_idx(Node_String_Cache::get_kv_to_id_block_idx());
  
  vector< bool > used_spat_parts(NODE_TAG_SPATIAL_PARTS);
  for (map< uint32, uint32 >::const_iterator it(ids_local.begin());
       it != ids_local.end(); ++it)
  {
    uint32 i(0);
    while (spatial_boundaries[i] <= it->second)
      ++i;
    used_spat_parts[i] = true;
  }
  set< uint16 > used_blocks;
  for (vector< uint16 >::const_iterator it(kv_to_id_block_idx[0].begin());
       it != kv_to_id_block_idx[0].end(); ++it)
    used_blocks.insert(*it);
  for (uint32 i(1); i < used_spat_parts.size()+1; ++i)
  {
    if (used_spat_parts[i-1])
    {
      for (vector< uint16 >::const_iterator it(kv_to_id_block_idx[i].begin());
	   it != kv_to_id_block_idx[i].end(); ++it)
	used_blocks.insert(*it);
    }
  }

  int string_fd = open64(NODE_STRING_DATA, O_RDONLY);
  if (string_fd < 0)
    throw File_Error(errno);
  
  char* string_idxs_buf = (char*) malloc(NODE_STRING_BLOCK_SIZE);
  for (set< uint16 >::const_iterator it(used_blocks.begin());
       it != used_blocks.end(); ++it)
  {
    lseek64(string_fd, ((uint64)(*it))*NODE_STRING_BLOCK_SIZE, SEEK_SET);
    read(string_fd, string_idxs_buf, NODE_STRING_BLOCK_SIZE);
    uint32 pos(sizeof(uint32));
    while (pos < *((uint32*)string_idxs_buf) + sizeof(uint32))
    {
      uint16& key_len(*((uint16*)&(string_idxs_buf[pos + 2*sizeof(uint32)])));
      uint16& value_len(*((uint16*)&(string_idxs_buf[pos + 2*sizeof(uint32) + sizeof(uint16)])));
      if ((ids_global.find(*((uint32*)&(string_idxs_buf[pos]))) != ids_global.end()) ||
	   (ids_local.find(*((uint32*)&(string_idxs_buf[pos]))) != ids_local.end()))
	kvs[*((uint32*)&(string_idxs_buf[pos]))] = make_pair< string, string >
	    (string(&(string_idxs_buf[pos + 2*sizeof(uint32) + 2*sizeof(uint16)]), key_len),
	     string(&(string_idxs_buf[pos + 2*sizeof(uint32) + 2*sizeof(uint16) + key_len]), value_len));
	    pos += 2*sizeof(uint32) + 2*sizeof(uint16) + key_len + value_len;
    }
  }
  free(string_idxs_buf);
  
  close(string_fd);
}
