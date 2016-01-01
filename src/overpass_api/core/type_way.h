/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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
*
* Modified varint en-/decoding helper functions ZigZagDecode64,
* ZigZagEncode64, VarintSize64, ReadVarint64Fallback,
* WriteVarint64ToArrayInline part of:
*
* Protocol Buffers - Google's data interchange format
* Copyright 2008 Google Inc.  All rights reserved.
* https://developers.google.com/protocol-buffers/
*
*/

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_WAY_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_WAY_H

#include "basic_types.h"
#include "index_computations.h"
#include "type_node.h"

#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <assert.h>


inline int64 ZigZagDecode64(const uint64 n) {
  return (n >> 1) ^ -static_cast<int64>(n & 1);
}

inline uint64 ZigZagEncode64(const signed long long n) {
  // Note:  the right-shift must be arithmetic
  return (static_cast<uint64>(n) << 1) ^ (n >> 63);
}

inline int VarintSize64(const signed long long value) {
  if (value < (1ull << 35)) {
    if (value < (1ull << 7)) {
      return 1;
    } else if (value < (1ull << 14)) {
      return 2;
    } else if (value < (1ull << 21)) {
      return 3;
    } else if (value < (1ull << 28)) {
      return 4;
    } else {
      return 5;
    }
  } else {
    if (value < (1ull << 42)) {
      return 6;
    } else if (value < (1ull << 49)) {
      return 7;
    } else if (value < (1ull << 56)) {
      return 8;
    } else if (value < (1ull << 63)) {
      return 9;
    } else {
      return 10;
    }
  }
}

inline std::pair<uint64, bool> ReadVarint64Fallback(uint8** buffer_) {

    uint8* ptr = *buffer_;
    uint32 b;

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32 part0 = 0, part1 = 0, part2 = 0;

    b = *(ptr++); part0  = b      ; if (!(b & 0x80)) goto done;
    part0 -= 0x80;
    b = *(ptr++); part0 += b <<  7; if (!(b & 0x80)) goto done;
    part0 -= 0x80 << 7;
    b = *(ptr++); part0 += b << 14; if (!(b & 0x80)) goto done;
    part0 -= 0x80 << 14;
    b = *(ptr++); part0 += b << 21; if (!(b & 0x80)) goto done;
    part0 -= 0x80 << 21;
    b = *(ptr++); part1  = b      ; if (!(b & 0x80)) goto done;
    part1 -= 0x80;
    b = *(ptr++); part1 += b <<  7; if (!(b & 0x80)) goto done;
    part1 -= 0x80 << 7;
    b = *(ptr++); part1 += b << 14; if (!(b & 0x80)) goto done;
    part1 -= 0x80 << 14;
    b = *(ptr++); part1 += b << 21; if (!(b & 0x80)) goto done;
    part1 -= 0x80 << 21;
    b = *(ptr++); part2  = b      ; if (!(b & 0x80)) goto done;
    part2 -= 0x80;
    b = *(ptr++); part2 += b <<  7; if (!(b & 0x80)) goto done;
    // "part2 -= 0x80 << 7" is irrelevant because (0x80 << 7) << 56 is 0.

    // We have overrun the maximum size of a varint (10 bytes).  The data
    // must be corrupt.
    return std::make_pair(0, false);

   done:
    *buffer_ = ptr;
    return std::make_pair((static_cast<uint64>(part0)) |
                              (static_cast<uint64>(part1) << 28) |
                              (static_cast<uint64>(part2) << 56),
                          true);
}

inline uint8* WriteVarint64ToArrayInline(
    uint64 value, uint8* target) {
  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  uint32 part0 = static_cast<uint32>(value      );
  uint32 part1 = static_cast<uint32>(value >> 28);
  uint32 part2 = static_cast<uint32>(value >> 56);

  int size;

  // Here we can't really optimize for small numbers, since the value is
  // split into three parts.  Checking for numbers < 128, for instance,
  // would require three comparisons, since you'd have to make sure part1
  // and part2 are zero.  However, if the caller is using 64-bit integers,
  // it is likely that they expect the numbers to often be very large, so
  // we probably don't want to optimize for small numbers anyway.  Thus,
  // we end up with a hardcoded binary search tree...
  if (part2 == 0) {
    if (part1 == 0) {
      if (part0 < (1 << 14)) {
        if (part0 < (1 << 7)) {
          size = 1; goto size1;
        } else {
          size = 2; goto size2;
        }
      } else {
        if (part0 < (1 << 21)) {
          size = 3; goto size3;
        } else {
          size = 4; goto size4;
        }
      }
    } else {
      if (part1 < (1 << 14)) {
        if (part1 < (1 << 7)) {
          size = 5; goto size5;
        } else {
          size = 6; goto size6;
        }
      } else {
        if (part1 < (1 << 21)) {
          size = 7; goto size7;
        } else {
          size = 8; goto size8;
        }
      }
    }
  } else {
    if (part2 < (1 << 7)) {
      size = 9; goto size9;
    } else {
      size = 10; goto size10;
    }
  }

  assert (1 == 0);    // cannot get here

  size10: target[9] = static_cast<uint8>((part2 >>  7) | 0x80);
  size9 : target[8] = static_cast<uint8>((part2      ) | 0x80);
  size8 : target[7] = static_cast<uint8>((part1 >> 21) | 0x80);
  size7 : target[6] = static_cast<uint8>((part1 >> 14) | 0x80);
  size6 : target[5] = static_cast<uint8>((part1 >>  7) | 0x80);
  size5 : target[4] = static_cast<uint8>((part1      ) | 0x80);
  size4 : target[3] = static_cast<uint8>((part0 >> 21) | 0x80);
  size3 : target[2] = static_cast<uint8>((part0 >> 14) | 0x80);
  size2 : target[1] = static_cast<uint8>((part0 >>  7) | 0x80);
  size1 : target[0] = static_cast<uint8>((part0      ) | 0x80);

  target[size-1] &= 0x7F;
  return target + size;
}


struct Way
{
  typedef Uint32_Index Id_Type;
  
  Id_Type id;
  uint32 index;
  std::vector< Node::Id_Type > nds;
//   std::vector< Uint31_Index > segment_idxs;
  std::vector< Quad_Coord > geometry;
  std::vector< std::pair< std::string, std::string > > tags;
  
  Way() : id(0u), index(0) {}
  
  Way(uint32 id_)
  : id(id_), index(0)
  {}
  
  Way(uint32 id_, uint32 index_, const std::vector< Node::Id_Type >& nds_)
  : id(id_), index(index_), nds(nds_) {}
  
  static uint32 calc_index(const std::vector< uint32 >& nd_idxs)
  {
    return ::calc_index(nd_idxs);
  }
  
  static bool indicates_geometry(Uint31_Index index)
  {
    return ((index.val() & 0x80000000) != 0 && ((index.val() & 0x1) == 0));
  }
};


struct Way_Comparator_By_Id {
  bool operator() (const Way* a, const Way* b)
  {
    return (a->id < b->id);
  }
};


struct Way_Equal_Id {
  bool operator() (const Way* a, const Way* b)
  {
    return (a->id == b->id);
  }
};


struct Way_Delta;


struct Way_Skeleton
{
  typedef Way::Id_Type Id_Type;
  typedef Way_Delta Delta;

  Id_Type id;
  std::vector< Node::Id_Type > nds;
  std::vector< Quad_Coord > geometry;
  
  Way_Skeleton() : id(0u) {}
  
  Way_Skeleton(Way::Id_Type id_) : id(id_) {}
  
  Way_Skeleton(void* data) : id(*(uint32*)data)
  {
    nds.reserve(*((uint16*)data + 2));
    uint16* start_ptr = (uint16*) decompress_nds(nds, *((uint16*)data + 2), ((uint8*)data + 10));

    geometry.reserve(*((uint16*)data + 3));
    for (int i(0); i < *((uint16*)data + 3); ++i)
      geometry.push_back(Quad_Coord(*(uint32*)(start_ptr + 4*i), *(uint32*)(start_ptr + 4*i + 2)));

  }
  
  Way_Skeleton(const Way& way)
      : id(way.id), nds(way.nds), geometry(way.geometry) {}
  
  Way_Skeleton(uint32 id_, const std::vector< Node::Id_Type >& nds_, const std::vector< Quad_Coord >& geometry_)
      : id(id_), nds(nds_), geometry(geometry_) {}
  
  uint32 size_of() const
  {
    uint32 compress_size = calculate_nds_compressed_size(nds);
    return 8 + 2 + compress_size + 8*geometry.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (8 + 2 +
            8 * *((uint16*)data + 3) +      // geometry size elements, 8 byte per element
            *((uint16*)data + 4));          // nds_compressed_size (in bytes)
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id.val();
    *((uint16*)data + 2) = nds.size();
    *((uint16*)data + 3) = geometry.size();

    uint16* start_ptr = (uint16*) compress_nds(nds, (uint8*)data + 10);
    uint16 nds_compressed_size = (uint16) ((uint8*)start_ptr - ((uint8*)data + 10));
    *((uint16*)data + 4) = nds_compressed_size;

    for (uint i(0); i < geometry.size(); ++i)
    {
      *(uint32*)(start_ptr + 4*i) = geometry[i].ll_upper;
      *(uint32*)(start_ptr + 4*i + 2) = geometry[i].ll_lower;
    }
  }
  
