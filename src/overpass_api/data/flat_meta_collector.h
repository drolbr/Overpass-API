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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__FLAT_META_COLLECTOR_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__FLAT_META_COLLECTOR_H

#include "../core/type_meta.h"
#include "../../template_db/block_backend.h"
#include "../../template_db/transaction.h"

#include <iostream>


template< class TIndex, class Id_Type >
struct Flat_Meta_Collector
{
  public:
    Flat_Meta_Collector(Transaction& transaction, const File_Properties* meta_file_prop = 0);

    void reset();
    const OSM_Element_Metadata_Skeleton< Id_Type >* get(const TIndex& index, Id_Type ref);

    ~Flat_Meta_Collector()
    {
      if (meta_db)
      {
	delete db_it;
	delete meta_db;
      }
    }

  private:
    Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >* meta_db;
    typename Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >::Flat_Iterator*
      db_it;
    TIndex* current_index;
    std::map< OSM_Element_Metadata_Skeleton< Id_Type >, bool > current_objects;
};


/** Implementation --------------------------------------------------------- */


template< class TIndex, class Id_Type >
Flat_Meta_Collector< TIndex, Id_Type >::Flat_Meta_Collector
    (Transaction& transaction, const File_Properties* meta_file_prop)
  : meta_db(0), db_it(0), current_index(0)
{
  meta_db = new Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >
      (transaction.data_index(meta_file_prop));

  reset();
}


template< class TIndex, class Id_Type >
void Flat_Meta_Collector< TIndex, Id_Type >::reset()
{
  if (!meta_db)
    return;

  if (db_it)
    delete db_it;
  if (current_index)
  {
    delete current_index;
    current_index = 0;
  }

  db_it = new typename Block_Backend< TIndex, OSM_Element_Metadata_Skeleton< Id_Type > >
      ::Flat_Iterator(meta_db->flat_begin());

  if (!(*db_it == meta_db->flat_end()))
    current_index = new TIndex(db_it->index());
  while (!(*db_it == meta_db->flat_end()) && (*current_index == db_it->index()))
  {
    current_objects.insert(std::make_pair(db_it->object(), false));
    ++(*db_it);
  }
}

template< class TIndex, class Id_Type >
const OSM_Element_Metadata_Skeleton< Id_Type >* Flat_Meta_Collector< TIndex, Id_Type >::get
    (const TIndex& index, Id_Type ref)
{
  if (!meta_db)
    return 0;

  if ((current_index) && (*current_index < index))
  {
    for (auto it = current_objects.begin(); it != current_objects.end(); ++it)
    {
      if (!it->second)
        std::cout<<"Skipping meta data of "<<std::dec<<it->first.ref.val()
            <<" at "<<std::hex<<current_index->val()<<'\n';
    }
    current_objects.clear();

    while (!(*db_it == meta_db->flat_end()) && (db_it->index() < index))
    {
      std::cout<<"Skipping meta data of "<<std::dec<<db_it->object().ref.val()
          <<" at "<<std::hex<<db_it->index().val()<<'\n';
      ++(*db_it);
    }
    if (!(*db_it == meta_db->flat_end()))
      *current_index = db_it->index();
    while (!(*db_it == meta_db->flat_end()) && (*current_index == db_it->index()))
    {
      current_objects.insert(std::make_pair(db_it->object(), false));
      ++(*db_it);
    }
  }

  typename std::map< OSM_Element_Metadata_Skeleton< Id_Type >, bool >::iterator it
      = current_objects.find(OSM_Element_Metadata_Skeleton< Id_Type >(ref));
  it->second = true;
  if (it != current_objects.end())
    return &it->first;
  else
    return 0;
}


#endif
