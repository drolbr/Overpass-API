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

#include "../core/datatypes.h"
#include "../frontend/output.h"
#include "map_file_replicator.h"


template< typename Index, typename Skeleton >
void compare_current_map_files(Transaction& transaction, typename Skeleton::Id_Type maxid)
{
  Random_File< typename Skeleton::Id_Type, Index >
      from_random(transaction.random_index(current_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::readonly, false, transaction.get_db_dir(), ".next");
  Random_File< typename Skeleton::Id_Type, Index >
      into_random(into_transaction.random_index(current_skeleton_file_properties< Skeleton >()));

  uint64_t total = 0;
  uint64_t num_zero = 0;
  uint64_t num_equal = 0;
  for (uint64_t i = 0ull; i <= maxid.val(); ++i)
  {
    Index from_idx = from_random.get(i);
    Index into_idx = into_random.get(i);

    ++total;
    if (from_idx == into_idx)
    {
      ++num_equal;
      num_zero += (from_idx.val() == 0);
    }
    else
      std::cout<<std::dec<<"Id "<<i<<": "<<std::hex<<from_idx.val()<<' '<<into_idx.val()
        <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(from_idx.val(), 0u)<<','<<lon(from_idx.val(), 0u)
        <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(into_idx.val(), 0u)<<','<<lon(into_idx.val(), 0u)
        <<'\n';
  }
  if (total == num_equal)
    std::cout<<"Compared "<<std::dec<<total<<" entries of which all are equal and "<<num_zero<<" are zero.\n";
  else
    std::cout<<"FAILED: Compared "<<std::dec<<total<<" entries of which only "<<num_equal<<" are equal and "<<num_zero<<" are zero.\n";
}


template< typename Index, typename Skeleton >
void compare_attic_map_files(Transaction& transaction, typename Skeleton::Id_Type maxid)
{
  Random_File< typename Skeleton::Id_Type, Index >
      from_random(transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::readonly, false, transaction.get_db_dir(), ".next");
  Random_File< typename Skeleton::Id_Type, Index >
      into_random(into_transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

  uint64_t total = 0;
  uint64_t num_zero = 0;
  uint64_t num_equal = 0;
  for (uint64_t i = 0ull; i <= maxid.val(); ++i)
  {
    Index from_idx = from_random.get(i);
    Index into_idx = into_random.get(i);

    ++total;
    if (from_idx == into_idx)
    {
      ++num_equal;
      num_zero += (from_idx.val() == 0);
    }
    else
      std::cout<<std::dec<<"Id "<<i<<": "<<std::hex<<from_idx.val()<<' '<<into_idx.val()
        <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(from_idx.val(), 0u)<<','<<lon(from_idx.val(), 0u)
        <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(into_idx.val(), 0u)<<','<<lon(into_idx.val(), 0u)
        <<'\n';
  }
  if (total == num_equal)
    std::cout<<"Compared "<<std::dec<<total<<" entries of which all are equal and "<<num_zero<<" are zero.\n";
  else
    std::cout<<"FAILED: Compared "<<std::dec<<total<<" entries of which only "<<num_equal<<" are equal and "<<num_zero<<" are zero.\n";
}


template< typename Index, typename Skeleton >
void compare_idx_lists(Transaction& transaction)
{
  Block_Backend< typename Skeleton::Id_Type, Index > from_db(
      transaction.data_index(attic_idx_list_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::readonly, false, transaction.get_db_dir(), ".next");
  Block_Backend< typename Skeleton::Id_Type, Index > into_db(
      into_transaction.data_index(attic_idx_list_properties< Skeleton >()));

  uint64_t missing_from = 0;
  uint64_t missing_into = 0;
  uint64_t matching = 0;
  auto into_it = into_db.flat_begin();
  for (auto from_it = from_db.flat_begin(); !(from_it == from_db.flat_end()); ++from_it)
  {
    //std::cout<<"DEBUG "<<std::dec<<from_it.index().val()<<' '<<std::hex<<from_it.object().val()<<'\n';

    while (!(into_it == into_db.flat_end()) && into_it.index() < from_it.index())
    {
      std::cout<<"Element "<<std::dec<<into_it.index().val()<<" with value "
          <<std::hex<<into_it.object().val()
          <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(into_it.object().val(), 0u)<<','<<lon(into_it.object().val(), 0u)
          <<" is missing in existing idx_list.\n";
      ++missing_from;
      ++into_it;
    }

    if (into_it == into_db.flat_end() || from_it.index() < into_it.index())
    {
      std::cout<<"Element "<<std::dec<<from_it.index().val()<<" with value "
          <<std::hex<<from_it.object().val()
          <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(from_it.object().val(), 0u)<<','<<lon(from_it.object().val(), 0u)
          <<" is missing from next.\n";
      ++missing_into;
    }
    else if (from_it.index() == into_it.index())
    {
      while (!(into_it == into_db.flat_end()) && from_it.index() == into_it.index()
          && into_it.object() < from_it.object())
      {
        std::cout<<"Element "<<std::dec<<from_it.index().val()<<" has extra value "
            <<std::hex<<into_it.object().val()
            <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(into_it.object().val(), 0u)<<','<<lon(into_it.object().val(), 0u)
            <<" in next idx_list.\n";
        ++missing_from;
        ++into_it;
      }

      if (!(into_it == into_db.flat_end()) && from_it.index() == into_it.index())
      {
        if (from_it.object() < into_it.object())
        {
          std::cout<<"Element "<<std::dec<<from_it.index().val()<<" has extra value "
              <<std::hex<<from_it.object().val()
              <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(from_it.object().val(), 0u)<<','<<lon(from_it.object().val(), 0u)
              <<" in existing idx_list.\n";
          ++missing_into;
        }
        else if (from_it.object() == into_it.object())
        {
          ++matching;
          ++into_it;
        }
      }
    }
  }
  while (!(into_it == into_db.flat_end()))
  {
    std::cout<<"Element "<<std::dec<<into_it.index().val()<<" with value "
        <<std::hex<<into_it.object().val()
        <<' '<<std::dec<<std::fixed<<std::setprecision(7)<<lat(into_it.object().val(), 0u)<<','<<lon(into_it.object().val(), 0u)
        <<" is missing in existing idx_list.\n";
    ++missing_from;
    ++into_it;
  }

  if (missing_from > 0 || missing_into > 0)
    std::cout<<"FAILED: Idx lists have "<<std::dec<<missing_into<<" extra objects in existing idx_list and "
        <<missing_from<<" extra objects in next idx_list.\n";
  else
    std::cout<<"Found "<<std::dec<<matching<<" objects in idx_lists.\n";
}


int main(int argc, char* argv[])
{
  // read command line arguments
  std::string db_dir;
  bool transactional = true;
  Database_Meta_State meta;
  bool abort = false;
  uint64_t flush_limit = 16*1024*1024; //4*1024*1024*1024ull;

  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((std::string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
        db_dir += '/';
      transactional = false;
    }
    else if (!(strncmp(argv[argpos], "--flush-size=", 13)))
    {
      flush_limit = atoll(std::string(argv[argpos]).substr(13).c_str()) *1024*1024;
      if (flush_limit == 0)
        flush_limit = std::numeric_limits< unsigned int >::max();
    }
    else
    {
      std::cerr<<"Unkown argument: "<<argv[argpos]<<'\n';
      abort = true;
    }
    ++argpos;
  }
  if (abort || transactional)
  {
    std::cerr<<"Usage: "<<argv[0]<<" --db-dir=DIR [--flush-size=FLUSH_SIZE]\n";
    return 1;
  }

  try
  {
    Nonsynced_Transaction transaction(Access_Mode::readonly, false, db_dir, "");

    //Uint64 current_max = replicate_current_map_file< Node::Index, Node_Skeleton >(nullptr, transaction, flush_limit);
    //Uint64 attic_max = replicate_attic_map_file< Node::Index, Node_Skeleton >(nullptr, transaction, flush_limit);

    compare_current_map_files< Node::Index, Node_Skeleton >(transaction, 1000000/*current_max*/);
    compare_attic_map_files< Node::Index, Node_Skeleton >(transaction, 1000000/*attic_max*/);
    compare_idx_lists< Node::Index, Node_Skeleton >(transaction);
    compare_current_map_files< Way::Index, Way_Skeleton >(transaction, 1000000/*current_max*/);
    compare_attic_map_files< Way::Index, Way_Skeleton >(transaction, 1000000/*attic_max*/);
    compare_idx_lists< Way::Index, Way_Skeleton >(transaction);
    compare_current_map_files< Relation::Index, Relation_Skeleton >(transaction, 1000000/*current_max*/);
    compare_attic_map_files< Relation::Index, Relation_Skeleton >(transaction, 1000000/*attic_max*/);
    compare_idx_lists< Relation::Index, Relation_Skeleton >(transaction);
  }
  catch (File_Error e)
  {
    report_file_error(e);
    return 2;
  }

  return 0;
}
