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

#include "index_computations.h"

#include <iomanip>
#include <iostream>
#include <set>
#include <vector>


void cout_ranges(const Ranges< Uint31_Index >& result)
{
  std::cout<<"0x__80: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0xff) == 0x80)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x__40: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x7f) == 0x40)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x__20: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x3f) == 0x20)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x__10: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x1f) == 0x10)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x___8: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0xf) == 0x8)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x___4: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x7) == 0x4)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x___2: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x3) == 0x2)
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"0x___1: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x1))
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<'\n';

  std::cout<<"plain: ";
  for (auto it = result.begin(); it != result.end(); ++it)
  {
    if (!(it->first.val() & 0x80000000))
      std::cout<<std::hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  std::cout<<"\n\n";
}


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute\n";
    return 0;
  }
  std::string test_to_execute = args[1];

  if ((test_to_execute == "1") || (test_to_execute == ""))
  {
    std::cout<<"Test ll_upper:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
      std::cout<<std::hex<<ll_upper(lat, 0)<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
      std::cout<<std::hex<<ll_upper(lat, 0)<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
      std::cout<<std::hex<<ll_upper(lat, 0)<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
      std::cout<<std::hex<<ll_upper(lat, 0)<<' ';
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
      std::cout<<std::hex<<ll_upper(0, lon)<<' ';
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
      std::cout<<std::hex<<ll_upper(0, lon)<<' ';
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
      std::cout<<std::hex<<ll_upper(0, lon)<<' ';
    std::cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
      std::cout<<std::hex<<ll_upper(0, lon)<<' ';
    std::cout<<'\n';
  }

  if ((test_to_execute == "2") || (test_to_execute == ""))
  {
    std::cout<<"\nTest upper_ilat:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
      std::cout<<std::hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
      std::cout<<std::hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
      std::cout<<std::hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
      std::cout<<std::hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    std::cout<<'\n';

    std::cout<<"\nTest upper_ilon:\n";

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
      std::cout<<std::hex<<upper_ilon(ll_upper(0, lon))<<' ';
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
      std::cout<<std::hex<<upper_ilon(ll_upper(0, lon))<<' ';
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
      std::cout<<std::hex<<upper_ilon(ll_upper(0, lon))<<' ';
    std::cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
      std::cout<<std::hex<<upper_ilon(ll_upper(0, lon))<<' ';
    std::cout<<'\n';
  }

  if ((test_to_execute == "3") || (test_to_execute == ""))
  {
    std::cout<<"\nTest calc_index with 1 entry:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    std::cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    std::cout<<'\n';

    std::cout<<"\nTest calc_index with 2 entries:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';

    std::cout<<"\nTest calc_index with already cumulated entries:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      std::cout<<std::hex<<calc_index(std::vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    std::cout<<'\n';

    std::cout<<"\nTest calc_index with an already cumulated and another entry:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
    {
      std::vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      std::cout<<std::hex<<calc_index(idxs)<<' ';
    }
    std::cout<<'\n';
  }

  if ((test_to_execute == "4") || (test_to_execute == ""))
  {
    std::cout<<"\nTest calc_node_children:\n";

    {
      std::vector< uint32 > idxs(2, 0x3f3f3f3f);
      std::vector< Uint32_Index > result = calc_node_children(idxs);
      for (std::vector< Uint32_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint32_Index > result = calc_node_children(std::vector< uint32 >(1, 0x40848400));
      for (std::vector< Uint32_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint32_Index > result = calc_node_children(std::vector< uint32 >(1, 0xc0848401));
      for (std::vector< Uint32_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint32_Index > result = calc_node_children(std::vector< uint32 >(1, 0xc0848402));
      for (std::vector< Uint32_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint32_Index > result = calc_node_children(std::vector< uint32 >(1, 0xc0848404));
      for (std::vector< Uint32_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    // Disabled due to their size.
/*    {
      std::vector< uint32 > result = calc_node_children(std::vector< uint32 >(1, 0xc0848008));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< uint32 > result = calc_node_children(std::vector< uint32 >(1, 0xc0840010));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< uint32 > result = calc_node_children(std::vector< uint32 >(1, 0xc0800020));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< uint32 > result = calc_node_children(std::vector< uint32 >(1, 0xc0000040));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }*/
  }

  if ((test_to_execute == "5") || (test_to_execute == ""))
  {
    std::cout<<"\nTest calc_children:\n";

    {
      std::vector< uint32 > idxs(2, 0x3f3f3f3f);
      std::vector< Uint31_Index > result = calc_children(idxs);
      for (std::vector< Uint31_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint31_Index > result = calc_children(std::vector< uint32 >(1, 0x40848400));
      for (std::vector< Uint31_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint31_Index > result = calc_children(std::vector< uint32 >(1, 0xc0848401));
      for (std::vector< Uint31_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint31_Index > result = calc_children(std::vector< uint32 >(1, 0xc0848402));
      for (std::vector< Uint31_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
    {
      std::vector< Uint31_Index > result = calc_children(std::vector< uint32 >(1, 0xc0848404));
      for (std::vector< Uint31_Index >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<it->val()<<' ';
      std::cout<<'\n';
    }
  }

  if ((test_to_execute == "6") || (test_to_execute == ""))
  {
    std::cout<<"\nTest calc_parents:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
    for (uint32 lon = 0; (uint32)lon < (uint32)0x80000000; lon += 0x10000000)
    {
      std::cout<<std::hex<<lon<<' ';
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >
          (1, ll_upper(0, (int32)lon) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    for (uint32 lon = 0x80000000; (uint32)lon != (uint32)0; lon += 0x10000000)
    {
      std::cout<<std::hex<<lon<<' ';
      std::vector< uint32 > result = calc_parents(std::vector< uint32 >
      (1, ll_upper(0, (int32)lon) ^ 0x40000000));
      for (std::vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	std::cout<<std::hex<<*it<<' ';
      std::cout<<'\n';
    }
    std::cout<<'\n';
  }

  if ((test_to_execute == "7") || (test_to_execute == ""))
  {
    std::cout<<"\nTest calc_parents(ranges):\n";

    for (uint32 lat = 0x40000000; lat <= 0x40100000; lat += 0x10000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    for (uint32 lat = 0x40000000; lat <= 0x41000000; lat += 0x100000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    for (uint32 lat = 0x40000000; lat <= 0x50000000; lat += 0x1000000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    for (uint32 lat = 0x40000000; lat < 0x80000000; lat += 0x10000000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';

    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(0x40000000, lon) ^ 0x40000000,
			     (ll_upper(0x40000000, lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(0x40000000, lon) ^ 0x40000000,
			     (ll_upper(0x40000000, lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair(ll_upper(0x40000000, lon) ^ 0x40000000,
			     (ll_upper(0x40000000, lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    for (uint32 lon = 0; (uint32)lon < (uint32)0x80000000; lon += 0x10000000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40000000ul, (int32)lon) ^ 0x40000000,
	   (ll_upper(0x40000000ul, (int32)lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    for (uint32 lon = 0x80000000; (uint32)lon != (uint32)0; lon += 0x10000000)
    {
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40000000ul, (int32)lon) ^ 0x40000000,
	   (ll_upper(0x40000000ul, (int32)lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 1\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40010000, 0) ^ 0x40000000, (ll_upper(0x40010000, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 2\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40020000, 0) ^ 0x40000000, (ll_upper(0x40030000, 0x10000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 3\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40040000, 0) ^ 0x40000000, (ll_upper(0x40070000, 0x30000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 4\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40080000, 0) ^ 0x40000000, (ll_upper(0x400f0000, 0x70000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 5\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40100000, 0) ^ 0x40000000, (ll_upper(0x401f0000, 0xf0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 6\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40200000, 0) ^ 0x40000000, (ll_upper(0x403f0000, 0x1f0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 7\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40400000, 0) ^ 0x40000000, (ll_upper(0x407f0000, 0x3f0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 8\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x40800000, 0) ^ 0x40000000, (ll_upper(0x40ff0000, 0x7f0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 9\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x41000000, 0) ^ 0x40000000, (ll_upper(0x41ff0000, 0xff0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 10\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x42000000, 0) ^ 0x40000000, (ll_upper(0x43ff0000, 0x1ff0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 11\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x44000000, 0) ^ 0x40000000, (ll_upper(0x47ff0000, 0x3ff0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 12\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x48000000, 0) ^ 0x40000000, (ll_upper(0x4fff0000, 0x7ff0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 13\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x50000000, 0) ^ 0x40000000, (ll_upper(0x5fff0000, 0xfff0000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Two Size 1\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x3fff0000, 0x7fff0000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 1 + Size 2\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x3fff0000, 0x7fff0000) ^ 0x40000000,
	   (ll_upper(0x40010000, 0x10000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 1 + Size 2 + Size 1\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x3fff0000, 0x7fff0000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0x20000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 3 + Size 2 + Size 1\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x3ffc0000, 0x7ffc0000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0x20000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
    {
      std::cout<<"Size 4 + Size 2 + Size 1\n";
      std::set< std::pair< Uint32_Index, Uint32_Index > > input;
      input.insert(std::make_pair
          (ll_upper(0x3ff80000, 0x7ff80000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0x20000) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    std::cout<<'\n';
  }

  return 0;
}
