/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Template_DB.
*
* Template_DB is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Template_DB is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Template_DB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_CACHE_H
#define DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_CACHE_H

#include "block_backend.h"
#include "transaction.h"

#include <vector>


// TODO: concurrency protection (blocked key), size control, statistics, lazy iterator for discrete


template< typename Key, typename Value >
struct Cache_Entry
{
  Cache_Entry(int used_timestamp_) : used_timestamp(used_timestamp_), size(0) {}
  
  int used_timestamp;
  int size;
  Key next_key;
  std::vector< Value > values;
};


template< typename Key, typename Value >
class Block_Backend_Basic_Cached_Request;


template< typename Key, typename Value >
class Block_Backend_Cache : public Block_Backend_Cache_Base
{
public:
  Block_Backend_Cache(File_Blocks_Index_Base& db_index_, Transaction& transaction_)
    : db_index(&db_index_), db(0), transaction(&transaction_) {}
  
  virtual long long get_total_size();
  virtual void trim_non_reserved();
  virtual void trim_reserved();
  
  virtual ~Block_Backend_Cache() { delete db; }
  
  std::pair< Key, const std::vector< Value >* > read_whole_key
      (Block_Backend_Basic_Cached_Request< Key, Value >& request);
  
  void register_request(Block_Backend_Basic_Cached_Request< Key, Value >& request);
  void unregister_request(Block_Backend_Basic_Cached_Request< Key, Value >& request);
  Block_Backend< Key, Value >& get_db();
  
private:
  File_Blocks_Index_Base* db_index;
  Block_Backend< Key, Value >* db;
  Transaction* transaction;
  std::map< Key, Cache_Entry< Key, Value > > cached_blocks;
  std::vector< Block_Backend_Basic_Cached_Request< Key, Value >* > registered_requests;
};


template< typename Key, typename Value >
class Block_Backend_Basic_Cached_Request
{
public:
  Block_Backend_Basic_Cached_Request(int used_timestamp) : used_timestamp_(used_timestamp) {}
  virtual ~Block_Backend_Basic_Cached_Request() {}
  
  virtual bool is_reserved(const Key& key) = 0;
  virtual const Key& frontend_index() const = 0;
  virtual void skip_frontend_index(const Key& target) = 0;
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values) = 0;
  
  int used_timestamp() const { return used_timestamp_; }
  
private:
  int used_timestamp_;
};


// Implementation of Block_Backend_Cache: --------------------------------------


template< typename Key, typename Value >
std::pair< Key, const std::vector< Value >* > Block_Backend_Cache< Key, Value >::read_whole_key
    (Block_Backend_Basic_Cached_Request< Key, Value >& request)
{
  while (true)
  {
    if (request.frontend_index() == Key())
      return std::pair< Key, const std::vector< Value >* >(request.frontend_index(), 0);
  
    typename std::map< Key, Cache_Entry< Key, Value > >::iterator
        cache_it = cached_blocks.find(request.frontend_index());
  
    if (cache_it == cached_blocks.end())
    {
      transaction->trim_cache();
    
      cache_it = cached_blocks.insert(std::make_pair
          (request.frontend_index(), Cache_Entry< Key, Value >(request.used_timestamp()))).first;
      std::pair< int, Key > size_and_next
          = request.read_whole_key_base(request.frontend_index(), cache_it->second.values);
      cache_it->second.size = size_and_next.first;
      cache_it->second.next_key = size_and_next.second;
    }
  
    std::pair< Key, const std::vector< Value >* > result
        = std::make_pair(request.frontend_index(), &cache_it->second.values);
    cache_it->second.used_timestamp = request.used_timestamp();
  
    request.skip_frontend_index(cache_it->second.next_key);

    if (!cache_it->second.values.empty())
      return result;
  }
}


template< typename Key, typename Value >
long long Block_Backend_Cache< Key, Value >::get_total_size()
{
  long long result = 0;
  for (typename std::map< Key, Cache_Entry< Key, Value > >::const_iterator it = cached_blocks.begin();
       it != cached_blocks.end(); ++it)
    result += it->second.size;
  
  return result;
}


template< typename Key, typename Value >
void Block_Backend_Cache< Key, Value >::trim_non_reserved()
{
  typename std::map< Key, Cache_Entry< Key, Value > >::iterator it = cached_blocks.begin();
  while (it != cached_blocks.end())
  {
    bool reserved = false;
    for (typename std::vector< Block_Backend_Basic_Cached_Request< Key, Value >* >::const_iterator
        it2 = registered_requests.begin(); it2 != registered_requests.end(); ++it2)
    {
      if ((*it2)->is_reserved(it->first))
      {
	reserved = true;
	break;
      }
    }
    if (reserved)
      ++it;
    else
    {
      typename std::map< Key, Cache_Entry< Key, Value > >::iterator erase_it = it;
      ++it;
      cached_blocks.erase(erase_it);
    }
  }
}


template< typename Key, typename Value >
void Block_Backend_Cache< Key, Value >::trim_reserved()
{
  cached_blocks.clear();
}


template< typename Key, typename Value >
Block_Backend< Key, Value >& Block_Backend_Cache< Key, Value >::get_db()
{
  if (!db)
    db = new Block_Backend< Key, Value >(db_index);
  return *db;
}


template< typename Key, typename Value >
void Block_Backend_Cache< Key, Value >::register_request(Block_Backend_Basic_Cached_Request< Key, Value >& request)
{
  registered_requests.push_back(&request);
}


