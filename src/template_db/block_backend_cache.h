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
    : db_index(&db_index_), transaction(&transaction_),
      size_cached_(0), size_total_requested_(0), size_read_from_disk_(0),
      num_total_requested_(0), num_read_from_disk_(0) {}
  
  virtual void trim_non_reserved();
  virtual void trim_reserved();
  
  virtual ~Block_Backend_Cache() {}
  
  std::pair< Key, const std::vector< Value >* > read_whole_key
      (Block_Backend_Basic_Cached_Request< Key, Value >& request);
  
  void register_request(Block_Backend_Basic_Cached_Request< Key, Value >& request);
  void unregister_request(Block_Backend_Basic_Cached_Request< Key, Value >& request);
  
  File_Blocks_Index_Base* get_db_index() const { return db_index; }
  
  virtual uint64 size_cached() const { return size_cached_; }
  virtual uint64 size_total_requested() const { return size_total_requested_; }
  virtual uint64 size_read_from_disk() const { return size_read_from_disk_; }
  virtual uint32 num_cached() const { return cached_blocks.size(); }
  virtual uint32 num_total_requested() const { return num_total_requested_; }
  virtual uint32 num_read_from_disk() const { return num_read_from_disk_; }
  
private:
  File_Blocks_Index_Base* db_index;
  Transaction* transaction;
  std::map< Key, Cache_Entry< Key, Value > > cached_blocks;
  std::vector< Block_Backend_Basic_Cached_Request< Key, Value >* > registered_requests;
  uint64 size_cached_, size_total_requested_, size_read_from_disk_;
  uint32 num_total_requested_, num_read_from_disk_;
};


template< typename Key, typename Value >
class Block_Backend_Basic_Cached_Request
{
public:
  Block_Backend_Basic_Cached_Request(int used_timestamp) : used_timestamp_(used_timestamp) {}
  virtual ~Block_Backend_Basic_Cached_Request() {}
  
  virtual const Key& frontend_key() const = 0;
  virtual Key backend_key() const = 0;
  virtual bool is_end() const = 0;
  virtual bool is_reserved(const Key& key) = 0;
  bool is_blocked(const Key& key) { return last_used_key == key; }
  
  virtual void skip_frontend_iterator(const Key& target) = 0;
  virtual void skip_backend_iterator() = 0;
  virtual void set_end() = 0;
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values) = 0;
  void set_last_used_key(const Key& key) { last_used_key = key; }
  
  int used_timestamp() const { return used_timestamp_; }
  
private:
  int used_timestamp_;
  Key last_used_key;
};


// Implementation of Block_Backend_Cache: --------------------------------------


template< typename Key, typename Value >
std::pair< Key, const std::vector< Value >* > Block_Backend_Cache< Key, Value >::read_whole_key
    (Block_Backend_Basic_Cached_Request< Key, Value >& request)
{
  bool values_found = false;
  while (!values_found)
  {
    if (request.is_end())
      return std::pair< Key, const std::vector< Value >* >(request.frontend_key(), 0);
    Key key = request.frontend_key();
  
    typename std::map< Key, Cache_Entry< Key, Value > >::iterator
        cache_it = cached_blocks.upper_bound(key);
    if (cache_it != cached_blocks.begin())
      --cache_it;
    
    if (cache_it != cached_blocks.end() && cache_it->first == key)
      values_found = (cache_it->second.size > 0);
    else if (cache_it != cached_blocks.end() &&
        cache_it->first < key && key < cache_it->second.next_key)
      values_found = false;
    else
    {
      transaction->trim_cache();
      
      request.skip_backend_iterator();
      if (request.backend_key() == key)
      {
        cache_it = cached_blocks.insert(std::make_pair
            (key, Cache_Entry< Key, Value >(request.used_timestamp()))).first;
        std::pair< int, Key > size_and_next
            = request.read_whole_key_base(key, cache_it->second.values);
        cache_it->second.size = size_and_next.first;
        cache_it->second.next_key = size_and_next.second;
	
        values_found = (cache_it->second.size > 0);
	size_cached_ += cache_it->second.size = size_and_next.first;
	++num_read_from_disk_;
	size_read_from_disk_ += cache_it->second.size = size_and_next.first;	
      }
      else
      {
        cache_it = cached_blocks.insert(std::make_pair
            (key, Cache_Entry< Key, Value >(request.used_timestamp()))).first;
        cache_it->second.size = 0;
        cache_it->second.next_key = request.backend_key();
	++num_read_from_disk_;
      }
    }
    
    if (cache_it->second.next_key == Key())
      request.set_end();
    else
      request.skip_frontend_iterator(cache_it->second.next_key);
    
    if (values_found)
    {
      ++num_total_requested_;
      size_total_requested_ += cache_it->second.size;
      request.set_last_used_key(key);
      return std::make_pair(key, &cache_it->second.values);
    }
  }
  
  // Will never be executed
  return std::pair< Key, const std::vector< Value >* >(Key(), 0);
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
      if ((*it2)->is_blocked(it->first) || (*it2)->is_reserved(it->first))
      {
	reserved = true;
	break;
      }
    }
    if (reserved)
      ++it;
    else
    {
      size_cached_ -= it->second.size;
      typename std::map< Key, Cache_Entry< Key, Value > >::iterator erase_it = it;
      ++it;
      cached_blocks.erase(erase_it);
    }
  }
}


