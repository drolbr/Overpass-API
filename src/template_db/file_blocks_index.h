/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_INDEX_H
#define DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_INDEX_H

#include "types.h"

#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <list>
#include <string>
#include <vector>

/** Declarations: -----------------------------------------------------------*/


template< class Index >
struct File_Block_Index_Entry
{
  static const int EMPTY = 1;
  static const int GROUP = 2;
  static const int SEGMENT = 3;
  static const int LAST_SEGMENT = 4;

  File_Block_Index_Entry(const Index& index_, uint32 pos_, uint32 size_)
    : index(index_), pos(pos_), size(size_) {}

  Index index;
  uint32 pos;
  uint32 size;
};


struct File_Blocks_Index_Structure_Params;


struct File_Blocks_Index_File
{
public:
  File_Blocks_Index_File(
      const File_Properties& file_prop, const std::string& db_dir,
      bool use_shadow, const std::string& file_name_extension);
  const uint8* header() const { return buf.ptr; }
  const uint8* begin() const { return buf.ptr ? buf.ptr + 8 : 0; }
  const uint8* end() const { return buf.ptr + size_; }
  uint32 size() const { return size_; }

  void clear_buf()
  {
    buf.resize(0);
    size_ = 0;
  }
  
  std::string file_name;
  
  template< typename Index >
  static void inc(const uint8*& ptr)
  {
    ptr += 12;
    ptr += Index::size_of((void*)ptr);
  }

  template< class Index >
  void rebuild_index_buf(
      const File_Blocks_Index_Structure_Params& params,
      const std::list< File_Block_Index_Entry< Index > >& block_list);

private:
  Void_Pointer< uint8 > buf;
  uint32 size_;
};


struct File_Blocks_Index_Mmap
{
public:
  File_Blocks_Index_Mmap(
      const File_Properties& file_prop, const std::string& db_dir,
      bool use_shadow, const std::string& file_name_extension);
  ~File_Blocks_Index_Mmap()
  {
    if (ptr)
      munmap(ptr, size_);
  }
  const uint8* header() const { return (uint8*)ptr; }
  const uint8* begin() const { return ptr ? ((uint8*)ptr) + 8 : 0; }
  const uint8* end() const { return ((uint8*)ptr) + size_; }
  uint32 size() const { return size_; }

  std::string file_name;

private:
  void* ptr;
  uint32 size_;
};


struct File_Blocks_Index_Structure_Params
{
  template< typename Idx_File >
  File_Blocks_Index_Structure_Params(
      const File_Properties& file_prop, const std::string& file_name_extension, int compression_method_,
      const Idx_File& idx_file, uint64 file_size);

  bool empty_;
  uint64 block_size_;
  uint32 compression_factor;
  int compression_method;

  uint32 block_count;

  static const int FILE_FORMAT_VERSION = 7560;
};


template< typename Index >
struct File_Blocks_Index_Iterator
{
public:
  File_Blocks_Index_Iterator(const uint8* ptr_) : ptr(ptr_), idx(0) {}
  ~File_Blocks_Index_Iterator() { delete idx; }
  File_Blocks_Index_Iterator(const File_Blocks_Index_Iterator& rhs) : ptr(rhs.ptr), idx(0) {}
  File_Blocks_Index_Iterator& operator=(const File_Blocks_Index_Iterator& rhs)
  {
    if (ptr != rhs.ptr)
    {
      delete idx;
      idx = 0;
      ptr = rhs.ptr;
    }
    return *this;
  }

  void operator++()
  {
    delete idx;
    idx = 0;
    ptr += 12;
    ptr += Index::size_of((void*)ptr);
  }
  bool operator==(File_Blocks_Index_Iterator rhs) const
  { return ptr == rhs.ptr; }
  bool operator!=(File_Blocks_Index_Iterator rhs) const { return ptr != rhs.ptr; }

  void* idx_ptr() const { return (void*)(ptr + 12); }
  Index index() const
  {
    if (!idx)
      idx = new Index((void*)(ptr + 12));
    return *idx;
  }
  uint32 pos() const { return *(uint32*)ptr; }
  uint32 size() const { return *(uint32*)(ptr + 4); }

private:
  const uint8* ptr;
  mutable Index* idx;
};