template< typename Key, typename Value >
void Block_Backend_Cache< Key, Value >::unregister_request(Block_Backend_Basic_Cached_Request< Key, Value >& request)
{
  typename std::vector< Block_Backend_Basic_Cached_Request< Key, Value >* >::iterator
      it = registered_requests.begin();
  while (it != registered_requests.end() && *it != &request)
    ++it;
  if (it != registered_requests.end())
  {
    *it = registered_requests.back();
    registered_requests.pop_back();
  }
  
  if (registered_requests.empty())
  {
    delete db;
    db = 0;
  }
}


// Implementation of Block_Backend_Flat_Cached_Request: ------------------------


template< typename Key, typename Value >
class Block_Backend_Flat_Cached_Request : Block_Backend_Basic_Cached_Request< Key, Value >
{
public:
  Block_Backend_Flat_Cached_Request(Block_Backend_Cache_Base& cache_, int used_timestamp);
  virtual ~Block_Backend_Flat_Cached_Request();
  
  virtual bool is_reserved(const Key& key) { return !(key < frontend_index_); }
  virtual const Key& frontend_index() const { return frontend_index_; }
  virtual void skip_frontend_index(const Key& target) { frontend_index_ = target; }
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values);
  
  std::pair< Key, const std::vector< Value >* > read_whole_key();  
  
private:
  Block_Backend_Cache< Key, Value >* cache;
  typename Block_Backend< Key, Value >::Flat_Iterator* backend_it;
  Key frontend_index_;
};


template< typename Key, typename Value >
Block_Backend_Flat_Cached_Request< Key, Value >::Block_Backend_Flat_Cached_Request
    (Block_Backend_Cache_Base& cache_, int used_timestamp)
    : Block_Backend_Basic_Cached_Request< Key, Value >(used_timestamp),
      cache(dynamic_cast< Block_Backend_Cache< Key, Value >* >(&cache_)),
      backend_it(new typename Block_Backend< Key, Value >::Flat_Iterator(cache->get_db().flat_begin())),
      frontend_index_((*backend_it == cache->get_db().flat_end()) ? Key() : backend_it->index())
{
  cache->register_request(*this);
}


template< typename Key, typename Value >
Block_Backend_Flat_Cached_Request< Key, Value >::~Block_Backend_Flat_Cached_Request()
{
  cache->unregister_request(*this);
  delete backend_it;
}


template< typename Key, typename Value >
std::pair< int, Key > Block_Backend_Flat_Cached_Request< Key, Value >
    ::read_whole_key_base(const Key& key, std::vector< Value >& result_values)
{
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value >::Flat_Iterator(cache->get_db().flat_begin());
  
  if (backend_it->index() == key)
    return backend_it->read_whole_key(result_values);
  result_values.clear();
  return std::make_pair(0, backend_it->index());
}


template< typename Key, typename Value >
std::pair< Key, const std::vector< Value >* > Block_Backend_Flat_Cached_Request< Key, Value >::read_whole_key()
{
  return cache->read_whole_key(*this);
}


// Implementation of Block_Backend_Discrete_Cached_Request: ------------------------


template< typename Key, typename Value, typename Iterator = typename std::set< Key >::const_iterator >
class Block_Backend_Discrete_Cached_Request : Block_Backend_Basic_Cached_Request< Key, Value >
{
public:
  Block_Backend_Discrete_Cached_Request(Block_Backend_Cache_Base& cache_, int used_timestamp,
      const Iterator& begin_, const Iterator& end_);
  virtual ~Block_Backend_Discrete_Cached_Request();
  
  virtual bool is_reserved(const Key& key) { return !(key < frontend_index_); }
  virtual const Key& frontend_index() const { return frontend_index_; }
  virtual void skip_frontend_index(const Key& target);
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values);
  
  std::pair< Key, const std::vector< Value >* > read_whole_key();  
  
private:
  Block_Backend_Discrete_Cached_Request(const Block_Backend_Discrete_Cached_Request&);

  Block_Backend_Cache< Key, Value >* cache;
  typename Block_Backend< Key, Value, Iterator >::Discrete_Iterator* backend_it;
  Key frontend_index_;
  Iterator begin;
  Iterator end;
};


template< typename Key, typename Value, typename Iterator >
Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::Block_Backend_Discrete_Cached_Request
    (Block_Backend_Cache_Base& cache_, int used_timestamp,
     const Iterator& begin_, const Iterator& end_)
    : Block_Backend_Basic_Cached_Request< Key, Value >(used_timestamp),
      cache(dynamic_cast< Block_Backend_Cache< Key, Value >* >(&cache_)),
      backend_it(0), frontend_index_(begin_ == end_ ? Key() : *begin_),
      begin(begin_), end(end_)
{
  cache->register_request(*this);
}


template< typename Key, typename Value, typename Iterator >
Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::~Block_Backend_Discrete_Cached_Request()
{
  cache->unregister_request(*this);
  delete backend_it;
}


template< typename Key, typename Value, typename Iterator >
void Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::skip_frontend_index(const Key& target)
{
  if (target == Key())
    frontend_index_ = target;
  else
  {
    while (!(begin == end) && *begin < target)
      ++begin;
    if (begin == end)
      frontend_index_ = Key();
    else
      frontend_index_ = *begin;
  }
}


template< typename Key, typename Value, typename Iterator >
std::pair< int, Key > Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >
    ::read_whole_key_base(const Key& key, std::vector< Value >& result_values)
{
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value >::Discrete_Iterator
        (cache->get_db().discrete_begin(begin, end));
  if (*backend_it == cache->get_db().discrete_end())
    return std::make_pair(0, Key());
  
  if (backend_it->index() == key)
    return backend_it->read_whole_key(result_values);
  result_values.clear();
  //TODO: return the correct next index
  return std::make_pair(0, backend_it->index());    
}


template< typename Key, typename Value, typename Iterator >
std::pair< Key, const std::vector< Value >* >
    Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::read_whole_key()
{
  return cache->read_whole_key(*this);
}


#endif
