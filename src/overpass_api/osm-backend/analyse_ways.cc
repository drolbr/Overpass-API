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



#include "../core/settings.h"
#include "../core/datatypes.h"
#include "../../template_db/block_backend.h"
#include "../../template_db/transaction.h"

#include <iostream>
#include <string>


uint64_t calc_delta(uint64_t lhs, uint64_t rhs)
{
  if (rhs > lhs)
    std::cout<<"DEBUG\t"<<lhs<<'\t'<<rhs<<'\n';
  
  static uint32_t days_per_month[] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
  
  uint full_days = (Timestamp::year(rhs) - Timestamp::year(lhs))*365
    + (Timestamp::year(rhs) - 2000)%4 - (Timestamp::year(lhs) - 2000)%4
    + days_per_month[Timestamp::month(rhs)] - days_per_month[Timestamp::month(lhs)]
    + Timestamp::day(rhs) - Timestamp::day(lhs);
  if ((Timestamp::year(rhs) - 2000)%4 == 0 && Timestamp::month(rhs) <= 2)
    --full_days;
  if ((Timestamp::year(lhs) - 2000)%4 == 0 && Timestamp::month(lhs) <= 2)
    ++full_days;
  return 60 + Timestamp::second(rhs) - Timestamp::second(lhs)
    + (60 + Timestamp::minute(rhs) - Timestamp::minute(lhs))*60
    + (24 + Timestamp::hour(rhs) - Timestamp::hour(lhs))*3600
    + full_days*86400 - (24*3600 + 60*60 + 60);
}


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" db_dir target\n";
    return 0;
  }

  std::string db_dir(args[1]);
  std::map< Uint31_Index, std::vector< Way_Skeleton::Id_Type > > multiidx_ways;
  Timestamp cur_date(2023, 1, 26, 21, 48, 34);

  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");

    {
      std::cerr<<"Read Way_Idx_List ...";
      Block_Backend< Way_Skeleton::Id_Type, Uint31_Index > db
          (transaction.data_index(attic_settings().WAY_IDX_LIST));
      for (Block_Backend< Way_Skeleton::Id_Type, Uint31_Index >::Flat_Iterator
            it(db.flat_begin()); !(it == db.flat_end()); ++it)
        multiidx_ways[it.object()].push_back(it.index());
      std::cerr<<" done.\n";
    }
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > cur_meta_db
          (transaction.data_index(meta_settings().WAYS_META));      
      auto cur_meta_it = cur_meta_db.flat_begin();
      
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_meta_db
          (transaction.data_index(attic_settings().WAYS_META));
      auto attic_meta_it = attic_meta_db.flat_begin();
      
      std::vector< uint64_t > short_lifespan_stat(86400 * 365, 0);
      std::map< uint64_t, uint64_t > long_lifespan_stat;
      
      uint64_t idx_cnt = 0;
      uint64_t meta_cnt = 0;
      while (!(cur_meta_it == cur_meta_db.flat_end()) || !(attic_meta_it == attic_meta_db.flat_end()))
      {
        Uint31_Index cur_idx = (
            cur_meta_it == cur_meta_db.flat_end() || 
                (!(attic_meta_it == attic_meta_db.flat_end()) && attic_meta_it.index() < cur_meta_it.index()) ?
          attic_meta_it.index() : cur_meta_it.index());
        
        auto multiidx_it = multiidx_ways.find(cur_idx);
        
        if (++idx_cnt % 65536 == 0)
          std::cerr<<" 0x"<<std::hex<<cur_idx.val();
        if (idx_cnt % 1048576 == 0)
          std::cerr<<'\n';
        
        std::vector< Attic< Way_Skeleton::Id_Type > > meta;
        while (!(cur_meta_it == cur_meta_db.flat_end()) && cur_meta_it.index() == cur_idx)
        {
          if (multiidx_it == multiidx_ways.end() ||
              std::find(multiidx_it->second.begin(), multiidx_it->second.end(), cur_meta_it.object().ref)
              == multiidx_it->second.end())
            meta.push_back(Attic< Way_Skeleton::Id_Type >(
                cur_meta_it.object().ref, cur_meta_it.object().timestamp));
          ++cur_meta_it;
        }
        
        //std::vector< Attic< Way_Skeleton::Id_Type > > attic_meta;
        while (!(attic_meta_it == attic_meta_db.flat_end()) && attic_meta_it.index() == cur_idx)
        {
          if (multiidx_it == multiidx_ways.end() ||
              std::find(multiidx_it->second.begin(), multiidx_it->second.end(), attic_meta_it.object().ref)
              == multiidx_it->second.end())
            meta.push_back(Attic< Way_Skeleton::Id_Type >(
              attic_meta_it.object().ref, attic_meta_it.object().timestamp));
          ++attic_meta_it;
        }
        meta_cnt += meta.size();
        std::sort(meta.begin(), meta.end());
        
        for (uint64_t i = 0; i+1 < meta.size(); ++i)
        {
          uint lifespan = 0;
          if (Way_Skeleton::Id_Type(meta[i]) == Way_Skeleton::Id_Type(meta[i+1]))
            lifespan = calc_delta(meta[i].timestamp, meta[i+1].timestamp);
          else
            lifespan = calc_delta(meta[i].timestamp, cur_date.timestamp);
          if (lifespan < short_lifespan_stat.size())
            ++short_lifespan_stat[lifespan];
          else
            ++long_lifespan_stat[lifespan];
        }
        if (!meta.empty())
        {
          uint lifespan = calc_delta(meta.back().timestamp, cur_date.timestamp);
          if (lifespan < short_lifespan_stat.size())
            ++short_lifespan_stat[lifespan];
          else
            ++long_lifespan_stat[lifespan];
        }

//         std::vector< Attic< Way_Skeleton::Id_Type > > cur_skel;
//         std::vector< Attic< Way_Skeleton::Id_Type > > attic_skel;
      }
      std::cerr<<"\n... "<<std::dec<<idx_cnt<<" indices and "<<meta_cnt<<" metas processed.\n";
      
      uint64_t partial_sum = 0;
      for (uint64_t i = 0; i < short_lifespan_stat.size(); ++i)
      {
        partial_sum += short_lifespan_stat[i];
        if (short_lifespan_stat[i] > 0)
          std::cout<<i<<'\t'<<short_lifespan_stat[i]<<'\t'<<partial_sum<<'\n';
      }
      for (auto i : long_lifespan_stat)
      {
        partial_sum += i.second;
        std::cout<<i.first<<'\t'<<i.second<<'\t'<<partial_sum<<'\n';
      }
    }
  }
  catch (File_Error e)
  {
    std::cerr<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }

  return 0;
}
