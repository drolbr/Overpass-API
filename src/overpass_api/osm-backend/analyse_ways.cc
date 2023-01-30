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
  if (rhs < lhs)
    std::cout<<"DEBUG\t"<<lhs<<'\t'<<rhs<<'\n';
  
  static uint32_t days_per_month[] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
  
  uint32_t full_days = (Timestamp::year(rhs) - Timestamp::year(lhs))*365
    + (Timestamp::year(rhs) - 2000)/4 - (Timestamp::year(lhs) - 2000)/4
    + days_per_month[Timestamp::month(rhs)] - days_per_month[Timestamp::month(lhs)]
    + Timestamp::day(rhs) - Timestamp::day(lhs);
  if ((Timestamp::year(lhs) - 2000)%4 == 0 && Timestamp::month(lhs) <= 2)
    ++full_days;
  if ((Timestamp::year(rhs) - 2000)%4 == 0 && Timestamp::month(rhs) <= 2)
    --full_days;
  return 60ull + Timestamp::second(rhs) - Timestamp::second(lhs)
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

      Block_Backend< Uint31_Index, Attic< Way_Delta > > delta_db
          (transaction.data_index(attic_settings().WAYS));
      auto delta_it = delta_db.flat_begin();
      
      std::vector< std::pair< uint32_t, uint32_t > > short_lifespan_stat(86400 * 365, { 0, 0 });
      std::map< uint64_t, std::pair< uint32_t, uint32_t > > long_lifespan_stat;
      
      uint64_t idx_cnt = 0;
      uint64_t meta_cnt = 0;
      uint64_t implicit_cnt = 0;
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
        
        if (Way::indicates_geometry(cur_idx))
        {
          std::vector< Attic< Way_Skeleton::Id_Type > > delta;
          while (!(delta_it == delta_db.flat_end()) && delta_it.index() < cur_idx)
          {
            //std::cout<<"DEBUG d 0x"<<std::hex<<delta_it.index().val()<<" 0x"<<cur_idx.val()<<'\n';
            ++delta_it;
          }
          while (!(delta_it == delta_db.flat_end()) && delta_it.index() == cur_idx)
          {
            if (multiidx_it == multiidx_ways.end() ||
                std::find(multiidx_it->second.begin(), multiidx_it->second.end(), delta_it.object().id)
                == multiidx_it->second.end())
              delta.push_back(Attic< Way_Skeleton::Id_Type >(
                delta_it.object().id, delta_it.object().timestamp));
            ++delta_it;
          }
          std::sort(delta.begin(), delta.end());
          
          auto d_it = delta.begin();
          for (uint64_t i = 0; i+1 < meta.size(); ++i)
          {
            while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) < Way_Skeleton::Id_Type(meta[i]))
            {
              std::cout<<"DEBUG e 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<' '<<meta[i].val()<<'\n';
              ++d_it;
            }
            uint lifespan = 0;
            bool detached_implicit = false;
            if (Way_Skeleton::Id_Type(meta[i]) == Way_Skeleton::Id_Type(meta[i+1]))
            {
              lifespan = calc_delta(meta[i].timestamp, meta[i+1].timestamp);
              while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i])
                  && d_it->timestamp < meta[i+1].timestamp)
              {
                ++implicit_cnt;
                if (calc_delta(meta[i].timestamp, d_it->timestamp) > 300
                    && calc_delta(d_it->timestamp, meta[i+1].timestamp) > 300)
                  detached_implicit = true;
//                 std::cout<<"DEBUG f 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<' '
//                     <<Timestamp(meta[i].timestamp).str()<<' '<<Timestamp(d_it->timestamp).str()<<' '
//                     <<Timestamp(meta[i+1].timestamp).str()<<'\n';
                ++d_it;
              }
              if (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i])
                  && d_it->timestamp == meta[i+1].timestamp)
                ++d_it;
            }
            else
            {
              lifespan = calc_delta(meta[i].timestamp, cur_date.timestamp);
              while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i]))
              {
                ++implicit_cnt;
                if (calc_delta(meta[i].timestamp, d_it->timestamp) > 300
                    && calc_delta(d_it->timestamp, cur_date.timestamp) > 300)
                  detached_implicit = true;
//                 std::cout<<"DEBUG g 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<' '
//                     <<Timestamp(meta[i].timestamp).str()<<' '<<Timestamp(d_it->timestamp).str()<<'\n';
                ++d_it;
              }
            }
            if (lifespan < short_lifespan_stat.size())
            {
              ++short_lifespan_stat[lifespan].first;
              if (detached_implicit)
                ++short_lifespan_stat[lifespan].second;
            }
            else
            {
              ++long_lifespan_stat[lifespan].first;
              if (detached_implicit)
                ++long_lifespan_stat[lifespan].second;
            }
          }
          if (!meta.empty())
          {
            uint lifespan = calc_delta(meta.back().timestamp, cur_date.timestamp);
            bool detached_implicit = false;
            while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta.back()))
            {
              ++implicit_cnt;
              if (calc_delta(meta.back().timestamp, d_it->timestamp) > 300
                  && calc_delta(d_it->timestamp, cur_date.timestamp) > 300)
                detached_implicit = true;
//               std::cout<<"DEBUG h 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<' '
//                   <<Timestamp(meta.back().timestamp).str()<<' '<<Timestamp(d_it->timestamp).str()<<'\n';
              ++d_it;
            }
            if (lifespan < short_lifespan_stat.size())
            {
              ++short_lifespan_stat[lifespan].first;
              if (detached_implicit)
                ++short_lifespan_stat[lifespan].second;
            }
            else
            {
              ++long_lifespan_stat[lifespan].first;
              if (detached_implicit)
                ++long_lifespan_stat[lifespan].second;
            }
          }
          while (d_it != delta.end())
          {
            //std::cout<<"DEBUG i 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<'\n';
            ++d_it;
          }
        }
        
