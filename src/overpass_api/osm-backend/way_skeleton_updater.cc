
#include "way_skeleton_updater.h"


namespace
{
  std::vector< Way_Implicit_Pre_Event > process_way(
      const Way_Skeleton& way, const Moved_Coords& moved_coords, uint64_t not_before, uint64_t before,
      bool add_initial_element)
  {
    std::vector< Way_Implicit_Pre_Event > result;

    std::vector< std::pair< decltype(way.geometry.size()), const Move_Coord_Event* > > pos_events;
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
            pos_events.push_back(std::make_pair(i, j));
        }
      }
      else
      {
        std::vector< const Move_Coord_Event* > events =
            moved_coords.get_coord(way.geometry[i].ll_upper, way.geometry[i].ll_lower);
        for (auto j : events)
        {
          if (j->timestamp < before)
            pos_events.push_back(std::make_pair(i, j));
        }
      }
    }

    if (pos_events.empty())
    {
      if (add_initial_element)
        result.push_back(Way_Implicit_Pre_Event{ way.id, not_before, way.geometry, way.nds });
      return result;
    }

    std::sort(pos_events.begin(), pos_events.end(),
        [](const decltype(pos_events.front())& lhs, const decltype(pos_events.front())& rhs)
        { return lhs.second->timestamp < rhs.second->timestamp ||
            (!(rhs.second->timestamp < lhs.second->timestamp) && lhs.first < rhs.first); });

    Way_Skeleton cur = way;
    uint64_t timestamp = pos_events.front().second->timestamp;
    if (add_initial_element && not_before < timestamp)
      timestamp = not_before;
    for (auto i : pos_events)
    {
      if (i.second->timestamp != timestamp)
      {
        if (not_before < timestamp)
          result.push_back(Way_Implicit_Pre_Event{ way.id, timestamp, cur.geometry, cur.nds });
        else if (not_before < i.second->timestamp)
          result.push_back(Way_Implicit_Pre_Event{ way.id, not_before, cur.geometry, cur.nds });
      }

      if (i.second->visible_after)
      {
        cur.geometry[i.first].ll_upper = i.second->idx_after.val();
        cur.geometry[i.first].ll_lower = i.second->ll_lower_after;
      }
      else
      {
        cur.geometry[i.first].ll_upper = 0u;
        cur.geometry[i.first].ll_lower = 0u;
      }

      if (i.second->multiple_after || !i.second->visible_after)
      {
        if (cur.nds.empty())
          cur.nds.resize(cur.geometry.size(), Node_Skeleton::Id_Type(0ull));
        cur.nds[i.first] = i.second->node_id;
      }
      else if (!cur.nds.empty())
        cur.nds[i.first] = 0ull;

      timestamp = i.second->timestamp;
    }
    if (not_before < timestamp)
      result.push_back(Way_Implicit_Pre_Event{ way.id, timestamp, cur.geometry, cur.nds });
    else
      result.push_back(Way_Implicit_Pre_Event{ way.id, not_before, cur.geometry, cur.nds });

    return result;
  }


  bool collect_events_for_way(const Way_Skeleton& way, uint64_t before, const Moved_Coords& moved_coords,
      std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      Way_Skeleton::Id_Type last_ref, uint64_t last_timestamp, bool last_implicitly_moved)
  {
    std::vector< Way_Implicit_Pre_Event > this_obj_implicit =
        process_way(way, moved_coords, last_ref == way.id ? last_timestamp : 0u, before,
            last_ref == way.id && last_implicitly_moved);
    bool events_found = !this_obj_implicit.empty();
    if (!this_obj_implicit.empty() &&
          (implicit_pre_events.empty() || !(implicit_pre_events.back().id == way.id)))
      implicit_pre_events.push_back(
          Way_Implicit_Pre_Event{ way.id, last_ref == way.id ? last_timestamp : 0u, way.geometry, way.nds });
    std::move(this_obj_implicit.begin(), this_obj_implicit.end(), std::back_inserter(implicit_pre_events));
    return events_found;
  }
}


void Way_Skeleton_Updater::extract_relevant_current_and_attic(
    const Way_Pre_Event_Refs& pre_event_refs,
    const Moved_Coords& moved_coords,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< const Way_Skeleton* >& current,
    const std::vector< const Attic< Way_Skeleton >* >& attic,
    std::vector< Way_Skeleton >& current_result,
    std::vector< Attic< Way_Skeleton > >& attic_result)
{
  auto i_cur = current.begin();
  Way_Skeleton::Id_Type last_ref = 0u;
  uint64_t last_timestamp = 0u;
  bool last_implicitly_moved = false;
  bool last_found = false;
  for (auto i : attic)
  {
    while (i_cur != current.end() && (*i_cur)->id < i->id)
    {
      bool found = false;
      for (const auto& j : pre_event_refs)
      {
        if ((*i_cur)->id == j.ref)
        {
          current_result.push_back(**i_cur);
          found = true;
        }
      }

      bool events_found = collect_events_for_way(
          **i_cur, NOW, moved_coords, implicit_pre_events, last_ref, last_timestamp, last_implicitly_moved);
      if (!found && events_found)
        current_result.push_back(**i_cur);

      ++i_cur;
    }

    bool found = false;
    if (last_ref == i->id && last_found)
    {
      attic_result.push_back(*i);
      found = true;
    }
    else
    {
      for (const auto& j : pre_event_refs)
      {
        if (i->id == j.ref && j.timestamp <= i->timestamp)
        {
          attic_result.push_back(*i);
          found = true;
        }
      }
    }

    bool events_found = collect_events_for_way(
        *i, i->timestamp, moved_coords, implicit_pre_events, last_ref, last_timestamp, last_implicitly_moved);
    if (!found && events_found)
      attic_result.push_back(*i);

    last_ref = i->id;
    last_timestamp = i->timestamp;
    last_implicitly_moved = events_found;
    last_found = found || events_found;
  }
  while (i_cur != current.end())
  {
    bool found = false;
    for (const auto& j : pre_event_refs)
    {
      if ((*i_cur)->id == j.ref)
      {
        current_result.push_back(**i_cur);
        found = true;
      }
    }

    bool events_found = collect_events_for_way(
        **i_cur, NOW, moved_coords, implicit_pre_events, last_ref, last_timestamp, last_implicitly_moved);
    if (!found && events_found)
      current_result.push_back(**i_cur);

    ++i_cur;
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


std::set< Uint31_Index, std::vector< Way_Complete_Pre_Event > > compute_geometry(
    const Moved_Coords& moved_coords,
    const Pre_Event_List< Way_Skeleton >& pre_events,
    const Way_Pre_Event_Refs& pre_event_refs)
{
  std::set< Uint31_Index, std::vector< Way_Complete_Pre_Event > > result;

  for (const auto& i : pre_events.data)
  {
    ...
  }

  return result;
}