template< typename Index >
struct Readonly_File_Blocks_Index : public File_Blocks_Index_Base
{
public:
  Readonly_File_Blocks_Index(
      const File_Properties& file_prop, bool use_shadow,
      const std::string& db_dir, const std::string& file_name_extension);
  bool writeable() const { return false; }
  const std::string& file_name_extension() const { return file_name_extension_; }

  virtual std::string get_data_file_name() const { return data_file_name; }
  virtual uint64 get_block_size() const { return params.block_size_; }
  virtual uint32 get_compression_factor() const { return params.compression_factor; }
  virtual uint32 get_compression_method() const { return params.compression_method; }
  virtual uint32 get_block_count() const { return params.block_count; }
  void increase_block_count(uint32 delta) { params.block_count += delta; }
  virtual bool empty() const { return params.empty_; }
  File_Blocks_Index_Iterator< Index > begin()
  { return File_Blocks_Index_Iterator< Index >(idx_file.begin()); }
  File_Blocks_Index_Iterator< Index > end()
  { return File_Blocks_Index_Iterator< Index >(idx_file.end()); }

private:
  File_Blocks_Index_Mmap idx_file;
  std::string data_file_name;
  File_Blocks_Index_Structure_Params params;
  std::string file_name_extension_;
};


template< class Index >
Void_Pointer< uint8 > make_index_buf(
    const File_Blocks_Index_Structure_Params& params,
    const std::list< File_Block_Index_Entry< Index > >& block_list,
    uint32& index_size);


template< class Index >
struct Writeable_File_Blocks_Index : public File_Blocks_Index_Base
{
public:
  Writeable_File_Blocks_Index(
      const File_Properties& file_prop, bool use_shadow,
      const std::string& db_dir, const std::string& file_name_extension,
      int compression_method_ = USE_DEFAULT);
  virtual ~Writeable_File_Blocks_Index();
  bool writeable() const { return true; }
  const std::string& file_name_extension() const { return file_name_extension_; }

  virtual std::string get_data_file_name() const { return data_file_name; }
  virtual uint64 get_block_size() const { return params.block_size_; }
  virtual uint32 get_compression_factor() const { return params.compression_factor; }
  virtual uint32 get_compression_method() const { return params.compression_method; }
  virtual uint32 get_block_count() const { return params.block_count; }
  void increase_block_count(uint32 delta) { params.block_count += delta; }
  virtual bool empty() const { return params.empty_; }
  File_Blocks_Index_Iterator< Index > begin()
  {
    if (!idx_file_buf_valid)
    {
      idx_file.rebuild_index_buf(params, block_list);
      idx_file_buf_valid = true;      
    }
    return File_Blocks_Index_Iterator< Index >(idx_file.begin());
  }
  File_Blocks_Index_Iterator< Index > end()
  { 
    if (!idx_file_buf_valid)
    {
      idx_file.rebuild_index_buf(params, block_list);
      idx_file_buf_valid = true;      
    }
    return File_Blocks_Index_Iterator< Index >(idx_file.end());
  }

  const typename std::list< File_Block_Index_Entry< Index > >::iterator wr_begin() { return block_list.begin(); }
  const typename std::list< File_Block_Index_Entry< Index > >::iterator wr_end() { return block_list.end(); }

  typename std::list< File_Block_Index_Entry< Index > >::iterator insert(
      typename std::list< File_Block_Index_Entry< Index > >::iterator& block_it,
      const File_Block_Index_Entry< Index >& entry)
  { return block_list.insert(block_it, entry); }
  typename std::list< File_Block_Index_Entry< Index > >::iterator erase(
      typename std::list< File_Block_Index_Entry< Index > >::iterator& block_it)
  { return block_list.erase(block_it); }

  std::vector< std::pair< uint32, uint32 > >& get_void_blocks()
  {
    if (!void_blocks_initialized)
      init_void_blocks();
    return void_blocks;
  }
  void drop_block_array()
  {
    idx_file_buf_valid = false;
  }

private:
  File_Blocks_Index_File idx_file;
  bool idx_file_buf_valid;
  std::string empty_index_file_name;
  std::string data_file_name;
  File_Blocks_Index_Structure_Params params;
  std::string file_name_extension_;

  std::list< File_Block_Index_Entry< Index > > block_list;
  std::vector< std::pair< uint32, uint32 > > void_blocks;
  bool void_blocks_initialized;

  void init_blocks();
  void init_void_blocks();
};


