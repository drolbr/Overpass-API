#include <map>
#include <set>
#include <vector>


struct Id_Dates_Global
{
  // Set of (id, min_date)
};
//TODO


Id_Dates_Global::Id_Dates_Global(const New_Data& new_data);
//TODO


struct Id_Dates
{
  // Per idx:
  // Set of (id, min_date)
};
//TODO


/* Uses files nodes.map, nodes_attic.map, node_idx_lists */
Id_Dates read_idx_list(
    const Id_Dates_Global& ids);
//TODO


typedef std::vector< Attic< Coord > > Coord_Dates_Per_Idx;


//TODO: prefill coords with new coords


//TODO: Read files nodes, nodes_attic, nodes_meta, nodes_meta_attic


// Adds to the result all coordinates from nodes whose ids appear in id_dates
void collect_relevant_coords_current(
    const Id_Dates_Per_Idx& id_dates,
    const std::vector< Node_Skeleton >& current,
    Coord_Dates_Per_Idx& result);
/* Assertions:
 * An object r is contained in the result if and only if
 * it is already contained in the result before the function or
 * if there exists a node n in attic and an entry e in id_dates such
 * that e.id == n.id == r.id and r.date == e.date
 */


// Adds to the result all coordinates from nodes whose ids appear in id_dates
void collect_relevant_coords_attic(
    const Id_Dates_Per_Idx& id_dates,
    const std::vector< Attic< Node_Skeleton > >& attic,
    Coord_Dates_Per_Idx& result);
/* Assertions:
 * An object r is contained in the result if and only if
 * it is already contained in the result before the function or
 * if there exists a node n in attic and an entry e in id_dates such
 * that e.id == n.id == r.id and r.date == e.date < n.end
 *
 * NB: Could be optimized away if we know beforehand
 * that no current_meta in this idx is younger than the oldest date from id_dates.
 */


/* Invariants:
 * - events are sorted by (id, time)
 * Note: specific for a given working_idx
 */
struct Node_Skeletons_Per_Idx
{
  std::vector< Node_Skeleton > current;
  std::vector< Attic< Node_Skeleton > > attic;
  std::vector< Attic< Node_Skeleton::Id_Type > > undeleted;
  Id_Dates_Per_Idx first_appearance;
};


// Keeps from all the objects in the idx only those that are relevant for the event list
std::vector< Node_Skeleton > extract_relevant_current(
    const Id_Dates_Per_Idx& id_dates,
    Id_Dates_Per_Idx& coord_sharing_ids,
    const Coord_Dates_Per_Idx& coord_dates,
    const std::vector< Node_Skeleton >& current);
/* Assertions:
 * An object r is in the result if and only if it is in attic and
 * - its id is in id_dates  or
 * - its coordinate is in coord_dates
 * NB: an extra condition could be applied to avoid too old objects,
 * but is avoided at the moment for the sake of simplicity.
 *
 * An object r is in coord_sharing_ids if r.id is not in id_dates
 * but the coord of the object e with e.id == r.id is in coord_dates.
 */


// Keeps from all the objects in the idx only those that are relevant for the event list
std::vector< Attic< Node_Skeleton > > extract_relevant_attic(
    const Id_Dates_Per_Idx& id_dates,
    Id_Dates_Per_Idx& coord_sharing_ids,
    const Coord_Dates_Per_Idx& coord_dates,
    const std::vector< Attic< Node_Skeleton > >& attic);
/* Assertions:
 * An object r is in the result if and only if it is in attic and
 * - its id is in id_dates  or
 * - its coordinate is in coord_dates
 * NB: an extra condition could be applied to avoid too old objects,
 * but is avoided at the moment for the sake of simplicity.
 *
 * An object r is in coord_sharing_ids if r.id is not in id_dates
 * but the coord of the object e with e.id == r.id is in coord_dates.
 */