//         for (uint64_t i = 0; i+1 < meta.size(); ++i)
//         {
//           uint lifespan = 0;
//           if (Way_Skeleton::Id_Type(meta[i]) == Way_Skeleton::Id_Type(meta[i+1]))
//           {
//             lifespan = calc_delta(meta[i].timestamp, meta[i+1].timestamp);
//             if (lifespan > 1000000000)
//               std::cout<<"DEBUG a 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta[i].val()<<' '<<lifespan<<' '<<meta[i].timestamp<<' '<<meta[i+1].timestamp<<'\n';
//           }
//           else
//           {
//             lifespan = calc_delta(meta[i].timestamp, cur_date.timestamp);
//             if (lifespan > 1000000000)
//               std::cout<<"DEBUG b 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta[i].val()<<' '<<lifespan<<' '<<meta[i].timestamp<<' '<<cur_date.timestamp<<'\n';
//           }
//           if (lifespan < short_lifespan_stat.size())
//             ++short_lifespan_stat[lifespan];
//           else
//             ++long_lifespan_stat[lifespan];
//         }
//         if (!meta.empty())
//         {
//           uint lifespan = calc_delta(meta.back().timestamp, cur_date.timestamp);
//           if (lifespan > 1000000000)
//             std::cout<<"DEBUG c 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta.back().val()<<' '<<lifespan<<' '<<meta.back().timestamp<<' '<<cur_date.timestamp<<'\n';
//           if (lifespan < short_lifespan_stat.size())
//             ++short_lifespan_stat[lifespan];
//           else
//             ++long_lifespan_stat[lifespan];
//         }
      }
      std::cerr<<"\n... "<<std::dec<<idx_cnt<<" indices and "<<meta_cnt<<" metas processed, "
          <<implicit_cnt<<" implicit versions found.\n";
      
      std::pair< uint32_t, uint32_t > partial_sum{ 0, 0 };
      for (uint64_t i = 0; i < short_lifespan_stat.size(); ++i)
      {
        partial_sum.first += short_lifespan_stat[i].first;
        partial_sum.second += short_lifespan_stat[i].second;
        if (short_lifespan_stat[i].first > 0)
          std::cout<<i<<'\t'<<short_lifespan_stat[i].first<<'\t'<<short_lifespan_stat[i].second<<'\t'<<partial_sum.first<<'\t'<<partial_sum.second<<'\n';
      }
      for (auto i : long_lifespan_stat)
      {
        partial_sum.first += i.second.first;
        partial_sum.second += i.second.second;
        std::cout<<i.first<<'\t'<<i.second.first<<'\t'<<i.second.second<<'\t'<<partial_sum.first<<'\t'<<partial_sum.second<<'\n';
      }
    }
  }
  catch (File_Error e)
  {
    std::cerr<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }

  return 0;
}
