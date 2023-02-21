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
#include "../data/collect_items.h"
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


struct Id_In_Set_Predicate
{
  Id_In_Set_Predicate(const std::vector< Way_Skeleton::Id_Type >* ids_) : ids(ids_) {}

  bool match(const Way_Skeleton& obj) const
  { return ids && std::find(ids->begin(), ids->end(), obj.id) != ids->end(); }
  bool match(const Attic< Way_Skeleton >& obj) const
  { return ids && std::find(ids->begin(), ids->end(), obj.id) != ids->end(); }
  
private:
  const std::vector< Way_Skeleton::Id_Type >* ids;
};


struct Id_Not_In_Set_Predicate
{
  Id_Not_In_Set_Predicate(const std::vector< Way_Skeleton::Id_Type >* ids_) : ids(ids_) {}

  bool match(const Way_Skeleton& obj) const
  { return !ids || std::find(ids->begin(), ids->end(), obj.id) == ids->end(); }
  bool match(const Attic< Way_Skeleton >& obj) const
  { return !ids || std::find(ids->begin(), ids->end(), obj.id) == ids->end(); }
  
private:
  const std::vector< Way_Skeleton::Id_Type >* ids;
};


template < class Index, class Object, class Attic_Iterator, class Current_Iterator, class Predicate >
void reconstruct_items(
    Current_Iterator& current_it, Current_Iterator current_end,
    Attic_Iterator& attic_it, Attic_Iterator attic_end, Index& idx,
    const Predicate& predicate,
    std::vector< Object >& result, std::vector< Attic< Object > >& attic_result)
{
  std::vector< Object > skels;
  std::vector< Attic< typename Object::Delta > > deltas;

  while (!(current_it == current_end) && current_it.index() < idx)
    ++current_it;
  while (!(current_it == current_end) && current_it.index() == idx)
  {
    skels.push_back(current_it.object());
    ++current_it;
  }

  while (!(attic_it == attic_end) && attic_it.index() < idx)
    ++attic_it;
  while (!(attic_it == attic_end) && attic_it.index() == idx)
  {
    deltas.push_back(attic_it.object());
    ++attic_it;
  }

  std::vector< const Attic< typename Object::Delta >* > delta_refs;
  delta_refs.reserve(deltas.size());
  for (typename std::vector< Attic< typename Object::Delta > >::const_iterator it = deltas.begin();
      it != deltas.end(); ++it)
    delta_refs.push_back(&*it);

  std::sort(skels.begin(), skels.end());
  std::sort(delta_refs.begin(), delta_refs.end(), Delta_Ref_Comparator< Attic< typename Object::Delta > >());

  std::vector< Attic< Object > > attics;
  typename std::vector< Object >::const_iterator skels_it = skels.begin();
  Object reference;
  for (typename std::vector< const Attic< typename Object::Delta >* >::const_iterator it = delta_refs.begin();
        it != delta_refs.end(); ++it)
  {
    if (!(reference.id == (*it)->id))
    {
      while (skels_it != skels.end() && skels_it->id < (*it)->id)
        ++skels_it;
      if (skels_it != skels.end() && skels_it->id == (*it)->id)
        reference = *skels_it;
      else
        reference = Object();
    }
    try
    {
      Attic< Object > attic_obj = Attic< Object >((*it)->expand(reference), (*it)->timestamp);
      if (attic_obj.id.val() != 0)
      {
        reference = attic_obj;
        attics.push_back(attic_obj);
      }
      else
      {
        // Relation_Delta without a reference of the same index
        std::cerr<<name_of_type< Object >()<<" "<<(*it)->id.val()<<" cannot be expanded at timestamp "
            <<Timestamp((*it)->timestamp).str()<<".\n";
        exit(0);
      }
    }
    catch (const std::exception& e)
    {
      std::cerr<<name_of_type< Object >()<<" "<<(*it)->id.val()<<" cannot be expanded at timestamp "
          <<Timestamp((*it)->timestamp).str()<<": "<<e.what()<<'\n';
      exit(0);
    }
  }

  for (typename std::vector< Attic< Object > >::const_iterator it = attics.begin(); it != attics.end(); ++it)
  {
    if (predicate.match(*it))
      attic_result.push_back(*it);
  }

  for (typename std::vector< Object >::const_iterator it = skels.begin(); it != skels.end(); ++it)
  {
    if (predicate.match(*it))
      result.push_back(*it);
  }
}


