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

#ifndef DE__OSM3S___OVERPASS_API__CORE__DATATYPES_H
#define DE__OSM3S___OVERPASS_API__CORE__DATATYPES_H

#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "basic_types.h"
#include "geometry.h"
#include "type_node.h"
#include "type_way.h"
#include "type_relation.h"
#include "type_tags.h"
#include "type_area.h"


struct String_Object
{
  typedef uint32 Id_Type;

  String_Object(std::string s) : value(s) {}

  String_Object(void* data) : value()
  {
    value = std::string(((int8*)data + 2), *(uint16*)data);
  }

  uint32 size_of() const
  {
    return value.length() + 2;
  }

  static uint32 size_of(void* data)
  {
    return *(uint16*)data + 2;
  }

  void to_data(void* data) const
  {
    *(uint16*)data = value.length();
    memcpy(((uint8*)data + 2), value.data(), value.length());
  }

  bool operator<(const String_Object& index) const
  {
    return this->value < index.value;
  }

  bool operator==(const String_Object& index) const
  {
    return this->value == index.value;
  }

  std::string val() const
  {
    return value;
  }

  protected:
    std::string value;
};


template< typename First, typename Second >
struct Pair_Comparator_By_Id {
  bool operator() (const std::pair< First, Second >& a, const std::pair< First, Second >& b)
  {
    return (a.first < b.first);
  }
};


template< typename First, typename Second >
struct Pair_Equal_Id {
  bool operator() (const std::pair< First, Second >& a, const std::pair< First, Second >& b)
  {
    return (a.first == b.first);
  }
};


template < class T >
const T* binary_search_for_id(const std::vector< T >& vect, typename T::Id_Type id)
{
  uint32 lower(0);
  uint32 upper(vect.size());

  while (upper > lower)
  {
    uint32 pos((upper + lower)/2);
    if (id < vect[pos].id)
      upper = pos;
    else if (vect[pos].id == id)
      return &(vect[pos]);
    else
      lower = pos + 1;
  }
  return 0;
}


template < class TObject >
TObject* binary_ptr_search_for_id(const std::vector< TObject* >& vect, typename TObject::Id_Type id)
{
  uint32 lower(0);
  uint32 upper(vect.size());

  while (upper > lower)
  {
    uint32 pos((upper + lower)/2);
    if (id < vect[pos]->id)
      upper = pos;
    else if (vect[pos]->id == id)
      return vect[pos];
    else
      lower = pos + 1;
  }
  return 0;
}


template < class Id_Type, class TObject >
const TObject* binary_pair_search(const std::vector< std::pair< Id_Type, TObject> >& vect, Id_Type id)
{
  uint32 lower(0);
  uint32 upper(vect.size());

  while (upper > lower)
  {
    uint32 pos((upper + lower)/2);
    if (id < vect[pos].first)
      upper = pos;
    else if (vect[pos].first == id)
      return &vect[pos].second;
    else
      lower = pos + 1;
  }
  return 0;
}


template< typename T >
struct Array
{
  Array(unsigned int size) : ptr(0), size_(size)
  {
    if (size > 0)
      ptr = new T[size];
  }
  ~Array() { delete[] ptr; }

  const T& operator[](unsigned int i) const { return ptr[i]; }
  T& operator[](unsigned int i) { return ptr[i]; }
  unsigned int size() const { return size_; }

private:
  T* ptr;
  unsigned int size_;
};


template< typename Object >
struct Owner
{
  Owner(Object* ptr_) : ptr(ptr_) {}
  ~Owner() { delete ptr; }

  operator bool() const { return ptr; }
  Object& operator*() const { return *ptr; }
  Object* operator->() const { return ptr; }

private:
  Owner(const Owner&);
  Owner& operator=(const Owner&);

  Object* ptr;
};


