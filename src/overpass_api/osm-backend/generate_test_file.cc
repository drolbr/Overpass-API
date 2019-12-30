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

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>


typedef unsigned int uint;
typedef unsigned long long uint64;

template < typename T >
struct V : public std::vector< T >
{
  V(const T& t) : std::vector< T >(1, t) {}
  V& operator()(const T& t)
  {
    this->push_back(t);
    return *this;
  }
};

struct Data_Modifier
{
  virtual ~Data_Modifier() {}

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

struct Accept_Query_151 : public Accept_All_Tags
{
  Accept_Query_151(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const
  {
    return id % 5 == 0 && pattern_size*pattern_size/4*5 < id && id < pattern_size*pattern_size/2*3
      && id % pattern_size <= pattern_size/2 && id % pattern_size > 0;
  }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Query_153 : public Accept_All_Tags
{
  Accept_Query_153(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return id == 18 || (id == 22 && pattern_size <= 362); }

  private:
    uint pattern_size;
};

struct Accept_Query_154 : public Accept_All_Tags
{
  Accept_Query_154(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return id % 5 == 0 && id < 100; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Query_155 : public Accept_All_Tags
{
  Accept_Query_155(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return id % 5 == 0 && id < 100; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Query_156 : public Accept_All_Tags
{
  Accept_Query_156(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return id % 5 == 0 && id % 4 != 3; }

  private:
    uint pattern_size;
};

struct Accept_Query_157 : public Accept_All_Tags
{
  Accept_Query_157(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return id == 11; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Query_158 : public Accept_All_Tags
{
  Accept_Query_158(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return id == 11; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Query_159 : public Accept_All_Tags
{
  Accept_Query_159(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return id == 11; }

  private:
    uint pattern_size;
};

struct Accept_Query_160 : public Accept_All_Tags
{
  Accept_Query_160(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return id == 1 || id == 2
      || id == pattern_size + 1 || id == pattern_size + 2; }
  virtual bool admit_way(uint id) const { return id == 1 || id == 2
      || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
      || id == pattern_size*pattern_size/4 + pattern_size/2 + 1; }
  virtual bool admit_relation(uint id) const { return id == 1 || id == 2; }

  private:
    uint pattern_size;
};

struct Accept_Query_161 : public Accept_All_Tags
{
  Accept_Query_161(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return id == 2 || id == pattern_size + 2; }
  virtual bool admit_way(uint id) const { return id == 1
      || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
      || id == pattern_size*pattern_size/4 + pattern_size/2 + 1; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Query_162 : public Accept_All_Tags
{
  Accept_Query_162(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return id == 2; }
  virtual bool admit_relation(uint id) const { return id == 1; }

  private:
    uint pattern_size;
};

struct Accept_Query_163 : public Accept_All_Tags
{
  Accept_Query_163(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return id % 15 == 0; }
  virtual bool admit_way(uint id) const { return false; }
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

struct Accept_Foreach_2 : public Accept_All_Tags
{
  Accept_Foreach_2(uint pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }
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

struct Accept_Difference_6 : public Accept_All_Tags
{
  Accept_Difference_6(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id == 1); }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint pattern_size;
};

struct Accept_Complete_1 : public Accept_All_Tags
{
  Accept_Complete_1(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return id == 1; }
  virtual bool admit_way(uint id) const { return id == 2; }
  virtual bool admit_relation(uint id) const { return id == 3; }

  private:
    uint pattern_size;
};

struct Accept_Complete_6 : public Accept_All_Tags
{
  Accept_Complete_6(uint pattern_size_, bool admit_node_4_) : pattern_size(pattern_size_), admit_node_4(admit_node_4_) {}

  virtual bool admit_node(uint id) const { return id == 1 || (id == 4 && admit_node_4); }
  virtual bool admit_way(uint id) const { return id == 2; }
  virtual bool admit_relation(uint id) const { return id == 3; }

  private:
    uint pattern_size;
    bool admit_node_4;
};

struct Accept_Complete_7 : public Accept_All_Tags
{
  Accept_Complete_7(uint pattern_size_, uint iteration_) : pattern_size(pattern_size_), iteration(iteration_) {}

  virtual bool admit_node(uint id) const { return id == 1; }
  virtual bool admit_way(uint id) const
  {
    if (iteration == 0)
      return id == 2;

    if (id <= pattern_size/2*9)
      return (id % (pattern_size/2) != 0
          && id % (pattern_size/2) <= 10
          && id % (pattern_size/2) + (id/(pattern_size/2)) < iteration + 3
          && id / (pattern_size/2) < iteration);

    if (id <= (pattern_size/2)*(pattern_size/2 - 1))
      return false;

    uint delta = id - (pattern_size/2)*(pattern_size/2 - 1);

    if (delta >= (pattern_size/2 - 1)*10)
      return false;

    return (delta % (pattern_size/2 - 1) != 0
        && delta % (pattern_size/2 - 1) <= 9
        && delta % (pattern_size/2 - 1) < iteration + 2
        && delta % (pattern_size/2 - 1) + (delta / (pattern_size/2 - 1)) < iteration + 3
        && delta / (pattern_size/2 - 1) < iteration + 1);
  }
  virtual bool admit_relation(uint id) const { return id == 3; }

  private:
    uint pattern_size;
    uint iteration;
};

struct Accept_If : public Accept_All_Tags
{
  Accept_If(uint target_way_id_) : target_way_id(target_way_id_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return id == target_way_id; }
  virtual bool admit_relation(uint id) const { return false; }

  private:
    uint target_way_id;
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


struct Accept_Around_10 : public Accept_All_Tags
{
  Accept_Around_10(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const
  { return id == 1 || id == 2 || id == 3 || id == pattern_size + 1 || id == pattern_size + 2
       || id == 2*pattern_size + 1; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }

private:
  uint pattern_size;
};


struct Accept_Around_11 : public Accept_All_Tags
{
  Accept_Around_11(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const
  { return id == 1 || id == 2 || id == 3 || id == pattern_size + 1 || id == pattern_size + 2; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return false; }

private:
  uint pattern_size;
};


struct Accept_Around_17 : public Accept_All_Tags
{
  Accept_Around_17(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const
  { return id == 1 || id == 2 || id == pattern_size/2+1
      || id == pattern_size*pattern_size/4 - pattern_size/2 + 1
      || id == pattern_size*pattern_size/4 - pattern_size/2 + 2
      || id == pattern_size*pattern_size/4; }
  virtual bool admit_relation(uint id) const { return false; }

private:
  uint pattern_size;
};


struct Accept_Around_18 : public Accept_All_Tags
{
  Accept_Around_18(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_relation(uint id) const
  { return id <= 11 && id != 5 && id != 7 && id != 9; }

private:
  uint pattern_size;
};


struct Accept_Around_19 : public Accept_All_Tags
{
  Accept_Around_19(uint pattern_size_) : pattern_size(pattern_size_) {}

  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return id == 1; }
  virtual bool admit_relation(uint id) const { return false; }

private:
  uint pattern_size;
};


std::vector< std::pair< std::string, std::string > > collect_tags(std::string prefix, uint id)
{
  std::vector< std::pair< std::string, std::string > > tags;
  if (id < 100)
    tags.push_back(std::make_pair< std::string, std::string >
        (prefix + "_key", prefix + "_few"));
  if (id % 11 == 0)
  {
    std::ostringstream buf;
    buf<<prefix<<"_value_"<<(id / 11 + 1);
    tags.push_back(std::make_pair< std::string, std::string >
    (prefix + "_key_11", buf.str()));
  }
  if (id % 15 == 0)
    tags.push_back(std::make_pair< std::string, std::string >
    (prefix + "_key_15", prefix + "_value_15"));
  if ((id % 2 == 0) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(std::make_pair< std::string, std::string >
        (prefix + "_key_2/4", prefix + "_value_0"));
  if ((id % 4 == 1) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(std::make_pair< std::string, std::string >
        (prefix + "_key_2/4", prefix + "_value_1"));
  if (id % 5 == 0)
    tags.push_back(std::make_pair< std::string, std::string >
        (prefix + "_key_5", prefix + "_value_5"));
  if (id % 7 == 0)
  {
    std::ostringstream buf;
    buf<<prefix<<"_value_"<<(id % 21 / 7);
    tags.push_back(std::make_pair< std::string, std::string >
        (prefix + "_key_7", buf.str()));
  }
  if (id == 2310)
    tags.push_back(std::make_pair< std::string, std::string >
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
	std::cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j + global_node_offset)<<"\"/>\n";
	continue;
      }
      std::cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j + global_node_offset)<<"\""
      " lat=\""<<std::fixed<<std::setprecision(7)<<((north - south)/sqrt_*(0.5 + i) + south)<<"\""
      " lon=\""<<std::fixed<<std::setprecision(7)<<((east - west)/sqrt_*(0.5 + j) + west)<<"\"";
      std::vector< std::pair< std::string, std::string > > tags
        = collect_tags("node", begin_id + i*sqrt_ + j);
      if ((tags.empty()) || !modifier->admit_node_tags(begin_id + i*sqrt_ + j))
	std::cout<<"/>\n";
      else
      {
	std::cout<<">\n";
	for (std::vector< std::pair< std::string, std::string > >::const_iterator
	    it = tags.begin(); it != tags.end(); ++it)
	  std::cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
	std::cout<<"  </node>\n";
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
    std::cout<<"  <way id=\""<<id<<"\"/>\n";
    return;
  }
  std::cout<<"  <way id=\""<<id<<"\">\n";
  if (start_node_ref < end_node_ref)
  {
    for (uint ref = start_node_ref; ref <= end_node_ref; ref += stepping)
      std::cout<<"    <nd ref=\""<<ref + global_node_offset<<"\"/>\n";
  }
  else
  {
    for (uint ref = start_node_ref; (int)ref >= (int)end_node_ref; ref -= stepping)
      std::cout<<"    <nd ref=\""<<ref + global_node_offset<<"\"/>\n";
  }
  if (modifier->admit_way_tags(id))
  {
    std::vector< std::pair< std::string, std::string > > tags = collect_tags("way", id);
    for (std::vector< std::pair< std::string, std::string > >::const_iterator
        it = tags.begin(); it != tags.end(); ++it)
      std::cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  }
  std::cout<<"  </way>\n";
}

void create_relation
    (uint id, const std::vector< uint >& refs, uint64 global_node_offset, const std::vector< uint >& types,
     const Data_Modifier* modifier)
{
  if (!modifier->admit_relation(id))
    return;
  if (!modifier->admit_relation_skeleton(id) &&
      !modifier->admit_relation_tags(id))
  {
    std::cout<<"  <relation id=\""<<id<<"\"/>\n";
    return;
  }

  std::vector< std::string > roles = V< std::string >("zero")("one")("two")("three");
  std::vector< std::string > typenames = V< std::string >("node")("way")("relation");

  std::cout<<"  <relation id=\""<<id<<"\">\n";
  for (uint i = 0; i < refs.size(); ++i)
  {
    std::cout<<"    <member type=\""<<typenames[types[i]]<<
    "\" ref=\""<<refs[i] + (types[i] == 0 ? global_node_offset : 0)<<
    "\" role=\""<<roles[(refs[i] + types[i]) % 4]<<"\"/>\n";
  }
  if (modifier->admit_relation_tags(id))
  {
    std::vector< std::pair< std::string, std::string > > tags = collect_tags("relation", id);
    for (std::vector< std::pair< std::string, std::string > >::const_iterator
        it = tags.begin(); it != tags.end(); ++it)
      std::cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  }
  std::cout<<"  </relation>\n";
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
   std::vector< uint > refs, types;
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


std::string lat_lon_of_node(unsigned int id, unsigned int pattern_size)
{
  std::ostringstream out;
  out<<std::fixed<<std::setprecision(7)
      <<"lat=\""<<(51. + (id / pattern_size)*(1./pattern_size) + .5/pattern_size)<<"\" "
      <<"lon=\""<<(7. + ((id-1) % pattern_size)*(1./pattern_size) + .5/pattern_size)<<"\"";
  return out.str();
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
    if (std::string(args[2]) == "print_1")
      modifier = new Accept_Print_1;
    else if (std::string(args[2]) == "print_2")
      modifier = new Accept_Print_2;
    else if (std::string(args[2]) == "print_3")
      modifier = new Accept_Print_3;
    else if (std::string(args[2]) == "print_4")
      modifier = new Accept_Print_1; //print_1 and print_4 are equal
    else if (std::string(args[2]) == "recurse_1")
      modifier = new Accept_Recurse_1(pattern_size);
    else if (std::string(args[2]) == "recurse_2")
      modifier = new Accept_Recurse_2(pattern_size);
    else if (std::string(args[2]) == "recurse_3")
      modifier = new Accept_Recurse_3(pattern_size);
    else if (std::string(args[2]) == "recurse_4")
      modifier = new Accept_Recurse_4(pattern_size);
    else if (std::string(args[2]) == "recurse_5")
      modifier = new Accept_Recurse_5(pattern_size);
    else if (std::string(args[2]) == "recurse_6")
      modifier = new Accept_Recurse_6(pattern_size);
    else if (std::string(args[2]) == "recurse_7")
      modifier = new Accept_Recurse_7(pattern_size);
    else if (std::string(args[2]) == "recurse_8")
      modifier = new Accept_Recurse_8(pattern_size);
    else if (std::string(args[2]) == "recurse_9")
      modifier = new Accept_Recurse_9(pattern_size);
    else if (std::string(args[2]) == "recurse_10")
      modifier = new Accept_Recurse_10(pattern_size);
    else if (std::string(args[2]) == "recurse_11")
      modifier = new Accept_Recurse_11(pattern_size);
    else if (std::string(args[2]) == "recurse_12")
      modifier = new Accept_Recurse_12(pattern_size);
    else if (std::string(args[2]) == "recurse_13")
      modifier = new Accept_Recurse_13(pattern_size);
    else if (std::string(args[2]) == "recurse_14")
      modifier = new Accept_Recurse_14(pattern_size);
    else if (std::string(args[2]) == "recurse_15")
      modifier = new Accept_Recurse_15(pattern_size);
    else if (std::string(args[2]) == "recurse_16")
      modifier = new Accept_Recurse_16(pattern_size);
    else if (std::string(args[2]) == "recurse_17")
      modifier = new Accept_Recurse_17(pattern_size);
    else if (std::string(args[2]) == "recurse_18")
      modifier = new Accept_Recurse_18(pattern_size);
    else if (std::string(args[2]) == "recurse_19")
      modifier = new Accept_Recurse_19(pattern_size);
    else if (std::string(args[2]) == "recurse_20")
      modifier = new Accept_Recurse_20(pattern_size);
    else if (std::string(args[2]) == "recurse_21")
      modifier = new Accept_Recurse_21(pattern_size);
    else if (std::string(args[2]) == "recurse_22")
      modifier = new Accept_Recurse_21(pattern_size); //recurse_21 and recurse_22 are equal
    else if (std::string(args[2]) == "recurse_23")
      modifier = new Accept_Recurse_23(pattern_size);
    else if (std::string(args[2]) == "recurse_24")
      modifier = new Accept_Recurse_24(pattern_size);
    else if (std::string(args[2]) == "recurse_25")
      modifier = new Accept_Recurse_25(pattern_size);
    else if (std::string(args[2]) == "recurse_26")
      modifier = new Accept_Recurse_26(pattern_size);
    else if (std::string(args[2]) == "recurse_27")
      modifier = new Accept_Recurse_27(pattern_size);
    else if (std::string(args[2]) == "recurse_28")
      modifier = new Accept_Recurse_28(pattern_size);
    else if (std::string(args[2]) == "bbox_query_1")
      modifier = new Accept_Bbox_Query_1(pattern_size);
    else if (std::string(args[2]) == "bbox_query_2")
      modifier = new Accept_Bbox_Query_2(pattern_size);
    else if (std::string(args[2]) == "bbox_query_3")
      modifier = new Accept_Bbox_Query_3(pattern_size);
    else if (std::string(args[2]) == "bbox_query_4")
      modifier = new Accept_Bbox_Query_4(pattern_size);
    else if (std::string(args[2]) == "bbox_query_5")
      modifier = new Accept_Bbox_Query_5(pattern_size);
    else if (std::string(args[2]) == "bbox_query_6")
      modifier = new Accept_Bbox_Query_6(pattern_size);
    else if (std::string(args[2]) == "bbox_query_7")
      // bbox-query five and bboy-query seven shall both return an empty std::set.
      modifier = new Accept_Bbox_Query_5(pattern_size);
    else if (std::string(args[2]) == "bbox_query_8")
      modifier = new Accept_Bbox_Query_8(pattern_size);
    else if (std::string(args[2]) == "query_1")
      modifier = new Accept_Query_1(pattern_size);
    else if (std::string(args[2]) == "query_2")
      modifier = new Accept_Query_2(pattern_size);
    else if (std::string(args[2]) == "query_3")
      modifier = new Accept_Query_3(pattern_size);
    else if (std::string(args[2]) == "query_4")
      modifier = new Accept_Query_4(pattern_size);
    else if (std::string(args[2]) == "query_5")
      modifier = new Accept_Query_5(pattern_size);
    else if (std::string(args[2]) == "query_6")
      modifier = new Accept_Query_6(pattern_size);
    else if (std::string(args[2]) == "query_7")
      modifier = new Accept_Query_7(pattern_size);
    else if (std::string(args[2]) == "query_8")
      modifier = new Accept_Query_8(pattern_size);
    else if (std::string(args[2]) == "query_9")
      modifier = new Accept_Query_9(pattern_size);
    else if (std::string(args[2]) == "query_10")
      modifier = new Accept_Query_10(pattern_size);
    else if (std::string(args[2]) == "query_11")
      modifier = new Accept_Query_11(pattern_size);
    else if (std::string(args[2]) == "query_12")
      modifier = new Accept_Query_12(pattern_size);
    else if (std::string(args[2]) == "query_13")
      modifier = new Accept_Query_13(pattern_size);
    else if (std::string(args[2]) == "query_14")
      modifier = new Accept_Query_14(pattern_size);
    else if (std::string(args[2]) == "query_15")
      modifier = new Accept_Query_15(pattern_size);
    else if (std::string(args[2]) == "query_16")
      // query 16 and query 5 shall both return an empty std::set.
      modifier = new Accept_Query_5(pattern_size);
    else if (std::string(args[2]) == "query_17")
      modifier = new Accept_Query_17(pattern_size);
    else if (std::string(args[2]) == "query_18")
      modifier = new Accept_Query_18(pattern_size);
    else if (std::string(args[2]) == "query_19")
      modifier = new Accept_Query_19(pattern_size);
    else if (std::string(args[2]) == "query_20")
      modifier = new Accept_Query_20(pattern_size);
    else if (std::string(args[2]) == "query_21")
      modifier = new Accept_Query_21(pattern_size);
    else if (std::string(args[2]) == "query_22")
      modifier = new Accept_Query_22(pattern_size);
    else if (std::string(args[2]) == "query_23")
      modifier = new Accept_Query_23(pattern_size);
    else if (std::string(args[2]) == "query_24")
      // query 24 and query 5 shall both return an empty std::set.
      modifier = new Accept_Query_5(pattern_size);
    else if (std::string(args[2]) == "query_25")
      modifier = new Accept_Query_25(pattern_size);
    else if (std::string(args[2]) == "query_26")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 11);
    else if (std::string(args[2]) == "query_27")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 7);
    else if (std::string(args[2]) == "query_28")
      modifier = new Accept_Query_28(pattern_size);
    else if (std::string(args[2]) == "query_29")
      modifier = new Accept_Query_29(pattern_size);
    else if (std::string(args[2]) == "query_30")
      modifier = new Accept_Query_30(pattern_size);
    else if (std::string(args[2]) == "query_31")
      modifier = new Accept_Query_31(pattern_size);
    else if (std::string(args[2]) == "query_32")
      modifier = new Accept_Query_1(pattern_size);
    else if (std::string(args[2]) == "query_33")
      modifier = new Accept_Query_1(pattern_size);
    else if (std::string(args[2]) == "query_34")
      modifier = new Accept_Query_25(pattern_size);
    else if (std::string(args[2]) == "query_35")
      modifier = new Accept_Query_25(pattern_size);
    else if (std::string(args[2]) == "query_36")
      modifier = new Accept_Query_17(pattern_size);
    else if (std::string(args[2]) == "query_37")
      modifier = new Accept_Query_37(pattern_size);
    else if (std::string(args[2]) == "query_38")
      modifier = new Accept_Query_38(pattern_size);
    else if (std::string(args[2]) == "query_39")
      modifier = new Accept_Query_39(pattern_size);
    else if (std::string(args[2]) == "query_40")
      modifier = new Accept_Query_40(pattern_size);
    else if (std::string(args[2]) == "query_41")
      modifier = new Accept_Query_41(pattern_size);
    else if (std::string(args[2]) == "query_42")
      modifier = new Accept_Query_42(pattern_size);
    else if (std::string(args[2]) == "query_43")
      modifier = new Accept_Query_43(pattern_size);
    else if (std::string(args[2]) == "query_44")
      modifier = new Accept_Query_44(pattern_size);
    else if (std::string(args[2]) == "query_45")
      modifier = new Accept_Query_45(pattern_size);
    else if (std::string(args[2]) == "query_46")
      modifier = new Accept_Query_46(pattern_size);
    else if (std::string(args[2]) == "query_47")
      modifier = new Accept_Query_47(pattern_size);
    else if (std::string(args[2]) == "query_48")
      modifier = new Accept_Query_45(pattern_size);
    else if (std::string(args[2]) == "query_49")
      modifier = new Accept_Query_46(pattern_size);
    else if (std::string(args[2]) == "query_50")
      modifier = new Accept_Query_47(pattern_size);
    else if (std::string(args[2]) == "query_51")
      modifier = new Accept_Query_51(pattern_size);
    else if (std::string(args[2]) == "query_52")
      modifier = new Accept_Query_52(pattern_size);
    else if (std::string(args[2]) == "query_53")
      modifier = new Accept_Query_53(pattern_size);
    else if (std::string(args[2]) == "query_54")
      modifier = new Accept_Query_54(pattern_size);
    else if (std::string(args[2]) == "query_55")
      modifier = new Accept_Query_55(pattern_size);
    else if (std::string(args[2]) == "query_56")
      modifier = new Accept_Query_56(pattern_size);
    else if (std::string(args[2]) == "query_57")
      modifier = new Accept_Query_57(pattern_size);
    else if (std::string(args[2]) == "query_58")
      modifier = new Accept_Query_58(pattern_size);
    else if (std::string(args[2]) == "query_59")
      modifier = new Accept_Query_59(pattern_size);
    else if (std::string(args[2]) == "query_60")
      modifier = new Accept_Query_60(pattern_size);
    else if (std::string(args[2]) == "query_61")
      modifier = new Accept_Query_61(pattern_size);
    else if (std::string(args[2]) == "query_62")
      modifier = new Accept_Query_62(pattern_size);
    else if (std::string(args[2]) == "query_63")
      modifier = new Accept_Query_63(pattern_size);
    else if (std::string(args[2]) == "query_64")
      modifier = new Accept_Query_64(pattern_size);
    else if (std::string(args[2]) == "query_65")
      modifier = new Accept_Query_65(pattern_size);
    else if (std::string(args[2]) == "query_66")
      modifier = new Accept_Query_66(pattern_size);
    else if (std::string(args[2]) == "query_67")
      modifier = new Accept_Query_67(pattern_size);
    else if (std::string(args[2]) == "query_68")
      modifier = new Accept_Query_68(pattern_size);
    else if (std::string(args[2]) == "query_69")
      modifier = new Accept_Query_69(pattern_size);
    else if (std::string(args[2]) == "query_70")
      modifier = new Accept_Query_70(pattern_size);
    else if (std::string(args[2]) == "query_71")
      modifier = new Accept_Query_71(pattern_size);
    else if (std::string(args[2]) == "query_72")
      modifier = new Accept_Query_72(pattern_size);
    else if (std::string(args[2]) == "query_73")
      modifier = new Accept_Query_73(pattern_size);
    else if (std::string(args[2]) == "query_74")
      modifier = new Accept_Query_74(pattern_size);
    else if (std::string(args[2]) == "query_75")
      modifier = new Accept_Query_75(pattern_size);
    else if (std::string(args[2]) == "query_76")
      modifier = new Accept_Query_76(pattern_size);
    else if (std::string(args[2]) == "query_77")
      modifier = new Accept_Query_77(pattern_size);
    else if (std::string(args[2]) == "query_78")
      modifier = new Accept_Query_78(pattern_size);
    else if (std::string(args[2]) == "query_79")
      modifier = new Accept_Query_79(pattern_size);
    else if (std::string(args[2]) == "query_80")
      modifier = new Accept_Query_80(pattern_size);
    else if (std::string(args[2]) == "query_81")
      modifier = new Accept_Query_81(pattern_size);
    else if (std::string(args[2]) == "query_82")
      modifier = new Accept_Query_82(pattern_size);
    else if (std::string(args[2]) == "query_83")
      modifier = new Accept_Query_83(pattern_size);
    else if (std::string(args[2]) == "query_84")
      modifier = new Accept_Query_84(pattern_size);
    else if (std::string(args[2]) == "query_85")
      modifier = new Accept_Query_85(pattern_size);
    else if (std::string(args[2]) == "query_86")
      modifier = new Accept_Query_86(pattern_size);
    else if (std::string(args[2]) == "query_87")
      modifier = new Accept_Query_87(pattern_size);
    else if (std::string(args[2]) == "query_88")
      modifier = new Accept_Query_88(pattern_size);
    else if (std::string(args[2]) == "query_89")
      modifier = new Accept_Query_89(pattern_size);
    else if (std::string(args[2]) == "query_90")
      modifier = new Accept_Query_90(pattern_size);
    else if (std::string(args[2]) == "query_91")
      modifier = new Accept_Query_91(pattern_size);
    else if (std::string(args[2]) == "query_92")
      modifier = new Accept_Query_92(pattern_size);
    else if (std::string(args[2]) == "query_93")
      modifier = new Accept_Query_93(pattern_size);
    else if (std::string(args[2]) == "query_94")
      modifier = new Accept_Query_94(pattern_size);
    else if (std::string(args[2]) == "query_95")
      modifier = new Accept_Query_95(pattern_size);
    else if (std::string(args[2]) == "query_96")
      modifier = new Accept_Query_96(pattern_size);
    else if (std::string(args[2]) == "query_97")
      modifier = new Accept_Query_97(pattern_size);
    else if (std::string(args[2]) == "query_98")
      modifier = new Accept_Query_98(pattern_size);
    else if (std::string(args[2]) == "query_99")
      modifier = new Accept_Query_99(pattern_size);
    else if (std::string(args[2]) == "query_100")
      modifier = new Accept_Query_100(pattern_size);
    else if (std::string(args[2]) == "query_101")
      modifier = new Accept_Query_101(pattern_size);
    else if (std::string(args[2]) == "query_102")
      modifier = new Accept_Query_102(pattern_size);
    else if (std::string(args[2]) == "query_103")
      modifier = new Accept_Query_103(pattern_size);
    else if (std::string(args[2]) == "query_104")
      modifier = new Accept_Query_104(pattern_size);
    else if (std::string(args[2]) == "query_105")
      modifier = new Accept_Query_105(pattern_size);
    else if (std::string(args[2]) == "query_106")
      modifier = new Accept_Query_106(pattern_size);
    else if (std::string(args[2]) == "query_107")
      modifier = new Accept_Query_107(pattern_size);
    else if (std::string(args[2]) == "query_108")
      modifier = new Accept_Query_108(pattern_size);
    else if (std::string(args[2]) == "query_109")
      modifier = new Accept_Query_109(pattern_size);
    else if (std::string(args[2]) == "query_110")
      modifier = new Accept_Query_110(pattern_size);
    else if (std::string(args[2]) == "query_111")
      modifier = new Accept_Query_111(pattern_size);
    else if (std::string(args[2]) == "query_112")
      // query 112 and query 5 shall both return an empty std::set.
      modifier = new Accept_Query_5(pattern_size);
    else if (std::string(args[2]) == "query_113")
      modifier = new Accept_Query_113(pattern_size);
    else if (std::string(args[2]) == "query_114")
      modifier = new Accept_Query_114(pattern_size);
    else if (std::string(args[2]) == "query_115")
      // query 115 and query 5 shall both return an empty std::set.
      modifier = new Accept_Query_5(pattern_size);
    else if (std::string(args[2]) == "query_116")
      // query 116 and 113 shall return the same result
      modifier = new Accept_Query_113(pattern_size);
    else if (std::string(args[2]) == "query_117")
      modifier = new Accept_Query_117(pattern_size);
    else if (std::string(args[2]) == "query_118")
      modifier = new Accept_Query_118(pattern_size);
    else if (std::string(args[2]) == "query_119")
      modifier = new Accept_Query_119(pattern_size);
    else if (std::string(args[2]) == "query_120")
      modifier = new Accept_Query_118(pattern_size);
    else if (std::string(args[2]) == "query_121")
      modifier = new Accept_Query_118(pattern_size);
    else if (std::string(args[2]) == "query_122")
      modifier = new Accept_Query_122(pattern_size);
    else if (std::string(args[2]) == "query_123")
      modifier = new Accept_Query_122(pattern_size);
    else if (std::string(args[2]) == "query_124")
      modifier = new Accept_Query_124(pattern_size);
    else if (std::string(args[2]) == "query_125")
      modifier = new Accept_Query_125(pattern_size);
    else if (std::string(args[2]) == "query_126")
      modifier = new Accept_Query_125(pattern_size);
    else if (std::string(args[2]) == "query_127")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 11);
    else if (std::string(args[2]) == "query_128")
      modifier = new Accept_Around_1(pattern_size, 200.1, false, 7);
    else if (std::string(args[2]) == "query_129")
      modifier = new Accept_Query_37(pattern_size);
    else if (std::string(args[2]) == "query_130")
      modifier = new Accept_Query_38(pattern_size);
    else if (std::string(args[2]) == "query_131")
      modifier = new Accept_Query_39(pattern_size);
    else if (std::string(args[2]) == "query_132")
      modifier = new Accept_Query_40(pattern_size);
    else if (std::string(args[2]) == "query_133")
      modifier = new Accept_Recurse_23(pattern_size);
    else if (std::string(args[2]) == "query_134")
      modifier = new Accept_Recurse_25(pattern_size);
    else if (std::string(args[2]) == "query_135")
      modifier = new Accept_Recurse_27(pattern_size);
    else if (std::string(args[2]) == "query_136")
      modifier = new Accept_Recurse_24(pattern_size);
    else if (std::string(args[2]) == "query_137")
      modifier = new Accept_Recurse_26(pattern_size);
    else if (std::string(args[2]) == "query_138")
      modifier = new Accept_Recurse_28(pattern_size);
    else if (std::string(args[2]) == "query_139")
      modifier = new Accept_Query_139(pattern_size);
    else if (std::string(args[2]) == "query_140")
      modifier = new Accept_Query_140(pattern_size);
    else if (std::string(args[2]) == "query_141")
      modifier = new Accept_Query_141(pattern_size);
    else if (std::string(args[2]) == "query_142")
      modifier = new Accept_Query_142(pattern_size);
    else if (std::string(args[2]) == "query_143")
      modifier = new Accept_Query_143(pattern_size);
    else if (std::string(args[2]) == "query_144")
      modifier = new Accept_Query_144(pattern_size);
    else if (std::string(args[2]) == "query_145")
      modifier = new Accept_Query_145(pattern_size);
    else if (std::string(args[2]) == "query_146")
      modifier = new Accept_Query_146(pattern_size);
    else if (std::string(args[2]) == "query_147")
      modifier = new Accept_Query_147(pattern_size);
    else if (std::string(args[2]) == "query_148")
      modifier = new Accept_Query_148(pattern_size);
    else if (std::string(args[2]) == "query_149")
      modifier = new Accept_Query_149(pattern_size);
    else if (std::string(args[2]) == "query_150")
      modifier = new Accept_Query_150(pattern_size);
    else if (std::string(args[2]) == "query_151")
      modifier = new Accept_Query_151(pattern_size);
    else if (std::string(args[2]) == "query_152")
      modifier = new Accept_Query_28(pattern_size);
    else if (std::string(args[2]) == "query_153")
      modifier = new Accept_Query_153(pattern_size);
    else if (std::string(args[2]) == "query_154")
      modifier = new Accept_Query_154(pattern_size);
    else if (std::string(args[2]) == "query_155")
      modifier = new Accept_Query_155(pattern_size);
    else if (std::string(args[2]) == "query_156")
      modifier = new Accept_Query_156(pattern_size);
    else if (std::string(args[2]) == "query_157")
      modifier = new Accept_Query_157(pattern_size);
    else if (std::string(args[2]) == "query_158")
      modifier = new Accept_Query_158(pattern_size);
    else if (std::string(args[2]) == "query_159")
      modifier = new Accept_Query_159(pattern_size);
    else if (std::string(args[2]) == "query_160")
      modifier = new Accept_Query_160(pattern_size);
    else if (std::string(args[2]) == "query_161")
      modifier = new Accept_Query_161(pattern_size);
    else if (std::string(args[2]) == "query_162")
      modifier = new Accept_Query_162(pattern_size);
    else if (std::string(args[2]) == "query_163")
      modifier = new Accept_Query_163(pattern_size);
    else if (std::string(args[2]) == "foreach_1")
      modifier = new Accept_Foreach_1(pattern_size);
    else if (std::string(args[2]) == "foreach_2")
      modifier = new Accept_Foreach_2(pattern_size);
    else if (std::string(args[2]) == "foreach_3")
      // query 1 and 3 shall both return the same set.
      modifier = new Accept_Foreach_1(pattern_size);
    else if (std::string(args[2]) == "foreach_4")
      // query 1 and 4 shall both return the same set.
      modifier = new Accept_Foreach_1(pattern_size);
    else if (std::string(args[2]) == "union_1")
      modifier = new Accept_Union_1(pattern_size);
    else if (std::string(args[2]) == "union_2")
      modifier = new Accept_Union_2(pattern_size);
    else if (std::string(args[2]) == "union_3")
      // query 1 and 3 shall return the same result
      modifier = new Accept_Union_1(pattern_size);
    else if (std::string(args[2]) == "union_4")
      modifier = new Accept_Union_4(pattern_size);
    else if (std::string(args[2]) == "union_5")
      modifier = new Accept_Union_5(pattern_size);
    else if (std::string(args[2]) == "union_6")
      modifier = new Accept_Union_6(pattern_size);
    else if (std::string(args[2]) == "difference_1")
      modifier = new Accept_Difference_1(pattern_size);
    else if (std::string(args[2]) == "difference_2")
      modifier = new Accept_Difference_2(pattern_size);
    else if (std::string(args[2]) == "difference_3")
      modifier = new Accept_Difference_1(pattern_size);
    else if (std::string(args[2]) == "difference_4")
      modifier = new Accept_Difference_4(pattern_size);
    else if (std::string(args[2]) == "difference_5")
      modifier = new Accept_Difference_5(pattern_size);
    else if (std::string(args[2]) == "difference_6")
      modifier = new Accept_Difference_6(pattern_size);
    else if (std::string(args[2]) == "complete_1")
      modifier = new Accept_Complete_1(pattern_size);
    else if (std::string(args[2]) == "complete_2")
      modifier = new Accept_Complete_1(pattern_size);
    else if (std::string(args[2]) == "complete_3")
      modifier = new Accept_Complete_1(pattern_size);
    else if (std::string(args[2]) == "complete_4")
      modifier = new Accept_Complete_1(pattern_size);
    else if (std::string(args[2]) == "complete_5")
      modifier = new Accept_Complete_1(pattern_size);
    else if (std::string(args[2]) == "complete_6")
      modifier = new Accept_Complete_6(pattern_size, false);
    else if (std::string(args[2]) == "complete_7")
      modifier = new Accept_Complete_7(pattern_size, 0);
    else if (std::string(args[2]) == "if_1")
      modifier = new Accept_If(2);
    else if (std::string(args[2]) == "if_2")
      modifier = new Accept_If(1);
    else if (std::string(args[2]) == "if_3")
      modifier = new Accept_If(2);
    else if (std::string(args[2]) == "if_4")
      modifier = new Accept_If(3);
    else if (std::string(args[2]) == "if_5")
      modifier = new Accept_If(2);
    else if (std::string(args[2]) == "if_6")
      modifier = new Accept_If(4);
    else if (std::string(args[2]) == "around_1")
      modifier = new Accept_Around_1(pattern_size, 20.01);
    else if (std::string(args[2]) == "around_2")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (std::string(args[2]) == "around_3")
      modifier = new Accept_Around_1(pattern_size, 2001);
    else if (std::string(args[2]) == "around_4")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (std::string(args[2]) == "around_5")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (std::string(args[2]) == "around_6")
      modifier = new Accept_Around_1(pattern_size, 200.1, true);
    else if (std::string(args[2]) == "around_7")
      modifier = new Accept_Around_1(pattern_size, 20.01);
    else if (std::string(args[2]) == "around_8")
      modifier = new Accept_Around_1(pattern_size, 200.1);
    else if (std::string(args[2]) == "around_9")
      modifier = new Accept_Around_1(pattern_size, 2001);
    else if (std::string(args[2]) == "around_10" || std::string(args[2]) == "around_14"
        || std::string(args[2]) == "around_15" || std::string(args[2]) == "around_16" )
      modifier = new Accept_Around_10(pattern_size);
    else if (std::string(args[2]) == "around_11" || std::string(args[2]) == "around_12"
        || std::string(args[2]) == "around_13")
      modifier = new Accept_Around_11(pattern_size);
    else if (std::string(args[2]) == "around_17")
      modifier = new Accept_Around_17(pattern_size);
    else if (std::string(args[2]) == "around_18")
      modifier = new Accept_Around_18(pattern_size);
    else if (std::string(args[2]) == "around_19")
      modifier = new Accept_Around_19(pattern_size);
    else if (std::string(args[2]) == "polygon_query_1")
      modifier = new Accept_Polygon_1(pattern_size);
    else if (std::string(args[2]) == "polygon_query_2")
      modifier = new Accept_Polygon_2(pattern_size);
    else if (std::string(args[2]) == "polygon_query_3")
      modifier = new Accept_Polygon_3(pattern_size);
    else if (std::string(args[2]) == "polygon_query_4")
      modifier = new Accept_Polygon_4(pattern_size);
    else if (std::string(args[2]) == "polygon_query_5")
      modifier = new Accept_Polygon_5(pattern_size);
    else if (std::string(args[2]) == "diff_do")
      modifier = new Accept_All;
    else if (std::string(args[2]) == "diff_compare")
      modifier = new Accept_All_But_5(pattern_size);
    else if (std::string(args[2]) == "")
      modifier = new Accept_All;
    else
      // return an empty osm file otherwise
      modifier = new Accept_Bbox_Query_5(pattern_size);
  }
  else
    modifier = new Accept_All;

  if (argc > 3)
    global_node_offset = atoll(args[3]);

  if ((argc > 2) && (std::string(args[2]) == "diff_do"))
  {
    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    std::cout<<
    "  <node id=\""<<5 + global_node_offset<<"\" lat=\"-10.0\" lon=\"-15.0\">\n"
    "    <tag k=\"node_key_5\" v=\"node_value_5\"/>\n"
    "  </node>\n";

    std::cout<<"<delete>\n";
    create_node_test_pattern(-10.0, 80.0, -15.0, 105.0, 1, pattern_size, global_node_offset, modifier);
    std::cout<<"</delete>\n";

    std::cout<<
    "  <way id=\"5\">\n"
    "    <nd ref=\""<<15 + global_node_offset<<"\"/>\n"
    "    <nd ref=\""<<16 + global_node_offset<<"\"/>\n"
    "    <tag k=\"way_key_5\" v=\"way_value_5\"/>\n"
    "  </way>\n";

    std::cout<<"<delete>\n";
    create_way_test_pattern(1, pattern_size, global_node_offset, modifier);
    std::cout<<"</delete>\n";

    std::cout<<
    "  <relation id=\"5\">\n"
    "    <member type=\"node\" ref=\""<<15 + global_node_offset<<"\" role=\"three\"/>\n"
    "    <member type=\"node\" ref=\""<<16 + global_node_offset<<"\" role=\"zero\"/>\n"
    "    <tag k=\"relation_key_5\" v=\"relation_value_5\"/>\n"
    "  </relation>\n";

    std::cout<<"<delete>\n";
    create_relation_test_pattern(1, pattern_size, global_node_offset, modifier);
    std::cout<<"</delete>\n";

    std::cout<<"</osm>\n";
  }
  else if ((argc > 2) && (std::string(args[2]) == "diff_compare"))
  {
    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(47.9, 48.1, -0.2, 0.2, 2, pattern_size, global_node_offset, modifier);
    create_node_test_pattern(30.0, 50.0, -120.0, -60.0, 3, pattern_size, global_node_offset, modifier);

    std::cout<<
    "  <node id=\""<<5 + global_node_offset<<"\" lat=\"-10.0000000\" lon=\"-15.0000000\">\n"
    "    <tag k=\"node_key_5\" v=\"node_value_5\"/>\n"
    "  </node>\n";

    create_way_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_way_test_pattern(2, pattern_size, global_node_offset, modifier);

    std::cout<<
    "  <way id=\"5\">\n"
    "    <nd ref=\""<<15 + global_node_offset<<"\"/>\n"
    "    <nd ref=\""<<16 + global_node_offset<<"\"/>\n"
    "    <tag k=\"way_key_5\" v=\"way_value_5\"/>\n"
    "  </way>\n";

    create_relation_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_relation_test_pattern(2, pattern_size, global_node_offset, modifier);

    std::cout<<
    "  <relation id=\"5\">\n"
    "    <member type=\"node\" ref=\""<<15 + global_node_offset<<"\" role=\"three\"/>\n"
    "    <member type=\"node\" ref=\""<<16 + global_node_offset<<"\" role=\"zero\"/>\n"
    "    <tag k=\"relation_key_5\" v=\"relation_value_5\"/>\n"
    "  </relation>\n";

    std::cout<<"</osm>\n";
  }
  else if ((argc > 2) && (std::string(args[2]).substr(0, 5) == "make_"))
  {
    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    if (std::string(args[2]) == "make_1")
      std::cout<<"  <one id=\"1\"/>\n";
    if (std::string(args[2]) == "make_2")
      std::cout<<"  <two id=\"1\"/>\n";
    if (std::string(args[2]) == "make_3")
      std::cout<<"  <into_target id=\"1\"/>\n";
    if (std::string(args[2]) == "make_4")
      std::cout<<
      "  <with-tags id=\"1\">\n"
      "    <tag k=\"single\" v=\"value\"/>\n"
      "  </with-tags>\n";
    if (std::string(args[2]) == "make_5")
      std::cout<<
      "  <with-tags id=\"1\">\n"
      "    <tag k=\"not\" v=\"in\"/>\n"
      "    <tag k=\"alphabetic\" v=\"order\"/>\n"
      "  </with-tags>\n";
    if (std::string(args[2]) == "make_6")
      std::cout<<
      "  <count-from-default id=\"2\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"deriveds\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"5\"/>\n"
      "    <tag k=\"members\" v=\"4\"/>\n"
      "  </count-from-default>\n";
    if (std::string(args[2]) == "make_7")
      std::cout<<
      "  <count-from-default id=\"2\">\n"
      "    <tag k=\"nodes\" v=\"0\"/>\n"
      "    <tag k=\"ways\" v=\"0\"/>\n"
      "    <tag k=\"relations\" v=\"0\"/>\n"
      "    <tag k=\"deriveds\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"0\"/>\n"
      "    <tag k=\"members\" v=\"0\"/>\n"
      "  </count-from-default>\n";
    if (std::string(args[2]) == "make_8")
      std::cout<<
      "  <count-from-foo id=\"2\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"deriveds\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"5\"/>\n"
      "    <tag k=\"members\" v=\"4\"/>\n"
      "  </count-from-foo>\n";
    if (std::string(args[2]) == "make_9")
      std::cout<<
      "  <test-and id=\"1\">\n"
      "    <tag k=\"and\" v=\"0\"/>\n"
      "  </test-and>\n";
    if (std::string(args[2]) == "make_10")
      std::cout<<
      "  <test-and id=\"1\">\n"
      "    <tag k=\"and\" v=\"0\"/>\n"
      "  </test-and>\n";
    if (std::string(args[2]) == "make_11")
      std::cout<<
      "  <test-and id=\"1\">\n"
      "    <tag k=\"and\" v=\"1\"/>\n"
      "  </test-and>\n";
    if (std::string(args[2]) == "make_12")
      std::cout<<
      "  <test-and id=\"1\">\n"
      "    <tag k=\"and\" v=\"0\"/>\n"
      "  </test-and>\n";
    if (std::string(args[2]) == "make_13")
      std::cout<<
      "  <test-or id=\"1\">\n"
      "    <tag k=\"or\" v=\"1\"/>\n"
      "  </test-or>\n";
    if (std::string(args[2]) == "make_14")
      std::cout<<
      "  <test-or id=\"1\">\n"
      "    <tag k=\"or\" v=\"1\"/>\n"
      "  </test-or>\n";
    if (std::string(args[2]) == "make_15")
      std::cout<<
      "  <test-or id=\"1\">\n"
      "    <tag k=\"or\" v=\"1\"/>\n"
      "  </test-or>\n";
    if (std::string(args[2]) == "make_16")
      std::cout<<
      "  <test-or id=\"1\">\n"
      "    <tag k=\"or\" v=\"0\"/>\n"
      "  </test-or>\n";
    if (std::string(args[2]) == "make_17")
      std::cout<<
      "  <test-not id=\"1\">\n"
      "    <tag k=\"not\" v=\"1\"/>\n"
      "  </test-not>\n";
    if (std::string(args[2]) == "make_18")
      std::cout<<
      "  <test-not id=\"1\">\n"
      "    <tag k=\"not\" v=\"0\"/>\n"
      "  </test-not>\n";
    if (std::string(args[2]) == "make_19")
      std::cout<<
      "  <test-not id=\"1\">\n"
      "    <tag k=\"not\" v=\"0\"/>\n"
      "  </test-not>\n";
    if (std::string(args[2]) == "make_20")
      std::cout<<
      "  <test-not id=\"1\">\n"
      "    <tag k=\"not\" v=\"1\"/>\n"
      "  </test-not>\n";
    if (std::string(args[2]) == "make_21")
      std::cout<<
      "  <test-equal id=\"1\">\n"
      "    <tag k=\"equal\" v=\"1\"/>\n"
      "  </test-equal>\n";
    if (std::string(args[2]) == "make_22")
      std::cout<<
      "  <test-equal id=\"1\">\n"
      "    <tag k=\"equal\" v=\"1\"/>\n"
      "  </test-equal>\n";
    if (std::string(args[2]) == "make_23")
      std::cout<<
      "  <test-equal id=\"1\">\n"
      "    <tag k=\"equal\" v=\"1\"/>\n"
      "  </test-equal>\n";
    if (std::string(args[2]) == "make_24")
      std::cout<<
      "  <test-equal id=\"1\">\n"
      "    <tag k=\"equal\" v=\"0\"/>\n"
      "  </test-equal>\n";
    if (std::string(args[2]) == "make_25")
      std::cout<<
      "  <test-equal id=\"1\">\n"
      "    <tag k=\"equal\" v=\"0\"/>\n"
      "  </test-equal>\n";
    if (std::string(args[2]) == "make_26")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"1\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_27")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"1\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_28")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"1\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_29")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"0\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_30")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"1\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_31")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"0\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_32")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"1\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_33")
      std::cout<<
      "  <test-less id=\"1\">\n"
      "    <tag k=\"less\" v=\"1\"/>\n"
      "  </test-less>\n";
    if (std::string(args[2]) == "make_34")
      std::cout<<
      "  <test-less-equal id=\"1\">\n"
      "    <tag k=\"less-equal\" v=\"1\"/>\n"
      "  </test-less-equal>\n";
    if (std::string(args[2]) == "make_35")
      std::cout<<
      "  <test-less-equal id=\"1\">\n"
      "    <tag k=\"less-equal\" v=\"0\"/>\n"
      "  </test-less-equal>\n";
    if (std::string(args[2]) == "make_36")
      std::cout<<
      "  <test-less-equal id=\"1\">\n"
      "    <tag k=\"less-equal\" v=\"1\"/>\n"
      "  </test-less-equal>\n";
    if (std::string(args[2]) == "make_37")
      std::cout<<
      "  <test-less-equal id=\"1\">\n"
      "    <tag k=\"less-equal\" v=\"1\"/>\n"
      "  </test-less-equal>\n";
    if (std::string(args[2]) == "make_38")
      std::cout<<
      "  <test-less-equal id=\"1\">\n"
      "    <tag k=\"less-equal\" v=\"0\"/>\n"
      "  </test-less-equal>\n";
    if (std::string(args[2]) == "make_39")
      std::cout<<
      "  <test-less-equal id=\"1\">\n"
      "    <tag k=\"less-equal\" v=\"1\"/>\n"
      "  </test-less-equal>\n";
    if (std::string(args[2]) == "make_40")
      std::cout<<
      "  <test-greater id=\"1\">\n"
      "    <tag k=\"greater\" v=\"0\"/>\n"
      "  </test-greater>\n";
    if (std::string(args[2]) == "make_41")
      std::cout<<
      "  <test-greater id=\"1\">\n"
      "    <tag k=\"greater\" v=\"1\"/>\n"
      "  </test-greater>\n";
    if (std::string(args[2]) == "make_42")
      std::cout<<
      "  <test-greater id=\"1\">\n"
      "    <tag k=\"greater\" v=\"0\"/>\n"
      "  </test-greater>\n";
    if (std::string(args[2]) == "make_43")
      std::cout<<
      "  <test-greater id=\"1\">\n"
      "    <tag k=\"greater\" v=\"0\"/>\n"
      "  </test-greater>\n";
    if (std::string(args[2]) == "make_44")
      std::cout<<
      "  <test-greater id=\"1\">\n"
      "    <tag k=\"greater\" v=\"0\"/>\n"
      "  </test-greater>\n";
    if (std::string(args[2]) == "make_45")
      std::cout<<
      "  <test-greater id=\"1\">\n"
      "    <tag k=\"greater\" v=\"1\"/>\n"
      "  </test-greater>\n";
    if (std::string(args[2]) == "make_46")
      std::cout<<
      "  <test-gr-equal id=\"1\">\n"
      "    <tag k=\"greater-equal\" v=\"0\"/>\n"
      "  </test-gr-equal>\n";
    if (std::string(args[2]) == "make_47")
      std::cout<<
      "  <test-gr-equal id=\"1\">\n"
      "    <tag k=\"greater-equal\" v=\"1\"/>\n"
      "  </test-gr-equal>\n";
    if (std::string(args[2]) == "make_48")
      std::cout<<
      "  <test-gr-equal id=\"1\">\n"
      "    <tag k=\"greater-equal\" v=\"1\"/>\n"
      "  </test-gr-equal>\n";
    if (std::string(args[2]) == "make_49")
      std::cout<<
      "  <test-gr-equal id=\"1\">\n"
      "    <tag k=\"greater-equal\" v=\"0\"/>\n"
      "  </test-gr-equal>\n";
    if (std::string(args[2]) == "make_50")
      std::cout<<
      "  <test-gr-equal id=\"1\">\n"
      "    <tag k=\"greater-equal\" v=\"1\"/>\n"
      "  </test-gr-equal>\n";
    if (std::string(args[2]) == "make_51")
      std::cout<<
      "  <test-gr-equal id=\"1\">\n"
      "    <tag k=\"greater-equal\" v=\"1\"/>\n"
      "  </test-gr-equal>\n";
    if (std::string(args[2]) == "make_52")
      std::cout<<
      "  <test-plus id=\"1\">\n"
      "    <tag k=\"sum\" v=\"9\"/>\n"
      "  </test-plus>\n";
    if (std::string(args[2]) == "make_53")
      std::cout<<
      "  <test-plus id=\"1\">\n"
      "    <tag k=\"sum\" v=\"10 \"/>\n"
      "  </test-plus>\n";
    if (std::string(args[2]) == "make_54")
      std::cout<<
      "  <test-plus id=\"1\">\n"
      "    <tag k=\"sum\" v=\"11\"/>\n"
      "  </test-plus>\n";
    if (std::string(args[2]) == "make_55")
      std::cout<<
      "  <test-plus id=\"1\">\n"
      "    <tag k=\"sum\" v=\" 12_\"/>\n"
      "  </test-plus>\n";
    if (std::string(args[2]) == "make_56")
      std::cout<<
      "  <test-plus id=\"1\">\n"
      "    <tag k=\"sum\" v=\"100000000000000001\"/>\n"
      "  </test-plus>\n";
    if (std::string(args[2]) == "make_57")
      std::cout<<
      "  <test-times id=\"1\">\n"
      "    <tag k=\"product\" v=\"13\"/>\n"
      "  </test-times>\n";
    if (std::string(args[2]) == "make_58")
      std::cout<<
      "  <test-times id=\"1\">\n"
      "    <tag k=\"product\" v=\"NaN\"/>\n"
      "  </test-times>\n";
    if (std::string(args[2]) == "make_59")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"difference\" v=\"-3\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_60")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"difference\" v=\"NaN\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_61")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"difference\" v=\"1\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_62")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"negation\" v=\"-3.14\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_63")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"negation\" v=\"3\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_64")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"negation\" v=\"-100000000000000000\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_65")
      std::cout<<
      "  <test-minus id=\"1\">\n"
      "    <tag k=\"negation\" v=\"NaN\"/>\n"
      "  </test-minus>\n";
    if (std::string(args[2]) == "make_66")
      std::cout<<
      "  <test-divided id=\"1\">\n"
      "    <tag k=\"quotient\" v=\"0.88888888888889\"/>\n"
      "  </test-divided>\n";
    if (std::string(args[2]) == "make_67")
      std::cout<<
      "  <test-divided id=\"1\">\n"
      "    <tag k=\"quotient\" v=\"NaN\"/>\n"
      "  </test-divided>\n";
    if (std::string(args[2]) == "make_68" || std::string(args[2]) == "make_69")
      std::cout<<
      "  <union-value id=\"1\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"way_key\" v=\"way_few\"/>\n"
      "    <tag k=\"relation_key\" v=\"relation_few\"/>\n"
      "    <tag k=\"unused_key\" v=\"\"/>\n"
      "  </union-value>\n";
    if (std::string(args[2]) == "make_70" || std::string(args[2]) == "make_71")
      std::cout<<
      "  <min-value id=\"1\">\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "    <tag k=\"unused_key_7\" v=\"\"/>\n"
      "  </min-value>\n";
    if (std::string(args[2]) == "make_72" || std::string(args[2]) == "make_73")
      std::cout<<
      "  <max-value id=\"1\">\n"
      "    <tag k=\"node_key_7\" v=\"node_value_2\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "    <tag k=\"unused_key_7\" v=\"\"/>\n"
      "  </max-value>\n";
    if (std::string(args[2]) == "make_74" || std::string(args[2]) == "make_75")
      std::cout<<
      "  <value-set id=\"1\">\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1;node_value_2\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "    <tag k=\"unused_key_7\" v=\"\"/>\n"
      "  </value-set>\n";
    if (std::string(args[2]) == "make_76")
      std::cout<<
      "  <id-and-type id=\"1\">\n"
      "    <tag k=\"id\" v=\"1;"<<global_node_offset + 1<<";"<<global_node_offset + 2<<"\"/>\n"
      "    <tag k=\"type\" v=\"node;relation;way\"/>\n"
      "    <tag k=\"is_closed\" v=\"0;NaW\"/>\n"
      "  </id-and-type>\n";
    if (std::string(args[2]) == "make_77")
      std::cout<<
      "  <key-id id=\"42\"/>\n";
    if (std::string(args[2]) == "make_78")
      std::cout<<
      "  <key-id id=\""<<global_node_offset + 2<<"\"/>\n";
    if (std::string(args[2]) == "make_79")
      std::cout<<
      "  <test-number id=\"1\">\n"
      "    <tag k=\"nan\" v=\"NaN\"/>\n"
      "    <tag k=\"three\" v=\"3\"/>\n"
      "    <tag k=\"one_trillion\" v=\"1000000000000\"/>\n"
      "    <tag k=\"minus_fourty-two\" v=\"-42\"/>\n"
      "    <tag k=\"is_nan\" v=\"0\"/>\n"
      "    <tag k=\"is_three\" v=\"1\"/>\n"
      "    <tag k=\"is_one_trillion\" v=\"1\"/>\n"
      "    <tag k=\"is_minus_fourty-two\" v=\"1\"/>\n"
      "    <tag k=\"empty_isnt_num\" v=\"0\"/>\n"
      "  </test-number>\n";
    if (std::string(args[2]) == "make_80")
      std::cout<<
      "  <test-date id=\"1\">\n"
      "    <tag k=\"year_only\" v=\"2006\"/>\n"
      "    <tag k=\"year_month_day\" v=\"2012.587890625\"/>\n"
      "    <tag k=\"full_iso\" v=\"2013.0671679527\"/>\n"
      "    <tag k=\"nonsense\" v=\"NaD\"/>\n"
      "    <tag k=\"is_year\" v=\"1\"/>\n"
      "    <tag k=\"is_year_month_day\" v=\"1\"/>\n"
      "    <tag k=\"is_full_iso\" v=\"1\"/>\n"
      "    <tag k=\"is_nonsense\" v=\"0\"/>\n"
      "    <tag k=\"empty_isnt_date\" v=\"0\"/>\n"
      "  </test-date>\n";
    if (std::string(args[2]) == "make_81")
      std::cout<<
      "  <test-suffix id=\"1\">\n"
      "    <tag k=\"empty\" v=\"\"/>\n"
      "    <tag k=\"pure\" v=\"\"/>\n"
      "    <tag k=\"unit\" v=\"m\"/>\n"
      "    <tag k=\"whitespace\" v=\"\"/>\n"
      "    <tag k=\"whitespace_and_unit\" v=\"m/s\"/>\n"
      "    <tag k=\"second_number\" v=\"2\"/>\n"
      "    <tag k=\"comma_sep_number\" v=\",14\"/>\n"
      "    <tag k=\"possible_exp\" v=\"e\"/>\n"
      "    <tag k=\"misc\" v=\"3/4\"/>\n"
      "  </test-suffix>\n";
    if (std::string(args[2]) == "make_82")
      std::cout<<
      "  <test-lrs id=\"1\">\n"
      "    <tag k=\"lrs_in_1_positive\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_1_negative\" v=\"0\"/>\n"
      "    <tag k=\"lrs_in_2_positive_1\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_2_positive_2\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_2_negative\" v=\"0\"/>\n"
      "    <tag k=\"lrs_in_3_positive_1\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_3_positive_2\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_3_positive_3\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_3_negative\" v=\"0\"/>\n"
      "    <tag k=\"lrs_in_space_1\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_space_2\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_space_3\" v=\"1\"/>\n"
      "    <tag k=\"lrs_in_space_4\" v=\"0\"/>\n"
      "    <tag k=\"lrs_isect_self_11\" v=\"foo\"/>\n"
      "    <tag k=\"lrs_isect_self_12\" v=\" \"/>\n"
      "    <tag k=\"lrs_isect_self_21\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_isect_self_22\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_isect_self_23\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_isect_self_24\" v=\"a\"/>\n"
      "    <tag k=\"lrs_isect_self_31\" v=\"a;b;c\"/>\n"
      "    <tag k=\"lrs_isect_self_32\" v=\"a;b;c\"/>\n"
      "    <tag k=\"lrs_isect_self_33\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_isect_zero_1\" v=\"\"/>\n"
      "    <tag k=\"lrs_isect_zero_2\" v=\"\"/>\n"
      "    <tag k=\"lrs_isect_one_1\" v=\"a\"/>\n"
      "    <tag k=\"lrs_isect_one_2\" v=\"b\"/>\n"
      "    <tag k=\"lrs_isect_two_1\" v=\"b;c\"/>\n"
      "    <tag k=\"lrs_isect_two_2\" v=\" ;a\"/>\n"
      "    <tag k=\"lrs_union_self_11\" v=\"foo\"/>\n"
      "    <tag k=\"lrs_union_self_12\" v=\" \"/>\n"
      "    <tag k=\"lrs_union_self_21\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_union_self_22\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_union_self_23\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_union_self_24\" v=\"a\"/>\n"
      "    <tag k=\"lrs_union_self_31\" v=\"a;b;c\"/>\n"
      "    <tag k=\"lrs_union_self_32\" v=\"a;b;c\"/>\n"
      "    <tag k=\"lrs_union_self_33\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_union_disjoint_1\" v=\"a;b;c;d\"/>\n"
      "    <tag k=\"lrs_union_zero_1\" v=\"a;b\"/>\n"
      "    <tag k=\"lrs_union_one_1\" v=\"a;b;c;d;e\"/>\n"
      "    <tag k=\"lrs_union_one_2\" v=\"a;b;c;d;e\"/>\n"
      "    <tag k=\"lrs_union_two_1\" v=\"a;b;c;d\"/>\n"
      "    <tag k=\"lrs_union_two_2\" v=\" ;a;b\"/>\n"
      "    <tag k=\"lrs_max_one_1\" v=\"foo\"/>\n"
      "    <tag k=\"lrs_max_one_2\" v=\"foo\"/>\n"
      "    <tag k=\"lrs_max_one_3\" v=\"1000\"/>\n"
      "    <tag k=\"lrs_max_one_4\" v=\"100000000000000001\"/>\n"
      "    <tag k=\"lrs_max_two_1\" v=\"b\"/>\n"
      "    <tag k=\"lrs_max_two_2\" v=\"10\"/>\n"
      "    <tag k=\"lrs_max_two_3\" v=\"0.1\"/>\n"
      "    <tag k=\"lrs_max_two_4\" v=\"9 bis\"/>\n"
      "    <tag k=\"lrs_max_three_1\" v=\"c\"/>\n"
      "    <tag k=\"lrs_max_three_2\" v=\"9\"/>\n"
      "    <tag k=\"lrs_min_one_1\" v=\"foo\"/>\n"
      "    <tag k=\"lrs_min_one_2\" v=\"foo\"/>\n"
      "    <tag k=\"lrs_min_one_3\" v=\"1000\"/>\n"
      "    <tag k=\"lrs_min_one_4\" v=\"100000000000000001\"/>\n"
      "    <tag k=\"lrs_min_two_1\" v=\"a\"/>\n"
      "    <tag k=\"lrs_min_two_2\" v=\"9\"/>\n"
      "    <tag k=\"lrs_min_two_3\" v=\"0.01\"/>\n"
      "    <tag k=\"lrs_min_two_4\" v=\"10 bis\"/>\n"
      "    <tag k=\"lrs_min_three_1\" v=\"a\"/>\n"
      "    <tag k=\"lrs_min_three_2\" v=\"10\"/>\n"
      "  </test-lrs>\n";
    if (std::string(args[2]) == "make_83")
      std::cout<<
      "  <test-ternary id=\"1\">\n"
      "    <tag k=\"ternary\" v=\"A\"/>\n"
      "  </test-ternary>\n";
    if (std::string(args[2]) == "make_84")
      std::cout<<
      "  <test-ternary id=\"1\">\n"
      "    <tag k=\"ternary\" v=\"A\"/>\n"
      "  </test-ternary>\n";
    if (std::string(args[2]) == "make_85")
      std::cout<<
      "  <test-ternary id=\"1\">\n"
      "    <tag k=\"ternary\" v=\"B\"/>\n"
      "  </test-ternary>\n";
    if (std::string(args[2]) == "make_86")
      std::cout<<
      "  <test-ternary id=\"1\">\n"
      "    <tag k=\"ternary\" v=\"B\"/>\n"
      "  </test-ternary>\n";
    if (std::string(args[2]) == "make_87" || std::string(args[2]) == "make_88")
      std::cout<<
      "  <generic-key id=\"1\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1;node_value_2\"/>\n"
      "    <tag k=\"relation_key\" v=\"relation_few\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "    <tag k=\"way_key\" v=\"way_few\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "  </generic-key>\n";
    if (std::string(args[2]) == "make_89")
      std::cout<<
      "  <generic-key id=\"1\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1;node_value_2\"/>\n"
      "    <tag k=\"relation_key\" v=\"relation_few\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "  </generic-key>\n";
    if (std::string(args[2]) == "make_90")
      std::cout<<
      "  <generic-key id=\"1\">\n"
      "    <tag k=\"node_key\" v=\"...\"/>\n"
      "    <tag k=\"node_key_7\" v=\"...\"/>\n"
      "    <tag k=\"relation_key\" v=\"...\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"...\"/>\n"
      "    <tag k=\"way_key\" v=\"...\"/>\n"
      "    <tag k=\"way_key_7\" v=\"...\"/>\n"
      "  </generic-key>\n";
    if (std::string(args[2]) == "make_91")
      std::cout<<
      "  <make-point id=\"1\">\n"
      "    <point lat=\"51.2500000\" lon=\"7.1500000\"/>\n"
      "  </make-point>\n";
    if (std::string(args[2]) == "make_92")
      std::cout<<
      "  <make-point-invalid-north id=\"1\"/>\n";
    if (std::string(args[2]) == "make_93")
      std::cout<<
      "  <make-point-invalid-east id=\"1\"/>\n";
    if (std::string(args[2]) == "make_94")
      std::cout<<
      "  <make-point-dependencies id=\"1\">\n"
      "    <point lat=\"51.2500000\" lon=\""<<std::fixed<<std::setprecision(7)
          <<.1/pattern_size/9.*cos((51.+(pattern_size < 70 ? 2.5 : 1.5)/pattern_size)/90.*acos(0))<<"\"/>\n"
      "  </make-point-dependencies>\n";
    if (std::string(args[2]) == "make_95")
      std::cout<<
      "  <make-linestring id=\"1\">\n"
      "  </make-linestring>\n";
    if (std::string(args[2]) == "make_96")
      std::cout<<
      "  <make-linestring id=\"1\">\n"
      "    <vertex lat=\"51.1000000\" lon=\"7.1000000\"/>\n"
      "  </make-linestring>\n";
    if (std::string(args[2]) == "make_97")
      std::cout<<
      "  <make-linestring id=\"1\">\n"
      "    <vertex lat=\"51.1000000\" lon=\"7.1000000\"/>\n"
      "    <vertex lat=\"51.2000000\" lon=\"7.2000000\"/>\n"
      "  </make-linestring>\n";
    if (std::string(args[2]) == "make_98")
      std::cout<<
      "  <make-linestring id=\"1\">\n"
      "    <vertex lat=\"51.1000000\" lon=\"7.1000000\"/>\n"
      "    <vertex lat=\"51.2000000\" lon=\"7.2000000\"/>\n"
      "    <vertex lat=\"51.3000000\" lon=\"7.3000000\"/>\n"
      "  </make-linestring>\n";
    if (std::string(args[2]) == "make_99")
      std::cout<<
      "  <make-linestring id=\"1\">\n"
      "    <vertex lat=\"51.1000000\" lon=\"7.1000000\"/>\n"
      "    <vertex lat=\"51.2000000\" lon=\"7.2000000\"/>\n"
      "    <vertex lat=\"51.3000000\" lon=\"7.3000000\"/>\n"
      "    <vertex lat=\"51.4000000\" lon=\""<<std::fixed<<std::setprecision(7)
          <<.1/pattern_size/9.*cos((51.+(pattern_size < 70 ? 2.5 : 1.5)/pattern_size)/90.*acos(0))<<"\"/>\n"
      "  </make-linestring>\n";
    if (std::string(args[2]) == "make_100")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_101")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_102")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_103")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"41.0100000\" lon=\"0.0100000\"/>\n"
      "      <vertex lat=\"41.0133336\" lon=\"0.0133325\"/>\n"
      "      <vertex lat=\"41.0166671\" lon=\"0.0166653\"/>\n"
      "      <vertex lat=\"41.0200004\" lon=\"0.0199985\"/>\n"
      "      <vertex lat=\"41.0233337\" lon=\"0.0233320\"/>\n"
      "      <vertex lat=\"41.0266669\" lon=\"0.0266658\"/>\n"
      "      <vertex lat=\"41.0300000\" lon=\"0.0300000\"/>\n"
      "      <vertex lat=\"41.0333334\" lon=\"0.0266670\"/>\n"
      "      <vertex lat=\"41.0366668\" lon=\"0.0233337\"/>\n"
      "      <vertex lat=\"41.0400000\" lon=\"0.0200000\"/>\n"
      "      <vertex lat=\"41.0357143\" lon=\"0.0185709\"/>\n"
      "      <vertex lat=\"41.0314287\" lon=\"0.0171419\"/>\n"
      "      <vertex lat=\"41.0271430\" lon=\"0.0157132\"/>\n"
      "      <vertex lat=\"41.0228572\" lon=\"0.0142846\"/>\n"
      "      <vertex lat=\"41.0185715\" lon=\"0.0128562\"/>\n"
      "      <vertex lat=\"41.0142858\" lon=\"0.0114280\"/>\n"
      "      <vertex lat=\"41.0100000\" lon=\"0.0100000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_104" || std::string(args[2]) == "make_105"
        || std::string(args[2]) == "make_106")
    {
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"41.0100000\" lon=\"0.0100000\"/>\n"
      "      <vertex lat=\"41.0066668\" lon=\"0.0066663\"/>\n"
      "      <vertex lat=\"41.0033334\" lon=\"0.0033330\"/>\n"
      "      <vertex lat=\"41.0000000\" lon=\"0.0000000\"/>\n"
      "      <vertex lat=\"41.0000000\" lon=\""<<std::fixed<<std::setprecision(7)
          <<.1/pattern_size/9.*cos((51.+(pattern_size < 70 ? 2.5 : 1.5)/pattern_size)/90.*acos(0))<<"\"/>\n"
      "      <vertex lat=\"41.0000000\" lon=\"0.0010000\"/>\n"
      "      <vertex lat=\"41.0037504\" lon=\"0.0046236\"/>\n"
      "      <vertex lat=\"41.0075007\" lon=\"0.0082475\"/>\n"
      "      <vertex lat=\"41.0112509\" lon=\"0.0118719\"/>\n"
      "      <vertex lat=\"41.0150009\" lon=\"0.0154967\"/>\n"
      "      <vertex lat=\"41.0187509\" lon=\"0.0191219\"/>\n"
      "      <vertex lat=\"41.0225007\" lon=\"0.0227475\"/>\n"
      "      <vertex lat=\"41.0262504\" lon=\"0.0263736\"/>\n"
      "      <vertex lat=\"41.0300000\" lon=\"0.0300000\"/>\n"
      "      <vertex lat=\"41.0333334\" lon=\"0.0266670\"/>\n"
      "      <vertex lat=\"41.0366668\" lon=\"0.0233337\"/>\n"
      "      <vertex lat=\"41.0400000\" lon=\"0.0200000\"/>\n"
      "      <vertex lat=\"41.0357143\" lon=\"0.0185709\"/>\n"
      "      <vertex lat=\"41.0314287\" lon=\"0.0171419\"/>\n"
      "      <vertex lat=\"41.0271430\" lon=\"0.0157132\"/>\n"
      "      <vertex lat=\"41.0228572\" lon=\"0.0142846\"/>\n"
      "      <vertex lat=\"41.0185715\" lon=\"0.0128562\"/>\n"
      "      <vertex lat=\"41.0142858\" lon=\"0.0114280\"/>\n"
      "      <vertex lat=\"41.0100000\" lon=\"0.0100000\"/>\n"
      "    </linestring>\n";
      if (std::string(args[2]) == "make_106")
        std::cout<<
        "    <linestring>\n"
        "      <vertex lat=\"41.0310000\" lon=\"0.0200000\"/>\n"
        "      <vertex lat=\"41.0290000\" lon=\"0.0210000\"/>\n"
        "      <vertex lat=\"41.0290000\" lon=\"0.0190000\"/>\n"
        "      <vertex lat=\"41.0310000\" lon=\"0.0200000\"/>\n"
        "    </linestring>\n";
      std::cout<<
      "  </make-polygon>\n";
    }
    if (std::string(args[2]) == "make_107")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"45.0000000\" lon=\"179.9900000\"/>\n"
      "      <vertex lat=\"44.9960000\" lon=\"179.9900000\"/>\n"
      "      <vertex lat=\"44.9920000\" lon=\"179.9900000\"/>\n"
      "      <vertex lat=\"44.9880000\" lon=\"179.9900000\"/>\n"
      "      <vertex lat=\"44.9840000\" lon=\"179.9900000\"/>\n"
      "      <vertex lat=\"44.9800000\" lon=\"179.9900000\"/>\n"
      "      <vertex lat=\"44.9825003\" lon=\"179.9949993\"/>\n"
      "      <vertex lat=\"44.9850004\" lon=\"179.9999991\"/>\n"
      "      <vertex lat=\"44.9875003\" lon=\"-179.9950007\"/>\n"
      "      <vertex lat=\"44.9900000\" lon=\"-179.9900000\"/>\n"
      "      <vertex lat=\"44.9925003\" lon=\"-179.9949993\"/>\n"
      "      <vertex lat=\"44.9950004\" lon=\"-179.9999991\"/>\n"
      "      <vertex lat=\"44.9975003\" lon=\"179.9950007\"/>\n"
      "      <vertex lat=\"45.0000000\" lon=\"179.9900000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_108")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0040000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0060000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"2\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.1060000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.1040000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.1060000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"3\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.2070000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.2060000\" lon=\"7.0070000\"/>\n"
      "      <vertex lat=\"51.2070000\" lon=\"7.0060000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.2040000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.2030000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.2040000\" lon=\"7.0030000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"4\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0040000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"5\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.1060000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.1040000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.1060000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"6\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.2070000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.2060000\" lon=\"7.0070000\"/>\n"
      "      <vertex lat=\"51.2070000\" lon=\"7.0060000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.2040000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.2030000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"7\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0040000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"8\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.1040000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.1060000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.1050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.1040000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"9\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.2030000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.2040000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.2030000\" lon=\"7.0040000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.2060000\" lon=\"7.0070000\"/>\n"
      "      <vertex lat=\"51.2070000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.2050000\" lon=\"7.0050000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"10\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0045000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0055000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0070000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0060000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0045000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0055000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0045000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"11\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0055000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0050000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0070000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0065000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0080000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0070000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0055000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0060000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0065000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0055000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_109")
      std::cout<<
      "  <make-polygon id=\"1\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"2\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"3\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"4\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"5\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"6\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"7\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"8\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"9\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"10\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"11\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"12\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"13\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"14\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"15\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"16\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"17\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"18\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"19\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"20\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"21\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"22\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"23\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n"
      "  <make-polygon id=\"24\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0030000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0040000\"/>\n"
      "      <vertex lat=\"51.0060000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0020000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "    </linestring>\n"
      "  </make-polygon>\n";
    if (std::string(args[2]) == "make_110")
      std::cout<<
      "  <geometry id=\"2\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(8, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(14, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 8, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 9, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <group>\n"
      "        <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <vertex "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "        <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <point "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "        <vertex "<<lat_lon_of_node(pattern_size + 3, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <point "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <vertex "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "        <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <point "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <vertex "<<lat_lon_of_node(2*pattern_size + 4, pattern_size)<<"/>\n"
      "        <vertex "<<lat_lon_of_node(3*pattern_size + 4, pattern_size)<<"/>\n"
      "      </group>\n"
      "      <group>\n"
      "        <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "      </group>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex lat=\"51.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"51.0050000\" lon=\"7.0015000\"/>\n"
      "    </group>\n"
      "  </geometry>\n";
    if (std::string(args[2]) == "make_111")
      std::cout<<
      "  <center id=\"1\">\n"
      "    <point lat=\"48.0000000\" lon=\"11.0000000\"/>\n"
      "  </center>\n";
    if (std::string(args[2]) == "make_112")
      std::cout<<
      "  <center id=\"1\">\n"
      "    <point lat=\"42.0000000\" lon=\"180.0000000\"/>\n"
      "  </center>\n";
    if (std::string(args[2]) == "make_113")
      std::cout<<
      "  <trace id=\"2\">\n"
      "    <group>\n"
      "      <point lat=\"51.5010000\" lon=\"7.0005000\"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "make_114")
      std::cout<<
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <point lat=\"51.5010000\" lon=\"7.0005000\"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point lat=\"51.5010000\" lon=\"7.0035000\"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "make_115")
      std::cout<<
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <point lat=\"51.5010000\" lon=\"7.0005000\"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "make_116")
      std::cout<<
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <vertex lat=\"52.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"52.0050000\" lon=\"7.0025000\"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "make_117")
      std::cout<<
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <vertex lat=\"52.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"52.0050000\" lon=\"7.0025000\"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex lat=\"52.0040000\" lon=\"7.0015000\"/>\n"
      "      <vertex lat=\"52.0050000\" lon=\"7.0015000\"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "make_118")
      std::cout<<
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <vertex lat=\"52.0040000\" lon=\"7.0025000\"/>\n"
      "      <vertex lat=\"52.0050000\" lon=\"7.0025000\"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "make_119")
      std::cout<<
      "  <hull id=\"1\"/>\n";
    if (std::string(args[2]) == "make_120")
      std::cout<<
      "  <hull id=\"2\">\n"
      "    <point lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_121")
      std::cout<<
      "  <hull id=\"2\">\n"
      "    <vertex lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "    <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "    <vertex lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_122")
      std::cout<<
      "  <hull id=\"2\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_123")
      std::cout<<
      "  <hull id=\"3\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "      <vertex lat=\"50.9993000\" lon=\"6.9993000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_124")
      std::cout<<
      "  <hull id=\"5\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "      <vertex lat=\"50.9993000\" lon=\"6.9993000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"50.9994000\" lon=\"7.0005000\"/>\n"
      "      <vertex lat=\"50.9995000\" lon=\"7.0006000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_125")
      std::cout<<
      "  <hull id=\"6\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9990000\"/>\n"
      "      <vertex lat=\"50.9993000\" lon=\"6.9993000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"50.9991000\" lon=\"7.0009000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_126")
      std::cout<<
      "  <hull id=\"7\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9989000\"/>\n"
      "      <vertex lat=\"50.9993000\" lon=\"6.9993000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"50.9991000\" lon=\"7.0009000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_127")
      std::cout<<
      "  <hull id=\"8\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"51.0005000\" lon=\"6.9989000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9989000\"/>\n"
      "      <vertex lat=\"50.9993000\" lon=\"6.9993000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"50.9991000\" lon=\"7.0009000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0010000\" lon=\"7.0000000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_128")
      std::cout<<
      "  <hull id=\"9\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0030000\" lon=\"6.9988000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"6.9989000\"/>\n"
      "      <vertex lat=\"50.9993000\" lon=\"6.9993000\"/>\n"
      "      <vertex lat=\"50.9990000\" lon=\"7.0000000\"/>\n"
      "      <vertex lat=\"50.9991000\" lon=\"7.0009000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"7.0010000\"/>\n"
      "      <vertex lat=\"51.0030000\" lon=\"6.9988000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_129")
      std::cout<<
      "  <hull id=\"2\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0000000\" lon=\"179.9980000\"/>\n"
      "      <vertex lat=\"50.9980000\" lon=\"179.9990000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"180.0000000\"/>\n"
      "      <vertex lat=\"51.0020000\" lon=\"-179.9990000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"179.9980000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_130")
      std::cout<<
      "  <hull id=\"2\">\n"
      "    <linestring>\n"
      "      <vertex lat=\"51.0020000\" lon=\"-179.9990000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"179.9980000\"/>\n"
      "      <vertex lat=\"50.9980000\" lon=\"179.9990000\"/>\n"
      "      <vertex lat=\"51.0000000\" lon=\"-179.9980000\"/>\n"
      "      <vertex lat=\"51.0020000\" lon=\"-179.9990000\"/>\n"
      "    </linestring>\n"
      "  </hull>\n";
    if (std::string(args[2]) == "make_131")
      std::cout<<
      "  <test-ternary id=\"1\">\n"
      "    <point lat=\"51.5000000\" lon=\"8.0000000\"/>\n"
      "  </test-ternary>\n";
    if (std::string(args[2]) == "make_132")
      std::cout<<
      "  <test-ternary id=\"1\">\n"
      "    <point lat=\"52.5000000\" lon=\"10.0000000\"/>\n"
      "  </test-ternary>\n";

    std::cout<<"</osm>\n";
  }
  else if ((argc > 2) && (std::string(args[2]).substr(0, 8) == "convert_"))
  {
    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    if (std::string(args[2]) == "convert_1" || std::string(args[2]) == "convert_3")
      std::cout<<
      "  <just-copy id=\"2\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1\"/>\n"
      "  </just-copy>\n"
      "  <just-copy id=\"3\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"node_key_7\" v=\"node_value_2\"/>\n"
      "  </just-copy>\n"
      "  <just-copy id=\"4\">\n"
      "    <tag k=\"way_key\" v=\"way_few\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "  </just-copy>\n"
      "  <just-copy id=\"5\">\n"
      "    <tag k=\"relation_key\" v=\"relation_few\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "  </just-copy>\n"
      "  <just-copy id=\"6\">\n"
      "    <tag k=\"number\" v=\"1000\"/>\n"
      "  </just-copy>\n";
    if (std::string(args[2]) == "convert_4" || std::string(args[2]) == "convert_5")
      std::cout<<
      "  <into id=\"2\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1\"/>\n"
      "  </into>\n"
      "  <into id=\"3\">\n"
      "    <tag k=\"node_key\" v=\"node_few\"/>\n"
      "    <tag k=\"node_key_7\" v=\"node_value_2\"/>\n"
      "  </into>\n"
      "  <into id=\"4\">\n"
      "    <tag k=\"way_key\" v=\"way_few\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "  </into>\n"
      "  <into id=\"5\">\n"
      "    <tag k=\"relation_key\" v=\"relation_few\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "  </into>\n"
      "  <into id=\"6\">\n"
      "    <tag k=\"number\" v=\"1000\"/>\n"
      "  </into>\n";
    if (std::string(args[2]) == "convert_6")
      std::cout<<
      "  <rewrite id=\"2\">\n"
      "    <tag k=\"node_key_7\" v=\"node_value_1\"/>\n"
      "    <tag k=\"extra_key\" v=\"extra_value\"/>\n"
      "  </rewrite>\n"
      "  <rewrite id=\"3\">\n"
      "    <tag k=\"node_key_7\" v=\"node_value_2\"/>\n"
      "    <tag k=\"extra_key\" v=\"extra_value\"/>\n"
      "  </rewrite>\n"
      "  <rewrite id=\"4\">\n"
      "    <tag k=\"way_key\" v=\"way_few\"/>\n"
      "    <tag k=\"way_key_7\" v=\"way_value_1\"/>\n"
      "    <tag k=\"extra_key\" v=\"extra_value\"/>\n"
      "  </rewrite>\n"
      "  <rewrite id=\"5\">\n"
      "    <tag k=\"relation_key\" v=\"relation_few\"/>\n"
      "    <tag k=\"relation_key_7\" v=\"relation_value_1\"/>\n"
      "    <tag k=\"extra_key\" v=\"extra_value\"/>\n"
      "  </rewrite>\n"
      "  <rewrite id=\"6\">\n"
      "    <tag k=\"extra_key\" v=\"extra_value\"/>\n"
      "  </rewrite>\n";
    if (std::string(args[2]) == "convert_7")
      std::cout<<
      "  <count-from-default id=\"1\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"1\"/>\n"
      "    <tag k=\"members\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members\" v=\"0\"/>\n"
      "    <tag k=\"by_role\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"0\"/>\n"
      "    <tag k=\"members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"0\"/>\n"
      "  </count-from-default>\n"
      "  <count-from-default id=\"2\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"2\"/>\n"
      "    <tag k=\"members\" v=\"2\"/>\n"
      "    <tag k=\"distinct_members\" v=\"2\"/>\n"
      "    <tag k=\"by_role\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"0\"/>\n"
      "    <tag k=\"members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"0\"/>\n"
      "  </count-from-default>\n"
      "  <count-from-default id=\"3\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"2\"/>\n"
      "    <tag k=\"members\" v=\"2\"/>\n"
      "    <tag k=\"distinct_members\" v=\"2\"/>\n"
      "    <tag k=\"by_role\" v=\"1\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"1\"/>\n"
      "    <tag k=\"members_with_type\" v=\"2\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"2\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"1\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"1\"/>\n"
      "  </count-from-default>\n";
    if (std::string(args[2]) == "convert_8")
      std::cout<<
      "  <is-tag id=\"2\">\n"
      "    <tag k=\"node_key\" v=\"1\"/>\n"
      "    <tag k=\"way_key\" v=\"0\"/>\n"
      "    <tag k=\"relation_key\" v=\"0\"/>\n"
      "    <tag k=\"number\" v=\"0\"/>\n"
      "  </is-tag>\n"
      "  <is-tag id=\"3\">\n"
      "    <tag k=\"node_key\" v=\"1\"/>\n"
      "    <tag k=\"way_key\" v=\"0\"/>\n"
      "    <tag k=\"relation_key\" v=\"0\"/>\n"
      "    <tag k=\"number\" v=\"0\"/>\n"
      "  </is-tag>\n"
      "  <is-tag id=\"4\">\n"
      "    <tag k=\"node_key\" v=\"0\"/>\n"
      "    <tag k=\"way_key\" v=\"1\"/>\n"
      "    <tag k=\"relation_key\" v=\"0\"/>\n"
      "    <tag k=\"number\" v=\"0\"/>\n"
      "  </is-tag>\n"
      "  <is-tag id=\"5\">\n"
      "    <tag k=\"node_key\" v=\"0\"/>\n"
      "    <tag k=\"way_key\" v=\"0\"/>\n"
      "    <tag k=\"relation_key\" v=\"1\"/>\n"
      "    <tag k=\"number\" v=\"0\"/>\n"
      "  </is-tag>\n"
      "  <is-tag id=\"6\">\n"
      "    <tag k=\"node_key\" v=\"0\"/>\n"
      "    <tag k=\"way_key\" v=\"0\"/>\n"
      "    <tag k=\"relation_key\" v=\"0\"/>\n"
      "    <tag k=\"number\" v=\"1\"/>\n"
      "  </is-tag>\n";
    if (std::string(args[2]) == "convert_9")
      std::cout<<
      "  <count-from-default id=\"1\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"1\"/>\n"
      "    <tag k=\"members\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members\" v=\"0\"/>\n"
      "    <tag k=\"by_role\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"0\"/>\n"
      "    <tag k=\"members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"0\"/>\n"
      "  </count-from-default>\n"
      "  <count-from-default id=\"2\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"2\"/>\n"
      "    <tag k=\"members\" v=\"2\"/>\n"
      "    <tag k=\"distinct_members\" v=\"2\"/>\n"
      "    <tag k=\"by_role\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"0\"/>\n"
      "    <tag k=\"members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"0\"/>\n"
      "  </count-from-default>\n"
      "  <count-from-default id=\"3\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"2\"/>\n"
      "    <tag k=\"members\" v=\"5\"/>\n"
      "    <tag k=\"distinct_members\" v=\"4\"/>\n"
      "    <tag k=\"by_role\" v=\"3\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"2\"/>\n"
      "    <tag k=\"members_with_type\" v=\"5\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"4\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"3\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"2\"/>\n"
      "  </count-from-default>\n";
    if (std::string(args[2]) == "convert_10")
      std::cout<<
      "  <count-from-default id=\"1\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"2\"/>\n"
      "    <tag k=\"members\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members\" v=\"0\"/>\n"
      "    <tag k=\"by_role\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"0\"/>\n"
      "    <tag k=\"members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"0\"/>\n"
      "  </count-from-default>\n"
      "  <count-from-default id=\"2\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"3\"/>\n"
      "    <tag k=\"members\" v=\"2\"/>\n"
      "    <tag k=\"distinct_members\" v=\"2\"/>\n"
      "    <tag k=\"by_role\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"0\"/>\n"
      "    <tag k=\"members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"0\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"0\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"0\"/>\n"
      "  </count-from-default>\n"
      "  <count-from-default id=\"3\">\n"
      "    <tag k=\"nodes\" v=\"1\"/>\n"
      "    <tag k=\"ways\" v=\"1\"/>\n"
      "    <tag k=\"relations\" v=\"1\"/>\n"
      "    <tag k=\"tags\" v=\"3\"/>\n"
      "    <tag k=\"members\" v=\"11\"/>\n"
      "    <tag k=\"distinct_members\" v=\"10\"/>\n"
      "    <tag k=\"by_role\" v=\"3\"/>\n"
      "    <tag k=\"distinct_by_role\" v=\"2\"/>\n"
      "    <tag k=\"members_with_type\" v=\"5\"/>\n"
      "    <tag k=\"distinct_members_with_type\" v=\"4\"/>\n"
      "    <tag k=\"by_role_with_type\" v=\"3\"/>\n"
      "    <tag k=\"distinct_by_role_with_type\" v=\"2\"/>\n"
      "  </count-from-default>\n";
    if (std::string(args[2]) == "convert_11")
      std::cout<<
      "  <geometry id=\"2\">\n"
      "    <point "<<lat_lon_of_node(8, pattern_size)<<"/>\n"
      "  </geometry>\n"
      "  <geometry id=\"3\">\n"
      "    <point "<<lat_lon_of_node(14, pattern_size)<<"/>\n"
      "  </geometry>\n"
      "  <geometry id=\"4\">\n"
      "    <vertex "<<lat_lon_of_node(pattern_size + 8, pattern_size)<<"/>\n"
      "    <vertex "<<lat_lon_of_node(pattern_size + 9, pattern_size)<<"/>\n"
      "  </geometry>\n"
      "  <geometry id=\"5\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 3, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(2*pattern_size + 4, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(3*pattern_size + 4, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </geometry>\n"
      "  <geometry id=\"6\"/>\n";
    if (std::string(args[2]) == "convert_12")
      std::cout<<
      "  <trace id=\"2\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n"
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(3, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n"
      "  <trace id=\"4\">\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 3, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n"
      "  <trace id=\"5\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n"
      "  <trace id=\"6\">\n"
      "  </trace>\n";
    if (std::string(args[2]) == "convert_13")
      std::cout<<
      "  <trace id=\"1\">\n"
      "  </trace>\n"
      "  <trace id=\"2\">\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 3, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(2*pattern_size + 4, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(3*pattern_size + 4, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n"
      "  <trace id=\"3\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 3, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <vertex "<<lat_lon_of_node(2*pattern_size + 4, pattern_size)<<"/>\n"
      "      <vertex "<<lat_lon_of_node(3*pattern_size + 4, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n"
      "  <trace id=\"4\">\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(2, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 1, pattern_size)<<"/>\n"
      "    </group>\n"
      "    <group>\n"
      "      <point "<<lat_lon_of_node(pattern_size + 2, pattern_size)<<"/>\n"
      "    </group>\n"
      "  </trace>\n";
    if (std::string(args[2]) == "convert_14")
      std::cout<<
      "  <lat-lon id=\"2\">\n"
      "    <tag k=\"lat\" v=\""<<std::fixed<<std::setprecision(7)<<(51. + .5/pattern_size)<<"\"/>\n"
      "    <tag k=\"lon\" v=\""<<std::fixed<<std::setprecision(7)<<(7. + 6.5/pattern_size)<<"\"/>\n"
      "  </lat-lon>\n"
      "  <lat-lon id=\"3\">\n"
      "    <tag k=\"lat\" v=\""<<std::fixed<<std::setprecision(7)<<(51. + .5/pattern_size)<<"\"/>\n"
      "    <tag k=\"lon\" v=\""<<std::fixed<<std::setprecision(7)<<(7. + 13.5/pattern_size)<<"\"/>\n"
      "  </lat-lon>\n"
      "  <lat-lon id=\"4\">\n"
      "    <tag k=\"lat\" v=\""<<std::fixed<<std::setprecision(7)<<(51. + 1.5/pattern_size)<<"\"/>\n"
      "    <tag k=\"lon\" v=\""<<std::fixed<<std::setprecision(7)<<(7. + 7./pattern_size)<<"\"/>\n"
      "  </lat-lon>\n"
      "  <lat-lon id=\"5\">\n"
      "    <tag k=\"lat\" v=\"51.5000000\"/>\n"
      "    <tag k=\"lon\" v=\"7.5000000\"/>\n"
      "  </lat-lon>\n"
      "  <lat-lon id=\"6\">\n"
      "    <tag k=\"lat\" v=\"NaN\"/>\n"
      "    <tag k=\"lon\" v=\"NaN\"/>\n"
      "  </lat-lon>\n"
      "  <lat-lon id=\"8\">\n"
      "    <tag k=\"lat\" v=\"-15.0000000\"/>\n"
      "    <tag k=\"lon\" v=\"5.5500000\"/>\n"
      "  </lat-lon>\n";

    std::cout<<"</osm>\n";
  }
  else if ((argc > 2) && (std::string(args[2]) == "complete_6"))
  {
    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
    create_way_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_relation_test_pattern(0, pattern_size, global_node_offset, modifier);

    delete modifier;
    modifier = new Accept_Complete_6(pattern_size, true);

    create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
    create_way_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_relation_test_pattern(0, pattern_size, global_node_offset, modifier);

    delete modifier;
    modifier = new Accept_Complete_6(pattern_size, true);

    create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
    create_way_test_pattern(0, pattern_size, global_node_offset, modifier);
    create_relation_test_pattern(0, pattern_size, global_node_offset, modifier);

    std::cout<<"</osm>\n";
  }
  else if ((argc > 2) && (std::string(args[2]) == "complete_7"))
  {
    std::cout<<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm>\n";

    for (uint i = 1; i <= 18; ++i)
    {
      create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, global_node_offset, modifier);
      create_way_test_pattern(0, pattern_size, global_node_offset, modifier);
      create_relation_test_pattern(0, pattern_size, global_node_offset, modifier);

      delete modifier;
      modifier = new Accept_Complete_7(pattern_size, i);
    }

    std::cout<<"</osm>\n";
  }
  else
  {
    std::cout<<
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

    std::cout<<"</osm>\n";
  }
}