struct Node_Timestamp_Hash
{
  Node_Timestamp_Hash() : data(0x10000) {}
  
  void push(Attic< Node_Skeleton::Id_Type > obj)
  { data[obj.val() & 0xffff].push_back(obj); }
  const std::vector< Attic< Node_Skeleton::Id_Type > >& candidates(Node_Skeleton::Id_Type ref) const
  { return data[ref.val() & 0xffff]; }
  
private:
  std::vector< std::vector< Attic< Node_Skeleton::Id_Type > > > data;
};


void collect_nd_events(
    uint64_t timestamp, const Attic< Way_Skeleton >& obj, const Node_Timestamp_Hash& hash,
    std::vector< Attic< Way_Skeleton::Id_Type > >& delta)
{
  auto old_size = delta.size();
  for (auto i : obj.nds)
  {
    auto candidates = hash.candidates(i);
    for (auto j : candidates)
    {
      if (i == j && timestamp < j.timestamp && j.timestamp < obj.timestamp)
        delta.push_back({ obj.id, j.timestamp });
    }
  }
  std::sort(delta.begin()+old_size, delta.end());
  delta.erase(std::unique(delta.begin()+old_size, delta.end()), delta.end());
}


void collect_nd_events(
    uint64_t timestamp, Way_Skeleton& obj, const Node_Timestamp_Hash& hash,
    std::vector< Attic< Way_Skeleton::Id_Type > >& delta)
{
  auto old_size = delta.size();
  for (auto i : obj.nds)
  {
    auto candidates = hash.candidates(i);
    for (auto j : candidates)
    {
      if (i == j && timestamp < j.timestamp)
        delta.push_back({ obj.id, j.timestamp });
    }
  }
  std::sort(delta.begin()+old_size, delta.end());
  delta.erase(std::unique(delta.begin()+old_size, delta.end()), delta.end());
}


