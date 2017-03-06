/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../template_db/random_file.h"
#include "../../expat/expat_justparse_interface.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"


uint32 osm_element_count;

struct Ofstream_Collection
{
  std::vector< std::ofstream* > streams;
  std::string prefix;
  std::string postfix;

  Ofstream_Collection(std::string prefix_, std::string postfix_)
  : prefix(prefix_), postfix(postfix_) {}

  std::ofstream* get(uint32 i)
  {
    while (streams.size() <= i)
    {
      std::ostringstream buf("");
      buf<<streams.size();
      streams.push_back(new std::ofstream((prefix + buf.str() + postfix).c_str()));
    }
    return streams[i];
  }

  ~Ofstream_Collection()
  {
    for (std::vector< std::ofstream* >::iterator it(streams.begin());
	 it != streams.end(); ++it)
    {
      (*it)->close();
      delete (*it);
    }
  }
};

void dump_nodes(Transaction& transaction, const std::string& db_dir, bool attic)
{
  Ofstream_Collection node_db_out(db_dir + "after_node_", "_db.csv");
  Ofstream_Collection node_tags_local_out(db_dir + "after_node_tags_", "_local.csv");
  Ofstream_Collection node_tags_global_out(db_dir + "after_node_tags_", "_global.csv");

  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (transaction.data_index(osm_base_settings().NODES));
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
      it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
  {
    std::ofstream* out(node_db_out.get(it.object().id.val() / 5000000));
    (*out)<<std::dec<<it.object().id.val()<<'\t'<<std::setprecision(10)
	<<::lat(it.index().val(), it.object().ll_lower)<<'\t'
	<<::lon(it.index().val(), it.object().ll_lower)<<'\t'
          <<std::hex<<it.index().val()<<'\n';
  }

  // check update_node_tags_local - compare both files for the result
  Block_Backend< Tag_Index_Local, Node_Skeleton::Id_Type > nodes_local_db
      (transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Node_Skeleton::Id_Type >::Flat_Iterator
      it(nodes_local_db.flat_begin());
      !(it == nodes_local_db.flat_end()); ++it)
  {
    std::ofstream* out(node_tags_local_out.get(it.object().val() / 5000000));
    (*out)<<std::dec<<std::hex<<it.index().index<<'\t'<<std::dec<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
  }

  // check update_node_tags_global - compare both files for the result
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > > nodes_global_db
      (transaction.data_index(osm_base_settings().NODE_TAGS_GLOBAL));
  for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > >::Flat_Iterator
      it(nodes_global_db.flat_begin());
      !(it == nodes_global_db.flat_end()); ++it)
  {
    std::ofstream* out(node_tags_global_out.get(it.object().id.val() / 5000000));
    (*out)<<std::dec<<it.object().id.val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
  }

  if (attic)
  {
    Ofstream_Collection node_meta_db_out(db_dir + "after_node_meta_", "_db.csv");
    Ofstream_Collection node_attic_db_out(db_dir + "after_node_attic_", "_db.csv");
    Ofstream_Collection node_undeleted_db_out(db_dir + "after_node_undeleted_", "_db.csv");
    Ofstream_Collection node_idx_list_out(db_dir + "after_node_idx_list_", "_db.csv");
    Ofstream_Collection node_attic_meta_db_out(db_dir + "after_node_attic_meta_", "_db.csv");
    Ofstream_Collection node_attic_tags_local_out(db_dir + "after_node_attic_tags_", "_local.csv");
    Ofstream_Collection node_attic_tags_global_out(db_dir + "after_node_attic_tags_", "_global.csv");

    Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > nodes_meta_db
        (transaction.data_index(meta_settings().NODES_META));
    for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >::Flat_Iterator
        it(nodes_meta_db.flat_begin()); !(it == nodes_meta_db.flat_end()); ++it)
    {
      std::ofstream* out(node_meta_db_out.get(it.object().ref.val() / 5000000));
      (*out)<<std::dec<<it.object().ref.val()<<'\t'
          <<it.object().version<<'\t'
          <<((it.object().timestamp)>>26)<<' '
          <<((it.object().timestamp & 0x3c00000)>>22)<<' '
          <<((it.object().timestamp & 0x3e0000)>>17)<<' '
          <<((it.object().timestamp & 0x1f000)>>12)<<' '
          <<((it.object().timestamp & 0xfc0)>>6)<<' '
          <<(it.object().timestamp & 0x3f)<<'\t'
          <<it.object().changeset<<'\t'<<it.object().user_id<<'\t'
          <<std::hex<<it.index().val()<<'\n';
    }

    Block_Backend< Uint31_Index, Attic< Node_Skeleton > > nodes_db
        (transaction.data_index(attic_settings().NODES));
    for (Block_Backend< Uint31_Index, Attic< Node_Skeleton > >::Flat_Iterator
        it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
    {
      std::ofstream* out(node_attic_db_out.get(it.object().id.val() / 5000000));
      (*out)<<std::dec<<it.object().id.val()<<'\t'<<std::setprecision(10)
          <<::lat(it.index().val(), it.object().ll_lower)<<'\t'
          <<::lon(it.index().val(), it.object().ll_lower)<<'\t'
          <<((it.object().timestamp)>>26)<<' '
          <<((it.object().timestamp & 0x3c00000)>>22)<<' '
          <<((it.object().timestamp & 0x3e0000)>>17)<<' '
          <<((it.object().timestamp & 0x1f000)>>12)<<' '
          <<((it.object().timestamp & 0xfc0)>>6)<<' '
          <<(it.object().timestamp & 0x3f)<<'\t'
          <<std::hex<<it.index().val()<<'\n';
    }

    Block_Backend< Uint31_Index, Attic< Node_Skeleton::Id_Type > > nodes_undeleted_db
        (transaction.data_index(attic_settings().NODES_UNDELETED));
    for (Block_Backend< Uint31_Index, Attic< Node_Skeleton::Id_Type > >::Flat_Iterator
        it(nodes_undeleted_db.flat_begin()); !(it == nodes_undeleted_db.flat_end()); ++it)
    {
      std::ofstream* out(node_undeleted_db_out.get(it.object().val() / 5000000));
      (*out)<<std::dec<<it.object().val()<<'\t'
          <<((it.object().timestamp)>>26)<<' '
          <<((it.object().timestamp & 0x3c00000)>>22)<<' '
          <<((it.object().timestamp & 0x3e0000)>>17)<<' '
          <<((it.object().timestamp & 0x1f000)>>12)<<' '
          <<((it.object().timestamp & 0xfc0)>>6)<<' '
          <<(it.object().timestamp & 0x3f)<<'\t'
          <<std::hex<<it.index().val()<<'\n';
    }

    Block_Backend< Node::Id_Type, Uint31_Index > idx_db
        (transaction.data_index(attic_settings().NODE_IDX_LIST));
    for (Block_Backend< Node::Id_Type, Uint31_Index >::Flat_Iterator
        it(idx_db.flat_begin()); !(it == idx_db.flat_end()); ++it)
    {
      std::ofstream* out(node_idx_list_out.get(it.index().val() / 5000000));
      (*out)<<std::dec<<it.index().val()<<'\t'<<std::hex<<it.object().val()<<'\n';
    }

    Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > nodes_attic_meta_db
        (transaction.data_index(attic_settings().NODES_META));
    for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >::Flat_Iterator
        it(nodes_attic_meta_db.flat_begin()); !(it == nodes_attic_meta_db.flat_end()); ++it)
    {
      std::ofstream* out(node_attic_meta_db_out.get(it.object().ref.val() / 5000000));
      (*out)<<std::dec<<it.object().ref.val()<<'\t'
          <<it.object().version<<'\t'
          <<((it.object().timestamp)>>26)<<' '
          <<((it.object().timestamp & 0x3c00000)>>22)<<' '
          <<((it.object().timestamp & 0x3e0000)>>17)<<' '
          <<((it.object().timestamp & 0x1f000)>>12)<<' '
          <<((it.object().timestamp & 0xfc0)>>6)<<' '
          <<(it.object().timestamp & 0x3f)<<'\t'
          <<it.object().changeset<<'\t'<<it.object().user_id<<'\t'
          <<std::hex<<it.index().val()<<'\n';
    }

    Block_Backend< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > > nodes_local_db
        (transaction.data_index(attic_settings().NODE_TAGS_LOCAL));
    for (Block_Backend< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > >::Flat_Iterator
        it(nodes_local_db.flat_begin());
        !(it == nodes_local_db.flat_end()); ++it)
    {
      std::ofstream* out(node_attic_tags_local_out.get(it.object().val() / 5000000));
      (*out)<<std::hex<<it.index().index<<'\t'<<std::dec<<it.object().val()<<'\t'
          <<it.index().key<<'\t'<<it.index().value<<'\t'
          <<((it.object().timestamp)>>26)<<' '
          <<((it.object().timestamp & 0x3c00000)>>22)<<' '
          <<((it.object().timestamp & 0x3e0000)>>17)<<' '
          <<((it.object().timestamp & 0x1f000)>>12)<<' '
          <<((it.object().timestamp & 0xfc0)>>6)<<' '
          <<(it.object().timestamp & 0x3f)<<'\n';
    }

    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > > nodes_global_db
        (transaction.data_index(attic_settings().NODE_TAGS_GLOBAL));
    for (Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > >::Flat_Iterator
        it(nodes_global_db.flat_begin());
        !(it == nodes_global_db.flat_end()); ++it)
    {
      std::ofstream* out(node_attic_tags_global_out.get(it.object().id.val() / 5000000));
      (*out)<<it.object().id.val()<<'\t'
          <<it.index().key<<'\t'<<it.index().value<<'\t'
          <<((it.object().timestamp)>>26)<<' '
          <<((it.object().timestamp & 0x3c00000)>>22)<<' '
          <<((it.object().timestamp & 0x3e0000)>>17)<<' '
          <<((it.object().timestamp & 0x1f000)>>12)<<' '
          <<((it.object().timestamp & 0xfc0)>>6)<<' '
          <<(it.object().timestamp & 0x3f)<<'\n';
    }
  }
}

