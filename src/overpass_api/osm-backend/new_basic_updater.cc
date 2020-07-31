#include "new_basic_updater.h"

#include <algorithm>


void keep_oldest_per_first(std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >& arg)
{
  std::sort(arg.begin(), arg.end(),
      [](const std::pair< Node_Skeleton::Id_Type, uint64_t >& lhs, const std::pair< Node_Skeleton::Id_Type, uint64_t >& rhs)
      { return lhs.first.val() < rhs.first.val() || (!(rhs.first.val() < lhs.first.val())
          && lhs.second < rhs.second); });
  auto i_to = arg.begin();
  Node_Skeleton::Id_Type last_id(0ull); 
  for (auto i_from = arg.begin(); i_from != arg.end(); ++i_from)
  {
    if (i_from == arg.begin() || !(last_id == i_from->first))
      *(i_to++) = *i_from;
    last_id = i_from->first;
  }
  arg.erase(i_to, arg.end());
}


void keep_oldest_per_coord(Coord_Dates& arg)
{
  std::sort(arg.begin(), arg.end(), [](const Attic< Uint32 >& lhs, const Attic< Uint32 >& rhs)
      { return lhs.val() < rhs.val() || (!(rhs.val() < lhs.val()) && lhs.timestamp < rhs.timestamp); });
  auto i_to = arg.begin();
  Uint32 last_coord(0u);
  for (auto i_from = arg.begin(); i_from != arg.end(); ++i_from)
  {
    if (i_from == arg.begin() || !(last_coord == i_from->val()))
      *(i_to++) = *i_from;
    last_coord = i_from->val();
  }
  arg.erase(i_to, arg.end());
}