void implicit_event(
    Uint31_Index cur_idx, Way_Skeleton::Id_Type id, uint64_t before, uint64_t timestamp, uint64_t after)
{
  std::cout<<std::hex<<"0x"<<cur_idx.val()
      <<'\t'<<std::dec<<id.val()
      <<'\t'<<Timestamp(before).str()
      <<'\t'<<Timestamp(timestamp).str()
      <<'\t'<<Timestamp(after).str()<<'\n';
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
      std::cerr<<"Read current and attic way idx file ";
      Random_File< Way_Skeleton::Id_Type, Uint31_Index > current(transaction.random_index(osm_base_settings().WAYS));
      Random_File< Way_Skeleton::Id_Type, Uint31_Index > attic(transaction.random_index(attic_settings().WAYS));

      for (uint i = 0; i < 1200*1000*1000; ++i)
      {
        if (i % (4*1024*1024) == 0)
          std::cerr<<'.';
        Uint31_Index cur_idx = current.get(Way_Skeleton::Id_Type(i));
        Uint31_Index attic_idx = attic.get(Way_Skeleton::Id_Type(i));
        if (!(cur_idx == attic_idx) && cur_idx.val() != 0 && attic_idx.val() != 0)
        {
          multiidx_ways[cur_idx].push_back(Way_Skeleton::Id_Type(i));
          multiidx_ways[attic_idx].push_back(Way_Skeleton::Id_Type(i));
        }
      }
      std::cerr<<" done.\n";
    }
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > cur_meta_db
          (transaction.data_index(meta_settings().WAYS_META));      
      auto cur_meta_it = cur_meta_db.flat_begin();
      
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_meta_db
          (transaction.data_index(attic_settings().WAYS_META));
      auto attic_meta_it = attic_meta_db.flat_begin();

      Block_Backend< Uint31_Index, Way_Skeleton > skel_db
          (transaction.data_index(osm_base_settings().WAYS));
      auto skel_it = skel_db.flat_begin();

      Block_Backend< Uint31_Index, Attic< Way_Delta > > delta_db
          (transaction.data_index(attic_settings().WAYS));
      auto delta_it = delta_db.flat_begin();
 
      Block_Backend< Uint31_Index, Attic< Way::Id_Type > > undel_db
          (transaction.data_index(attic_settings().WAYS_UNDELETED));
      auto undel_db_it = undel_db.flat_begin();
      
      uint64_t idx_cnt = 0;
      std::vector< Attic< Way_Skeleton::Id_Type > > meta;
      std::vector< Attic< Way_Skeleton::Id_Type > > delta;
      while (!(cur_meta_it == cur_meta_db.flat_end()) || !(attic_meta_it == attic_meta_db.flat_end()))
      {
        Uint31_Index cur_idx = (
            cur_meta_it == cur_meta_db.flat_end() || 
                (!(attic_meta_it == attic_meta_db.flat_end()) && attic_meta_it.index() < cur_meta_it.index()) ?
          attic_meta_it.index() : cur_meta_it.index());
        
        auto multiidx_it = multiidx_ways.find(cur_idx);
        
        if (++idx_cnt % 16384 == 0)
          std::cerr<<" 0x"<<std::hex<<cur_idx.val();
        if (idx_cnt % 262144 == 0)
          std::cerr<<'\n';
        
        while (!(cur_meta_it == cur_meta_db.flat_end()) && cur_meta_it.index() == cur_idx)
        {
          if (multiidx_it != multiidx_ways.end() &&
              std::find(multiidx_it->second.begin(), multiidx_it->second.end(), cur_meta_it.object().ref)
              != multiidx_it->second.end())
            meta.push_back(Attic< Way_Skeleton::Id_Type >(
                cur_meta_it.object().ref, cur_meta_it.object().timestamp));
          ++cur_meta_it;
        }
        
        //std::vector< Attic< Way_Skeleton::Id_Type > > attic_meta;
        while (!(attic_meta_it == attic_meta_db.flat_end()) && attic_meta_it.index() == cur_idx)
        {
          if (multiidx_it != multiidx_ways.end() &&
              std::find(multiidx_it->second.begin(), multiidx_it->second.end(), attic_meta_it.object().ref)
              != multiidx_it->second.end())
            meta.push_back(Attic< Way_Skeleton::Id_Type >(
              attic_meta_it.object().ref, attic_meta_it.object().timestamp));
          ++attic_meta_it;
        }
        
        if (Way::indicates_geometry(cur_idx))
        {
          while (!(delta_it == delta_db.flat_end()) && delta_it.index() < cur_idx)
            ++delta_it;
          while (!(delta_it == delta_db.flat_end()) && delta_it.index() == cur_idx)
          {
            if (multiidx_it != multiidx_ways.end() &&
                std::find(multiidx_it->second.begin(), multiidx_it->second.end(), delta_it.object().id)
                != multiidx_it->second.end())
              delta.push_back(Attic< Way_Skeleton::Id_Type >(
                delta_it.object().id, delta_it.object().timestamp));
            ++delta_it;
          }
        }
        else
        {
          Block_Backend< Uint32_Index, Attic< Node_Skeleton > > attic_node_db
              (transaction.data_index(attic_settings().NODES));

          Node_Timestamp_Hash hash;
          auto node_idxs = calc_children(Ranges< Uint31_Index >(cur_idx, inc(cur_idx)));
          for (auto it = attic_node_db.range_begin(node_idxs); !(it == attic_node_db.range_end()); ++it)
            hash.push({ it.object().id, it.object().timestamp });

          Uint31_Index cur_idx_ = cur_idx;
          std::vector< Way_Skeleton > skels;
          std::vector< Attic< Way_Skeleton > > attics;
          reconstruct_items(
              skel_it, skel_db.flat_end(), delta_it, delta_db.flat_end(), cur_idx_,
              Id_Not_In_Set_Predicate(multiidx_it == multiidx_ways.end() ? nullptr : &multiidx_it->second),
              skels, attics);
          std::sort(attics.begin(), attics.end());
          std::sort(skels.begin(), skels.end());
          
          std::vector< Attic< Way_Skeleton::Id_Type > > undel;
          while (!(undel_db_it == undel_db.flat_end()) && undel_db_it.index() < cur_idx)
            ++undel_db_it;
          while (!(undel_db_it == undel_db.flat_end()) && undel_db_it.index() == cur_idx)
          {
            if (multiidx_it == multiidx_ways.end() ||
                std::find(multiidx_it->second.begin(), multiidx_it->second.end(),
                    Way_Skeleton::Id_Type(undel_db_it.object()))
                == multiidx_it->second.end())
              undel.push_back(undel_db_it.object());
            ++undel_db_it;
          }
          std::sort(undel.begin(), undel.end());
          auto undel_it = undel.begin();
          
          auto skel_it = skels.begin();
          
          for (const auto& i : attics)
            delta.push_back(Attic< Way_Skeleton::Id_Type >(i.id, i.timestamp));
          
          for (uint64_t i = 0; i < attics.size(); ++i)
          {
            while (skel_it != skels.end() && skel_it->id < attics[i].id)
            {
              while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) < skel_it->id)
                ++undel_it;

              if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id)
                collect_nd_events(undel_it->timestamp, *skel_it, hash, delta);
              else
                collect_nd_events(0, *skel_it, hash, delta);
              ++skel_it;
            }
            
            while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) < attics[i].id)
              ++undel_it;
            if (i > 0 && attics[i-1].id == attics[i].id)
            {
              while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == attics[i].id
                  && undel_it->timestamp < attics[i-1].timestamp)
                ++undel_it;
              
              if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == attics[i].id
                  && undel_it->timestamp < attics[i].timestamp)
                collect_nd_events(undel_it->timestamp, attics[i], hash, delta);
              else
                collect_nd_events(attics[i-1].timestamp, attics[i], hash, delta);
            }
            else
            {
              if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == attics[i].id
                  && undel_it->timestamp < attics[i].timestamp)
                collect_nd_events(undel_it->timestamp, attics[i], hash, delta);
              else
                collect_nd_events(0, attics[i], hash, delta);
              
              if (skel_it != skels.end() && skel_it->id == attics[i].id)
              {
                while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id
                    && undel_it->timestamp < attics[i].timestamp)
                  ++undel_it;
                
                if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id)
                  collect_nd_events(undel_it->timestamp, *skel_it, hash, delta);
                else
                  collect_nd_events(attics[i].timestamp, *skel_it, hash, delta);
                ++skel_it;
              }
            }
          }
          while (skel_it != skels.end())
          {
            while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) < skel_it->id)
              ++undel_it;

            if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id)
              collect_nd_events(undel_it->timestamp, *skel_it, hash, delta);
            else
              collect_nd_events(0, *skel_it, hash, delta);
            ++skel_it;
          }
        }
      }

      std::cerr<<"\nDEBUG: "<<std::dec<<meta.size()<<" multiidx metas before sorting.\n";
      
      std::sort(meta.begin(), meta.end());
      meta.erase(std::unique(meta.begin(), meta.end()), meta.end());
      
      std::cerr<<"DEBUG: "<<std::dec<<meta.size()<<" multiidx metas after sorting.\n";

      std::cerr<<"DEBUG: "<<std::dec<<delta.size()<<" multiidx deltas before sorting.\n";
      
      std::sort(delta.begin(), delta.end());
      delta.erase(std::unique(delta.begin(), delta.end()), delta.end());
      
      std::cerr<<"DEBUG: "<<std::dec<<delta.size()<<" multiidx deltas after sorting.\n";

      auto d_it = delta.begin();
      for (uint64_t i = 0; i+1 < meta.size(); ++i)
      {
        while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) < Way_Skeleton::Id_Type(meta[i]))
        {
          std::cout<<"DEBUG j 0x0 "<<std::dec<<d_it->val()<<' '<<meta[i].val()<<'\n';
          ++d_it;
        }
        while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i])
            && d_it->timestamp <= meta[i].timestamp)
        {
//             std::cout<<"DEBUG f 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta[i].val()
//                 <<' '<<Timestamp(d_it->timestamp).str()<<'\n';
          ++d_it;
        }
        uint lifespan = 0;
        if (Way_Skeleton::Id_Type(meta[i]) == Way_Skeleton::Id_Type(meta[i+1]))
        {
          lifespan = calc_delta(meta[i].timestamp, meta[i+1].timestamp);
          while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i])
              && d_it->timestamp < meta[i+1].timestamp)
          {
            implicit_event(Uint31_Index(0xffu), meta[i], meta[i].timestamp, d_it->timestamp, meta[i+1].timestamp);
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
            implicit_event(Uint31_Index(0xffu), meta[i], meta[i].timestamp, d_it->timestamp, cur_date.timestamp);
            ++d_it;
          }
        }
      }
      if (!meta.empty())
      {
        while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) < Way_Skeleton::Id_Type(meta.back()))
        {
          std::cout<<"DEBUG k 0x0 "<<std::dec<<d_it->val()<<' '<<meta.back().val()<<'\n';
          ++d_it;
        }
        while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta.back())
            && d_it->timestamp <= meta.back().timestamp)
        {
//             std::cout<<"DEBUG h 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta.back().val()
//                 <<' '<<Timestamp(d_it->timestamp).str()<<'\n';
          ++d_it;
        }
        uint lifespan = calc_delta(meta.back().timestamp, cur_date.timestamp);
        while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta.back()))
        {
          implicit_event(Uint31_Index(0xffu), meta.back(), meta.back().timestamp, d_it->timestamp, cur_date.timestamp);
          ++d_it;
        }
      }
      while (d_it != delta.end())
      {
        //std::cout<<"DEBUG i 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<'\n';
        ++d_it;
      }
    }
    return 0;
    {
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > cur_meta_db
          (transaction.data_index(meta_settings().WAYS_META));      
      auto cur_meta_it = cur_meta_db.flat_begin();
      
      Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_meta_db
          (transaction.data_index(attic_settings().WAYS_META));
      auto attic_meta_it = attic_meta_db.flat_begin();

      Block_Backend< Uint31_Index, Way_Skeleton > skel_db
          (transaction.data_index(osm_base_settings().WAYS));
      auto skel_it = skel_db.flat_begin();

      Block_Backend< Uint31_Index, Attic< Way_Delta > > delta_db
          (transaction.data_index(attic_settings().WAYS));
      auto delta_it = delta_db.flat_begin();

      Block_Backend< Uint31_Index, Attic< Way::Id_Type > > undel_db
          (transaction.data_index(attic_settings().WAYS_UNDELETED));
      auto undel_db_it = undel_db.flat_begin();
      
      std::vector< uint32_t > implicit_after_version(366*86400, 0);
      std::vector< uint32_t > implicit_before_version(366*86400, 0);
      
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
        
        if (++idx_cnt % 16384 == 0)
          std::cerr<<" 0x"<<std::hex<<cur_idx.val();
        if (idx_cnt % 262144 == 0)
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
        
        std::vector< Attic< Way_Skeleton::Id_Type > > delta;
        if (Way::indicates_geometry(cur_idx))
        {
          while (!(delta_it == delta_db.flat_end()) && delta_it.index() < cur_idx)
            ++delta_it;
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
        }
        else
        {
          Block_Backend< Uint32_Index, Attic< Node_Skeleton > > attic_node_db
              (transaction.data_index(attic_settings().NODES));

          Node_Timestamp_Hash hash;
          auto node_idxs = calc_children(Ranges< Uint31_Index >(cur_idx, inc(cur_idx)));
          for (auto it = attic_node_db.range_begin(node_idxs); !(it == attic_node_db.range_end()); ++it)
            hash.push({ it.object().id, it.object().timestamp });

          Uint31_Index cur_idx_ = cur_idx;
          std::vector< Way_Skeleton > skels;
          std::vector< Attic< Way_Skeleton > > attics;
          reconstruct_items(
              skel_it, skel_db.flat_end(), delta_it, delta_db.flat_end(), cur_idx_,
              Id_Not_In_Set_Predicate(multiidx_it == multiidx_ways.end() ? nullptr : &multiidx_it->second),
              skels, attics);
          std::sort(attics.begin(), attics.end());
          std::sort(skels.begin(), skels.end());
          
          std::vector< Attic< Way_Skeleton::Id_Type > > undel;
          while (!(undel_db_it == undel_db.flat_end()) && undel_db_it.index() < cur_idx)
            ++undel_db_it;
          while (!(undel_db_it == undel_db.flat_end()) && undel_db_it.index() == cur_idx)
          {
            if (multiidx_it == multiidx_ways.end() ||
                std::find(multiidx_it->second.begin(), multiidx_it->second.end(),
                    Way_Skeleton::Id_Type(undel_db_it.object()))
                == multiidx_it->second.end())
              undel.push_back(undel_db_it.object());
            ++undel_db_it;
          }
          std::sort(undel.begin(), undel.end());
          auto undel_it = undel.begin();
          
          auto skel_it = skels.begin();
          
          for (const auto& i : attics)
            delta.push_back(Attic< Way_Skeleton::Id_Type >(i.id, i.timestamp));
          
          for (uint64_t i = 0; i < attics.size(); ++i)
          {
            while (skel_it != skels.end() && skel_it->id < attics[i].id)
            {
              while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) < skel_it->id)
                ++undel_it;

              if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id)
                collect_nd_events(undel_it->timestamp, *skel_it, hash, delta);
              else
                collect_nd_events(0, *skel_it, hash, delta);
              ++skel_it;
            }
            
            while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) < attics[i].id)
              ++undel_it;
            if (i > 0 && attics[i-1].id == attics[i].id)
            {
              while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == attics[i].id
                  && undel_it->timestamp < attics[i-1].timestamp)
                ++undel_it;
              
              if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == attics[i].id
                  && undel_it->timestamp < attics[i].timestamp)
                collect_nd_events(undel_it->timestamp, attics[i], hash, delta);
              else
                collect_nd_events(attics[i-1].timestamp, attics[i], hash, delta);
            }
            else
            {
              if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == attics[i].id
                  && undel_it->timestamp < attics[i].timestamp)
                collect_nd_events(undel_it->timestamp, attics[i], hash, delta);
              else
                collect_nd_events(0, attics[i], hash, delta);
              
              if (skel_it != skels.end() && skel_it->id == attics[i].id)
              {
                while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id
                    && undel_it->timestamp < attics[i].timestamp)
                  ++undel_it;
                
                if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id)
                  collect_nd_events(undel_it->timestamp, *skel_it, hash, delta);
                else
                  collect_nd_events(attics[i].timestamp, *skel_it, hash, delta);
                ++skel_it;
              }
            }
          }
          while (skel_it != skels.end())
          {
            while (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) < skel_it->id)
              ++undel_it;

            if (undel_it != undel.end() && Way_Skeleton::Id_Type(*undel_it) == skel_it->id)
              collect_nd_events(undel_it->timestamp, *skel_it, hash, delta);
            else
              collect_nd_events(0, *skel_it, hash, delta);
            ++skel_it;
          }
          
          std::sort(delta.begin(), delta.end());
          delta.erase(std::unique(delta.begin(), delta.end()), delta.end());
        }
          
        auto d_it = delta.begin();
        for (uint64_t i = 0; i+1 < meta.size(); ++i)
        {
          while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) < Way_Skeleton::Id_Type(meta[i]))
          {
            std::cout<<"DEBUG e 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<' '<<meta[i].val()<<'\n';
            ++d_it;
          }
          while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i])
              && d_it->timestamp <= meta[i].timestamp)
          {
//             std::cout<<"DEBUG f 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta[i].val()
//                 <<' '<<Timestamp(d_it->timestamp).str()<<'\n';
            ++d_it;
          }
          uint lifespan = 0;
          if (Way_Skeleton::Id_Type(meta[i]) == Way_Skeleton::Id_Type(meta[i+1]))
          {
            lifespan = calc_delta(meta[i].timestamp, meta[i+1].timestamp);
            while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta[i])
                && d_it->timestamp < meta[i+1].timestamp)
            {
              ++implicit_cnt;
              implicit_event(cur_idx, meta[i], meta[i].timestamp, d_it->timestamp, meta[i+1].timestamp);
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
              implicit_event(cur_idx, meta[i], meta[i].timestamp, d_it->timestamp, cur_date.timestamp);
              ++d_it;
            }
          }
        }
        if (!meta.empty())
        {
          while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) < Way_Skeleton::Id_Type(meta.back()))
          {
            std::cout<<"DEBUG g 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<' '<<meta.back().val()<<'\n';
            ++d_it;
          }
          while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta.back())
              && d_it->timestamp <= meta.back().timestamp)
          {
//             std::cout<<"DEBUG h 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<meta.back().val()
//                 <<' '<<Timestamp(d_it->timestamp).str()<<'\n';
            ++d_it;
          }
          uint lifespan = calc_delta(meta.back().timestamp, cur_date.timestamp);
          while (d_it != delta.end() && Way_Skeleton::Id_Type(*d_it) == Way_Skeleton::Id_Type(meta.back()))
          {
            ++implicit_cnt;
            implicit_event(cur_idx, meta.back(), meta.back().timestamp, d_it->timestamp, cur_date.timestamp);
            ++d_it;
          }
        }
        while (d_it != delta.end())
        {
          //std::cout<<"DEBUG i 0x"<<std::hex<<cur_idx.val()<<' '<<std::dec<<d_it->val()<<'\n';
          ++d_it;
        }
      }
      std::cerr<<"\n... "<<std::dec<<idx_cnt<<" indices and "<<meta_cnt<<" metas processed, "
          <<implicit_cnt<<" implicit versions found.\n";      
    
//       uint32_t partial_sum = 0;
//       for (uint64_t i = 0; i < implicit_after_version.size(); ++i)
//       {
//         partial_sum += implicit_after_version[i];
//         std::cout<<"after\t"<<i<<'\t'<<implicit_after_version[i]<<'\t'<<partial_sum<<'\n';
//       }
//       
//       partial_sum = 0;
//       for (uint64_t i = 0; i < implicit_before_version.size(); ++i)
//       {
//         partial_sum += implicit_before_version[i];
//         std::cout<<"before\t"<<i<<'\t'<<implicit_before_version[i]<<'\t'<<partial_sum<<'\n';
//       }    
    }
  }
  catch (File_Error e)
  {
    std::cerr<<e.origin<<' '<<e.filename<<' '<<e.error_number<<'\n';
  }

  return 0;
}