template< typename Object >
struct Clonable_Owner
{
  Clonable_Owner(Object* ptr_) : ptr(ptr_) {}
  Clonable_Owner(const Clonable_Owner& rhs) : ptr(rhs.ptr ? rhs.ptr->clone() : 0) {}
  Clonable_Owner& operator=(const Clonable_Owner& rhs)
  {
    if (this != &rhs)
    {
      delete ptr;
      ptr = rhs.ptr ? rhs.ptr->clone() : 0;
    }
    return *this;
  }
  ~Clonable_Owner() { delete ptr; }

  operator bool() const { return ptr; }
  Object& operator*() const { return *ptr; }
  void acquire(Object* ptr_)
  {
    delete ptr;
    ptr = ptr_;
  }

private:

  Object* ptr;
};


template< typename Pointer >
struct Owning_Array
{
  Owning_Array() {}
  ~Owning_Array()
  {
    for (typename std::vector< Pointer >::iterator it = content.begin(); it != content.end(); ++it)
      delete *it;
  }

  const Pointer& operator[](uint i) const { return content[i]; }
  void push_back(Pointer ptr) { content.push_back(ptr); }
  uint size() const { return content.size(); }

private:
  Owning_Array(const Owning_Array&);
  Owning_Array& operator=(const Owning_Array&);

  std::vector< Pointer > content;
};


struct Derived_Skeleton
{
  typedef Uint64 Id_Type;

  Derived_Skeleton(const std::string& type_name_, Id_Type id_) : type_name(type_name_), id(id_) {}

  std::string type_name;
  Id_Type id;
};


struct Derived_Structure : public Derived_Skeleton
{
  Derived_Structure(const std::string& type_name_, Id_Type id_)
      : Derived_Skeleton(type_name_, id_), geometry(0) {}
  Derived_Structure(const std::string& type_name_, Id_Type id_,
      const std::vector< std::pair< std::string, std::string > >& tags_, Opaque_Geometry* geometry_)
      : Derived_Skeleton(type_name_, id_), tags(tags_), geometry(geometry_) {}

  std::vector< std::pair< std::string, std::string > > tags;

  const Opaque_Geometry* get_geometry() const { return &*geometry; }
  const void acquire_geometry(Opaque_Geometry* geometry_)
  {
    geometry.acquire(geometry_);
  }

  bool operator<(const Derived_Structure& a) const
  {
    return this->id.val() < a.id.val();
  }

  bool operator==(const Derived_Structure& a) const
  {
    return this->id.val() == a.id.val();
  }

private:
  Clonable_Owner< Opaque_Geometry > geometry;
};


/**
  * A dataset that is referred in the scripts by a variable.
  */
struct Set
{
  std::map< Uint32_Index, std::vector< Node_Skeleton > > nodes;
  std::map< Uint31_Index, std::vector< Way_Skeleton > > ways;
  std::map< Uint31_Index, std::vector< Relation_Skeleton > > relations;

  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > attic_nodes;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_ways;
  std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_relations;

  std::map< Uint31_Index, std::vector< Area_Skeleton > > areas;
  std::map< Uint31_Index, std::vector< Derived_Structure > > deriveds;

  void swap(Set& rhs)
  {
    nodes.swap(rhs.nodes);
    ways.swap(rhs.ways);
    relations.swap(rhs.relations);
    attic_nodes.swap(rhs.attic_nodes);
    attic_ways.swap(rhs.attic_ways);
    attic_relations.swap(rhs.attic_relations);
    areas.swap(rhs.areas);
    deriveds.swap(rhs.deriveds);
  }

  void clear()
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    attic_nodes.clear();
    attic_ways.clear();
    attic_relations.clear();
    areas.clear();
    deriveds.clear();
  }
};


struct Error_Output
{
  virtual void add_encoding_error(const std::string& error) = 0;
  virtual void add_parse_error(const std::string& error, int line_number) = 0;
  virtual void add_static_error(const std::string& error, int line_number) = 0;
  // void add_sanity_error(const std::string& error);

  virtual void add_encoding_remark(const std::string& error) = 0;
  virtual void add_parse_remark(const std::string& error, int line_number) = 0;
  virtual void add_static_remark(const std::string& error, int line_number) = 0;
  // void add_sanity_remark(const std::string& error);