// Collects the relevant first appearance
Id_Dates_Per_Idx extract_first_appearance(
    const Id_Dates_Per_Idx& id_dates,
    const Id_Dates_Per_Idx& coord_sharing_ids,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& attic);
/* Assertions:
 * An object r is in the result if and only if
 * there is an entry e in id_dates or coord_sharing_ids such that e.id == r.id and
 * from all elements E={f|f in current or attic, f.id == r.id} it is r.date = min_{f\in E}(f.date)
 */


// Collects the relevant undelete entries
/* Uses file nodes_undelete.bin */
std::vector< Attic< Node_Skeleton > > extract_relevant_undeleted(
    const Id_Dates_Per_Idx& id_dates, const Id_Dates_Per_Idx& coord_sharing_ids);
/* Assertions:
 * An object r is in the result if and only if
 * there is an entry e in id_dates or coord_sharing_id_dates such that e.id == r.id and e.min_date < r.date
 */


/* Invariants:
 * - events are sorted by (id, time)
 */
struct Pre_Event_List
{
  // id
  // begin
  // end
  // visible
  // coord
};


Pre_Event_List::Pre_Event_List(New_Data& new_data);
// TODO


// Changes the entry of end in the pre_event_list where applicable.
// Runs twice, once for current and once for attic
void adapt_pre_event_list(
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
    Pre_Event_List& pre_events);
/* Assertions:
 * The of elements in pre_events are the same before and after the call except their value for end.
 * For every entry e in pre_events the value of e.end
 * is the minimum of e.end before the call and the begin dates of all entries m in meta
 * with m.id == e.id and e.begin < m.time
 */
//TODO: to_delete für current_meta ebenfalls hier erzeugen?


//TODO: compute_update_attic_meta


/* Invariants:
 * - events are sorted by (id, time)
 * Note: specific for a given working_idx
 */
struct Event_List
{
  // id
  // time
  // visible_before
  // coord_before
  // visible_after
  // coord_after
  // mult_after
};


// Creates the list of events of relevant nodes in the given section
Event_List::Event_List(const Node_Skeletons_Per_Idx& skels, const Pre_Event_List& pre_events);
/* Assertions:
 * - for every last entry e
 *   it is e.visible if and only if there is a current node for this id in this idx
 * - for every attic node n with min_date(id) < n.end
 *   there exists a successive pair of entries e, f with f.time = n.end
 * - for every last entry e and current node n with e.id == n.id
 *   it is e.coord_before == n.coord
 * - for each pair of successive entries e, f for the same id
 *   it is (e.coord_before, e.visible_before) != (f.coord_before, f.visible_before)
 *   if and only if there is an attic node n in skels
 *   with n.id == e.id, e.visible_before, e.coord_before == n.coord and n.end = f.time
 *   or an entry u in undelete
 *   with u.id = e.id, !e.visible_before and u.time = f.time
 * - for entry e in pre_events and every time t with e.begin <= t < e.end
 *   there is an entry f with e.id == f.id, e.visible == f.visible_after, e.coord == f.coord_after,
 *   and f is the last entry or f.time <= t < g.time for g the successor of f
 * - for every entry f and every time t with f.time <= t and f is the last entry or t < g.time for the successor g
 *   and f.visible_before != f.visible_after or f.coord_before != f.coord_after
 *   there is an entry e in pre_events with e.id == f.id, e.visible == f.visible_after, and e.coord == f.coord_after
 * - first_elements_date and the first_elements_date from pre_events are identical
 * - no entry e has e.time < first_elements_date
 * - for any timestamp t where two or more entries e_0, .., e_n exist such that all have the same coord_after,
 *   that time is smaller or equal to t and the successive time is later than t
 *   there is also mult_after set for all these entries.
 * - no other entry has mult_after set to true.
 */


// Creates the set of deletions and additions for a single index for the current nodes file
/* No preconditions
 */
