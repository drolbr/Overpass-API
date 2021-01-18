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
  //uint64_t timestamp_end;
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
  std::vector< Way_Skeleton > current_to_del;
  std::vector< Attic< Way_Skeleton > > attic_to_del;
  auto undeletes_to_del;
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_meta_to_del;
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_meta_to_del;
  Way_Event_Container events;
};


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

  map< Uint31_Index, Way_Event_Container > arrived_objects;

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
        current_ways, attic_ways, changes.current_to_del, changes.attic_to_del, implicit_events);

// extract_undeleted( ids_and_timestamps_of(_Pre_Events_Per_Idx_, _Events_), _Events_ ) -> { vec< Undeleted > undeleted_to_touch, _Events_ }
    Update_Events_Preparer::extract_relevant_undeleted(
        ids_and_timestamps_of(i_idx.second, implicit_events), ways_undeleted_bin.obj_with_idx(working_idx),
        changes.undeletes_to_del, implicit_events);

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_meta =
        ways_meta_bin.obj_with_idx(working_idx);
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_meta =
        ways_attic_meta_bin.obj_with_idx(working_idx);

// extract_meta( ids_and_timestamps_of(_Pre_Events_Per_Idx_, _Events_) ) -> { vec< Meta > current, vec< Meta > attic }
    //TODO: changes.current_meta_to_del, changes.attic_meta_to_del

// set_minimum_timestamps(vec< Meta > current, vec< Meta > attic, _Events_&)
    Update_Events_Preparer::extract_first_appearance(current_meta, attic_meta, implicit_events);

// adapt_pre_event_list(vec< Meta > current, vec< Meta > attic, _Pre_Events_Per_Idx_&)
    //note: not relevant for found_implicit_pre_events
    Meta_Updater::adapt_pre_event_list(working_idx, current_meta, i_idx.second, pre_events);
    Meta_Updater::adapt_pre_event_list(working_idx, attic_meta, i_idx.second, pre_events);

// prune_nonexistant_events(_Pre_Events_Per_Idx_, _Events_&)
    Update_Events_Preparer::prune_nonexistant_events(i_idx.second, implicit_events);

// assign_meta(vec< Meta > current, vec< Meta > attic, _Events_& )
    auto events_with_meta = assign_meta(current_meta, attic_meta, implicit_events);
// Ab hier mit integriertem Meta. Nicht früher, um die Funktion davor nicht zu verkomplizieren

// resolve_coord_events(_Events_) -> { _Events_, map< Idx, _Events_ > arrived_objects }
    Way_Skeleton_Updater::resolve_coord_events(implicit_events, changes.events, arrived_objects);
    //TODO
  }
// Nach Schleife verfügbar: { _Pre_Events_, map< Idx, { current_to_touch, attic_to_touch, undelete_to_touch, current_meta_to_touch, attic_meta_to_touch, _Events_ } >, arrived_objects }

// join_arrived_objects(map< Idx, _Events_ >, map< Idx, {..., _Events_ } >&)
  join_arrived_objects(arrived_objects, changes_per_idx);
// resolve_coord_events(_Pre_Events_, map< Idx, _Events_ >&)
  Way_Skeleton_Updater::resolve_coord_events(moved_coords, pre_events, changes_per_idx);

// Jetzt: map< Idx, {..., _Events_ } > enthält die vollständige zukünftige Struktur