template< typename Key, typename Value >
void Block_Backend_Cache< Key, Value >::trim_reserved()
{
  typename std::map< Key, Cache_Entry< Key, Value > >::iterator it = cached_blocks.begin();
  while (it != cached_blocks.end())
  {
    bool reserved = false;
    for (typename std::vector< Block_Backend_Basic_Cached_Request< Key, Value >* >::const_iterator
        it2 = registered_requests.begin(); it2 != registered_requests.end(); ++it2)
    {
      if ((*it2)->is_blocked(it->first))
      {
	reserved = true;
	break;
      }
    }
    if (reserved)
      ++it;
    else
    {
      size_cached_ -= it->second.size;
      typename std::map< Key, Cache_Entry< Key, Value > >::iterator erase_it = it;
      ++it;
      cached_blocks.erase(erase_it);
    }
  }
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
}


// Implementation of Block_Backend_Flat_Cached_Request: ------------------------


template< typename Key, typename Value >
class Block_Backend_Flat_Cached_Request : Block_Backend_Basic_Cached_Request< Key, Value >
{
public:
  Block_Backend_Flat_Cached_Request(Block_Backend_Cache_Base& cache_, int used_timestamp);
  virtual ~Block_Backend_Flat_Cached_Request();
  
  virtual const Key& frontend_key() const { return frontend_index_; }
  virtual Key backend_key() const;
  virtual bool is_end() const { return frontend_index_ == Key(); }
  virtual bool is_reserved(const Key& key) { return !(key < frontend_index_); }
  
  virtual void skip_frontend_iterator(const Key& target) { frontend_index_ = target; }
  virtual void skip_backend_iterator();
  virtual void set_end() { frontend_index_ = Key(); }
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values);
  
  std::pair< Key, const std::vector< Value >* > read_whole_key();  
  
private:
  Block_Backend_Cache< Key, Value >* cache;
  Block_Backend< Key, Value >* db;
  typename Block_Backend< Key, Value >::Flat_Iterator* backend_it;
  Key frontend_index_;
};


template< typename Key, typename Value >
Block_Backend_Flat_Cached_Request< Key, Value >::Block_Backend_Flat_Cached_Request
    (Block_Backend_Cache_Base& cache_, int used_timestamp)
    : Block_Backend_Basic_Cached_Request< Key, Value >(used_timestamp),
      cache(dynamic_cast< Block_Backend_Cache< Key, Value >* >(&cache_)),
      db(new Block_Backend< Key, Value >(cache->get_db_index())),
      backend_it(new typename Block_Backend< Key, Value >::Flat_Iterator(db->flat_begin())),
      frontend_index_((*backend_it == db->flat_end()) ? Key() : backend_it->index())
{
  cache->register_request(*this);
}


template< typename Key, typename Value >
Block_Backend_Flat_Cached_Request< Key, Value >::~Block_Backend_Flat_Cached_Request()
{
  cache->unregister_request(*this);
  delete backend_it;
  delete db;
}


template< typename Key, typename Value >
Key Block_Backend_Flat_Cached_Request< Key, Value >::backend_key() const
{
  if (is_end())
    return Key();
  
  if (backend_it)
    return backend_it->index();
  else
    return Key();
}


