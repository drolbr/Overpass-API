
#include "way_skeleton_updater.h"


namespace
{
  std::vector< std::pair< decltype(Way_Skeleton::geometry.size()), const Move_Coord_Event* > > process_way(
      const Way_Skeleton& way, const Moved_Coords& moved_coords, uint64_t before)
  {
    std::vector< std::pair< decltype(way.geometry.size()), const Move_Coord_Event* > > result;
    if (!way.nds.empty() && way.nds.size() != way.geometry.size())
      throw std::logic_error("Way_Skeleton_Updater::process_way: size of nds does not match size of geometry");
    for (decltype(way.geometry.size()) i = 0; i < way.geometry.size(); ++i)
    {
      if (!way.nds.empty() && way.nds[i].val())
      {
        std::vector< const Move_Coord_Event* > events = moved_coords.get_id(way.nds[i]);
        for (auto j : events)
        {
          if (j->timestamp < before)
            result.push_back(std::make_pair(i, j));
        }
      }
      else
      {
        std::vector< const Move_Coord_Event* > events =
            moved_coords.get_coord(way.geometry[i].ll_upper, way.geometry[i].ll_lower);
        for (auto j : events)
        {
          if (j->timestamp < before)
            result.push_back(std::make_pair(i, j));
        }
      }
    }

    std::sort(result.begin(), result.end(),
        [](const decltype(result.front())& lhs, const decltype(result.front())& rhs)
        { return lhs.second->timestamp < rhs.second->timestamp ||
            (!(rhs.second->timestamp < lhs.second->timestamp) && lhs.first < rhs.first); });

    return result;
  }
}


void Way_Skeleton_Updater::extract_relevant_current_and_attic(
    const Way_Pre_Event_Refs& pre_event_refs,
    const Moved_Coords& moved_coords,
    const std::vector< const Way_Skeleton* >& current,
    const std::vector< const Attic< Way_Skeleton >* >& attic,
    std::vector< Way_Skeleton >& current_result,
    std::vector< Attic< Way_Skeleton > >& attic_result,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events)
{
  auto i_cur = current.begin();
  Way_Skeleton::Id_Type last_ref = 0u;
  uint64_t last_timestamp = 0u;
  bool last_found = false;
  for (auto i : attic)
  {
    while (i_cur != current.end() && (*i_cur)->id < i->id)
    {
      bool found = (last_ref == (*i_cur)->id && last_found);
      if (!found)
      {
        for (const auto& j : pre_event_refs)
          found |= ((*i_cur)->id == j.ref);
      }

      auto pos_events = process_way(**i_cur, moved_coords, NOW);
      if (found || !pos_events.empty())
      {
        current_result.push_back(**i_cur);
        implicit_pre_events.push_back(
            Way_Implicit_Pre_Event{ **i_cur, last_ref == (*i_cur)->id ? last_timestamp : 0u, NOW, std::move(pos_events) });
      }

      ++i_cur;
    }

    bool found = (last_ref == i->id && last_found);
    if (!found)
    {
      for (const auto& j : pre_event_refs)
        found |= (i->id == j.ref && j.timestamp <= i->timestamp);
    }

    auto pos_events = process_way(*i, moved_coords, i->timestamp);
    last_found = found || !pos_events.empty();
    if (found || !pos_events.empty())
    {
      attic_result.push_back(*i);
      implicit_pre_events.push_back(
          Way_Implicit_Pre_Event{ *i, last_ref == (*i_cur)->id ? last_timestamp : 0u, i->timestamp, std::move(pos_events) });
    }

    last_ref = i->id;
    last_timestamp = i->timestamp;
  }
  while (i_cur != current.end())
  {
    bool found = (last_ref == (*i_cur)->id && last_found);
    if (!found)
    {
      for (const auto& j : pre_event_refs)
        found |= ((*i_cur)->id == j.ref);
    }

    auto pos_events = process_way(**i_cur, moved_coords, NOW);
    if (found || !pos_events.empty())
    {
      current_result.push_back(**i_cur);
      implicit_pre_events.push_back(
          Way_Implicit_Pre_Event{ **i_cur, last_ref == (*i_cur)->id ? last_timestamp : 0u, NOW, std::move(pos_events) });
    }

    ++i_cur;
  }
}


void Way_Skeleton_Updater::resolve_coord_events(
    Uint31_Index cur_idx,
    const std::vector< Proto_Way >& proto_events,
    Way_Event_Container& events_for_this_idx,
    std::map< Uint31_Index, Way_Event_Container >& arrived_objects)
{
  for (const auto& i : proto_events)
  {
    Way_Skeleton cur = i.base;
    uint64_t timestamp = i.not_before;
    for (auto j : i.pos_events)
    {
      if (timestamp < j.second->timestamp)
      {
        Uint31_Index skel_idx = calc_index(cur.geometry);
        if (skel_idx == cur_idx)
          events_for_this_idx.push_back(Way_Event{ cur, i.meta, timestamp, j.second->timestamp });
        else
          arrived_objects[skel_idx].push_back(Way_Event{ cur, i.meta, timestamp, j.second->timestamp });

        timestamp = j.second->timestamp;
      }

      if (j.second->visible_after)
      {
        cur.geometry[j.first].ll_upper = j.second->idx_after.val();
        cur.geometry[j.first].ll_lower = j.second->ll_lower_after;
      }
      else
      {
        cur.geometry[j.first].ll_upper = 0u;
        cur.geometry[j.first].ll_lower = 0u;
      }

      if (j.second->multiple_after || !j.second->visible_after)
      {
        if (cur.nds.empty())
          cur.nds.resize(cur.geometry.size(), Node_Skeleton::Id_Type(0ull));
        cur.nds[j.first] = j.second->node_id;
      }
      else if (!cur.nds.empty())
        cur.nds[j.first] = 0ull;
    }

    Uint31_Index skel_idx = calc_index(cur.geometry);
    if (skel_idx == cur_idx)
      events_for_this_idx.push_back(Way_Event{ cur, i.meta, timestamp, i.before });
    else
      arrived_objects[skel_idx].push_back(Way_Event{ cur, i.meta, timestamp, i.before });
  }
}