// vor update_meta: steht als Struktur für alle Idxe fertig bereit
// - Sonderregel zum Meta für schlussendlich gelöschtes Objekt
// current_meta gehört hierher genau dann, wenn letztes Way_Skeleton nicht null ist
// - zusätzlich current_meta_to_delete_benötigt, kann Idx-lokal aus Bestand berechnet werden
// alle attic_meta werden dedupliziert und nur dann nicht (erneut) angelegt, wenn sie sich gegen attic_meta_to_delete aufheben
// - zusätzlich attic_meta_to_delete_benötigt, kann Idx-lokal aus Bestand berechnet werden
// current gehört hierher genau dann, wenn letztes Way_Skeleton nicht null ist
// - zusätzlich current_to_delete (für Gleichheit wohl nur Id) benötigt, kann Idx-lokal aus Bestand berechnet werden
// alle attic werden dedupliziert und nur dann nicht (erneut) angelegt, wenn sie sich gegen attic_to_delete aufheben
// - zusätzlich attic_to_delete benötigt, kann Idx-lokal aus Bestand berechnet werden
// alle undelete werden dedupliziert und nur dann nicht (erneut) angelegt, wenn sie sich gegen undelete_to_delete aufheben
// - zusätzlich undelete_to_delete benötigt, kann Idx-lokal aus Bestand berechnet werden


  // Ansatz: aus pre_events die pre_events_per_computed_idx berechnen.
  // Sind per Idx ein sortierter Container von { Elem, Meta, timestamp_end },
  // wobei nur in wenigen Fällen mehr als ein Objekt entstehen wird.

  // Mechanismus für Meta:
  // - Pre_Events hat Vorrang, kann aber Löcher haben (selten)
  // - Pre_Events bringen ihr Meta mit
  // - implicit_events als zweites, aber ggf. von Undelete betroffene Abschnitte (andere Idxe!) ignorieren
  // - current und attic nachrangig, lösen keine Veränderung aus
  // - dupliziertes Attic ist kein Problem, da vom Update-Meachnismus bereinigt

  // Idx-Sichten?
  // - für pre_events nach der Schleife möglich (wegen Zeitbegrenzung)
  // - für implicit_pre_events ab Einkürzen möglich (bereits am Ende des Schleifenrumpfs)
  // - beides notwendig für Meta-Zuordnung

//   std::cerr<<"A "<<skels_per_idx.size()<<' '<<coord_dates.size()<<'\n';
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("nodes_meta_to_add ff"));
// 
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
      nodes_meta_to_add;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
      nodes_attic_meta_to_add;
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("Node_Meta_Updater::create_update_for_nodes_meta"));
      // templatisieren!
      // deduplizieren
  Meta_Updater::create_update_for_nodes_meta(
      pre_events, nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);
  // ways_meta_to_add += meta of pre_events_per_computed_idx with timestamp_end == NOW ...
  // ways_meta_to_delete += ways_to_move
  // attic_ways_meta_to_add += ways_to_move + meta of pre_events_per_computed_idx with timestamp_end != NOW
  // attic_ways_meta_to_delete already complete

//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("mapfile_io.compute_and_write_idx_lists"));
  // compute nodes_map, nodes_attic_map and nodes_idx_lists from pre_event_refs_by_idx, nodes_meta_to_add, and nodes_attic_meta_to_add
  // write meta, nodes_map, nodes_attic_map, nodes_idx_lists
  mapfile_io.compute_and_write_idx_lists(nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(meta)"));
  update_elements(nodes_meta_to_move_to_attic, nodes_meta_to_add, transaction, *meta_settings().NODES_META);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(attic_meta)"));
  update_elements(
      decltype(nodes_attic_meta_to_add)(), nodes_attic_meta_to_add, transaction, *attic_settings().NODES_META);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("nodes_to_delete ff"));
// 
//   std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_delete;
//   std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_add;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_delete;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_add;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_delete;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_add;
// 
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("second pass by idx"));
//   for (auto i_idx : pre_event_refs_by_idx)
//   {
//     Uint31_Index working_idx = i_idx.first;
//     Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];
// 
//     Node_Event_List events(working_idx, skels, i_idx.second, pre_events);
//     Prepare_Node_Update::create_update_for_nodes(
//         events, nodes_to_delete[working_idx], nodes_to_add[working_idx]);
//     Prepare_Node_Update::create_update_for_nodes_attic(
//         events, nodes_attic_to_delete[working_idx], nodes_attic_to_add[working_idx]);
//     Prepare_Node_Update::create_update_for_nodes_undelete(
//         events, nodes_undelete_to_delete[working_idx], nodes_undelete_to_add[working_idx]);
//   }
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(nodes)"));
// 
//   // write nodes, nodes_attic, nodes_undelete
//   update_elements(nodes_to_delete, nodes_to_add, transaction, *osm_base_settings().NODES);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(nodes_attic)"));
//   update_elements(nodes_attic_to_delete, nodes_attic_to_add, transaction, *attic_settings().NODES);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(nodes_undeleted)"));
//   update_elements(
//       nodes_undelete_to_delete, nodes_undelete_to_add, transaction, *attic_settings().NODES_UNDELETED);

  //TODO: tags, nodes_for_ways
}