template< class Index >
std::vector< bool > get_data_index_footprint(const File_Properties& file_prop,
					std::string db_dir);

/** Implementation File_Blocks_Index: ---------------------------------------*/

inline uint64 file_size_of(const std::string& data_file_name)
{
  try
  {
    Raw_File val_file(data_file_name, O_RDONLY, S_666, "File_Blocks_Index::File_Blocks_Index::1");
    return val_file.size("File_Blocks_Index::File_Blocks_Index::2");
  }
  catch (File_Error e)
  {
    if (e.error_number != ENOENT)
      throw;
  }
  return 0;
}


inline File_Blocks_Index_File::File_Blocks_Index_File(
    const File_Properties& file_prop, const std::string& db_dir,
    bool use_shadow, const std::string& file_name_extension)
    : file_name(db_dir + file_prop.get_file_name_trunk()
        + file_name_extension + file_prop.get_data_suffix()
        + file_prop.get_index_suffix()
        + (use_shadow ? file_prop.get_shadow_suffix() : "")),
      buf(0), size_(0)
{
  try
  {
    Raw_File source_file(file_name, O_RDONLY, S_666, "File_Blocks_Index::File_Blocks_Index::3");

    // read index file
    size_ = source_file.size("File_Blocks_Index::File_Blocks_Index::4");
    buf.resize(size_);
    source_file.read(buf.ptr, size_, "File_Blocks_Index::File_Blocks_Index::5");
  }
  catch (File_Error e)
  {
    if (e.error_number != ENOENT)
      throw;
    buf.resize(0);
  }
}


inline File_Blocks_Index_Mmap::File_Blocks_Index_Mmap(
    const File_Properties& file_prop, const std::string& db_dir,
    bool use_shadow, const std::string& file_name_extension)
    : file_name(db_dir + file_prop.get_file_name_trunk()
        + file_name_extension + file_prop.get_data_suffix()
        + file_prop.get_index_suffix()
        + (use_shadow ? file_prop.get_shadow_suffix() : "")),
      ptr(0), size_(0)
{
  try
  {
    Raw_File source_file(file_name, O_RDONLY, S_666, "File_Blocks_Index_Mmap::File_Blocks_Index_Mmap::1");
    
    // read index file
    size_ = source_file.size("File_Blocks_Index_Mmap::File_Blocks_Index_Mmap::2");
    if (size_)
      ptr = mmap(0, size_, PROT_READ, MAP_PRIVATE, source_file.fd(), 0);
    if (ptr == (void*)(-1))
      throw File_Error(errno, file_name, "File_Blocks_Index_Mmap::File_Blocks_Index_Mmap::3");
  }
  catch (File_Error e)
  {
    if (e.error_number != ENOENT)
      throw;
  }
}


template< typename Idx_File >
File_Blocks_Index_Structure_Params::File_Blocks_Index_Structure_Params(
    const File_Properties& file_prop, const std::string& file_name_extension, int compression_method_,
    const Idx_File& idx_file, uint64 file_size)
    : empty_(false),
     block_size_(file_prop.get_block_size()), // can be overwritten by index file
     compression_factor(file_prop.get_compression_factor()), // can be overwritten by index file
     compression_method(compression_method_ == File_Blocks_Index_Base::USE_DEFAULT ?
        file_prop.get_compression_method() : compression_method_), // can be overwritten by index file
     block_count(0)
{
  const uint8* header = idx_file.header();
  if (header)
  {
    if (file_name_extension != ".legacy")
    {
      if (*(int32*)header != FILE_FORMAT_VERSION && *(int32*)header != 7512)
	throw File_Error(0, idx_file.file_name, "File_Blocks_Index: Unsupported index file format version");
      block_size_ = 1ull<<*(uint8*)(header + 4);
      if (!block_size_)
        throw File_Error(0, idx_file.file_name, "File_Blocks_Index: Illegal block size");
      compression_factor = 1u<<*(uint8*)(header + 5);
      if (!compression_factor || compression_factor > block_size_)
        throw File_Error(0, idx_file.file_name, "File_Blocks_Index: Illegal compression factor");
      compression_method = *(uint16*)(header + 6);
    }
    if (file_size % block_size_)
      throw File_Error(0, idx_file.file_name, "File_Blocks_Index: Data file size does not match block size");
    block_count = file_size / block_size_;
  }
  empty_ = (file_size == 0);
}


