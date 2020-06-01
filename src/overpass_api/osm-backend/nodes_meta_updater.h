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

#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "basic_updater.h"

#include <vector>


struct New_Object_Meta_Context
{
  New_Object_Meta_Context(
      OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta_, Uint31_Index own_idx_, uint32 ll_lower_)
      : meta(meta_), own_idx(own_idx_), ll_lower(ll_lower_),
      replaced_idx(0u), replaced_timestamp(0), idx_of_next_version(0u), end_timestamp(0) {}

  OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta;
  Uint31_Index own_idx;
  uint32 ll_lower;
  Uint31_Index replaced_idx;
  uint64 replaced_timestamp;
  Uint31_Index idx_of_next_version;
  uint64 end_timestamp;

  bool operator<(const New_Object_Meta_Context& rhs)
  {
    return meta.ref < rhs.meta.ref || (!(rhs.meta.ref < meta.ref) &&
        meta.timestamp < rhs.meta.timestamp);
  }

  bool operator==(const New_Object_Meta_Context& rhs)
  {
    return meta.ref == rhs.meta.ref && meta.timestamp == rhs.meta.timestamp;
  }
};


class Perflog_Tree
{
public:
  Perflog_Tree(const std::string& name) : starttime(clock())
  {
    std::cerr<<"("<<name<<' ';
  }

  ~Perflog_Tree()
  {
    std::cerr<<(clock() - starttime)/1000<<") ";
  }

private:
  clock_t starttime;
};


/* Invariants:
 * - The current nodes meta file contains for each id at most one entry.
 * - The attic nodes meta file for each id has only an entry if an entry for that id exists in current
 *   and then all those entries have older timestamp than the current entry.
 * - If there is an entry for a given id at index idx in the current nodes meta file
 *   then there is an entry in the nodes meta map file mapping the id to idx.
 * - If there are one or more entries for a given id at index idx in the attic nodes meta file
 *   then there is an entry in the nodes meta map file or the idx list file mapping the id to idx resp idxs.
 * Preconditions: none
 */
std::vector< New_Object_Meta_Context > read_and_update_meta(
    Transaction& transaction, const Data_By_Id< Node_Skeleton >& new_data, meta_modes strategy);
/* Postconditions:
 * - For every entry that has existed before the function call in the current or attic nodes meta file
 *   there is still an entry in the current or attic nodes meta call;
 *   entries may have moved from current to attic due to the age rule.
 * - For every entry in new_data there is an entry
 *   with the same id, timestamp, and idx in the current nodes meta file or attic nodes meta file;
 *   the age rule governs which is in which of the two files.
 */