template< typename Key, typename Value >
void Block_Backend_Flat_Cached_Request< Key, Value >::skip_backend_iterator()
{
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value >::Flat_Iterator(db->flat_begin());
  
  if (!(*backend_it == db->flat_end()))
    backend_it->skip_to_index(frontend_index_);
  
  if (*backend_it == db->flat_end())
  {
    set_end();
    delete backend_it;
    backend_it = 0;
  }
}

      
template< typename Key, typename Value >
std::pair< int, Key > Block_Backend_Flat_Cached_Request< Key, Value >
    ::read_whole_key_base(const Key& key, std::vector< Value >& result_values)
{
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value >::Flat_Iterator(db->flat_begin());
  if (*backend_it == db->flat_end())
    return std::make_pair(0, Key());  
  
  return backend_it->read_whole_key(result_values);
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
  
  virtual const Key& frontend_key() const { return frontend_index_; }
  virtual Key backend_key() const;
  virtual bool is_end() const { return request_it == end; }
  virtual bool is_reserved(const Key& key) { return !(key < frontend_index_); }
  
  virtual void skip_frontend_iterator(const Key& target);
  virtual void skip_backend_iterator();
  virtual void set_end() { request_it = end; }
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values);
  
  std::pair< Key, const std::vector< Value >* > read_whole_key();  
  
private:
  Block_Backend_Discrete_Cached_Request(const Block_Backend_Discrete_Cached_Request&);

  Block_Backend_Cache< Key, Value >* cache;
  Block_Backend< Key, Value, Iterator >* db;
  typename Block_Backend< Key, Value, Iterator >::Discrete_Iterator* backend_it;
  Key frontend_index_;
  Iterator request_it;
  Iterator end;
};


template< typename Key, typename Value, typename Iterator >
Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::Block_Backend_Discrete_Cached_Request
    (Block_Backend_Cache_Base& cache_, int used_timestamp,
     const Iterator& begin_, const Iterator& end_)
    : Block_Backend_Basic_Cached_Request< Key, Value >(used_timestamp),
      cache(dynamic_cast< Block_Backend_Cache< Key, Value >* >(&cache_)), db(0),
      backend_it(0), frontend_index_(begin_ == end_ ? Key() : *begin_),
      request_it(begin_), end(end_)
{
  cache->register_request(*this);
}


template< typename Key, typename Value, typename Iterator >
Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::~Block_Backend_Discrete_Cached_Request()
{
  cache->unregister_request(*this);
  delete backend_it;
  delete db;
}


template< typename Key, typename Value, typename Iterator >
Key Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::backend_key() const
{
  if (is_end())
    return Key();
  
  if (backend_it)
    return backend_it->index();
  else
    return Key();
}


template< typename Key, typename Value, typename Iterator >
void Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::skip_frontend_iterator(const Key& target)
{
  if (target == Key())
    request_it = end;
  else
  {
    while (!(request_it == end) && *request_it < target)
      ++request_it;
    if (request_it == end)
      frontend_index_ = Key();
    else
      frontend_index_ = *request_it;
  }
}


template< typename Key, typename Value, typename Iterator >
void Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::skip_backend_iterator()
{
  if (!db)
    db = new Block_Backend< Key, Value >(cache->get_db_index());
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value, Iterator >::Discrete_Iterator
        (db->discrete_begin(request_it, end));
  
  if (!(*backend_it == db->discrete_end()))
    backend_it->skip_to_index(frontend_index_);
  
  if (*backend_it == db->discrete_end())
  {
    set_end();
    delete backend_it;
    backend_it = 0;
  }
}

      
template< typename Key, typename Value, typename Iterator >
std::pair< int, Key > Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >
    ::read_whole_key_base(const Key& key, std::vector< Value >& result_values)
{
  if (!db)
    db = new Block_Backend< Key, Value >(cache->get_db_index());
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value, Iterator >::Discrete_Iterator
        (db->discrete_begin(request_it, end));
  if (*backend_it == db->discrete_end())
    return std::make_pair(0, Key());
  
  return backend_it->read_whole_key(result_values);
}


template< typename Key, typename Value, typename Iterator >
std::pair< Key, const std::vector< Value >* >
    Block_Backend_Discrete_Cached_Request< Key, Value, Iterator >::read_whole_key()
{
  return cache->read_whole_key(*this);
}


// Implementation of Block_Backend_Range_Cached_Request: ------------------------


template< typename Key, typename Value, typename Iterator = typename std::set< Key >::const_iterator >
class Block_Backend_Range_Cached_Request : Block_Backend_Basic_Cached_Request< Key, Value >
{
public:
  Block_Backend_Range_Cached_Request(Block_Backend_Cache_Base& cache_, int used_timestamp,
      const Default_Range_Iterator< Key >& begin_, const Default_Range_Iterator< Key >& end_);
  virtual ~Block_Backend_Range_Cached_Request();
  