void check_nodes(Transaction& transaction)
{
  uint element_count = 0;
  std::ofstream out("/dev/null");

  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (transaction.data_index(osm_base_settings().NODES));
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
    it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
  {
    out<<it.object().id.val()<<'\t'<<std::setprecision(10)
        <<::lat(it.index().val(), it.object().ll_lower)<<'\t'
        <<::lon(it.index().val(), it.object().ll_lower)<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.object().id.val()<<' ';
  }

  // check update_node_tags_local - compare both files for the result
  Block_Backend< Tag_Index_Local, Uint32_Index > nodes_local_db
      (transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
    it(nodes_local_db.flat_begin());
  !(it == nodes_local_db.flat_end()); ++it)
  {
    out<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.index().index<<' ';
  }

  // check update_node_tags_global - compare both files for the result
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > > nodes_global_db
      (transaction.data_index(osm_base_settings().NODE_TAGS_GLOBAL));
  for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > >::Flat_Iterator
    it(nodes_global_db.flat_begin());
  !(it == nodes_global_db.flat_end()); ++it)
  {
    out<<it.object().id.val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.index().key<<' ';
  }
}

void dump_ways(Transaction& transaction, const std::string& db_dir)
{
  Ofstream_Collection way_db_out(db_dir + "after_way_", "_db.csv");
  Ofstream_Collection way_tags_local_out(db_dir + "after_way_tags_", "_local.csv");
  Ofstream_Collection way_tags_global_out(db_dir + "after_way_tags_", "_global.csv");

  // check update_members - compare both files for the result
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (transaction.data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
      it(ways_db.flat_begin()); !(it == ways_db.flat_end()); ++it)
  {
    std::ofstream* out(way_db_out.get(it.object().id.val() / 1000000));
    (*out)<<it.object().id.val()<<'\t';
    for (uint i(0); i < it.object().nds.size(); ++i)
      (*out)<<it.object().nds[i].val()<<' ';
    (*out)<<'\n';
  }

  // check update_way_tags_local - compare both files for the result
  Block_Backend< Tag_Index_Local, Uint32_Index > ways_local_db
      (transaction.data_index(osm_base_settings().WAY_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
      it(ways_local_db.flat_begin());
      !(it == ways_local_db.flat_end()); ++it)
  {
    std::ofstream* out(way_tags_local_out.get(it.object().val() / 1000000));
    (*out)<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
  }

  // check update_way_tags_global - compare both files for the result
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Way_Skeleton::Id_Type > > ways_global_db
      (transaction.data_index(osm_base_settings().WAY_TAGS_GLOBAL));
  for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Way_Skeleton::Id_Type > >::Flat_Iterator
      it(ways_global_db.flat_begin());
      !(it == ways_global_db.flat_end()); ++it)
  {
    std::ofstream* out(way_tags_global_out.get(it.object().id.val() / 1000000));
    (*out)<<it.object().id.val()<<'\t'
	<<it.index().key<<'\t'<<it.index().value<<'\n';
  }
}