template< class Index >
Readonly_File_Blocks_Index< Index >::Readonly_File_Blocks_Index(
    const File_Properties& file_prop, bool use_shadow,
    const std::string& db_dir, const std::string& file_name_extension)
    : idx_file(file_prop, db_dir, use_shadow, file_name_extension),
      data_file_name(db_dir + file_prop.get_file_name_trunk()
          + file_name_extension + file_prop.get_data_suffix()),
      params(file_prop, file_name_extension, USE_DEFAULT, idx_file, file_size_of(data_file_name)), 
      file_name_extension_(file_name_extension) {}


template< class Index >
Writeable_File_Blocks_Index< Index >::Writeable_File_Blocks_Index
    (const File_Properties& file_prop, bool use_shadow,
     const std::string& db_dir, const std::string& file_name_extension,
     int compression_method_) :
     idx_file(file_prop, db_dir, use_shadow, file_name_extension), idx_file_buf_valid(true),
     empty_index_file_name(db_dir + file_prop.get_file_name_trunk()
         + file_name_extension + file_prop.get_data_suffix()
         + file_prop.get_shadow_suffix()),
     data_file_name(db_dir + file_prop.get_file_name_trunk()
         + file_name_extension + file_prop.get_data_suffix()),
     params(file_prop, file_name_extension, compression_method_, idx_file, file_size_of(data_file_name)), 
     file_name_extension_(file_name_extension), void_blocks_initialized(false)
{
  init_blocks();
  init_void_blocks();
}


template< class Index >
void Writeable_File_Blocks_Index< Index >::init_blocks()
{
  if (idx_file.header())
  {
//     clock_t start = clock();
    
    const uint8* ptr = idx_file.begin();
    while (ptr < idx_file.end())
    {
      Index index((void*)(ptr + 12));
      File_Block_Index_Entry< Index >
          entry(index, *(uint32*)(ptr), *(uint32*)(ptr + 4));
      if (entry.pos >= params.block_count)
        throw File_Error(0, idx_file.file_name, "File_Blocks_Index: bad pos in index file");
      if (entry.pos + entry.size > params.block_count)
        throw File_Error(0, idx_file.file_name, "File_Blocks_Index: bad size in index file");
      File_Blocks_Index_File::inc< Index >(ptr);

      block_list.push_back(entry);
    }
    
//     clock_t end = clock();
//     std::cout<<std::dec<<params.block_size_<<'\t'<<(end - start)<<'\t'<<data_file_name<<'\n';
  }
}


inline std::vector< std::pair< uint32, uint32 > > compute_void_blocks(const std::vector< bool >& is_referred)
{
  std::vector< std::pair< uint32, uint32 > > void_blocks;
  // determine void_blocks
  uint32 last_start = 0;
  for (uint32 i = 0; i < is_referred.size(); ++i)
  {
    if (is_referred[i])
    {
      if (last_start < i)
        void_blocks.push_back(std::make_pair(i - last_start, last_start));
      last_start = i+1;
    }
  }
  if (last_start < is_referred.size())
    void_blocks.push_back(std::make_pair(is_referred.size() - last_start, last_start));

  return void_blocks;
}


template< typename Iterator >
std::vector< std::pair< uint32, uint32 > > compute_void_blocks(Iterator begin, Iterator end, uint32 block_count)
{
  std::vector< bool > is_referred(block_count, false);
  for (auto it = begin; it != end; ++it)
  {
    for (uint32 i = 0; i < it.size(); ++i)
      is_referred[it.pos() + i] = true;
  }
  
  return compute_void_blocks(is_referred);
}


template< typename List >
std::vector< std::pair< uint32, uint32 > > compute_void_blocks(const List& block_list, uint32 block_count)
{
  std::vector< bool > is_referred(block_count, false);
  for (auto it = block_list.begin(); it != block_list.end(); ++it)
  {
    for (uint32 i = 0; i < it->size; ++i)
      is_referred[it->pos + i] = true;
  }
  
  return compute_void_blocks(is_referred);
}


