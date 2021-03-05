// #include "data_from_osc.h"
// #include "mapfile_io.h"
// #include "new_node_updater.h"
// #include "node_event_list.h"
// #include "node_meta_updater.h"
// #include "node_skeleton_updater.h"
// #include "prepare_node_update.h"
// #include "update_events_preparer.h"
// 
// #include "../../template_db/block_backend.h"
// #include "../../template_db/transaction.h"
// 
// #include <map>
// #include <set>
// #include <vector>
// 
// 
// // Nodes_Delta nodes_delta(Event_List)
// // is feasible from the event list, using event_date, id, old_cord, new_coord, and new_mult
// 
// // void amend_nodes_dict(Event_List, Nodes_Dict&)
// // is feasible from the event list, using event_date, id, new_coord, and new_mult
// 
// 
// template< typename Index, typename Object >
// class File_Handle
// {
// public:
//   File_Handle(File_Blocks_Index_Base* data_index, const std::vector< Index >& req)
//       : db(data_index), it(db.discrete_begin(req.begin(), req.end())), end(db.discrete_end()) {}
// 
//   std::vector< Object > obj_with_idx(Index idx);
// 
// private:
//   Block_Backend< Index, Object, typename std::vector< Index >::const_iterator > db;
//   typename Block_Backend< Index, Object, typename std::vector< Index >::const_iterator >::Discrete_Iterator it;
//   typename Block_Backend< Index, Object, typename std::vector< Index >::const_iterator >::Discrete_Iterator end;
// };
// 
// 
// template< typename Index, typename Object >
// std::vector< Object > File_Handle< Index, Object >::obj_with_idx(Index idx)
// {
//   std::vector< Object > result;
// 
//   while (!(it == end) && it.index() < idx)
//     ++it;
//   while (!(it == end) && !(idx < it.index()))
//   {
//     result.push_back(it.object());
//     ++it;
//   }
// 
//   return result;
// }
// 
// 
// template< typename Index, typename Object >
// std::vector< Index > idx_list(const std::map< Index, Object >& arg)
// {
//   std::vector< Index > result;
//   result.reserve(arg.size());
//   for (const auto& i : arg)
//     result.push_back(i.first);
//   return result;
// }
// 
// 
// class Perflog_Tree
// {
// public:
//   Perflog_Tree(const std::string& name) : starttime(clock())
//   {
//     std::cerr<<"("<<name<<' ';
//   }
// 
//   ~Perflog_Tree()
//   {
//     std::cerr<<(clock() - starttime)/1000<<") ";
//   }
// 
// private:
//   clock_t starttime;
// };


//template Pre_Event_Refs


struct Way_Event
{
  uint64_t timestamp_start;
  uint64_t timestamp_end;
  OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
  Way_Skeleton* skel;
};


class Way_Event_Container
{
public:
  std::vector< Way_Event > events;
private:
  std::list< Way_Skeleton > skel_storage;
};


struct Changed_Objects_In_An_Idx
{
  std::vector< Way_Skeleton > existing_current;
  std::vector< Attic< Way_Skeleton > > existing_attic;
  std::vector< Attic< Way_Skeleton::Id_Type > > existing_undeletes;
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > existing_current_meta;
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > existing_attic_meta;
  std::vector< Attic< Way_Skeleton::Id_Type > > unchanged_before;
  Way_Event_Container events;
};


void extract_relevant_undeleted(
    std::vector< Attic< Way_Skeleton::Id_Type > >& undeletes,
    Way_Event_Container& events,
    std::vector< Attic< Way_Skeleton::Id_Type > >& undeletes_to_del,
    std::vector< Way_Skeleton::Id_Type >& deleted_after_unchanged)
{
  /*
  {
    for (i : events)
    {
      while (undeletes.(id, timestamp) <= i.(id, not_before))
        ++undeletes;

      if (undeletes.(id, timestamp) < i.(id, before))
      {
        if (i.prev.id < i.id)
          deleted_after_unchanged.push(i.id);

        i.not_before = undeletes.timestamp;
        undeletes_to_del.push(undeletes);
        ++undeletes;
      }
    }
  }
   */
}


void update_ways(Transaction& transaction, Data_From_Osc& new_data)
{
  //needs: id |-> (begin, new_pos, new_mult)+
  //TODO: new_data.compute_way_geometry(node_delta);
  //doch nicht, stattdessen parent_geom

//   Perflog_Tree perf("update_nodes");
//   std::cerr<<new_data.nodes.data.size()<<' '<<new_data.ways.data.size()<<'\n';

  // before the first pass by idx
  Mapfile_IO mapfile_io(transaction); //< Way_Skeleton, Pre_Event_Refs depends on, filenames >
//   std::unique_ptr< Perflog_Tree > dyn_perf(new Perflog_Tree("new_data.node_pre_events"));
  //TODO: timestamp_start?
  Pre_Event_List pre_events = new_data.way_pre_events();
//   std::cerr<<pre_events.data.size()<<' '<<pre_events.timestamp_last_not_deleted.size()<<'\n';
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("new_data.pre_event_refs_by_idx"));
  std::map< Uint31_Index, Pre_Event_Refs > pre_event_refs_by_idx;// new_data.parent_geom_for_ways();
//   std::cerr<<pre_event_refs_by_idx.size()<<' ';
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("mapfile_io.read_idx_list"));
  mapfile_io.read_idx_list(new_data.way_pre_event_refs(pre_events), pre_event_refs_by_idx);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("idx_list ff"));
  auto req = idx_list(pre_event_refs_by_idx);
//   std::cerr<<pre_event_refs_by_idx.size()<<' '<<req.size()<<'\n';
// 
//   std::map< Uint31_Index, Node_Skeletons_Per_Idx > skels_per_idx;
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("nodes_bin ff"));
// 
//   File_Handle< Uint31_Index, Way_Skeleton > ways_bin(
//       transaction.data_index(osm_base_settings().NODES), req);
//   File_Handle< Uint31_Index, Attic< Way_Skeleton > > ways_attic_bin(
//       transaction.data_index(attic_settings().NODES), req);
//   File_Handle< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > ways_meta_bin(
//       transaction.data_index(meta_settings().NODES_META), req);
//   File_Handle< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > ways_attic_meta_bin(
//       transaction.data_index(attic_settings().NODES_META), req);
//   File_Handle< Uint31_Index, Attic< Way_Skeleton::Id_Type > > ways_undeleted_bin(
//       transaction.data_index(attic_settings().NODES_UNDELETED), req);
// 
//   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
//       ways_meta_to_move_to_attic;
// 
//   //NB: indirection necessary to convey timestamp_end between different idxs if any.
// 
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("first pass by idx"));

  std::map< Uint31_Index, Way_Event_Container > arrived_objects;
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;

  for (auto i_idx : pre_event_refs_by_idx)
  {
    Uint31_Index working_idx = i_idx.first;
    auto& changes = changes_per_idx[working_idx];

    //TODO: File_Handle needs implicit sort
    std::vector< Way_Skeleton > current_ways = ways_bin.obj_with_idx(working_idx);
    std::vector< Attic< Way_Skeleton > > attic_ways = ways_attic_bin.obj_with_idx(working_idx);

// ad _Events_ ohne Meta: [ timestamp_end, Skel* ] eintragen, timestamp_start eintragen, ids_and_timestamps_of(), { timestamp_end, nullptr } spleißen, timestamp_start-Minimum setzen, { Id, timestamp_start, timestamp_end } erzwingen
// ... Meta einflechten ...
// move, da Anzahl auf 0 bis unendlich ändern kann
// Lösch-Abschnitte mit neuen Objekten spleißen

// extract_relevant(vec< Way_Skeleton > current, vec< Attic< Way_Skeleton > > attic, _Pre_Events_Per_Idx_, _Moved_Coords_) -> { vec< Way_Skeleton > current_to_touch, vec< Attic< Way_Skeleton > > attic_to_touch, _Events_ }
    Way_Skeleton_Updater::extract_relevant_current_and_attic(
        i_idx.second, moved_coords,
        current_ways, attic_ways, changes.current_to_del, changes.attic_to_del, changes.unchanged_before, implicit_events);

// extract_undeleted( ids_and_timestamps_of(_Pre_Events_Per_Idx_, _Events_), _Events_ ) -> { vec< Undeleted > undeleted_to_touch, _Events_ }
    /*Update_Events_Preparer*/::extract_relevant_undeleted(
        ids_and_timestamps_of(i_idx.second, implicit_events), ways_undeleted_bin.obj_with_idx(working_idx),
        changes.undeletes_to_del, implicit_events);
//   std::vector< Way_Skeleton::Id_Type > deleted_after_unchanged;

    // NB: unchanged_before remains unaffected from undeletes
    Way_Skeleton_Updater::adjust_implicit_events(changes.existing_undeletes, implicit_events);

    changes.existing_current_meta = Way_Meta_Updater::extract_meta(
        changes.unchanged_before, ways_meta_bin.obj_with_idx(working_idx));
    changes.existing_attic_meta = Way_Meta_Updater::extract_meta(
        changes.unchanged_before, ways_attic_meta_bin.obj_with_idx(working_idx));
    Way_Meta_Updater::detect_deletions(
        changes.existing_current_meta, changes.existing_attic_meta, implicit_events, deletions);
    Way_Meta_Updater::prune_first_skeletons(
        changes.existing_current_meta, changes.existing_attic_meta, implicit_events);

    Meta_Updater::adapt_pre_event_list(working_idx, changes.existing_current_meta, i_idx.second, pre_events);
    Meta_Updater::adapt_pre_event_list(working_idx, changes.existing_attic_meta, i_idx.second, pre_events);

    Update_Events_Preparer::prune_nonexistant_events(i_idx.second, pre_events, implicit_events);

    Way_Skeleton_Updater::resolve_coord_events(
        Way_Meta_Updater::assign_meta(current_meta, attic_meta, implicit_events), changes.events, arrived_objects);
  }

  merge_values(arrived_objects, changes_per_idx);
  {
    std::map< Uint31_Index, Way_Event_Container > pre_event_changes;
    Way_Skeleton_Updater::resolve_coord_events(pre_events, moved_coords, pre_event_changes, deletions);
    merge_values(pre_event_changes, changes_per_idx);
  }
  std::sort(deletions.begin(), deletions.end());

  //TODO: idx_mapfile
  //mapfile_io.compute_and_write_idx_lists(nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);
  {
    std::map< Uint31_Index, std::vector< Way_Skeleton > > current_to_delete;
    std::map< Uint31_Index, std::vector< Way_Skeleton > > current_to_add;
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_to_delete;
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_to_add;

    for (const auto& i : changes_per_idx)
    {
      Way_Skeleton_Updater::Way_Skeleton_Delta skel_delta(
          i.events, i.existing_current, i.existing_attic);
      current_to_delete[i.idx].swap(skel_delta.current_to_delete);
      attic_to_delete[i.idx].swap(skel_delta.attic_to_delete);
      current_to_add[i.idx].swap(skel_delta.current_to_add);
      attic_to_add[i.idx].swap(skel_delta.attic_to_add);
    }

    update_elements(current_to_delete, current_to_add, transaction, *osm_base_settings().WAYS);
    update_elements(attic_to_delete, attic_to_add, transaction, *attic_settings().WAYS);
  }
  {
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton::Id_Type > > > undeletes_to_delete;
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton::Id_Type > > > undeletes_to_add;

    for (const auto& i : changes_per_idx)
    {
      Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
          i.events, i.existing_current, i.unchanged_before);
      undeletes_to_delete[i.idx].swap(undel_delta.undeletes_to_delete);
      undeletes_to_add[i.idx].swap(undel_delta.undeletes_to_add);
    }

    update_elements(undeletes_to_delete, undeletes_to_add, transaction, *attic_settings().WAYS_UNDELETED);
  }
  {
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        current_meta_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        attic_meta_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        current_meta_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        attic_meta_to_add;

    for (const auto& i : changes_per_idx)
    {
      Way_Meta_Updater::Way_Meta_Delta meta_delta(
          i.events, i.existing_current_meta, i.existing_attic_meta, deletions, i.unchanged_before);
      current_meta_to_delete[i.idx].swap(meta_delta.current_to_delete);
      attic_meta_to_delete[i.idx].swap(meta_delta.attic_to_delete);
      current_meta_to_add[i.idx].swap(meta_delta.current_to_add);
      attic_meta_to_add[i.idx].swap(meta_delta.attic_to_add);
    }

    update_elements(current_meta_to_delete, current_meta_to_add, transaction, *meta_settings().WAYS_META);
    update_elements(attic_meta_to_delete, attic_meta_to_add, transaction, *attic_settings().WAYS_META);
  }
}