void check_ways(Transaction& transaction)
{
  uint element_count = 0;
  std::ofstream out("/dev/null");

  // check update_members - compare both files for the result
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (transaction.data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
      it(ways_db.flat_begin()); !(it == ways_db.flat_end()); ++it)
  {
    out<<it.object().id.val()<<'\t';
    for (uint i(0); i < it.object().nds.size(); ++i)
      out<<it.object().nds[i].val()<<' ';
    out<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.object().id.val()<<' ';
  }

  // check update_way_tags_local - compare both files for the result
  Block_Backend< Tag_Index_Local, Uint32_Index > ways_local_db
      (transaction.data_index(osm_base_settings().WAY_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
      it(ways_local_db.flat_begin());
      !(it == ways_local_db.flat_end()); ++it)
  {
    out<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.index().index<<' ';
  }

  // check update_way_tags_global - compare both files for the result
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Way_Skeleton::Id_Type > > ways_global_db
      (transaction.data_index(osm_base_settings().WAY_TAGS_GLOBAL));
  for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Way_Skeleton::Id_Type > >::Flat_Iterator
      it(ways_global_db.flat_begin());
      !(it == ways_global_db.flat_end()); ++it)
  {
    out<<it.object().id.val()<<'\t'
	<<it.index().key<<'\t'<<it.index().value<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.index().key<<' ';
  }
}