void Way_Skeleton_Updater::resolve_coord_events(
    const Pre_Event_List< Way_Skeleton >& pre_events,
    const Moved_Coords& moved_coords,
    std::map< Uint31_Index, Way_Event_Container >& changes_per_idx,
    std::vector< Way_Deletion >& deletions)
{
  for (const auto& i : pre_events.data)
  {
    Way_Skeleton cur = i.entry->elem;
    uint64_t timestamp = i.entry->meta.timestamp;

    if (cur.id == Way_Skeleton::Id_Type(0u))
      deletions.push_back(Way_Deletion{ i.entry->meta, timestamp, i.timestamp_end });
    else
    {
      auto pos_events = process_way(i.entry->elem, moved_coords, i.timestamp_end);
      for (auto j : pos_events)
      {
        if (timestamp < j.second->timestamp)
        {
          Uint31_Index skel_idx = calc_index(cur.geometry);
          changes_per_idx[skel_idx].push_back(Way_Event{ cur, i.entry->meta, timestamp, j.second->timestamp });

          timestamp = j.second->timestamp;
        }

        if (j.second->visible_after)
        {
          cur.geometry[j.first].ll_upper = j.second->idx_after.val();
          cur.geometry[j.first].ll_lower = j.second->ll_lower_after;
        }
        else
        {
          cur.geometry[j.first].ll_upper = 0u;
          cur.geometry[j.first].ll_lower = 0u;
        }

        if (j.second->multiple_after || !j.second->visible_after)
        {
          if (cur.nds.empty())
            cur.nds.resize(cur.geometry.size(), Node_Skeleton::Id_Type(0ull));
          cur.nds[j.first] = j.second->node_id;
        }
        else if (!cur.nds.empty())
          cur.nds[j.first] = 0ull;
      }

      Uint31_Index skel_idx = calc_index(cur.geometry);
      changes_per_idx[skel_idx].push_back(Way_Event{ cur, i.entry->meta, timestamp, i.timestamp_end });
    }
  }
}


// Alternative Datenstruktur:
// - vec< Way_SKeleton > aller referenzierten Way_Skeleton
// - { id, vec({ timestamp_start, meta, Way_Skeleton* }) } als komplette Historie je Index
// ggf. meta als Zeiger

// Wann interessiere ich mich überhaupt für ein Way_Skeleton?
// - Wenn es in pre_events vorkommt und timestamp_end später als die erste Version in pre_events liegt
// - Wenn mindestens ein Node (aufgrund seiner Koordinate) ein Änderungsereignis hat, das früher als timestamp_end liegt
// Sonderfall: Abgleich aller Ways mit ungeklärten Nodes (normalerweise null Ways), ob diese jetzt bekannt werden. In Index 0 abwickeln.

// Hier bleibt Way_Implicit_Pre_Event als Struktur, um auf Alternative auf Event-Basis wechseln zu können.
// Auch: komplette Bestandsobjekte speichern (bzw. reicht ohne Geometrie und Refs) für Löschvorgabe.

// Zusammen mit undelete wird aus den gesammelten Attic< Way_Skeleton > und Way_Skeleton die Struktur belegt:
// - erstes timestamp_start vorläufig auf timestamp_end des nächstälteren Attic< Way_Skeleton >, sonst 0
// - timestamp_end wird timestamp_start des nächsten Nachfolgers. Abwesenheit des current führt zu einem Löschvermerk.
// - meta ist vorläufig leer
// Mit Meta wird:
// - jedem Event das jüngste nicht echt jüngere Meta zugeordnet
// - für die erste timestamp_start dann das Meta auf Basis wie oben der nächsten timestamp_start zugeordnet, wenn timestamp_start null ist

// ...
// Brainstorming: Einfluss der Komponenten:
// current, attic, undelete: zusammen vec({ timestamp_end, Way_Skeleton* }). Kontrollieren, wann Objekte ausgewertet werden
// implicit_events: { timestamp, Ref* }
// pre_events: Zeiträume ausblenden. Zumeist wohl nur irgendwo in current. Als vec({ timestamp_start, _flag_begin }) plus vec({ timestamp_end, _flag_end }) einsortieren.
// current_meta, attic_meta: nur ein timestamp relevant, und zwar das jüngste das nicht jünger als ältester current/attic ist

// Fließband:
// current, attic, undelete: zusammen vec({ timestamp_end, Way_Skeleton* }) sortieren

// mit current_meta, attic_meta shiften zu vec({ timestamp_start, Way_Skeleton* })
// pre_events einflechten: vec({ timestamp_start, Way_Skeleton* }). Fast immer genau 1 Eintrag. Null falls innerhalb von undelete. Zwei (mit separater timestamp_end, Way_Skeleton* wiederholt) falls strikt innerhalb von Objekt.
// implicit_events einflechten: sortieren, Way_Skeleton* sweepen.

// Kann gut aus pre_events nachbelegt werden: In den betreffenden Indexen ist ja bereits ein entsprechender Löschblock eingetragen.
// process_way -> { timestamp_start, Way_Skeleton }: danach ggf. auf mehrere Index-Anteile verteilen.

// Entwurf 2:
