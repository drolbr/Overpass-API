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
*/

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

typedef unsigned int uint;
typedef unsigned long long uint64;

template < typename T >
struct V : public vector< T >
{
  V(const T& t) : vector< T >(1, t) {}
  V& operator()(const T& t)
  {
    this->push_back(t);
    return *this;
  }
};

struct Data_Modifier
{
  virtual bool admit_node(uint id) const = 0;
  virtual bool admit_node_skeleton(uint id) const = 0;
  virtual bool admit_node_tags(uint id) const = 0;
  virtual bool admit_way(uint id) const = 0;
  virtual bool admit_way_skeleton(uint id) const = 0;
  virtual bool admit_way_tags(uint id) const = 0;
  virtual bool admit_relation(uint id) const = 0;
  virtual bool admit_relation_skeleton(uint id) const = 0;
  virtual bool admit_relation_tags(uint id) const = 0;
};

struct Accept_All_Tags : public Data_Modifier
{
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
};

struct Accept_All : public Accept_All_Tags
{
  virtual bool admit_node(uint id) const { return true; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return true; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return true; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
};

struct Accept_Print_1 : public Accept_All_Tags
{
  virtual bool admit_node(uint id) const { return (id % 10000 == 0); }
  virtual bool admit_way(uint id) const { return (id % 1000 == 0); }
  virtual bool admit_relation(uint id) const { return (id % 4 == 0); }
};

struct Accept_Print_2 : public Data_Modifier
{
  virtual bool admit_node(uint id) const { return (id % 10000 == 0); }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 1000 == 0); }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 4 == 0); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return false; }
};

struct Accept_Print_3 : public Data_Modifier
{
  virtual bool admit_node(uint id) const { return (id % 10000 == 0); }
  virtual bool admit_node_skeleton(uint id) const { return false; }
  virtual bool admit_node_tags(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 1000 == 0); }
  virtual bool admit_way_skeleton(uint id) const { return false; }
  virtual bool admit_way_tags(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 4 == 0); }
  virtual bool admit_relation_skeleton(uint id) const { return false; }
  virtual bool admit_relation_tags(uint id) const { return false; }
};

