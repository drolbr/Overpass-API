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


//TODO: Tags

// hinterer Teil, Problem: unterschiedliche Index-Grobheit

// Ansatz 1: Id-Hashes
// Problem: Quadratisch beim Auslesen

// Ansatz 2: pre_event_changes um Tags erweitern und sofort projizieren
// lokal-quadratisches Einfügen oder Einfügen und Sortieren
// { { Idx, K,V }, [ { Id, not_before, before } ]

// Ansatz 3: Way_Event_With_Tags
// { [ {Id, [{k,v}]} ] tags_at_last_unchanged, [ {Id, not_before, before, [{k,v}] ] }

// Regeln: !old: nop, v[old_k] == v[new_k]: nop, !v[old_k] && old: marker, v[old_k] != v[new_k]: value
// letzte nach cur für NOW, sonst nach attic mit before

/*
{
  skel = begin
  for (id)
  {
    for (i : [V..])
    {
      for (key)
      {
        ...

        reset skel to id

        if (i.before == NOW)
          current.push({idx,k,v}, id)
        else if (i.v != i.next.v)
          attic.push({idx,k,v}, {id,i.before})

        while (skel.{id, before} <= i.{id, not_before})
          ++skel
        if (i.prev.id == skel.id && i.prev.before < skel.prev.before)
          attic.push({idx,k,void}, {id,i.not_before})
      }
    }
  }

  sort(attic)
  sort(current)
  ...
*/


// Modifiziert: { Idx, [ { K,V, [ { Id, not_before, before } ] } ], [ { Id, not_before, before } ] }


// Existierend: auch Tags am Minimum
// - Liste der { id, unchanged_before } aufbauen
// - Nur zutreffende Tags {id, >unchanged_before} behalten, current einsortieren
// - Nach {id, k, timestamp} sortieren

// Modifiziert: { Idx, [ { Id, not_before, before, [ { K, V } ] } ] }

// [ { Id, K }, [ { V, not_before, before } ] } ]
/*
{
  skel = begin
  for (id)
  {
    for (key)
    {
      reset skel to id
      for (i : [V..])
      {
        if (i.before == NOW)
          current.push({idx,k,v}, id)
        else if (i.v != i.next.v)
          attic.push({idx,k,v}, {id,i.before})

        while (skel.{id, before} <= i.{id, not_before})
          ++skel
        if (i.prev.id == skel.id && i.prev.before < skel.prev.before)
          attic.push({idx,k,void}, {id,i.not_before})
      }
    }
  }

  sort(attic)
  sort(current)

  for (i : attic)
  {
    while (exist_attic.{k,v,id,time} < i.{k,v,id,time})
    {
      del(exist_attic)
      ++exist_attic;
    }
    if (exist_attic.{k,v,id,time} == i.{k,v,id,time})
      ++exist_attic
    else
      ins(i)
  }
  while (exist_attic)
  {
    del(exist_attic)
    ++exist_attic;
  }

  for (i : current)
  {
    while (exist_current.{k,v,id} < i.{k,v,id})
    {
      del(exist_current)
      ++exist_current;
    }
    if (exist_current.{k,v,id} == i.{k,v,id})
      ++exist_current
    else
      ins(i)
  }
  while (exist_current)
  {
    del(exist_current)
    ++exist_current;
  }
}
*/

/*
Tag_Delta::Tag_Delta(idx, existing_local, tag_events.begin, tag_events.end)
{
  for (i : tag_events)
  {
    while (existing_local.{kv, id} < i.{kv, id})
    {
      to_del.add(existing_local)
      ++existing_local;
    }

    // Kill-Events
    // Positiv-Events
    ...
  }
  while (existing_local.idx == idx)
  {
    to_del.add(existing_local)
    ++existing_local;
  }
}
*/

/* Problem: Wir wissen nicht, ob das Objekt bereits gelöscht war, in einem anderen Feinindex lebt
 * oder ob es frisch überschrieben, verschoben oder gelöscht ist.
 * 
 * Komplikation: implizit verschobene Objekte, bei denen nicht die komplette Folgegeschichte bekannt ist.

Für Tags: andere Datenstruktur [ { id, key, [ { not_before, before, value } ] } ]


Schritt 1: bestehende Tags extrahieren
{
  for (i : events)
  {
    while (unchanged.id < i.id)
    {
      extract_key_timeline(unchanged.id, unchanged.not_before, true, it_tags, result);
      ++unchanged;
    }
    if (unchanged.id == i.id)
    {
      extract_key_timeline(unchanged.id, unchanged.not_before, true, it_tags, result);
      ++unchanged;
    }
    else if (it_tags->id <= i.id)
      extract_key_timeline(i.id, i.not_before, false, it_tags, result);
  }
}


extract_key_timeline(id, not_before, including_not_before, source, result)
{
  while (source.id < id)
    ++source;
  while (source.id == id)
  {
    if (last_key != source.key)
      last_time = 0;

    if (including_not_before && not_before == source.before)
    {
      result.unchanged.push({ id, source.before, source.key, source.value });
      last_time = not_before;
    }
    else if (not_before < source.before)
    {
      if (source.value != invalid)
        result.tags.push({ id, last_time, source.before, source.key, source.value });
      last_time = source.before;
    }

    last_key = source.key;
    ++source;
  }
}


Schritt 2: deduplicate, da Index gröber
*/