template< class Index >
void Writeable_File_Blocks_Index< Index >::init_void_blocks()
{
  bool empty_index_file_used = false;
  if (empty_index_file_name != "")
  {
    try
    {
      Raw_File void_blocks_file(empty_index_file_name, O_RDONLY, S_666, "");
      uint32 void_index_size = void_blocks_file.size("File_Blocks_Index::File_Blocks_Index::6");
      Void_Pointer< uint8 > index_buf(void_index_size);
      void_blocks_file.read(index_buf.ptr, void_index_size,
			      "File_Blocks_Index::File_Blocks_Index::7");
      for (uint32 i = 0; i < void_index_size/8; ++i)
        void_blocks.push_back(*(std::pair< uint32, uint32 >*)(index_buf.ptr + 8*i));
      empty_index_file_used = true;
    }
    catch (File_Error e) {}
  }

  if (!empty_index_file_used)
    void_blocks = compute_void_blocks(block_list, params.block_count);

  std::stable_sort(void_blocks.begin(), void_blocks.end());
  void_blocks_initialized = true;
}


template< class Index >
void File_Blocks_Index_File::rebuild_index_buf(
    const File_Blocks_Index_Structure_Params& params,
    const std::list< File_Block_Index_Entry< Index > >& block_list)
{
  // Keep space for file version and size information
  size_ = 8;
  uint32 pos = 8;

  for (typename std::list< File_Block_Index_Entry< Index > >::const_iterator
      it(block_list.begin()); it != block_list.end(); ++it)
    size_ += 12 + it->index.size_of();

  buf.resize(size_);

  *(uint32*)buf.ptr = File_Blocks_Index_Structure_Params::FILE_FORMAT_VERSION;
  *(uint8*)(buf.ptr + 4) = shift_log(params.block_size_);
  *(uint8*)(buf.ptr + 5) = shift_log(params.compression_factor);
  *(uint16*)(buf.ptr + 6) = params.compression_method;

  for (typename std::list< File_Block_Index_Entry< Index > >::const_iterator
      it(block_list.begin()); it != block_list.end(); ++it)
  {
    *(uint32*)(buf.ptr+pos) = it->pos;
    pos += 4;
    *(uint32*)(buf.ptr+pos) = it->size;
    pos += 4;
    *(uint32*)(buf.ptr+pos) = 0;
    pos += 4;
    it->index.to_data(buf.ptr+pos);
    pos += it->index.size_of();
  }
}


template< class Index >
Writeable_File_Blocks_Index< Index >::~Writeable_File_Blocks_Index()
{
  // Keep space for file version and size information
  if (!idx_file_buf_valid)
    idx_file.rebuild_index_buf(params, block_list);

  Raw_File dest_file(idx_file.file_name, O_RDWR|O_CREAT, S_666,
		     "File_Blocks_Index::~File_Blocks_Index::1");

  if (idx_file.size() < dest_file.size("File_Blocks_Index::~File_Blocks_Index::2"))
    dest_file.resize(idx_file.size(), "File_Blocks_Index::~File_Blocks_Index::3");
  dest_file.write((void*)idx_file.header(), idx_file.size(), "File_Blocks_Index::~File_Blocks_Index::4");

  // Write void blocks
  Void_Pointer< uint8 > void_index_buf(void_blocks.size() * 8);
  std::pair< uint32, uint32 >* it_ptr = (std::pair< uint32, uint32 >*)(void_index_buf.ptr);
  for (std::vector< std::pair< uint32, uint32 > >::const_iterator it(void_blocks.begin());
      it != void_blocks.end(); ++it)
    *(it_ptr++) = *it;

  try
  {
    Raw_File void_file(empty_index_file_name, O_RDWR|O_TRUNC, S_666,
		       "File_Blocks_Index::~File_Blocks_Index::5");
    void_file.write(void_index_buf.ptr, void_blocks.size() * 8,
		    "File_Blocks_Index::~File_Blocks_Index::6");
  }
  catch (File_Error e) {}
}


/** Implementation non-members: ---------------------------------------------*/

template< class Index >
std::vector< bool > get_data_index_footprint
    (const File_Properties& file_prop, std::string db_dir)
{
  Readonly_File_Blocks_Index< Index > index(file_prop, false, db_dir, "");
  auto void_blocks = compute_void_blocks(index.begin(), index.end(), index.get_block_count());

  std::vector< bool > result(index.get_block_count(), true);
  for (typename std::vector< std::pair< uint32, uint32 > >::const_iterator
      it = void_blocks.begin(); it != void_blocks.end(); ++it)
  {
    for (uint32 i = 0; i < it->first; ++i)
      result[it->second + i] = false;
  }
  return result;
}

#endif