  virtual void runtime_error(const std::string& error) = 0;
  virtual void runtime_remark(const std::string& error) = 0;

  virtual void display_statement_progress
    (uint timer, const std::string& name, int progress, int line_number,
     const std::vector< std::pair< uint, uint > >& stack) = 0;

  virtual bool display_encoding_errors() = 0;
  virtual bool display_parse_errors() = 0;
  virtual bool display_static_errors() = 0;

  static const uint QUIET = 1;
  static const uint CONCISE = 2;
  static const uint PROGRESS = 3;
  static const uint ASSISTING = 4;
  static const uint VERBOSE = 5;
};


class Area_Usage_Listener
{
  public:
    virtual ~Area_Usage_Listener() {}
    virtual void flush() = 0;
};


class Osm_Backend_Callback
{
  public:
    virtual void update_started() = 0;
    virtual void compute_indexes_finished() = 0;
    virtual void update_ids_finished() = 0;
    virtual void update_coords_finished() = 0;
    virtual void prepare_delete_tags_finished() = 0;
    virtual void tags_local_finished() = 0;
    virtual void tags_global_finished() = 0;
    virtual void flush_roles_finished() = 0;
    virtual void update_finished() = 0;
    virtual void partial_started() = 0;
    virtual void partial_finished() = 0;

    virtual void parser_started() = 0;
    virtual void node_elapsed(Node::Id_Type id) = 0;
    virtual void nodes_finished() = 0;
    virtual void way_elapsed(Way::Id_Type id) = 0;
    virtual void ways_finished() = 0;
    virtual void relation_elapsed(Relation::Id_Type id) = 0;
    virtual void relations_finished() = 0;
    virtual void parser_succeeded() = 0;
};


struct User_Data
{
  typedef uint32 Id_Type;

  Id_Type id;
  std::string name;

  User_Data() : id(0) {}

  User_Data(void* data)
  {
    id = *(uint32*)data;
    name = std::string(((int8*)data + 6), *(uint16*)((int8*)data + 4));
  }

  uint32 size_of() const
  {
    return 6 + name.length();
  }

  static uint32 size_of(void* data)
  {
    return 6 + *(uint16*)((int8*)data + 4);
  }

  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *(uint16*)((int8*)data + 4) = name.length();
    memcpy(((int8*)data + 6), name.data(), name.length());
  }

  bool operator<(const User_Data& a) const
  {
    return (id < a.id);
  }

  bool operator==(const User_Data& a) const
  {
    return (id == a.id);
  }
};


struct OSM_Element_Metadata
{
  OSM_Element_Metadata() : user_id(0) {}

  uint32 version;
  uint64 timestamp;
  uint32 changeset;
  uint32 user_id;
  std::string user_name;

  bool operator<(const OSM_Element_Metadata&) const { return false; }
};


template< typename Id_Type_ >
struct OSM_Element_Metadata_Skeleton
{
  typedef Id_Type_ Id_Type;

  Id_Type ref;
  uint32 version;
  uint64 timestamp;
  uint32 changeset;
  uint32 user_id;

  OSM_Element_Metadata_Skeleton() : version(0), timestamp(0), changeset(0), user_id(0) {}

  OSM_Element_Metadata_Skeleton(Id_Type ref_)
    : ref(ref_), version(0), timestamp(0), changeset(0), user_id(0) {}

  OSM_Element_Metadata_Skeleton(Id_Type ref_, const OSM_Element_Metadata& meta)
    : ref(ref_),
      version(meta.version), timestamp(meta.timestamp),
      changeset(meta.changeset), user_id(meta.user_id) {}

  OSM_Element_Metadata_Skeleton(Id_Type ref_, uint64 timestamp_)
    : ref(ref_), version(0), timestamp(timestamp_),
      changeset(0), user_id(0) {}