struct Accept_Recurse_1 : public Accept_All_Tags
{
  Accept_Recurse_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return ((id > pattern_size) && (id <= pattern_size*3/2+1)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_2 : public Accept_All_Tags
{
  Accept_Recurse_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return ((id % pattern_size == pattern_size/2+1)
          && (id < 2*pattern_size*pattern_size)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_3 : public Accept_All_Tags
{
  Accept_Recurse_3(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_4 : public Accept_All_Tags
{
  Accept_Recurse_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
      { return (id == pattern_size*pattern_size - pattern_size/2); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_5 : public Accept_All_Tags
{
  Accept_Recurse_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
      { return (id <= pattern_size*(pattern_size/2-1)); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_6 : public Accept_All_Tags
{
  Accept_Recurse_6(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return ((id == 1) || (id == 2)
          || (id == pattern_size+1) || (id == pattern_size+2)
	  || (id == pattern_size*pattern_size)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_7 : public Accept_All_Tags
{
  Accept_Recurse_7(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
      { return ((id == 2) || (id == 8) || (id == 10) || (id == 11)); }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_8 : public Accept_All_Tags
{
  Accept_Recurse_8(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
      { return ((id == 1) || (id == 2)
          || (id == pattern_size/2*(pattern_size/2-1) + 1)
	  || (id == pattern_size/2*(pattern_size/2+1) + 1)); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_9 : public Accept_All_Tags
{
  Accept_Recurse_9(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
      { return ((id == 6) || (id == 8) || (id == 10)); }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_10 : public Accept_All_Tags
{
  Accept_Recurse_10(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
      { return ((id == 1) || (id == 2)); }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_11 : public Accept_All_Tags
{
  Accept_Recurse_11(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
      { return ((id == 9) || (id == 10)); }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_12 : public Accept_All_Tags
{
  Accept_Recurse_12(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1 || id == pattern_size + 2);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_13 : public Accept_All_Tags
{
  Accept_Recurse_13(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 2
        || id == pattern_size + 1 || id == pattern_size + 2 || id == pattern_size + 3
	|| id == 2*pattern_size + 4 || id == 3*pattern_size + 4);
  }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2
        || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
	|| id == pattern_size*pattern_size/4 + pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_14 : public Accept_All_Tags
{
  Accept_Recurse_14(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1 || id == 2 || id == pattern_size
    || id == pattern_size + 1 || id == pattern_size + 2 || id == pattern_size + 3
    || id == 2*pattern_size + 4 || id == 3*pattern_size + 4
    || id == pattern_size*pattern_size/4
    || id == pattern_size*pattern_size - pattern_size
    || id == pattern_size*pattern_size);
  }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2
    || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
    || id == pattern_size*pattern_size/4 + pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const
  {
    return ((id >= 1 && id <= 6) || id == 9);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_15 : public Accept_All_Tags
{
  Accept_Recurse_15(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1 || id == 2
    || id == pattern_size + 1 || id == pattern_size + 2 || id == pattern_size + 3
    || id == 2*pattern_size + 4 || id == 3*pattern_size + 4);
  }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2
    || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
    || id == pattern_size*pattern_size/4 + pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const
  {
    return (id == 1 || id == 2 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_16 : public Accept_All_Tags
{
  Accept_Recurse_16(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2
        || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
        || id == pattern_size*pattern_size/4);
  }
  virtual bool admit_relation(uint id) const
  {
    return (id == 1 || id == 2 || id == 6 || id == 8 || id == 10 || id == 11);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_17 : public Accept_All_Tags
{
  Accept_Recurse_17(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 6 || id == 8 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_18 : public Accept_All_Tags
{
  Accept_Recurse_18(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == pattern_size*pattern_size/4 - pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const
  {
    return (id == 2 || id == 6 || id == 8 || id == 9 || id == 10 || id == 11);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_19 : public Accept_All_Tags
{
  Accept_Recurse_19(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 6 || id == 8 || id == 9 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_20 : public Accept_All_Tags
{
  Accept_Recurse_20(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 1 || id == 9 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_21 : public Accept_All_Tags
{
  Accept_Recurse_21(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == pattern_size + 1 || id == pattern_size + 2);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_23 : public Accept_All_Tags
{
  Accept_Recurse_23(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_24 : public Accept_All_Tags
{
  Accept_Recurse_24(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  { 
    return (id == 11);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_25 : public Accept_All_Tags
{
  Accept_Recurse_25(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == pattern_size*(pattern_size/2-1) + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_26 : public Accept_All_Tags
{
  Accept_Recurse_26(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 6 || id == 8 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_27 : public Accept_All_Tags
{
  Accept_Recurse_27(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 3);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_28 : public Accept_All_Tags
{
  Accept_Recurse_28(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 9);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_1 : public Accept_All_Tags
{
  Accept_Bbox_Query_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
       && (id <= pattern_size*pattern_size*6/5)
       && (id % pattern_size > 0)
       && (id % pattern_size <= pattern_size/5)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_2 : public Accept_All_Tags
{
  Accept_Bbox_Query_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
       && (id <= pattern_size*pattern_size*11/10)
       && (id % pattern_size > 0)
       && (id % pattern_size <= pattern_size/10)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_3 : public Accept_All_Tags
{
  Accept_Bbox_Query_3(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
  && (id <= pattern_size*pattern_size*11/10)
  && ((id-1) % pattern_size >= pattern_size*9/10)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_4 : public Accept_All_Tags
{
  Accept_Bbox_Query_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
  && (id <= pattern_size*pattern_size*11/10)
  && ((id % pattern_size > pattern_size*9/10)
    || (id % pattern_size <= pattern_size/10))); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_5 : public Accept_All_Tags
{
  Accept_Bbox_Query_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_6 : public Accept_All_Tags
{
  Accept_Bbox_Query_6(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
      && (id <= pattern_size*pattern_size*11/10)
      && (id % pattern_size == 1)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Bbox_Query_8 : public Accept_All_Tags
{
  Accept_Bbox_Query_8(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
      && (id <= pattern_size*pattern_size + pattern_size)
      && ((id-1) % pattern_size < pattern_size/10)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_1 : public Accept_All_Tags
{
  Accept_Query_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id == 11); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_2 : public Accept_All_Tags
{
  Accept_Query_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id % 5 == 0); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_3 : public Accept_All_Tags
{
  Accept_Query_3(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id % 11 == 0); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_4 : public Accept_All_Tags
{
  Accept_Query_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id % 15 == 0); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_5 : public Accept_All_Tags
{
  Accept_Query_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_6 : public Accept_All_Tags
{
  Accept_Query_6(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id == 77); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_7 : public Accept_All_Tags
{
  Accept_Query_7(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id % 105 == 0); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_8 : public Accept_All_Tags
{
  Accept_Query_8(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id == 11); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_9 : public Accept_All_Tags
{
  Accept_Query_9(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
       && (id <= pattern_size*pattern_size*11/10)
       && (id % pattern_size > 0)
       && (id % pattern_size <= pattern_size/10)
       && (id % 5 == 0)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_10 : public Accept_All_Tags
{
  Accept_Query_10(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return ((id > pattern_size*pattern_size)
       && (id <= pattern_size*pattern_size*11/10)
       && (id % pattern_size > 0)
       && (id % pattern_size <= pattern_size/10)
       && (id % 7 == 0)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_11 : public Accept_All_Tags
{
  Accept_Query_11(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { return (id % 105 == 0); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_12 : public Accept_All_Tags
{
  Accept_Query_12(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
      { return (id == 11); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_13 : public Accept_All_Tags
{
  Accept_Query_13(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
      { return (id % 5 == 0); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_14 : public Accept_All_Tags
{
  Accept_Query_14(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 11 == 0); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_15 : public Accept_All_Tags
{
  Accept_Query_15(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 15 == 0); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_17 : public Accept_All_Tags
{
  Accept_Query_17(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 77); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_18 : public Accept_All_Tags
{
  Accept_Query_18(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 105 == 0); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_19 : public Accept_All_Tags
{
  Accept_Query_19(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 105 == 0); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_20 : public Accept_All_Tags
{
  Accept_Query_20(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id == 11); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_21 : public Accept_All_Tags
{
  Accept_Query_21(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 4 == 1); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_22 : public Accept_All_Tags
{
  Accept_Query_22(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 4 != 3); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_23 : public Accept_All_Tags
{
  Accept_Query_23(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 5 == 0); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_25 : public Accept_All_Tags
{
  Accept_Query_25(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 10 == 0); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_28 : public Accept_All_Tags
{
  Accept_Query_28(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    if (id % 5 != 0)
      return false;
    if (id / (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2) != 1)
      return false;
    uint remainder = id % (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2);
    if (remainder <= pattern_size/2*(pattern_size/4-1))
      return false;
    if (remainder <= pattern_size/2*(pattern_size/2-1))
      return true;
    if (remainder <= (pattern_size/2-1)*(pattern_size/4-1) + pattern_size/2*(pattern_size/2-1))
      return false;
    if (remainder <= 2*pattern_size/2*(pattern_size/2-1))
      return true;
    if (remainder > pattern_size/2*(pattern_size/2-1)*2 + pattern_size/2*2-2)
      return true;
    return false;
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_29 : public Accept_All_Tags
{
  Accept_Query_29(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    if (id % 5 != 0)
      return false;
    if (id / (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2) != 1)
      return false;
    uint remainder = id % (2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2);
    if (remainder <= 2*pattern_size/2*(pattern_size/2-1) + pattern_size/4-1)
      return false;
    if (remainder <= 2*pattern_size/2*(pattern_size/2-1) + pattern_size/2-1)
      return true;
    if (remainder <= 2*pattern_size/2*(pattern_size/2-1) + pattern_size/2-1 + pattern_size/4-1)
      return false;
    if (remainder <= pattern_size/2*(pattern_size/2-1)*2 + pattern_size/2*2-2)
      return true;
    return false;
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_30 : public Accept_All_Tags
{
  Accept_Query_30(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    if (id == 18)
      return true;
    return (id == 22 && pattern_size <= 360);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_31 : public Accept_All_Tags
{
  Accept_Query_31(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    if (id == 14)
      return true;
    return (id == 22 && pattern_size <= 200);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_37 : public Accept_All_Tags
{
  Accept_Query_37(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 2*(2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2) - pattern_size + 1
        || id == 2*(2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2) - 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_38 : public Accept_All_Tags
{
  Accept_Query_38(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 2*(2*(pattern_size/2+1)*(pattern_size/2-1) + pattern_size/2));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_39 : public Accept_All_Tags
{
  Accept_Query_39(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  { 
    return ((id >= 12 && id <= 15) || id == 17 || id == 19 || id == 21 || id == 22);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_40 : public Accept_All_Tags
{
  Accept_Query_40(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  { 
    return (id == 18 || id == 22);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_41 : public Accept_All_Tags
{
  Accept_Query_41(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == pattern_size*pattern_size + 1 || id == pattern_size*pattern_size + 2
        || id == pattern_size*pattern_size + pattern_size + 1
	|| id == pattern_size*pattern_size + pattern_size + 2
	|| id == pattern_size*pattern_size + pattern_size + 3
	|| id == pattern_size*pattern_size + 2*pattern_size + 1
	|| id == pattern_size*pattern_size + 2*pattern_size + 2);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_42 : public Accept_All_Tags
{
  Accept_Query_42(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return ((id >= 2*(pattern_size*pattern_size/2 - 2) + 1
        && id <= 2*(pattern_size*pattern_size/2 - 2) + pattern_size/2)
	|| id == 2*(pattern_size*pattern_size/2 - 2 + pattern_size/2) - 1
	|| id == 2*(pattern_size*pattern_size/2 - 2 + pattern_size/2));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_43 : public Accept_All_Tags
{
  Accept_Query_43(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == pattern_size*pattern_size + pattern_size/2*pattern_size/2 - pattern_size
        || id == pattern_size*pattern_size + pattern_size/2*pattern_size/2 - 1
	|| id == pattern_size*pattern_size + pattern_size/2*pattern_size/2
	|| id == pattern_size*pattern_size + pattern_size/2*pattern_size/2 + pattern_size);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_44 : public Accept_All_Tags
{
  Accept_Query_44(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == pattern_size*pattern_size/2 - 2 + pattern_size/2 + 1
        || id == pattern_size*pattern_size/2 - 2 + pattern_size/2 + 2
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2 + 3
	|| id == pattern_size*pattern_size/2 - 2 + 2*pattern_size/2 + 3
	|| id == pattern_size*pattern_size/2 - 2 + 2*pattern_size/2 + 4
	|| id == pattern_size*pattern_size/2 - 2 + 3*pattern_size/2 + 3
	|| id == pattern_size*pattern_size/2 - 2 + 3*pattern_size/2 + 4
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2) + 1
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2) + 2
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2+1)
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2+1) + 1
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2+1) + 2
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2+2) + 1
	|| id == pattern_size*pattern_size/2 - 2 + pattern_size/2*(pattern_size/2+3));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_45 : public Accept_All_Tags
{
  Accept_Query_45(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 21 != 14); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_46 : public Accept_All_Tags
{
  Accept_Query_46(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id < 100 && id % 21 != 0); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_47 : public Accept_All_Tags
{
  Accept_Query_47(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 21 != 14 && id % 21 != 0); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_51 : public Accept_All_Tags
{
  Accept_Query_51(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id > pattern_size*(pattern_size/2) && id <= pattern_size*(pattern_size/2+1));
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_52 : public Accept_All_Tags
{
  Accept_Query_52(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id > pattern_size*(pattern_size/2) && id <= pattern_size*(pattern_size/2+1)
        && id % 5 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_53 : public Accept_All_Tags
{
  Accept_Query_53(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id > (pattern_size+1)*(pattern_size/2) && id <= pattern_size*(pattern_size/2+1));
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_54 : public Accept_All_Tags
{
  Accept_Query_54(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id > (pattern_size+1)*(pattern_size/2) && id <= pattern_size*(pattern_size/2+1)
        && id % 5 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_55 : public Accept_All_Tags
{
  Accept_Query_55(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == pattern_size*(pattern_size/2+1) - 1);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_56 : public Accept_All_Tags
{
  Accept_Query_56(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id <= pattern_size*pattern_size && id <= 32767);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_57 : public Accept_All_Tags
{
  Accept_Query_57(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id <= pattern_size*pattern_size && id <= 32767 && id % 5 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_58 : public Accept_All_Tags
{
  Accept_Query_58(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id > pattern_size*pattern_size/2 && id <= pattern_size*pattern_size
        && (id % pattern_size > pattern_size/2 || id % pattern_size == 0)
        && id <= 32767);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_59 : public Accept_All_Tags
{
  Accept_Query_59(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id > pattern_size*pattern_size/2 && id <= pattern_size*pattern_size
        && (id % pattern_size > pattern_size/2 || id % pattern_size == 0)
        && id <= 32767 && id % 5 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_60 : public Accept_All_Tags
{
  Accept_Query_60(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1 || id == pattern_size + 2);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_61 : public Accept_All_Tags
{
  Accept_Query_61(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2 || id == pattern_size/2*(pattern_size/2-1) + 1
        || id == pattern_size/2*(pattern_size/2-1) + pattern_size + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_62 : public Accept_All_Tags
{
  Accept_Query_62(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return ((id == 1 || id == pattern_size/2*(pattern_size/2-1) + 1
        || id == pattern_size/2*(pattern_size/2-1) + pattern_size + 1) && id % 4 == 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_63 : public Accept_All_Tags
{
  Accept_Query_63(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2 || id == pattern_size/2*(pattern_size/2-1) + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_64 : public Accept_All_Tags
{
  Accept_Query_64(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return ((id == 1 || id == pattern_size/2*(pattern_size/2-1) + 1) && id % 4 == 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_65 : public Accept_All_Tags
{
  Accept_Query_65(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2 || id == pattern_size/2*(pattern_size/2-1) + 1
        || id == pattern_size/2*(pattern_size/2-1) + pattern_size + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_66 : public Accept_All_Tags
{
  Accept_Query_66(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id <= 6);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_67 : public Accept_All_Tags
{
  Accept_Query_67(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id <= 6 && id % 2 == 0);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_68 : public Accept_All_Tags
{
  Accept_Query_68(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 3 || id == 4);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_69 : public Accept_All_Tags
{
  Accept_Query_69(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 4);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_70 : public Accept_All_Tags
{
  Accept_Query_70(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 1 || id == 2);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_71 : public Accept_All_Tags
{
  Accept_Query_71(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id >= (pattern_size+1)*(pattern_size/2-1) - 3
        && id <= (pattern_size+1)*(pattern_size/2-1));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_72 : public Accept_All_Tags
{
  Accept_Query_72(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == (pattern_size+1)*(pattern_size/2-1) - 3
        || id == (pattern_size+1)*(pattern_size/2-1) - 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_73 : public Accept_All_Tags
{
  Accept_Query_73(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == (pattern_size+1)*(pattern_size/2-1) - 1
    || id == (pattern_size+1)*(pattern_size/2-1));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_74 : public Accept_All_Tags
{
  Accept_Query_74(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == (pattern_size+1)*(pattern_size/2-1) - 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_75 : public Accept_All_Tags
{
  Accept_Query_75(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == (pattern_size+1)*(pattern_size/2-1) - 3
        || id == (pattern_size+1)*(pattern_size/2-1) - 2);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_76 : public Accept_All_Tags
{
  Accept_Query_76(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 1 || id == 2 || id == 4 || id == 8 || id == 10 || id == 11);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_77 : public Accept_All_Tags
{
  Accept_Query_77(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 2 || id == 4 || id == 8 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_78 : public Accept_All_Tags
{
  Accept_Query_78(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 4 || (id == 11 && pattern_size*pattern_size < 32768));
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_79 : public Accept_All_Tags
{
  Accept_Query_79(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 4);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_80 : public Accept_All_Tags
{
  Accept_Query_80(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 4 || (id == 11 && pattern_size*pattern_size < 32768));
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_81 : public Accept_All_Tags
{
  Accept_Query_81(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 6 || id == 8 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_82 : public Accept_All_Tags
{
  Accept_Query_82(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_83 : public Accept_All_Tags
{
  Accept_Query_83(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 6 || id == 8 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_84 : public Accept_All_Tags
{
  Accept_Query_84(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_85 : public Accept_All_Tags
{
  Accept_Query_85(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 6 || id == 8 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_86 : public Accept_All_Tags
{
  Accept_Query_86(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 9 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_87 : public Accept_All_Tags
{
  Accept_Query_87(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_88 : public Accept_All_Tags
{
  Accept_Query_88(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_89 : public Accept_All_Tags
{
  Accept_Query_89(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_90 : public Accept_All_Tags
{
  Accept_Query_90(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 9 || id == 10);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_91 : public Accept_All_Tags
{
  Accept_Query_91(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id <= 10); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_92 : public Accept_All_Tags
{
  Accept_Query_92(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id == 5 || id == 10); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_93 : public Accept_All_Tags
{
  Accept_Query_93(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id <= 5); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_94 : public Accept_All_Tags
{
  Accept_Query_94(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id == 5); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_95 : public Accept_All_Tags
{
  Accept_Query_95(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id == 9 || id == 10); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_96 : public Accept_All_Tags
{
  Accept_Query_96(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id <= 10); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_97 : public Accept_All_Tags
{
  Accept_Query_97(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 5 || id == 10); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_98 : public Accept_All_Tags
{
  Accept_Query_98(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id <= 5); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_99 : public Accept_All_Tags
{
  Accept_Query_99(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 5); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_100 : public Accept_All_Tags
{
  Accept_Query_100(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 9 || id == 10); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_101 : public Accept_All_Tags
{
  Accept_Query_101(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id <= 10); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_102 : public Accept_All_Tags
{
  Accept_Query_102(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id == 5 || id == 10); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_103 : public Accept_All_Tags
{
  Accept_Query_103(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id <= 10 && id != 5 && id != 9); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_104 : public Accept_All_Tags
{
  Accept_Query_104(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id == 10); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_105 : public Accept_All_Tags
{
  Accept_Query_105(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id == 9 || id == 10); }
  
  private:
    uint pattern_size;
};

struct Accept_Query_106 : public Accept_All_Tags
{
  Accept_Query_106(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1 || id == 2
    || id == pattern_size + 1 || id == pattern_size + 2 || id == pattern_size + 3
    || id == 2*pattern_size + 4 || id == 3*pattern_size + 4);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_107 : public Accept_All_Tags
{
  Accept_Query_107(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2
    || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
    || id == pattern_size*pattern_size/4 + pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_108 : public Accept_All_Tags
{
  Accept_Query_108(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_109 : public Accept_All_Tags
{
  Accept_Query_109(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == 1 || id == 2 || id == pattern_size
    || id == pattern_size + 1 || id == pattern_size + 2 || id == pattern_size + 3
    || id == 2*pattern_size + 4 || id == 3*pattern_size + 4
    || id == pattern_size*pattern_size/4
    || id == pattern_size*pattern_size - pattern_size
    || id == pattern_size*pattern_size);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_110 : public Accept_All_Tags
{
  Accept_Query_110(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == 1 || id == 2
    || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
    || id == pattern_size*pattern_size/4 + pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_111 : public Accept_All_Tags
{
  Accept_Query_111(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return ((id >= 1 && id <= 6) || id == 9);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_113 : public Accept_All_Tags
{
  Accept_Query_113(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id == pattern_size*pattern_size/4 - pattern_size/2 + 1);
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_114 : public Accept_All_Tags
{
  Accept_Query_114(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 2 || (id >= 6 && id <= 8) || id == 10 || id == 11);
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_117 : public Accept_All_Tags
{
  Accept_Query_117(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  {
    return (id == 2 || (id >= 6 && id <= 11));
  }
  
  private:
    uint pattern_size;
};

struct Accept_Query_118 : public Accept_All_Tags
{
  Accept_Query_118(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return (3*pattern_size*pattern_size < id &&
        id <= 3*pattern_size*pattern_size + pattern_size &&
        id % 35 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_119 : public Accept_All_Tags
{
  Accept_Query_119(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return (3*pattern_size*pattern_size < id &&
        id <= 3*pattern_size*pattern_size + pattern_size &&
        id % 7 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_122 : public Accept_All_Tags
{
  Accept_Query_122(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return (3*pattern_size*pattern_size < id &&
        id <= 3*pattern_size*pattern_size + pattern_size &&
        id % 7 == 0 && id % 3 != 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_124 : public Accept_All_Tags
{
  Accept_Query_124(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return (3*pattern_size*pattern_size < id &&
        id <= 3*pattern_size*pattern_size + pattern_size &&
        id % 7 == 0 && id % 11 != 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_125 : public Accept_All_Tags
{
  Accept_Query_125(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return (3*pattern_size*pattern_size < id &&
        id <= 3*pattern_size*pattern_size + pattern_size &&
        id % 7 == 0 && id % 3 == 0);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_139 : public Accept_All_Tags
{
  Accept_Query_139(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id % 11 == 0 && id >= 99 && id <= 1078); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_140 : public Accept_All_Tags
{
  Accept_Query_140(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id % 55 == 0 && id >= 99 && id <= 1078); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_141 : public Accept_All_Tags
{
  Accept_Query_141(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 22 == 0 && id >= 99 && id <= 1078); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_142 : public Accept_All_Tags
{
  Accept_Query_142(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 110 == 0 && id >= 99 && id <= 1078); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_143 : public Accept_All_Tags
{
  Accept_Query_143(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 11 == 0 && id % 4 != 3 && id >= 99 && id <= 1078); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_144 : public Accept_All_Tags
{
  Accept_Query_144(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 55 == 0 && id % 4 != 3 && id >= 99 && id <= 1078); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_145 : public Accept_All_Tags
{
  Accept_Query_145(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id % 11 == 0 && id >= 99 && id <= 1078 && id <= pattern_size*pattern_size/2);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_146 : public Accept_All_Tags
{
  Accept_Query_146(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id % 55 == 0 && id >= 99 && id <= 1078 && id <= pattern_size*pattern_size/2);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_147 : public Accept_All_Tags
{
  Accept_Query_147(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  { 
    return (id % 22 == 0 && id >= 99 && id <= 1078 &&
      (id <= (pattern_size+1)*(pattern_size/2-1)
      || (id > (pattern_size+2)*(pattern_size/2-1) && id <= (pattern_size+3)*(pattern_size/2-1)+1)));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_148 : public Accept_All_Tags
{
  Accept_Query_148(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id % 110 == 0 && id >= 99 && id <= 1078 &&
      (id <= (pattern_size+1)*(pattern_size/2-1)
      || (id > (pattern_size+2)*(pattern_size/2-1) && id <= (pattern_size+3)*(pattern_size/2-1)+1)));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_149 : public Accept_All_Tags
{
  Accept_Query_149(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id % 11 == 0 && id % 4 != 3 && id >= 99 && id <= 1078 &&
      (id <= (pattern_size+1)*(pattern_size/2-1)
      || (id > (pattern_size+2)*(pattern_size/2-1) && id <= (pattern_size+3)*(pattern_size/2-1)+1)));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Query_150 : public Accept_All_Tags
{
  Accept_Query_150(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  {
    return (id % 55 == 0 && id % 4 != 3 && id >= 99 && id <= 1078 &&
      (id <= (pattern_size+1)*(pattern_size/2-1)
      || (id > (pattern_size+2)*(pattern_size/2-1) && id <= (pattern_size+3)*(pattern_size/2-1)+1)));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Foreach_1 : public Accept_All_Tags
{
  Accept_Foreach_1(uint pattern_size_)
  {
    way_id_offset = (2*(pattern_size_/2+1)*(pattern_size_/2-1) + pattern_size_/2);
  }
  
  virtual bool admit_node(uint id) const
  { return ((id == 1) || (id == 2) || (id == 3)); }
  virtual bool admit_way(uint id) const
  { return ((id % way_id_offset == 1) && (id / way_id_offset <= 3) && (id != 1)); }
  virtual bool admit_relation(uint id) const
  { return (/*(id == 10) || */(id == 21) || (id == 32)); }
  
  private:
    uint way_id_offset;
};

struct Accept_Union_1 : public Accept_All_Tags
{
  Accept_Union_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id == 2); }
  virtual bool admit_way(uint id) const { return (id == 11); }
  virtual bool admit_relation(uint id) const
  { return ((id == 2) || (id == 8) || (id == 10) || (id == 11)); }
  
  private:
    uint pattern_size;
};

struct Accept_Union_2 : public Accept_All_Tags
{
  Accept_Union_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return ((id == 1) || (id == 2)); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Union_4 : public Accept_All_Tags
{
  Accept_Union_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 1); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Union_5 : public Accept_All_Tags
{
  Accept_Union_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id == 2); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Union_6 : public Accept_All_Tags
{
  Accept_Union_6(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  { return ((id == 2) || (id == 8) || (id == 10) || (id == 11)); }
  
  private:
    uint pattern_size;
};

struct Accept_Difference_1 : public Accept_All_Tags
{
  Accept_Difference_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Difference_2 : public Accept_All_Tags
{
  Accept_Difference_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return (id == 2); }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Difference_4 : public Accept_All_Tags
{
  Accept_Difference_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 1 || id == 4); }
  virtual bool admit_relation(uint id) const { return (id == 1 || id == 4); }
  
  private:
    uint pattern_size;
};

struct Accept_Difference_5 : public Accept_All_Tags
{
  Accept_Difference_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 1 || id == 4); }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Polygon_1 : public Accept_All_Tags
{
  Accept_Polygon_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return ((id >= pattern_size*pattern_size*3 + pattern_size*5 + 8
            && id <= pattern_size*pattern_size*3 + pattern_size*5 + 10)
        || (id >= pattern_size*pattern_size*3 + pattern_size*6 + 7
            && id <= pattern_size*pattern_size*3 + pattern_size*6 + 11)
        || (id >= pattern_size*pattern_size*3 + pattern_size*7 + 6
            && id <= pattern_size*pattern_size*3 + pattern_size*7 + 12)
        || (id >= pattern_size*pattern_size*3 + pattern_size*8 + 6
            && id <= pattern_size*pattern_size*3 + pattern_size*8 + 12)
        || (id >= pattern_size*pattern_size*3 + pattern_size*9 + 6
            && id <= pattern_size*pattern_size*3 + pattern_size*9 + 12)
        || (id >= pattern_size*pattern_size*3 + pattern_size*10 + 7
            && id <= pattern_size*pattern_size*3 + pattern_size*10 + 11)
        || (id >= pattern_size*pattern_size*3 + pattern_size*11 + 8
            && id <= pattern_size*pattern_size*3 + pattern_size*11 + 10)); 
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Polygon_2 : public Accept_All_Tags
{
  Accept_Polygon_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  {
    return (id == pattern_size*pattern_size*2
        || id == pattern_size*pattern_size*4 - pattern_size + 1);
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Polygon_3 : public Accept_All_Tags
{
  Accept_Polygon_3(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
  { 
    return (id % 5 == 0 &&
        ((id >= pattern_size*pattern_size*3 + pattern_size*5 + 8
            && id <= pattern_size*pattern_size*3 + pattern_size*5 + 10)
        || (id >= pattern_size*pattern_size*3 + pattern_size*6 + 7
            && id <= pattern_size*pattern_size*3 + pattern_size*6 + 11)
        || (id >= pattern_size*pattern_size*3 + pattern_size*7 + 6
            && id <= pattern_size*pattern_size*3 + pattern_size*7 + 12)
        || (id >= pattern_size*pattern_size*3 + pattern_size*8 + 6
            && id <= pattern_size*pattern_size*3 + pattern_size*8 + 12)
        || (id >= pattern_size*pattern_size*3 + pattern_size*9 + 6
            && id <= pattern_size*pattern_size*3 + pattern_size*9 + 12)
        || (id >= pattern_size*pattern_size*3 + pattern_size*10 + 7
            && id <= pattern_size*pattern_size*3 + pattern_size*10 + 11)
        || (id >= pattern_size*pattern_size*3 + pattern_size*11 + 8
            && id <= pattern_size*pattern_size*3 + pattern_size*11 + 10))); 
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Polygon_4 : public Accept_All_Tags
{
  Accept_Polygon_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  { 
    return ((id >= pattern_size*pattern_size - pattern_size/2 - 1
            && id <= pattern_size*pattern_size - pattern_size/4 - 2)
        || (id >= pattern_size*pattern_size + pattern_size/2 - 3
            && id <= pattern_size*pattern_size + pattern_size/4*3 - 3));
  }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
};

struct Accept_Polygon_5 : public Accept_All_Tags
{
  Accept_Polygon_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  { 
    return (id == 18 || id == 22);
  }
  
  private:
    uint pattern_size;
};

struct Accept_All_But_5 : public Accept_All_Tags
{
  Accept_All_But_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return (id != 5); }
  virtual bool admit_way(uint id) const
      { return (id != 5); }
  virtual bool admit_relation(uint id) const
      { return (id != 5); }
  
  private:
    uint pattern_size;
};

double great_circle_dist(double lat1, double lon1, double lat2, double lon2)
{
  double scalar_prod =
  sin(lat1/90.0*acos(0))*sin(lat2/90.0*acos(0)) +
  cos(lat1/90.0*acos(0))*sin(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*sin(lon2/90.0*acos(0)) +
  cos(lat1/90.0*acos(0))*cos(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*cos(lon2/90.0*acos(0));
  if (scalar_prod > 1)
    scalar_prod = 1;
  return acos(scalar_prod)*(10*1000*1000/acos(0));
}

struct Accept_Around_1 : public Accept_All_Tags
{
  Accept_Around_1(uint pattern_size_, double radius_, bool northeast_ = false, uint divisor_ = 0)
    : pattern_size(pattern_size_), radius(radius_), northeast(northeast_), divisor(divisor_)
  {
    north = 48.1;
    south = 47.9;
    west = -0.2;
    east = 0.2;
    
    lat = (north - south)/pattern_size*0.5 + south;
    lon = (east - west)/pattern_size*0.5 + west;
    
    lat_ne = (north - south)/pattern_size*(-0.5 + pattern_size) + south;
    lon_ne = (east - west)/pattern_size*(-0.5 + pattern_size) + west;
  }
  
  virtual bool admit_node(uint id) const
  {
    if ((id > 3*pattern_size*pattern_size) || (id <= 2*pattern_size*pattern_size))
      return false;
    
    if (divisor > 0)
    {
      if ((divisor == 11) && (id % 11 != 0))
	return false;      
      if ((divisor == 7) && (id % 21 != 7))
	return false;
    }
    
    int i = (id-2*pattern_size*pattern_size-1) / pattern_size;
    int j = (id-2*pattern_size*pattern_size-1) % pattern_size;
    double arg_lat = (north - south)/pattern_size*(0.5 + i) + south;
    double arg_lon = (east - west)/pattern_size*(0.5 + j) + west;
    
    if (great_circle_dist(lat, lon, arg_lat, arg_lon) <= radius)
      return true;
    if (!northeast)
      return false;
    return (great_circle_dist(lat_ne, lon_ne, arg_lat, arg_lon) <= radius);
  }
  
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
  
  private:
    uint pattern_size;
    double radius;
    bool northeast;
    uint divisor;
    double lat, lon, lat_ne, lon_ne;
    double north, south, west, east;
};

vector< pair< string, string > > collect_tags(string prefix, uint id)
{
  vector< pair< string, string > > tags;
  if (id < 100)
    tags.push_back(make_pair< string, string >
        (prefix + "_key", prefix + "_few"));
  if (id % 11 == 0)
  {
    ostringstream buf;
    buf<<prefix<<"_value_"<<(id / 11 + 1);
    tags.push_back(make_pair< string, string >
    (prefix + "_key_11", buf.str()));
  }
  if (id % 15 == 0)
    tags.push_back(make_pair< string, string >
    (prefix + "_key_15", prefix + "_value_15"));
  if ((id % 2 == 0) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(make_pair< string, string >
        (prefix + "_key_2/4", prefix + "_value_0"));
  if ((id % 4 == 1) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(make_pair< string, string >
        (prefix + "_key_2/4", prefix + "_value_1"));
  if (id % 5 == 0)
    tags.push_back(make_pair< string, string >
        (prefix + "_key_5", prefix + "_value_5"));
  if (id % 7 == 0)
  {
    ostringstream buf;
    buf<<prefix<<"_value_"<<(id % 21 / 7);
    tags.push_back(make_pair< string, string >
        (prefix + "_key_7", buf.str()));
  }
  if (id == 2310)
    tags.push_back(make_pair< string, string >
        (prefix + "_unique", prefix + "_2310"));
  return tags;
}

void fill_bbox_with_nodes
    (double south, double north, double west, double east,
     uint begin_id, uint end_id, uint64 global_node_offset,
     const Data_Modifier* modifier)
{
  int sqrt_ = sqrt(end_id - begin_id);
  for (int i = 0; i < sqrt_; ++i)
  {
    for (int j = 0; j < sqrt_; ++j)
    {
      if (!modifier->admit_node(begin_id + i*sqrt_ + j))
	continue;
      if (!modifier->admit_node_skeleton(begin_id + i*sqrt_ + j) &&
	  !modifier->admit_node_tags(begin_id + i*sqrt_ + j))
      {
	cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j + global_node_offset)<<"\"/>\n";
	continue;
      }
      cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j + global_node_offset)<<"\""
      " lat=\""<<fixed<<setprecision(7)<<((north - south)/sqrt_*(0.5 + i) + south)<<"\""
      " lon=\""<<fixed<<setprecision(7)<<((east - west)/sqrt_*(0.5 + j) + west)<<"\"";
      vector< pair< string, string > > tags
        = collect_tags("node", begin_id + i*sqrt_ + j);
      if ((tags.empty()) || !modifier->admit_node_tags(begin_id + i*sqrt_ + j))
	cout<<"/>\n";
      else
      {
	cout<<">\n";
	for (vector< pair< string, string > >::const_iterator
	    it = tags.begin(); it != tags.end(); ++it)
	  cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
	cout<<"  </node>\n";
      }
    }
  }
}

void create_way
  (uint id, uint start_node_ref, uint end_node_ref, uint stepping, uint64 global_node_offset,
   const Data_Modifier* modifier)
{
  if (!modifier->admit_way(id))
    return;
  if (!modifier->admit_way_skeleton(id) && !modifier->admit_way_tags(id))
  {
    cout<<"  <way id=\""<<id<<"\"/>\n";
    return;
  }
  cout<<"  <way id=\""<<id<<"\">\n";
  if (start_node_ref < end_node_ref)
  {
    for (uint ref = start_node_ref; ref <= end_node_ref; ref += stepping)
      cout<<"    <nd ref=\""<<ref + global_node_offset<<"\"/>\n";
  }
  else
  {
    for (uint ref = start_node_ref; (int)ref >= (int)end_node_ref; ref -= stepping)
      cout<<"    <nd ref=\""<<ref + global_node_offset<<"\"/>\n";
  }
  if (modifier->admit_way_tags(id))
  {
    vector< pair< string, string > > tags = collect_tags("way", id);
    for (vector< pair< string, string > >::const_iterator
        it = tags.begin(); it != tags.end(); ++it)
      cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  }
  cout<<"  </way>\n";
}

void create_relation
    (uint id, const vector< uint >& refs, uint64 global_node_offset, const vector< uint >& types,
     const Data_Modifier* modifier)
{
  if (!modifier->admit_relation(id))
    return;
  if (!modifier->admit_relation_skeleton(id) &&
      !modifier->admit_relation_tags(id))
  {
    cout<<"  <relation id=\""<<id<<"\"/>\n";
    return;
  }
  
  vector< string > roles = V< string >("zero")("one")("two")("three");
  vector< string > typenames = V< string >("node")("way")("relation");
  
  cout<<"  <relation id=\""<<id<<"\">\n";
  for (uint i = 0; i < refs.size(); ++i)
  {
    cout<<"    <member type=\""<<typenames[types[i]]<<
    "\" ref=\""<<refs[i] + (types[i] == 0 ? global_node_offset : 0)<<
    "\" role=\""<<roles[(refs[i] + types[i]) % 4]<<"\"/>\n";
  }
  if (modifier->admit_relation_tags(id))
  {
    vector< pair< string, string > > tags = collect_tags("relation", id);
    for (vector< pair< string, string > >::const_iterator
        it = tags.begin(); it != tags.end(); ++it)
      cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  }
  cout<<"  </relation>\n";
}

void create_node_test_pattern
    (double south, double north, double west, double east, uint id, uint size, uint64 global_node_offset,
     const Data_Modifier* modifier)
{
  fill_bbox_with_nodes
      (south, north, west, east,
       id*size*size + 1, (id+1)*size*size + 1, global_node_offset, modifier);
}

void create_way_test_pattern(uint id, uint size, uint64 global_node_offset, const Data_Modifier* modifier)
{
  uint way_id_offset = id * (2*(size/2+1)*(size/2-1) + size/2);
  uint node_id_offset = id*size*size;
  
  // Draw a rectangular mesh of streets of length 1 segment.
  for (uint i = 1; i < size/2; ++i)
    for (uint j = 1; j <= size/2; ++j)
      create_way(way_id_offset + (i-1)*size/2 + j,
		 node_id_offset + i*size + j, node_id_offset + i*size + j + 1, 1, global_node_offset,
		 modifier);
  way_id_offset += size/2*(size/2-1);
  for (uint i = 1; i <= size/2; ++i)
    for (uint j = 1; j < size/2; ++j)
      create_way(way_id_offset + (i-1)*(size/2-1) + j,
		 node_id_offset + (i-1)*size + j + 1,
		 node_id_offset + i*size + j + 1, size, global_node_offset, modifier);
		 
  way_id_offset += size/2*(size/2-1);
  
  // Draw long straight ways from south to north.
  for (uint j = size/2+1; j < size; ++j)
    create_way(way_id_offset + j - size/2,
	       node_id_offset + j,
	       node_id_offset + size*(size-1) + j, size, global_node_offset, modifier);  
  way_id_offset += size/2-1;
  // Draw long straight ways from east to west.
  for (uint i = size/2+1; i < size; ++i)
    create_way(way_id_offset + i - size/2,
	       node_id_offset + size*i,
	       node_id_offset + size*(i-1) + 1, 1, global_node_offset, modifier);
	       
  way_id_offset += size/2-1;
  // Draw diagonal ways from northwest to southeast
  for (uint i = 0; i < size/2; ++i)
    create_way(way_id_offset + i + 1,
	       node_id_offset + size*(i+size/2) + 1,
	       node_id_offset + i+size/2 + 1, (size-1)*(i+size/2), global_node_offset, modifier);
}

void create_relation_test_pattern(uint id, uint size, uint64 global_node_offset, const Data_Modifier* modifier)
{
  uint way_id_offset = id * (2*(size/2+1)*(size/2-1) + size/2);
  uint node_id_offset = id*size*size;
  uint relation_id_offset = id*11;

  //create three small and two big relations based only on nodes
  create_relation(relation_id_offset + 1,
		  V< uint >(node_id_offset + 1)(node_id_offset + size + 2), global_node_offset,
		  V< uint >(0)(0), modifier);
  create_relation
      (relation_id_offset + 2,
       V< uint >(node_id_offset + 1)(node_id_offset + 2)
         (node_id_offset + size + 2)(node_id_offset + size + 1)(node_id_offset + 1), global_node_offset,
       V< uint >(0)(0)(0)(0)(0), modifier);
  create_relation(relation_id_offset + 3,
		  V< uint >(node_id_offset + 1)(node_id_offset + size*size), global_node_offset,
		  V< uint >(0)(0), modifier);
  create_relation
      (relation_id_offset + 4,
       V< uint >(node_id_offset + 1)(node_id_offset + size)
         (node_id_offset + size*size)(node_id_offset + size*(size-1)), global_node_offset,
       V< uint >(0)(0)(0)(0), modifier);
  create_relation(relation_id_offset + 5,
		  V< uint >(node_id_offset + size/2*size/2), global_node_offset,
		  V< uint >(0), modifier);

  //create one small and one big relation based only on ways
  create_relation
      (relation_id_offset + 6,
       V< uint >(way_id_offset + 1)(way_id_offset + 2)
         (way_id_offset + size/2*(size/2-1) + 1)(way_id_offset + size/2*(size/2+1) + 1), global_node_offset,
       V< uint >(1)(1)(1)(1), modifier);
  create_relation
      (relation_id_offset + 7,
       V< uint >(way_id_offset + size*(size/2-1) + 1)
                (way_id_offset + size*(size/2-1) + size/2)
		(way_id_offset + size*(size/2-1) + size - 1), global_node_offset,
       V< uint >(1)(1)(1), modifier);
       
  // mixed
  create_relation
      (relation_id_offset + 8,
       V< uint >(node_id_offset + 1)
         (way_id_offset + 1)
         (node_id_offset + 2)
	 (way_id_offset + 2)
         (node_id_offset + size + 2)
	 (way_id_offset + size/2*(size/2-1) + 1)
	 (node_id_offset + size + 1)
	 (way_id_offset + size/2*(size/2+1) + 1)
	 (node_id_offset + 1), global_node_offset,
       V< uint >(0)(1)(0)(1)(0)(1)(0)(1)(0), modifier);
  
  // relations on relations
  create_relation
      (relation_id_offset + 9,
       V< uint >(relation_id_offset + 1)(relation_id_offset + 2)(relation_id_offset + 3)
         (relation_id_offset + 4)(relation_id_offset + 5)(relation_id_offset + 6), global_node_offset,
       V< uint >(2)(2)(2)(2)(2)(2), modifier);
  create_relation
      (relation_id_offset + 10,
       V< uint >(relation_id_offset + 1)
         (node_id_offset + 1)
         (way_id_offset + 1)
         (node_id_offset + 2)
	 (way_id_offset + 2)
         (node_id_offset + size + 2)
	 (way_id_offset + size/2*(size/2-1) + 1)
	 (node_id_offset + size + 1)
	 (way_id_offset + size/2*(size/2+1) + 1)
	 (node_id_offset + 1)
	 (relation_id_offset + 2), global_node_offset,
       V< uint >(2)(0)(1)(0)(1)(0)(1)(0)(1)(0)(2), modifier);
       
   // a big relation
   vector< uint > refs, types;
   int max_num_node_members = size*size;
   if (max_num_node_members > 32767)
     max_num_node_members = 32767;
   for (int i = 0; i < max_num_node_members; ++i)
   {
     refs.push_back(node_id_offset + i + 1);
     types.push_back(0);
   }
   for (int i = 0; i < max_num_node_members; ++i)
   {
     refs.push_back(node_id_offset + i + 1);
     types.push_back(0);
   }
   create_relation(relation_id_offset + 11, refs, global_node_offset, types, modifier);
}

int main(int argc, char* args[])
{
  uint64 global_node_offset = 0;  
  uint pattern_size = 2;
  if (argc > 1)
    pattern_size = atoi(args[1]);
  Data_Modifier* modifier = 0;
  if (argc > 2)
  {
    if (string(args[2]) == "print_1")
      modifier = new Accept_Print_1;
    else if (string(args[2]) == "print_2")
      modifier = new Accept_Print_2;
    else if (string(args[2]) == "print_3")
      modifier = new Accept_Print_3;
    else if (string(args[2]) == "print_4")
      modifier = new Accept_Print_1; //print_1 and print_4 are equal
    else if (string(args[2]) == "recurse_1")
      modifier = new Accept_Recurse_1(pattern_size);
    else if (string(args[2]) == "recurse_2")
      modifier = new Accept_Recurse_2(pattern_size);
    else if (string(args[2]) == "recurse_3")
      modifier = new Accept_Recurse_3(pattern_size);
    else if (string(args[2]) == "recurse_4")
      modifier = new Accept_Recurse_4(pattern_size);
    else if (string(args[2]) == "recurse_5")
      modifier = new Accept_Recurse_5(pattern_size);
    else if (string(args[2]) == "recurse_6")
      modifier = new Accept_Recurse_6(pattern_size);
    else if (string(args[2]) == "recurse_7")
      modifier = new Accept_Recurse_7(pattern_size);
    else if (string(args[2]) == "recurse_8")
      modifier = new Accept_Recurse_8(pattern_size);
    else if (string(args[2]) == "recurse_9")
      modifier = new Accept_Recurse_9(pattern_size);
    else if (string(args[2]) == "recurse_10")
      modifier = new Accept_Recurse_10(pattern_size);
    else if (string(args[2]) == "recurse_11")
      modifier = new Accept_Recurse_11(pattern_size);
    else if (string(args[2]) == "recurse_12")
      modifier = new Accept_Recurse_12(pattern_size);
    else if (string(args[2]) == "recurse_13")
      modifier = new Accept_Recurse_13(pattern_size);
    else if (string(args[2]) == "recurse_14")
      modifier = new Accept_Recurse_14(pattern_size);
    else if (string(args[2]) == "recurse_15")
      modifier = new Accept_Recurse_15(pattern_size);
    else if (string(args[2]) == "recurse_16")
      modifier = new Accept_Recurse_16(pattern_size);
    else if (string(args[2]) == "recurse_17")
      modifier = new Accept_Recurse_17(pattern_size);
    else if (string(args[2]) == "recurse_18")
      modifier = new Accept_Recurse_18(pattern_size);
    else if (string(args[2]) == "recurse_19")
      modifier = new Accept_Recurse_19(pattern_size);
    else if (string(args[2]) == "recurse_20")
      modifier = new Accept_Recurse_20(pattern_size);
    else if (string(args[2]) == "recurse_21")
      modifier = new Accept_Recurse_21(pattern_size);
    else if (string(args[2]) == "recurse_22")
      modifier = new Accept_Recurse_21(pattern_size); //recurse_21 and recurse_22 are equal
    else if (string(args[2]) == "recurse_23")
      modifier = new Accept_Recurse_23(pattern_size);
    else if (string(args[2]) == "recurse_24")
      modifier = new Accept_Recurse_24(pattern_size);
    else if (string(args[2]) == "recurse_25")
      modifier = new Accept_Recurse_25(pattern_size);
    else if (string(args[2]) == "recurse_26")
      modifier = new Accept_Recurse_26(pattern_size);
    else if (string(args[2]) == "recurse_27")
      modifier = new Accept_Recurse_27(pattern_size);
    else if (string(args[2]) == "recurse_28")
      modifier = new Accept_Recurse_28(pattern_size);
    else if (string(args[2]) == "bbox_query_1")
      modifier = new Accept_Bbox_Query_1(pattern_size);
    else if (string(args[2]) == "bbox_query_2")
      modifier = new Accept_Bbox_Query_2(pattern_size);
    else if (string(args[2]) == "bbox_query_3")
      modifier = new Accept_Bbox_Query_3(pattern_size);
    else if (string(args[2]) == "bbox_query_4")
      modifier = new Accept_Bbox_Query_4(pattern_size);
    else if (string(args[2]) == "bbox_query_5")
      modifier = new Accept_Bbox_Query_5(pattern_size);
    else if (string(args[2]) == "bbox_query_6")
      modifier = new Accept_Bbox_Query_6(pattern_size);
    else if (string(args[2]) == "bbox_query_7")
      // bbox-query five and bboy-query seven shall both return an empty set.
      modifier = new Accept_Bbox_Query_5(pattern_size);
    else if (string(args[2]) == "bbox_query_8")
      modifier = new Accept_Bbox_Query_8(pattern_size);
    else if (string(args[2]) == "query_1")
      modifier = new Accept_Query_1(pattern_size);
    else if (string(args[2]) == "query_2")
      modifier = new Accept_Query_2(pattern_size);
    else if (string(args[2]) == "query_3")
      modifier = new Accept_Query_3(pattern_size);
    else if (string(args[2]) == "query_4")
      modifier = new Accept_Query_4(pattern_size);
    else if (string(args[2]) == "query_5")
      modifier = new Accept_Query_5(pattern_size);
    else if (string(args[2]) == "query_6")
      modifier = new Accept_Query_6(pattern_size);
    else if (string(args[2]) == "query_7")
      modifier = new Accept_Query_7(pattern_size);
    else if (string(args[2]) == "query_8")
      modifier = new Accept_Query_8(pattern_size);
    else if (string(args[2]) == "query_9")
      modifier = new Accept_Query_9(pattern_size);
    else if (string(args[2]) == "query_10")
      modifier = new Accept_Query_10(pattern_size);
    else if (string(args[2]) == "query_11")
      modifier = new Accept_Query_11(pattern_size);
    else if (string(args[2]) == "query_12")
      modifier = new Accept_Query_12(pattern_size);
    else if (string(args[2]) == "query_13")
      modifier = new Accept_Query_13(pattern_size);
    else if (string(args[2]) == "query_14")
      modifier = new Accept_Query_14(pattern_size);
    else if (string(args[2]) == "query_15")
      modifier = new Accept_Query_15(pattern_size);
    else if (string(args[2]) == "query_16")
      // query 16 and query 5 shall both return an empty set.
      modifier = new Accept_Query_5(pattern_size);
    else if (string(args[2]) == "query_17")
      modifier = new Accept_Query_17(pattern_size);
    else if (string(args[2]) == "query_18")
      modifier = new Accept_Query_18(pattern_size);
    else if (string(args[2]) == "query_19")
      modifier = new Accept_Query_19(pattern_size);
    else if (string(args[2]) == "query_20")
      modifier = new Accept_Query_20(pattern_size);
    else if (string(args[2]) == "query_21")
      modifier = new Accept_Query_21(pattern_size);
    else if (string(args[2]) == "query_22")
      modifier = new Accept_Query_22(pattern_size);
    else if (string(args[2]) == "query_23")
      modifier = new Accept_Query_23(pattern_size);
    else if (string(args[2]) == "query_24")
      // query 24 and query 5 shall both return an empty set.
      modifier = new Accept_Query_5(pattern_size);
    else if (string(args[2]) == "query_25")
      modifier = new Accept_Query_25(pattern_size);
    else if (string(args[2]) == "query_26")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 11);
    else if (string(args[2]) == "query_27")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 7);
    else if (string(args[2]) == "query_28")
      modifier = new Accept_Query_28(pattern_size);
    else if (string(args[2]) == "query_29")
      modifier = new Accept_Query_29(pattern_size);
    else if (string(args[2]) == "query_30")
      modifier = new Accept_Query_30(pattern_size);
    else if (string(args[2]) == "query_31")
      modifier = new Accept_Query_31(pattern_size);
    else if (string(args[2]) == "query_32")
      modifier = new Accept_Query_1(pattern_size);
    else if (string(args[2]) == "query_33")
      modifier = new Accept_Query_1(pattern_size);
    else if (string(args[2]) == "query_34")
      modifier = new Accept_Query_25(pattern_size);
    else if (string(args[2]) == "query_35")
      modifier = new Accept_Query_25(pattern_size);
    else if (string(args[2]) == "query_36")
      modifier = new Accept_Query_17(pattern_size);
    else if (string(args[2]) == "query_37")
      modifier = new Accept_Query_37(pattern_size);
    else if (string(args[2]) == "query_38")
      modifier = new Accept_Query_38(pattern_size);
    else if (string(args[2]) == "query_39")
      modifier = new Accept_Query_39(pattern_size);
    else if (string(args[2]) == "query_40")
      modifier = new Accept_Query_40(pattern_size);
    else if (string(args[2]) == "query_41")
      modifier = new Accept_Query_41(pattern_size);
    else if (string(args[2]) == "query_42")
      modifier = new Accept_Query_42(pattern_size);
    else if (string(args[2]) == "query_43")
      modifier = new Accept_Query_43(pattern_size);
    else if (string(args[2]) == "query_44")
      modifier = new Accept_Query_44(pattern_size);
    else if (string(args[2]) == "query_45")
      modifier = new Accept_Query_45(pattern_size);
    else if (string(args[2]) == "query_46")
      modifier = new Accept_Query_46(pattern_size);
    else if (string(args[2]) == "query_47")
      modifier = new Accept_Query_47(pattern_size);
    else if (string(args[2]) == "query_48")
      modifier = new Accept_Query_45(pattern_size);
    else if (string(args[2]) == "query_49")
      modifier = new Accept_Query_46(pattern_size);
    else if (string(args[2]) == "query_50")
      modifier = new Accept_Query_47(pattern_size);
    else if (string(args[2]) == "query_51")
      modifier = new Accept_Query_51(pattern_size);
    else if (string(args[2]) == "query_52")
      modifier = new Accept_Query_52(pattern_size);
    else if (string(args[2]) == "query_53")
      modifier = new Accept_Query_53(pattern_size);
    else if (string(args[2]) == "query_54")
      modifier = new Accept_Query_54(pattern_size);
    else if (string(args[2]) == "query_55")
      modifier = new Accept_Query_55(pattern_size);
    else if (string(args[2]) == "query_56")
      modifier = new Accept_Query_56(pattern_size);
    else if (string(args[2]) == "query_57")
      modifier = new Accept_Query_57(pattern_size);
    else if (string(args[2]) == "query_58")
      modifier = new Accept_Query_58(pattern_size);
    else if (string(args[2]) == "query_59")
      modifier = new Accept_Query_59(pattern_size);
    else if (string(args[2]) == "query_60")
      modifier = new Accept_Query_60(pattern_size);
    else if (string(args[2]) == "query_61")
      modifier = new Accept_Query_61(pattern_size);
    else if (string(args[2]) == "query_62")
      modifier = new Accept_Query_62(pattern_size);
    else if (string(args[2]) == "query_63")
      modifier = new Accept_Query_63(pattern_size);
    else if (string(args[2]) == "query_64")
      modifier = new Accept_Query_64(pattern_size);
    else if (string(args[2]) == "query_65")
      modifier = new Accept_Query_65(pattern_size);
    else if (string(args[2]) == "query_66")
      modifier = new Accept_Query_66(pattern_size);
    else if (string(args[2]) == "query_67")
      modifier = new Accept_Query_67(pattern_size);
    else if (string(args[2]) == "query_68")
      modifier = new Accept_Query_68(pattern_size);
    else if (string(args[2]) == "query_69")
      modifier = new Accept_Query_69(pattern_size);
    else if (string(args[2]) == "query_70")
      modifier = new Accept_Query_70(pattern_size);
    else if (string(args[2]) == "query_71")
      modifier = new Accept_Query_71(pattern_size);
    else if (string(args[2]) == "query_72")
      modifier = new Accept_Query_72(pattern_size);
    else if (string(args[2]) == "query_73")
      modifier = new Accept_Query_73(pattern_size);
    else if (string(args[2]) == "query_74")
      modifier = new Accept_Query_74(pattern_size);
    else if (string(args[2]) == "query_75")
      modifier = new Accept_Query_75(pattern_size);
    else if (string(args[2]) == "query_76")
      modifier = new Accept_Query_76(pattern_size);
    else if (string(args[2]) == "query_77")
      modifier = new Accept_Query_77(pattern_size);
    else if (string(args[2]) == "query_78")
      modifier = new Accept_Query_78(pattern_size);
    else if (string(args[2]) == "query_79")
      modifier = new Accept_Query_79(pattern_size);
    else if (string(args[2]) == "query_80")
      modifier = new Accept_Query_80(pattern_size);
    else if (string(args[2]) == "query_81")
      modifier = new Accept_Query_81(pattern_size);
    else if (string(args[2]) == "query_82")
      modifier = new Accept_Query_82(pattern_size);
    else if (string(args[2]) == "query_83")
      modifier = new Accept_Query_83(pattern_size);
    else if (string(args[2]) == "query_84")
      modifier = new Accept_Query_84(pattern_size);
    else if (string(args[2]) == "query_85")
      modifier = new Accept_Query_85(pattern_size);
    else if (string(args[2]) == "query_86")
      modifier = new Accept_Query_86(pattern_size);
    else if (string(args[2]) == "query_87")
      modifier = new Accept_Query_87(pattern_size);
    else if (string(args[2]) == "query_88")
      modifier = new Accept_Query_88(pattern_size);
    else if (string(args[2]) == "query_89")
      modifier = new Accept_Query_89(pattern_size);
    else if (string(args[2]) == "query_90")
      modifier = new Accept_Query_90(pattern_size);
    else if (string(args[2]) == "query_91")
      modifier = new Accept_Query_91(pattern_size);
    else if (string(args[2]) == "query_92")
      modifier = new Accept_Query_92(pattern_size);
    else if (string(args[2]) == "query_93")
      modifier = new Accept_Query_93(pattern_size);
    else if (string(args[2]) == "query_94")
      modifier = new Accept_Query_94(pattern_size);
    else if (string(args[2]) == "query_95")
      modifier = new Accept_Query_95(pattern_size);
    else if (string(args[2]) == "query_96")
      modifier = new Accept_Query_96(pattern_size);
    else if (string(args[2]) == "query_97")
      modifier = new Accept_Query_97(pattern_size);
    else if (string(args[2]) == "query_98")
      modifier = new Accept_Query_98(pattern_size);
    else if (string(args[2]) == "query_99")
      modifier = new Accept_Query_99(pattern_size);
    else if (string(args[2]) == "query_100")
      modifier = new Accept_Query_100(pattern_size);
    else if (string(args[2]) == "query_101")
      modifier = new Accept_Query_101(pattern_size);
    else if (string(args[2]) == "query_102")
      modifier = new Accept_Query_102(pattern_size);
    else if (string(args[2]) == "query_103")
      modifier = new Accept_Query_103(pattern_size);
    else if (string(args[2]) == "query_104")
      modifier = new Accept_Query_104(pattern_size);
    else if (string(args[2]) == "query_105")
      modifier = new Accept_Query_105(pattern_size);
    else if (string(args[2]) == "query_106")
      modifier = new Accept_Query_106(pattern_size);
    else if (string(args[2]) == "query_107")
      modifier = new Accept_Query_107(pattern_size);
    else if (string(args[2]) == "query_108")
      modifier = new Accept_Query_108(pattern_size);
    else if (string(args[2]) == "query_109")
      modifier = new Accept_Query_109(pattern_size);
    else if (string(args[2]) == "query_110")
      modifier = new Accept_Query_110(pattern_size);
    else if (string(args[2]) == "query_111")
      modifier = new Accept_Query_111(pattern_size);
    else if (string(args[2]) == "query_112")
      // query 112 and query 5 shall both return an empty set.
      modifier = new Accept_Query_5(pattern_size);
    else if (string(args[2]) == "query_113")
      modifier = new Accept_Query_113(pattern_size);
    else if (string(args[2]) == "query_114")
      modifier = new Accept_Query_114(pattern_size);
    else if (string(args[2]) == "query_115")
      // query 115 and query 5 shall both return an empty set.
      modifier = new Accept_Query_5(pattern_size);
    else if (string(args[2]) == "query_116")
      // query 116 and 113 shall return the same result
      modifier = new Accept_Query_113(pattern_size);
    else if (string(args[2]) == "query_117")
      modifier = new Accept_Query_117(pattern_size);
    else if (string(args[2]) == "query_118")
      modifier = new Accept_Query_118(pattern_size);
    else if (string(args[2]) == "query_119")
      modifier = new Accept_Query_119(pattern_size);
    else if (string(args[2]) == "query_120")
      modifier = new Accept_Query_118(pattern_size);
    else if (string(args[2]) == "query_121")
      modifier = new Accept_Query_118(pattern_size);
    else if (string(args[2]) == "query_122")
      modifier = new Accept_Query_122(pattern_size);
    else if (string(args[2]) == "query_123")
      modifier = new Accept_Query_122(pattern_size);
    else if (string(args[2]) == "query_124")
      modifier = new Accept_Query_124(pattern_size);
    else if (string(args[2]) == "query_125")
      modifier = new Accept_Query_125(pattern_size);
    else if (string(args[2]) == "query_126")
      modifier = new Accept_Query_125(pattern_size);
    else if (string(args[2]) == "query_127")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 11);
    else if (string(args[2]) == "query_128")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 7);
    else if (string(args[2]) == "query_129")
      modifier = new Accept_Query_37(pattern_size);
    else if (string(args[2]) == "query_130")
      modifier = new Accept_Query_38(pattern_size);
    else if (string(args[2]) == "query_131")
      modifier = new Accept_Query_39(pattern_size);
    else if (string(args[2]) == "query_132")
      modifier = new Accept_Query_40(pattern_size);
    else if (string(args[2]) == "query_133")
      modifier = new Accept_Recurse_23(pattern_size);
    else if (string(args[2]) == "query_134")
      modifier = new Accept_Recurse_25(pattern_size);
    else if (string(args[2]) == "query_135")
      modifier = new Accept_Recurse_27(pattern_size);
    else if (string(args[2]) == "query_136")
      modifier = new Accept_Recurse_24(pattern_size);
    else if (string(args[2]) == "query_137")
      modifier = new Accept_Recurse_26(pattern_size);
    else if (string(args[2]) == "query_138")
      modifier = new Accept_Recurse_28(pattern_size);
    else if (string(args[2]) == "query_139")
      modifier = new Accept_Query_139(pattern_size);
    else if (string(args[2]) == "query_140")
      modifier = new Accept_Query_140(pattern_size);
    else if (string(args[2]) == "query_141")
      modifier = new Accept_Query_141(pattern_size);
    else if (string(args[2]) == "query_142")
      modifier = new Accept_Query_142(pattern_size);
    else if (string(args[2]) == "query_143")
      modifier = new Accept_Query_143(pattern_size);
    else if (string(args[2]) == "query_144")
      modifier = new Accept_Query_144(pattern_size);
    else if (string(args[2]) == "query_145")
      modifier = new Accept_Query_145(pattern_size);
    else if (string(args[2]) == "query_146")
      modifier = new Accept_Query_146(pattern_size);
    else if (string(args[2]) == "query_147")
      modifier = new Accept_Query_147(pattern_size);
    else if (string(args[2]) == "query_148")
      modifier = new Accept_Query_148(pattern_size);
    else if (string(args[2]) == "query_149")
      modifier = new Accept_Query_149(pattern_size);
    else if (string(args[2]) == "query_150")
      modifier = new Accept_Query_150(pattern_size);
    else if (string(args[2]) == "union_1")
      modifier = new Accept_Union_1(pattern_size);
    else if (string(args[2]) == "union_2")
      modifier = new Accept_Union_2(pattern_size);
    else if (string(args[2]) == "foreach_1")
      modifier = new Accept_Foreach_1(pattern_size);
    else if (string(args[2]) == "foreach_2")
      // query 1 and 2 shall both return an empty set.
      modifier = new Accept_Foreach_1(pattern_size);
    else if (string(args[2]) == "foreach_3")
      // query 1 and 3 shall both return an empty set.
      modifier = new Accept_Foreach_1(pattern_size);
    else if (string(args[2]) == "foreach_4")
      // query 1 and 3 shall both return an empty set.
      modifier = new Accept_Foreach_1(pattern_size);
    else if (string(args[2]) == "union_3")
      // query 1 and 3 shall return the same result
      modifier = new Accept_Union_1(pattern_size);
    else if (string(args[2]) == "union_4")
      modifier = new Accept_Union_4(pattern_size);
    else if (string(args[2]) == "union_5")
      modifier = new Accept_Union_5(pattern_size);
    else if (string(args[2]) == "union_6")
      modifier = new Accept_Union_6(pattern_size);
    else if (string(args[2]) == "difference_1")
      modifier = new Accept_Difference_1(pattern_size);
    else if (string(args[2]) == "difference_2")
      modifier = new Accept_Difference_2(pattern_size);
    else if (string(args[2]) == "difference_3")
      modifier = new Accept_Difference_1(pattern_size);
    else if (string(args[2]) == "difference_4")
      modifier = new Accept_Difference_4(pattern_size);
    else if (string(args[2]) == "difference_5")
      modifier = new Accept_Difference_5(pattern_size);
    else if (string(args[2]) == "around_1")
      modifier = new Accept_Around_1(pattern_size, 20.01);
    else if (string(args[2]) == "around_2")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (string(args[2]) == "around_3")
      modifier = new Accept_Around_1(pattern_size, 2001);
    else if (string(args[2]) == "around_4")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (string(args[2]) == "around_5")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (string(args[2]) == "around_6")
      modifier = new Accept_Around_1(pattern_size, 200.1, true);
    else if (string(args[2]) == "around_7")
      modifier = new Accept_Around_1(pattern_size, 20.01);
    else if (string(args[2]) == "around_8")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (string(args[2]) == "around_9")
      modifier = new Accept_Around_1(pattern_size, 2001);
    else if (string(args[2]) == "polygon_query_1")
      modifier = new Accept_Polygon_1(pattern_size);
    else if (string(args[2]) == "polygon_query_2")
      modifier = new Accept_Polygon_2(pattern_size);
    else if (string(args[2]) == "polygon_query_3")
      modifier = new Accept_Polygon_3(pattern_size);
    else if (string(args[2]) == "polygon_query_4")
      modifier = new Accept_Polygon_4(pattern_size);
    else if (string(args[2]) == "polygon_query_5")
      modifier = new Accept_Polygon_5(pattern_size);
    else if (string(args[2]) == "diff_do")
      modifier = new Accept_All;
    else if (string(args[2]) == "diff_compare")
      modifier = new Accept_All_But_5(pattern_size);
    else if (string(args[2]) == "")
      modifier = new Accept_All;
    else
      // return an empty osm file otherwise
      modifier = new Accept_Bbox_Query_5(pattern_size);
  }
  else
    modifier = new Accept_All;
  
  if (argc > 3)
    global_node_offset = atoll(args[3]);

  if ((argc > 2) && (string(args[2]) == "diff_do"))
  {
    cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";
    
    cout<<
    "  <node id=\""<<5 + global_node_offset<<"\" lat=\"-10.0\" lon=\"-15.0\">\n"
    "    <tag k=\"node_key_5\" v=\"node_value_5\"/>\n"
    "  </node>\n";
    
    cout<<"<delete>\n";
    create_node_test_pattern(-10.0, 80.0, -15.0, 105.0, 1, pattern_size, global_node_offset, modifier);
    cout<<"</delete>\n";
    
    cout<<
    "  <way id=\"5\">\n"
    "    <nd ref=\""<<15 + global_node_offset<<"\"/>\n"
    "    <nd ref=\""<<16 + global_node_offset<<"\"/>\n"
    "    <tag k=\"way_key_5\" v=\"way_value_5\"/>\n"
    "  </way>\n";
    
    cout<<"<delete>\n";
    create_way_test_pattern(1, pattern_size, global_node_offset, modifier);
    cout<<"</delete>\n";
    
    cout<<
    "  <relation id=\"5\">\n"
    "    <member type=\"node\" ref=\""<<15 + global_node_offset<<"\" role=\"three\"/>\n"
    "    <member type=\"node\" ref=\""<<16 + global_node_offset<<"\" role=\"zero\"/>\n"
    "    <tag k=\"relation_key_5\" v=\"relation_value_5\"/>\n"
    "  </relation>\n";
    
    cout<<"<delete>\n";
    create_relation_test_pattern(1, pattern_size, global_node_offset, modifier);
    cout<<"</delete>\n";
    
    cout<<"</osm>\n";
  }
  else if ((argc > 2) && (string(args[2]) == "diff_compare"))
  {
    cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";
    
    create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(47.9, 48.1, -0.2, 0.2, 2, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(30.0, 50.0, -120.0, -60.0, 3, pattern_size, global_node_offset, modifier);

    cout<<
    "  <node id=\""<<5 + global_node_offset<<"\" lat=\"-10.0000000\" lon=\"-15.0000000\">\n"
    "    <tag k=\"node_key_5\" v=\"node_value_5\"/>\n"
    "  </node>\n";
    
    create_way_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_way_test_pattern(2, pattern_size, global_node_offset, modifier);
    
    cout<<
    "  <way id=\"5\">\n"
    "    <nd ref=\""<<15 + global_node_offset<<"\"/>\n"
    "    <nd ref=\""<<16 + global_node_offset<<"\"/>\n"
    "    <tag k=\"way_key_5\" v=\"way_value_5\"/>\n"
    "  </way>\n";
    
    create_relation_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_relation_test_pattern(2, pattern_size, global_node_offset, modifier);
    
    cout<<
    "  <relation id=\"5\">\n"
    "    <member type=\"node\" ref=\""<<15 + global_node_offset<<"\" role=\"three\"/>\n"
    "    <member type=\"node\" ref=\""<<16 + global_node_offset<<"\" role=\"zero\"/>\n"
    "    <tag k=\"relation_key_5\" v=\"relation_value_5\"/>\n"
    "  </relation>\n";
    
    cout<<"</osm>\n";
  }
  else if ((argc > 2) && (string(args[2]).substr(0, 5) == "make_"))
  {
    cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";
    
    if (string(args[2]) == "make_1")
      cout<<"  <one id=\"0\"/>\n";
    if (string(args[2]) == "make_2")
      cout<<"  <two id=\"0\"/>\n";
    if (string(args[2]) == "make_3")
      cout<<"  <into_target id=\"0\"/>\n";
    if (string(args[2]) == "make_4")
      cout<<
      "  <with-tags id=\"0\">\n"
      "    <tag k=\"single\" v=\"value\"/>\n"
      "  </with-tags>\n";
    if (string(args[2]) == "make_5")
      cout<<
      "  <with-tags id=\"0\">\n"
      "    <tag k=\"not\" v=\"in\"/>\n"
      "    <tag k=\"alphabetic\" v=\"order\"/>\n"
      "  </with-tags>\n";
    
    cout<<"</osm>\n";
  }
  else
  {
    cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";
    
    create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(-10.0, 80.0, -15.0, 105.0, 1, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(47.9, 48.1, -0.2, 0.2, 2, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(30.0, 50.0, -120.0, -60.0, 3, pattern_size, global_node_offset, modifier);
    
    for (uint i = 0; i < 3; ++i)
      create_way_test_pattern(i, pattern_size, global_node_offset, modifier);
    
    for (uint i = 0; i < 3; ++i)
      create_relation_test_pattern(i, pattern_size, global_node_offset, modifier);
    
    cout<<"</osm>\n";
  }
}