void create_update_for_nodes(
    const Event_List& events, std::set< Node_Skeleton >& to_delete, std::set< Node_Skeleton >& to_insert);
/* Assertions:
 * - for each latest event n per id i with (n.visible_before, n.coord_before) != (n.visible_after, n.coord_after) there is an entry in to_delete if n.visible_before and an entry in to_insert if n.visible_after
 * - no other entries exist in to_delete or to_insert
 */


// Creates the set of deletions and additions for a single index for the current nodes file
/* No preconditions
 */
void create_update_for_nodes_attic(
    const Event_List& events, std::set< Attic< Node_Skeleton > >& to_delete, std::set< Attic< Node_Skeleton > >& to_insert);
/* Assertions:
 * - for each but the latest event n per id i with m the following event, (n.visible_before, n.coord_before) != (n.visible_after, n.coord_after):
 * -- if n.visible_before and (n.visible_before, n.coord_before) != (m.visible_before, m.coord_before) then there is an entry in to_delete
 * -- if n.visible_after and (n.visible_after, n.coord_after) != (m.visible_after, m.coord_after) then there is an entry in to_insert
 * - no other entries exist in to_delete or to_insert
 */


// Creates the set of deletions and additions for a single index for the current nodes file
/* No preconditions
 */
void create_update_for_nodes_undelete(
    const Event_List& events, std::set< Attic< Node_Skeleton::Id_Type > >& to_delete, std::set< Attic< Node_Skeleton::Id_Type > >& to_insert);
/* Assertions:
 * - for each but the latest event n per id i with m the following event, !n.visible_before, n.visible_after, m.visible_before, m.visible_after and there exists an earlier event l with l.visible_before there is an entry in to_delete
 * - for each but the latest event n per id i with m the following event, !n.visible_after, m.visible_after and there exists an earlier event l with l.visible_after and (n.visible_before || !m.visible_before) there is an entry in to_insert
 * - no other entries exist in to_delete or to_insert
 */


// Nodes_Delta nodes_delta(Event_List)
// is feasible from the event list, using event_date, id, old_cord, new_coord, and new_mult

// void amend_nodes_dict(Event_List, Nodes_Dict&)
// is feasible from the event list, using event_date, id, new_coord, and new_mult


void update_nodes()
{
  // before the first pass by idx
  ...

  std::map< Uint31_Index, Node_Skeletons_Per_Idx > skels_per_idx;

  // first pass by idx
  for (auto i_idx : id_dates_bx_idx)
  {
    Uint31_Index working_idx = i_idx->first;
    Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];
    Coord_Dates_Per_Idx coord_dates(/*TODO*/);

    collect_relevant_coords_current(i_idx->second, current_nodes, coord_dates);
    collect_relevant_coords_attic(i_idx->second, attic_nodes, coord_dates);

    extract_relevant_current(i_idx->second, ..., coord_dates, current_nodes).swap(skels.current);
    extract_relevant_attic(i_idx->second, ..., coord_dates, attic_nodes).swap(skels.attic);

    extract_first_appearance(i_idx->second, ..., current_meta, attic_meta).swap(skels.first_appearance);
    extract_relevant_undeleted(i_idx->second, ...).swap(skels.undeleted);
    ...
  }

  std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_delete;
  std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_add;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_delete;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_add;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_delete;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_add;

  // second pass by idx
  for (auto i_idx : id_dates_bx_idx)
  {
    Uint31_Index working_idx = i_idx->first;
    Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];

    Event_List events(skels, pre_events);
    create_update_for_nodes(
        events, nodes_to_delete[working_idx], nodes_to_add[working_idx]);
    create_update_for_nodes_attic(
        events, nodes_attic_to_delete[working_idx], nodes_attic_to_add[working_idx]);
    create_update_for_nodes_undelete(
        events, nodes_undelete_to_delete[working_idx], nodes_undelete_to_add[working_idx]);
  }

  // write the files
  ...
}