void dump_relations(Transaction& transaction, const std::string& db_dir)
{
  Ofstream_Collection relation_db_out(db_dir + "after_relation_", "_db.csv");
  Ofstream_Collection relation_tags_local_out(db_dir + "after_relation_tags_", "_local.csv");
  Ofstream_Collection relation_tags_global_out(db_dir + "after_relation_tags_", "_global.csv");

    // prepare check update_members - load roles
    std::map< uint32, std::string > roles;
    Block_Backend< Uint32_Index, String_Object > roles_db
      (transaction.data_index(osm_base_settings().RELATION_ROLES));
    for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
        it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
      roles[it.index().val()] = it.object().val();

    // check update_members - compare both files for the result
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
	(transaction.data_index(osm_base_settings().RELATIONS));
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
	 it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
    {
      std::ofstream* out(relation_db_out.get(it.object().id.val() / 200000));
      (*out)<<it.object().id.val()<<'\t';
      for (uint i(0); i < it.object().members.size(); ++i)
	(*out)<<it.object().members[i].ref.val()<<' '
	    <<it.object().members[i].type<<' '
	    <<roles[it.object().members[i].role]<<' ';
      (*out)<<'\n';
    }

    // check update_relation_tags_local - compare both files for the result
    Block_Backend< Tag_Index_Local, Uint32_Index > relations_local_db
	(transaction.data_index(osm_base_settings().RELATION_TAGS_LOCAL));
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(relations_local_db.flat_begin());
         !(it == relations_local_db.flat_end()); ++it)
    {
      std::ofstream* out(relation_tags_local_out.get(it.object().val() / 200000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }

    // check update_relation_tags_global - compare both files for the result
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Relation_Skeleton::Id_Type > > relations_global_db
	(transaction.data_index(osm_base_settings().RELATION_TAGS_GLOBAL));
    for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Relation_Skeleton::Id_Type > >::Flat_Iterator
	 it(relations_global_db.flat_begin());
         !(it == relations_global_db.flat_end()); ++it)
    {
      std::ofstream* out(relation_tags_global_out.get(it.object().id.val() / 200000));
      (*out)<<it.object().id.val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
}

void check_relations(Transaction& transaction)
{
  uint element_count = 0;
  std::ofstream out("/dev/null");

  // prepare check update_members - load roles
  std::map< uint32, std::string > roles;
  Block_Backend< Uint32_Index, String_Object > roles_db
  (transaction.data_index(osm_base_settings().RELATION_ROLES));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
    it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
    roles[it.index().val()] = it.object().val();

  // check update_members - compare both files for the result
  Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
  (transaction.data_index(osm_base_settings().RELATIONS));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
    it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
  {
    out<<it.object().id.val()<<'\t';
    for (uint i(0); i < it.object().members.size(); ++i)
      out<<it.object().members[i].ref.val()<<' '
          <<it.object().members[i].type<<' '
          <<roles[it.object().members[i].role]<<' ';
    out<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.object().id.val()<<' ';
  }

  // check update_relation_tags_local - compare both files for the result
  Block_Backend< Tag_Index_Local, Uint32_Index > relations_local_db
  (transaction.data_index(osm_base_settings().RELATION_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
      it(relations_local_db.flat_begin());
      !(it == relations_local_db.flat_end()); ++it)
  {
    out<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.index().index<<' ';
  }

  // check update_relation_tags_global - compare both files for the result
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Relation_Skeleton::Id_Type > > relations_global_db
  (transaction.data_index(osm_base_settings().RELATION_TAGS_GLOBAL));
  for (Block_Backend< Tag_Index_Global, Tag_Object_Global< Relation_Skeleton::Id_Type > >::Flat_Iterator
    it(relations_global_db.flat_begin());
  !(it == relations_global_db.flat_end()); ++it)
  {
    out<<it.object().id.val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    if (++element_count % 1000000 == 0)
      std::cerr<<it.index().key<<' ';
  }
}

int main(int argc, char* argv[])
{
  // read command line arguments
  std::string db_dir;
  bool dump = true;
  bool attic = false;

  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((std::string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    else if (!(strncmp(argv[argpos], "--only-check", 12)))
      dump = false;
    else if (!(strncmp(argv[argpos], "--attic", 7)))
      attic = true;
    ++argpos;
  }

  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    if (dump)
    {
      dump_nodes(transaction, db_dir, attic);
      dump_ways(transaction, db_dir);
      dump_relations(transaction, db_dir);
    }
    else
    {
      check_nodes(transaction);
      check_ways(transaction);
      check_relations(transaction);
    }
  }
  catch (File_Error e)
  {
    report_file_error(e);
  }

  return 0;
}