  uint8* compress_nds(const std::vector< Node::Id_Type >& nds_, uint8* buffer_) const
  {
    uint8* current = buffer_;
    Node::Id_Type prev = (uint64) 0;

    for (std::vector< Node_Skeleton::Id_Type>::const_iterator it = nds_.begin();
         it != nds_.end(); ++it)
    {
      signed long long delta = (signed long long) it->val() - (signed long long) prev.val();
      uint64 zigzag = ZigZagEncode64(delta);
      current = WriteVarint64ToArrayInline(zigzag, current);
      prev = it->val();
    }

    if ((current - buffer_) & 1)    // add padding byte
      *current++ = 0;

    return current;
  }

  uint8* decompress_nds(std::vector< Node::Id_Type >& nds_, const uint16 nodes_count, uint8* buffer_)
  {
    uint8* current = buffer_;
    Node::Id_Type nodeid = (uint64) 0;

    for (int i=0; i<nodes_count;i++)
    {
      std::pair<uint64, bool> res = ReadVarint64Fallback(&current);
      assert(res.second == true);
      signed long long delta = ZigZagDecode64(res.first);
      nodeid += delta;
      nds_.push_back(nodeid);
    }
    if ((current - buffer_) & 1)    // add padding byte
      current++;
    return current;
  }

  uint32 calculate_nds_compressed_size(const std::vector< Node::Id_Type >& nds_) const
  {
    Node::Id_Type prev = (uint64) 0;
    uint32 compressed_size = 0;

    for (std::vector< Node_Skeleton::Id_Type>::const_iterator it = nds_.begin();
        it != nds_.end(); ++it)
    {
      signed long long diff = (signed long long) it->val() - (signed long long) prev.val();
      compressed_size += VarintSize64(ZigZagEncode64(diff));
      prev = it->val();
    }
    compressed_size += compressed_size & 1;
    return compressed_size;
  }

  bool operator<(const Way_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Way_Skeleton& a) const
  {
    return this->id == a.id;
  }
};


struct Way_Delta
{
  typedef Way_Skeleton::Id_Type Id_Type;
  
  Id_Type id;
  bool full;
  std::vector< uint > nds_removed;
  std::vector< std::pair< uint, Node::Id_Type > > nds_added;
  std::vector< uint > geometry_removed;
  std::vector< std::pair< uint, Quad_Coord > > geometry_added;
  
  Way_Delta() : id(0u), full(false) {}
  
  Way_Delta(void* data) : id(*(uint32*)data), full(false)
  {
    if (*((uint32*)data + 1) == 0xffffffff)
    {
      full = true;
      nds_removed.clear();
      nds_added.resize(*((uint32*)data + 2));
      geometry_removed.clear();
      geometry_added.resize(*((uint32*)data + 3), std::make_pair(0, Quad_Coord()));
      
      uint8* ptr = ((uint8*)data) + 16;
      
      for (uint i(0); i < nds_added.size(); ++i)
      {
        nds_added[i].first = i;
        nds_added[i].second = *(uint64*)((uint32*)ptr);
        ptr += 8;
      }
      
      for (uint i = 0; i < geometry_added.size(); ++i)
      {
        geometry_added[i].first = i;
        geometry_added[i].second = *((Quad_Coord*)ptr);
        ptr += 8;
      }
    }
    else
    {
      nds_removed.resize(*((uint32*)data + 1));
      nds_added.resize(*((uint32*)data + 2));
      geometry_removed.resize(*((uint32*)data + 3));
      geometry_added.resize(*((uint32*)data + 4), std::make_pair(0, Quad_Coord()));
      
      uint8* ptr = ((uint8*)data) + 20;
      
      for (uint i(0); i < nds_removed.size(); ++i)
      {
        nds_removed[i] = *((uint32*)ptr);
        ptr += 4;
      }
      
      for (uint i(0); i < nds_added.size(); ++i)
      {
        nds_added[i].first = *((uint32*)ptr);
        nds_added[i].second = *(uint64*)((uint32*)(ptr + 4));
        ptr += 12;
      }
      
      for (uint i = 0; i < geometry_removed.size(); ++i)
      {
        geometry_removed[i] = *((uint32*)ptr);
        ptr += 4;
      }
      
      for (uint i = 0; i < geometry_added.size(); ++i)
      {
        geometry_added[i].first = *((uint32*)ptr);
        geometry_added[i].second = *((Quad_Coord*)(ptr + 4));
        ptr += 12;
      }
    }
  }
  