  OSM_Element_Metadata_Skeleton(void* data)
    : ref(*(Id_Type*)data)
  {
    version = *(uint32*)((int8*)data + sizeof(Id_Type));
    timestamp = (*(uint64*)((int8*)data + sizeof(Id_Type) + 4) & 0xffffffffffull);
    changeset = *(uint32*)((int8*)data + sizeof(Id_Type) + 9);
    user_id = *(uint32*)((int8*)data + sizeof(Id_Type) + 13);
  }

  uint32 size_of() const
  {
    return 17 + sizeof(Id_Type);
  }

  static uint32 size_of(void* data)
  {
    return 17 + sizeof(Id_Type);
  }

  void to_data(void* data) const
  {
    *(Id_Type*)data = ref;
    *(uint32*)((int8*)data + sizeof(Id_Type)) = version;
    *(uint64*)((int8*)data + sizeof(Id_Type) + 4) = timestamp;
    *(uint32*)((int8*)data + sizeof(Id_Type) + 9) = changeset;
    *(uint32*)((int8*)data + sizeof(Id_Type) + 13) = user_id;
  }

  bool operator<(const OSM_Element_Metadata_Skeleton& a) const
  {
    if (ref < a.ref)
      return true;
    else if (a.ref < ref)
      return false;
    return (timestamp < a.timestamp);
  }

  bool operator==(const OSM_Element_Metadata_Skeleton& a) const
  {
    return (ref == a.ref);
  }
};


template< class TIndex, class TObject >
const std::pair< TIndex, const TObject* >* binary_search_for_pair_id
    (const std::vector< std::pair< TIndex, const TObject* > >& vect, typename TObject::Id_Type id)
{
  uint32 lower(0);
  uint32 upper(vect.size());

  while (upper > lower)
  {
    uint32 pos((upper + lower)/2);
    if (id < vect[pos].second->id)
      upper = pos;
    else if (vect[pos].second->id == id)
      return &(vect[pos]);
    else
      lower = pos + 1;
  }
  return 0;
}


template< typename Id_Type_ >
struct Change_Entry
{
  typedef Id_Type_ Id_Type;

  Change_Entry(const Id_Type& elem_id_, const Uint31_Index& old_idx_, const Uint31_Index& new_idx_)
      : old_idx(old_idx_), new_idx(new_idx_), elem_id(elem_id_) {}

  Uint31_Index old_idx;
  Uint31_Index new_idx;
  Id_Type elem_id;

  Change_Entry(void* data)
    : old_idx((uint8*)data), new_idx((uint8*)data + 4), elem_id(Id_Type((uint8*)data + 8)) {}

  uint32 size_of() const
  {
    return elem_id.size_of() + 8;
  }

  static uint32 size_of(void* data)
  {
    return Id_Type::size_of((uint8*)data + 8) + 8;
  }

  void to_data(void* data) const
  {
    old_idx.to_data((uint8*)data);
    new_idx.to_data((uint8*)data + 4);
    elem_id.to_data((uint8*)data + 8);
  }

  bool operator<(const Change_Entry& rhs) const
  {
    if (old_idx < rhs.old_idx)
      return true;
    if (rhs.old_idx < old_idx)
      return true;
    if (new_idx < rhs.new_idx)
      return true;
    if (rhs.new_idx < new_idx)
      return true;
    return (elem_id < rhs.elem_id);
  }

  bool operator==(const Change_Entry& rhs) const
  {
    return (old_idx == rhs.old_idx && new_idx == rhs.new_idx && elem_id == rhs.elem_id);
  }
};


struct Timestamp
{
  Timestamp(uint64 timestamp_) : timestamp(timestamp_) {}

  uint64 timestamp;

  Timestamp(void* data)
    : timestamp((*(uint64*)(uint8*)data) & 0xffffffffffull) {}

  Timestamp(int year, int month, int day, int hour, int minute, int second)
    : timestamp(0)
  {
    timestamp |= (uint64(year & 0x3fff)<<26); //year
    timestamp |= ((month & 0xf)<<22); //month
    timestamp |= ((day & 0x1f)<<17); //day
    timestamp |= ((hour & 0x1f)<<12); //hour
    timestamp |= ((minute & 0x3f)<<6); //minute
    timestamp |= (second & 0x3f); //second
  }