/* Überblick:
- Bestands-Meta (und kein Skel, Undel, Tags) entscheidet über Scope von Neu-Objekten
- Für implizite Änderungen anytime-recurse(bn) mit voller Objektrekonstruktion, dann ggf. Neuverteilung auf Indexe
- Zusätzlich Einkürzen bestehender Objekte aufgrund von Neu-Objekten

- Bem: Nachermitteln von Objekten an Zielindizes der Verschiebung, ohne Einkürzen, aber für Kontext

- Berechnung der Indexe für Neu-Objekte

Vier Fälle für Tags:
- keine Änderung des Grobindexes, keine Neu-Objekte: nicht relevant fürs Tagging
- Änderung des Grobindexes, keine Neu-Objekte: Tags am Quell- und Zielort ?!?, Quelldaten als [ { id, key, [ { not_before, before, value } ] } ], Attic setzen
- keine Änderung des Grobindexes, Neu-Objekte: Tags am Zielort ?!?, Quelldaten als [ { id, key, [ { not_before, before, value } ] } ]
- Änderung des Grobindexes und Neu-Objekte: Tags am Quell- und Zielort ?!?, Attic setzen,  Quelldaten als [ { id, key, [ { not_before, before, value } ] } ] und zusammenführen

Wir kennen die per Index betroffenen Objekte erst nach dem ersten Durchlauf. Es sind wenig genug zur Enumeration.

Undelete-Grenzen: [ { id, not_before } ]
Move-Meldungen: [ { id, [{ idx, not_before, before }] } ]
Löschmeldungen: [ { id, before } ]

Undelete-Grenzen: [ { id, not_before } ]
Datenbestand
~> Bestand: [ { id, key, [ { before ab inkl unchanged, value inkl. invalid } ] } ]

Bestand: [ { id, key, [ { before ab inkl unchanged, value inkl. invalid } ] } ]
Move-Meldungen: [ { id, [{ idx, not_before, before }] } ]
~> Neu anzulegen: [ { id, [{ not_before, before }], [{ key, [ { not_before, before, value } ] }] ]

Löschmeldungen: [ { Id, before } ]
Bestand: [ { id, key, [ { before ab inkl unchanged, value inkl. invalid } ] } ]
Neu anzulegen: [ { id, [{ not_before, before }], [{ key, [ { not_before, before, value } ] }] ]


Move-Meldungen: [ { id, [{ idx, not_before, before }] } ]
Neue Objekte

<~> vervollständige Move-Meldungen
  expires aus Neue Objekte plus Move-Meldungen
  not_before aus Neue Objekte und Move-Targets

Move-Meldungen: [ { id, not_before, expires, [{ idx, not_before, before }] } ]

<~ read()
  per idx,k,v
    falls id in Liste und not_before <= before
      nach Bestand

Bestand: [ { id, key, [ { before ab inkl unchanged, value inkl. invalid } ] } ]
Move-Meldungen: [ { id, expires, [{ idx, not_before, before }] } ]

<~> record_tags_of_moved_objects
  per id
    am alten Idx ggf.
      nach Neu anzulegen { not_before oder expires, NOW } ohne Keys
    sort per idx
    per idx
      min(not_before), max(before)
      nach Neu anzulegen alle [{ not_before, before }]
      und alle [{ key, [ { not_before, before, value } ] }] aus dem Bestand mit not_before = 0 oder before vom Vorgänger

Bestand: [ { id, key, [ { before ab inkl unchanged, value inkl. invalid } ] } ]
Neu anzulegen: [ { id, [{ not_before, before }], [{ key, [ { not_before, before, value } ] }] ]

<~> merge_values ergänzt (nur) neu anzulegen
  und nur
    per {id, not_before, before}
      push {not_before, before}
      per key
        push {not_before, before, value}

danach sortieren

Bestand: [ { id, key, [ { before ab inkl unchanged, value inkl. invalid } ] } ]
Neu anzulegen: [ { id, [{ not_before, before }], [{ key, [ { not_before, before, value } ] }] ]

~> Way_Delta
Bem: Idee, Löschmeldungen in neu_anzulegen zu integrieren

for neu_anzulegen:
  falls bestand kleiner:
    ++bestand
  falls bestand gleiche id:
    for bestand.keys
      if (neu.key < key)
        process_key(i_neu_id, i_neu_key, null, to_del, to_ins)
      process_key(i_neu_id, i_neu_key, bestand, to_del, to_ins)
    while neu.key
      process_key(i_neu_id, i_neu_key, null, to_del, to_ins)
  sonst:
    for keys:
      vorher.before = i_neu_key.not_before
      for timeline:
        falls vorher.before echt kleiner not_before dann { idx, key, invalid }.insert { id, before }
        falls NOW dann { idx, key, value }.insert id
        sonst { idx, key, value }.insert { id, before }


process_key(i_neu_id, i_neu_key, bestand, to_del&, to_ins&)
  Per_Key_Collector coll
  for i_neu_id
    while i_neu_key.before <= i_neu_id.not_before
      ++i_neu_key
    vorher.before = i_neu_id.not_before
    while i_neu_key.before <= i_neu_id.before
      if vorher.before < max(not_before, not_before)
        coll.set(vorher.before, max(not_before, not_before), inval)
      coll.set(max(not_before, not_before), i_neu_key.before, value)
      vorher.before = i_neu_key.before
      ++i_neu_key
    if i_neu_key.not_before < i_neu_id.before
      if vorher.before < max(not_before, not_before)
        coll.set(vorher.before, max(not_before, not_before), inval)
      coll.set(max(not_before, not_before), i_neu_id.before)
  ~coll


Per_Key_Collector
{ bestand, i_bestand, to_del_intern, to_ins_intern, to_del&, to_ins& }

  ~():
    copy to_del_intern, to_ins_intern

  set(not_before, before, value):
    if to_ins_intern.back.{before, value} == {not_before, value}
      to_ins_intern.pop
    else
      while i_bestand.before < not_before
        ++i_bestand
      if i_bestand.before == not_before
        if i_bestand.value == value
          to_del_intern i_bestand
        ++i_bestand
    while i_bestand.before < before
      to_del_intern i_bestand
    if i_bestand.before == before
      if i_bestand.value != value
        to_del_intern i_bestand
        to_ins_intern { before, value }
    else
      to_ins_intern { before, value }
*/



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
  std::map< Uint31_Index, Tagdata_By_Idx_Id > tags_by_id;

  for (auto i_idx : pre_event_refs_by_idx)
  {
    Uint31_Index working_idx = i_idx.first;
    auto& changes = changes_per_idx[working_idx];

    //TODO: File_Handle needs implicit sort
    std::vector< Way_Skeleton > current_ways = ways_bin.obj_with_idx(working_idx);
    std::vector< Attic< Way_Skeleton > > attic_ways = ways_attic_bin.obj_with_idx(working_idx);

    Way_Skeleton_Updater::extract_relevant_current_and_attic(
        i_idx.second, moved_coords,
        changes.existing_current, changes.existing_attic, changes.unchanged_before, implicit_events);

    changes.existing_undeletes = Way_Skeleton_Updater::extract_undeleted(
        changes.unchanged_before, ways_undeleted_bin.obj_with_idx(working_idx));
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

    // eval_tag_idx_movements(working_idx, arrived_objects) ~> [ { id, old_idx, new_idx, not_before, before } ]
    // (merge adjacent)

    // merge_values(arrived_objects, arrived_objects)

//     Way_Tag_Updater::tags_of_unchanged_before(
//         changes.unchanged_before, Way_Tag_Updater::Full_Tag_Store::get_by_idx(working_idx), working_idx, tags_by_id);
// 
//     // Full_Tag_Store: id -> [ { key, [ { id, timestamp, Tag_Idx_Local* } ] ]
//     Way_Tag_Updater::eval_tags(
//         changes.events, Way_Tag_Updater::Full_Tag_Store::get_by_idx(working_idx), working_idx, tags_by_id);
//     for (auto i : arrived_objects)
//       Way_Tag_Updater::eval_tags(i.second, Way_Tag_Updater::Full_Tag_Store::get_by_idx(working_idx), i.first, tags_by_id);
  }

  //TODO: Konflikte, wenn in Indexe mit alten Versionen hineingeschrieben wird. Reicht Anpassung von Undelete?
  /* skel nur für Anschlüsse betroffen, meta kann Duplikate haben, undel voll betroffen: existierende fallen für die Neueinträge weg und zusätzliche können erforderlich werden zur Abgrenzung von Neueinträgen werden, Tags wie undel und zusätzlich Anschlüsse. */
  // Näherungsweise ist es möglich, die umgezogenen Einträge als Neueinträge zu behandeln, außer für Meta

  merge_values(arrived_objects, changes_per_idx);
  {
    std::map< Uint31_Index, Way_Event_Container > pre_event_changes;
    Way_Skeleton_Updater::resolve_coord_events(pre_events, moved_coords, pre_event_changes, deletions);
    Way_Tag_Updater::merge_values(pre_event_changes, tags_by_id);
    merge_values(pre_event_changes, changes_per_idx);
  }
  std::sort(deletions.begin(), deletions.end());
  sort(tags_by_id);

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
  {
    Way_Tag_Delta tag_delta(
        tags_by_id, tag_changes.existing_current, tag_changes.existing_attic);

    update_elements(
        tag_delta.current_to_delete, tag_delta.current_to_add, transaction, *osm_base_settings().WAY_TAGS_LOCAL);
    update_elements(
        tag_delta.attic_to_delete, tag_delta.attic_to_add, transaction, *attic_settings().WAY_TAGS_LOCAL);

    //TODO: Global
  }
}