  virtual const Key& frontend_key() const { return frontend_index_; }
  virtual Key backend_key() const;
  virtual bool is_end() const { return request_it == end; }
  virtual bool is_reserved(const Key& key) { return !(key < frontend_index_); }
  
  virtual void skip_frontend_iterator(const Key& target);
  virtual void skip_backend_iterator();
  virtual void set_end() { request_it = end; }
  virtual std::pair< int, Key > read_whole_key_base(const Key& key, std::vector< Value >& result_values);
  
  std::pair< Key, const std::vector< Value >* > read_whole_key();  
  
private:
  Block_Backend_Range_Cached_Request(const Block_Backend_Range_Cached_Request&);

  Block_Backend_Cache< Key, Value >* cache;
  Block_Backend< Key, Value, Iterator >* db;
  typename Block_Backend< Key, Value, Iterator >::Range_Iterator* backend_it;
  Key frontend_index_;
  Default_Range_Iterator< Key > request_it;
  Default_Range_Iterator< Key > end;
};


template< typename Key, typename Value, typename Iterator >
Block_Backend_Range_Cached_Request< Key, Value, Iterator >::Block_Backend_Range_Cached_Request
    (Block_Backend_Cache_Base& cache_, int used_timestamp,
     const Default_Range_Iterator< Key >& begin_, const Default_Range_Iterator< Key >& end_)
    : Block_Backend_Basic_Cached_Request< Key, Value >(used_timestamp),
      cache(dynamic_cast< Block_Backend_Cache< Key, Value >* >(&cache_)), db(0),
      backend_it(0), frontend_index_(begin_ == end_ ? Key() : begin_.lower_bound()),
      request_it(begin_), end(end_)
{
  cache->register_request(*this);
}


template< typename Key, typename Value, typename Iterator >
Block_Backend_Range_Cached_Request< Key, Value, Iterator >::~Block_Backend_Range_Cached_Request()
{
  cache->unregister_request(*this);
  delete backend_it;
  delete db;
}


template< typename Key, typename Value, typename Iterator >
Key Block_Backend_Range_Cached_Request< Key, Value, Iterator >::backend_key() const
{
  if (is_end())
    return Key();
  
  if (backend_it)
    return backend_it->index();
  else
    return Key();
}


template< typename Key, typename Value, typename Iterator >
void Block_Backend_Range_Cached_Request< Key, Value, Iterator >::skip_frontend_iterator(const Key& target)
{
  if (target == Key())
    request_it = end;
  else
  {
    while (!(request_it == end) && !(target < request_it.upper_bound()))
      ++request_it;
    if (request_it == end)
      frontend_index_ = Key();
    else if (target < request_it.lower_bound())
      frontend_index_ = request_it.lower_bound();
    else
      frontend_index_ = target;
  }
}


template< typename Key, typename Value, typename Iterator >
void Block_Backend_Range_Cached_Request< Key, Value, Iterator >::skip_backend_iterator()
{
  if (!db)
    db = new Block_Backend< Key, Value >(cache->get_db_index());
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value >::Range_Iterator
        (db->range_begin(request_it, end));  
  if (!(*backend_it == db->range_end()))
    backend_it->skip_to_index(frontend_index_);
  
  if (*backend_it == db->range_end())
  {
    set_end();
    delete backend_it;
    backend_it = 0;
  }
}

      
template< typename Key, typename Value, typename Iterator >
std::pair< int, Key > Block_Backend_Range_Cached_Request< Key, Value, Iterator >
    ::read_whole_key_base(const Key& key, std::vector< Value >& result_values)
{
  if (!db)
    db = new Block_Backend< Key, Value >(cache->get_db_index());
  if (!backend_it)
    backend_it = new typename Block_Backend< Key, Value >::Range_Iterator
        (db->range_begin(request_it, end));
  if (*backend_it == db->range_end())
    return std::make_pair(0, Key());
  
  return backend_it->read_whole_key(result_values);
}


template< typename Key, typename Value, typename Iterator >
std::pair< Key, const std::vector< Value >* >
    Block_Backend_Range_Cached_Request< Key, Value, Iterator >::read_whole_key()
{
  return cache->read_whole_key(*this);
}


#endif