  Timestamp(const std::string& input) : timestamp(0)
  {
    if (input.size() < 19
        || !isdigit(input[0]) || !isdigit(input[1])
        || !isdigit(input[2]) || !isdigit(input[3])
        || !isdigit(input[5]) || !isdigit(input[6])
        || !isdigit(input[8]) || !isdigit(input[9])
        || !isdigit(input[11]) || !isdigit(input[12])
        || !isdigit(input[14]) || !isdigit(input[15])
        || !isdigit(input[17]) || !isdigit(input[18]))
      return;

    timestamp |= (uint64(four_digits(&input[0]) & 0x3fff)<<26); //year
    timestamp |= ((two_digits(&input[5]) & 0xf)<<22); //month
    timestamp |= ((two_digits(&input[8]) & 0x1f)<<17); //day
    timestamp |= ((two_digits(&input[11]) & 0x1f)<<12); //hour
    timestamp |= ((two_digits(&input[14]) & 0x3f)<<6); //minute
    timestamp |= (two_digits(&input[17]) & 0x3f); //second
  }

  static int two_digits(const char* input) { return (input[0] - '0')*10 + (input[1] - '0'); }
  static int four_digits(const char* input)
  { return (input[0] - '0')*1000 + (input[1] - '0')*100 + (input[2] - '0')*10 + (input[3] - '0'); }

  static int year(uint64 timestamp) { return ((timestamp>>26) & 0x3fff); }
  static int month(uint64 timestamp) { return ((timestamp>>22) & 0xf); }
  static int day(uint64 timestamp) { return ((timestamp>>17) & 0x1f); }
  static int hour(uint64 timestamp) { return ((timestamp>>12) & 0x1f); }
  static int minute(uint64 timestamp) { return ((timestamp>>6) & 0x3f); }
  static int second(uint64 timestamp) { return (timestamp & 0x3f); }

  int year() const { return year(timestamp); }
  int month() const { return month(timestamp); }
  int day() const { return day(timestamp); }
  int hour() const { return hour(timestamp); }
  int minute() const { return minute(timestamp); }
  int second() const { return second(timestamp); }

  std::string str() const
  {
    if (timestamp == std::numeric_limits< unsigned long long >::max())
      return "NOW";

    std::ostringstream out;
    out<<std::setw(4)<<std::setfill('0')<<year()<<"-"
        <<std::setw(2)<<std::setfill('0')<<month()<<"-"
	<<std::setw(2)<<std::setfill('0')<<day()<<"T"
	<<std::setw(2)<<std::setfill('0')<<hour()<<":"
	<<std::setw(2)<<std::setfill('0')<<minute()<<":"
	<<std::setw(2)<<std::setfill('0')<<second()<<"Z";
    return out.str();
  }

  uint32 size_of() const
  {
    return 5;
  }

  static uint32 size_of(void* data)
  {
    return 5;
  }

  void to_data(void* data) const
  {
    void* pos = (uint8*)data;
    *(uint32*)(pos) = (timestamp & 0xffffffffull);
    *(uint8*)((uint8*)pos+4) = ((timestamp & 0xff00000000ull)>>32);
  }

  bool operator<(const Timestamp& rhs) const
  {
    return (timestamp < rhs.timestamp);
  }

  bool operator==(const Timestamp& rhs) const
  {
    return (timestamp == rhs.timestamp);
  }

  static uint32 max_size_of()
  {
    throw Unsupported_Error("static uint32 Timestamp::max_size_of()");
    return 0;
  }
};


typedef enum { only_data, keep_meta, keep_attic } meta_modes;


template< typename Object >
std::string name_of_type() { return "[undefined]"; }

template< > inline std::string name_of_type< Node_Skeleton >() { return "Node"; }
template< > inline std::string name_of_type< Way_Skeleton >() { return "Way"; }
template< > inline std::string name_of_type< Relation_Skeleton >() { return "Relation"; }
template< > inline std::string name_of_type< Area_Skeleton >() { return "Area"; }


#endif