  Way_Delta(const Way_Skeleton& reference, const Way_Skeleton& skel)
    : id(skel.id), full(false)
  {
    if (!(id == skel.id))
      full = true;
    else
    {  
      make_delta(skel.nds, reference.nds, nds_removed, nds_added);
      make_delta(skel.geometry, reference.geometry, geometry_removed, geometry_added);
    }
    
    if (nds_added.size() >= skel.nds.size()/2)
    {
      nds_removed.clear();
      nds_added.clear();
      geometry_removed.clear();
      geometry_added.clear();
      full = true;
    }
    
    if (full)
    {
      copy_elems(skel.nds, nds_added);
      copy_elems(skel.geometry, geometry_added);
    }
  }
  
  Way_Skeleton expand(const Way_Skeleton& reference) const
  {
    Way_Skeleton result(id);

    if (full)
    {
      result.nds.reserve(nds_added.size());
      for (uint i = 0; i < nds_added.size(); ++i)
        result.nds.push_back(nds_added[i].second);
      
      result.geometry.reserve(geometry_added.size());
      for (uint i = 0; i < geometry_added.size(); ++i)
        result.geometry.push_back(geometry_added[i].second);
    }
    else if (reference.id == id)
    {
      expand_diff(reference.nds, nds_removed, nds_added, result.nds);
      expand_diff(reference.geometry, geometry_removed, geometry_added, result.geometry);
      if (!result.geometry.empty() && result.nds.size() != result.geometry.size())
      {
	std::ostringstream out;
	out<<"Bad geometry for way "<<id.val();
	throw std::logic_error(out.str());
      }
    }
    else
      result.id = 0u;
    
    return result;
  }
  
  uint32 size_of() const
  {
    if (full)
      return 16 + 8*nds_added.size() + 8*geometry_added.size();
    else
      return 20 + 4*nds_removed.size() + 12*nds_added.size()
          + 4*geometry_removed.size() + 12*geometry_added.size();
  }
  
  static uint32 size_of(void* data)
  {
    if (*((uint32*)data + 1) == 0xffffffff)
      return 16 + 8 * *((uint32*)data + 2) + 8 * *((uint32*)data + 3);
    else
      return 20 + 4 * *((uint32*)data + 1) + 12 * *((uint32*)data + 2)
          + 4 * *((uint32*)data + 3) + 12 * *((uint32*)data + 4);
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id.val();
    if (full)
    {
      *((uint32*)data + 1) = 0xffffffff;
      *((uint32*)data + 2) = nds_added.size();
      *((uint32*)data + 3) = geometry_added.size();
      
      uint8* ptr = ((uint8*)data) + 16;
      
      for (uint i = 0; i < nds_added.size(); ++i)
      {
        *(uint64*)((uint32*)ptr) = nds_added[i].second.val();
        ptr += 8;
      }
      
      for (uint i = 0; i < geometry_added.size(); ++i)
      {
        *((Quad_Coord*)ptr) = geometry_added[i].second;
        ptr += 8;
      }
    }
    else
    {
      *((uint32*)data + 1) = nds_removed.size();
      *((uint32*)data + 2) = nds_added.size();
      *((uint32*)data + 3) = geometry_removed.size();
      *((uint32*)data + 4) = geometry_added.size();
      
      uint8* ptr = ((uint8*)data) + 20;
      
      for (uint i = 0; i < nds_removed.size(); ++i)
      {
        *((uint32*)ptr) = nds_removed[i];
        ptr += 4;
      }
      
      for (uint i = 0; i < nds_added.size(); ++i)
      {
        *((uint32*)ptr) = nds_added[i].first;
        *(uint64*)((uint32*)(ptr + 4)) = nds_added[i].second.val();
        ptr += 12;
      }
      
      for (uint i = 0; i < geometry_removed.size(); ++i)
      {
        *((uint32*)ptr) = geometry_removed[i];
        ptr += 4;
      }
      
      for (uint i = 0; i < geometry_added.size(); ++i)
      {
        *((uint32*)ptr) = geometry_added[i].first;
        *((Quad_Coord*)(ptr + 4)) = geometry_added[i].second;
        ptr += 12;
      }
    }
  }
  
  bool operator<(const Way_Delta& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Way_Delta& a) const
  {
    return this->id == a.id;
  }
};


inline std::vector< Uint31_Index > calc_segment_idxs(const std::vector< uint32 >& nd_idxs)
{
  std::vector< Uint31_Index > result;
  std::vector< uint32 > segment_nd_idxs(2, 0);
  for (std::vector< uint32 >::size_type i = 1; i < nd_idxs.size(); ++i)
  {
    segment_nd_idxs[0] = nd_idxs[i-1];
    segment_nd_idxs[1] = nd_idxs[i];
    Uint31_Index segment_index = Way::calc_index(segment_nd_idxs);
    if ((segment_index.val() & 0x80000000) != 0)
      result.push_back(segment_index);
  }
  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());
  
  return result;
}


#endif
