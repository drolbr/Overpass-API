#include "moved_coords.h"
#include "way_skeleton_updater.h"
#include "test_tools.h"


template< typename Object >
std::vector< const Object* > refs_of(const std::vector< Object >& arg)
{
  std::vector< const Object* > result;
  for (decltype(arg.size()) i = 0; i < arg.size(); ++i)
    result.push_back(&arg[i]);
  return result;
}


bool operator==(const Way_Event& lhs, const Way_Event& rhs)
{
  return lhs.skel == rhs.skel && lhs.skel.nds == rhs.skel.nds && lhs.skel.geometry == rhs.skel.geometry
      && lhs.meta == rhs.meta && lhs.not_before == rhs.not_before && lhs.before == rhs.before;
}


OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > make_way_meta(
    Way_Skeleton::Id_Type id, uint32 version, uint64 timestamp, uint32 changeset, uint32 user_id)
{
    OSM_Element_Metadata meta;
    meta.version = version;
    meta.timestamp = timestamp;
    meta.changeset = changeset;
    meta.user_id = user_id;
    return OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{ id, meta };
}


Way_Event make_way_event(
    const Way_Skeleton& way, uint64_t not_before, uint64_t before,
    uint32 version, uint64 timestamp, uint32 changeset, uint32 user_id)
{
    return Way_Event{
        way,
        make_way_meta(way.id, version, timestamp, changeset, user_id),
        not_before, before };
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";

    // Moved_Coords moved_coords;
    // Way_Pre_Event_Refs pre_event_refs;

    // std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    // std::vector< Way_Skeleton > current_result;
    // std::vector< Attic< Way_Skeleton > > attic_result;
    // Way_Skeleton_Updater::extract_relevant_current_and_attic(
        // pre_event_refs, moved_coords, implicit_pre_events,
        // std::vector< const Way_Skeleton* >(), std::vector< const Attic< Way_Skeleton >* >(),
        // current_result, attic_result);
    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >(), Moved_Coords{},
        changes_per_idx, deletions);

    Way_Skeleton_Updater::Way_Skeleton_Delta skel_delta(
        std::vector< Way_Event >{},
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        std::vector< Way_Event >{},
        std::vector< Attic< Way_Skeleton::Id_Type > >{}, std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    // all_ok &= Compare_Vector< Way_Skeleton >("current_result")
        // (current_result);
    // all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
        // (attic_result);
    // all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
        // (implicit_pre_events);
    all_ok &= Compare_Map< Uint31_Index, std::vector< Way_Event > >("resolve_coord_events::changes_per_idx")
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);

    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (skel_delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (skel_delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (skel_delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (skel_delta.current_to_add);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        (undel_delta.undeletes_to_add);
  }
//   {
//     std::cerr<<"\nTest irrelevant current way:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), std::vector< const Attic< Way_Skeleton >* >(),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest irrelevant attic way:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ), 1000) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         std::vector< const Way_Skeleton* >(), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest current way that is relevant by pre_event_refs:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs =
//         { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 } };
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), std::vector< const Attic< Way_Skeleton >* >(),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[0])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest attic way that is relevant by pre_event_refs:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs
//         { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 } };
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ), 2000) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         std::vector< const Way_Skeleton* >(), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic[0])
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest within mixed input a current way that is relevant by pre_event_refs:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs =
//         { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 } };
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } } ) };
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } } ), 2000) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[0])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest current way that is relevant by moved_coords:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(496001ull), 1000,
//             true, ll_lower(51.25, 7.15002), true, ll_lower(51.25, 7.150021), false } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), std::vector< const Attic< Way_Skeleton >* >(),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[0])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 1000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150021) } }, std::vector< Node::Id_Type >() })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest attic way that is relevant by moved_coords:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(496001ull), 1000,
//             true, ll_lower(51.25, 7.15002), true, ll_lower(51.25, 7.150021), false } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ), 2000) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         std::vector< const Way_Skeleton* >(), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic[0])
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 1000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150021) } }, std::vector< Node::Id_Type >() })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest for current ways combinations of moved_coords and pre_event_refs:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(495001ull), 5000,
//             true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
//         Node_Event{ Uint64(496001ull), 6000,
//             true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false },
//         Node_Event{ Uint64(497001ull), 7100,
//             true, ll_lower(51.25, 7.150071), true, ll_lower(51.25, 7.1500711), false },
//         Node_Event{ Uint64(497002ull), 7200,
//             true, ll_lower(51.25, 7.150072), true, ll_lower(51.25, 7.1500721), false },
//         Node_Event{ Uint64(498001ull), 8100,
//             true, ll_lower(51.25, 7.150081), true, ll_lower(51.25, 7.1500811), false },
//         Node_Event{ Uint64(498002ull), 8200,
//             true, ll_lower(51.25, 7.150082), true, ll_lower(51.25, 7.1500821), false } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(494u), 1000ull, 0 },
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 },
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 1000ull, 0 } };
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(493u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } }),
//         Way_Skeleton(494u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150041) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }),
//         Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }),
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }),
//         Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }),
//         Way_Skeleton(498u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150081) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), std::vector< const Attic< Way_Skeleton >* >(),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[1])
//         (current[2])
//         (current[3])
//         (current[4])
//         (current[5])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500621) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7100, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7200, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500721) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(498u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150081) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8100, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8200, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500821) } }, std::vector< Node::Id_Type >() })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest for attic ways combinations of moved_coords and pre_event_refs:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(495001ull), 5000,
//             true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
//         Node_Event{ Uint64(496001ull), 6000,
//             true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false },
//         Node_Event{ Uint64(497001ull), 7100,
//             true, ll_lower(51.25, 7.150071), true, ll_lower(51.25, 7.1500711), false },
//         Node_Event{ Uint64(497002ull), 7200,
//             true, ll_lower(51.25, 7.150072), true, ll_lower(51.25, 7.1500721), false },
//         Node_Event{ Uint64(498001ull), 8100,
//             true, ll_lower(51.25, 7.150081), true, ll_lower(51.25, 7.1500811), false },
//         Node_Event{ Uint64(498002ull), 8200,
//             true, ll_lower(51.25, 7.150082), true, ll_lower(51.25, 7.1500821), false } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(494u), 1000ull, 0 },
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 },
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 1000ull, 0 } };
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(493u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } }), 13000),
//         Attic< Way_Skeleton >(Way_Skeleton(494u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150041) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }), 14000),
//         Attic< Way_Skeleton >(Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }), 15000),
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }), 16000),
//         Attic< Way_Skeleton >(Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }), 17000),
//         Attic< Way_Skeleton >(Way_Skeleton(498u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150081) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }), 18000) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         std::vector< const Way_Skeleton* >(), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic[1])
//         (attic[2])
//         (attic[3])
//         (attic[4])
//         (attic[5])
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500621) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7100, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7200, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500721) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(498u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150081) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8100, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8200, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500821) } }, std::vector< Node::Id_Type >() })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest for current ways concurrency of id based and coord based changes:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(493002ull), 3000,
//             true, ll_lower(51.25, 7.150032), true, ll_lower(51.25, 7.1500321), false },
//         Node_Event{ Uint64(494001ull), 4100,
//             true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500411), false },
//         Node_Event{ Uint64(494002ull), 4200,
//             true, ll_lower(51.25, 7.150042), true, ll_lower(51.25, 7.1500421), false },
//         Node_Event{ Uint64(495001ull), 5100,
//             true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500511), false },
//         Node_Event{ Uint64(495002ull), 5200,
//             true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500521), false },
//         Node_Event{ Uint64(496001ull), 6000,
//             true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500611), false },
//         Node_Event{ Uint64(496002ull), 6000,
//             true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500621), false } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(493u, std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) },
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } }),
//         Way_Skeleton(494u, std::vector< Node::Id_Type >{ Uint64(494001ull), Uint64(0ull) },
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }),
//         Way_Skeleton(495u, std::vector< Node::Id_Type >{ Uint64(495001ull), Uint64(495002ull) },
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } }),
//         Way_Skeleton(496u, std::vector< Node::Id_Type >{ Uint64(496001ull), Uint64(0ull) },
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), std::vector< const Attic< Way_Skeleton >* >(),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[0])
//         (current[1])
//         (current[2])
//         (current[3])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(493u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(493u), 3000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500321) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(494u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } },
//             std::vector< Node::Id_Type >{ Uint64(494001ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(494u), 4100, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500411) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(494u), 4200, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500411) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500421) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } },
//             std::vector< Node::Id_Type >{ Uint64(495001ull), Uint64(495002ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5100, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500511) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(495002ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5200, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500511) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } },
//             std::vector< Node::Id_Type >{ Uint64(496001ull), Uint64(0ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500611) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest whether timestamp after attic ways end is ignored:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(495001ull), 1900,
//             true, ll_lower(51.25, 7.150051), true, ll_lower(51.25, 7.1500511), false },
//         Node_Event{ Uint64(495002ull), 2100,
//             true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
//         Node_Event{ Uint64(496001ull), 6000,
//             true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }), 2000),
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }), 2000) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         std::vector< const Way_Skeleton* >(), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic[0])
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }, std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 1900, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500511) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }, std::vector< Node::Id_Type >() })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest for current ways whether deletion and node conflation works:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(495002ull), 5000,
//             true, ll_lower(51.25, 7.150052), false, 0u, false },
//         Node_Event{ Uint64(496002ull), 6000,
//             true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.15), true } } );
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(495u, std::vector< Node::Id_Type >(),
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }),
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(),
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), std::vector< const Attic< Way_Skeleton >* >(),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[0])
//         (current[1])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ 0u, 0u } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(495002ull) } })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } },
//             std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(496002ull) } })
//         (implicit_pre_events);
//   }
//   {
//     std::cerr<<"\nTest mixed current and attic whether events appear only once:\n";
// 
//     Moved_Coords moved_coords;
//     moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
//         Node_Event{ Uint64(494001ull), 2000,
//             true, ll_lower(51.25, 7.150041), true, ll_lower(51.25, 7.1500411), false },
//         Node_Event{ Uint64(495002ull), 2000,
//             true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
//         Node_Event{ Uint64(495002ull), 3000,
//             true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500522), false },
//         Node_Event{ Uint64(495003ull), 2000,
//             true, ll_lower(51.25, 7.150053), true, ll_lower(51.25, 7.1500531), false },
//         Node_Event{ Uint64(495003ull), 3000,
//             true, ll_lower(51.25, 7.150053), true, ll_lower(51.25, 7.1500532), false },
//         Node_Event{ Uint64(496002ull), 2000,
//             true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false },
//         Node_Event{ Uint64(496002ull), 3000,
//             true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500622), false },
//         Node_Event{ Uint64(496003ull), 2000,
//             true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.1500631), false },
//         Node_Event{ Uint64(496003ull), 3000,
//             true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.1500632), false },
//         Node_Event{ Uint64(496004ull), 2000,
//             true, ll_lower(51.25, 7.150064), true, ll_lower(51.25, 7.1500641), false },
//         Node_Event{ Uint64(497003ull), 2000,
//             true, ll_lower(51.25, 7.150073), true, ll_lower(51.25, 7.1500731), false } });
//     moved_coords.build_hash();
//     Way_Pre_Event_Refs pre_event_refs;
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     std::vector< Way_Skeleton > current = {
//         Way_Skeleton(494u, std::vector< Node::Id_Type >(),
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150041) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }),
//         Way_Skeleton(495u, std::vector< Node::Id_Type >(),
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150053) } }),
//         Way_Skeleton(496u, std::vector< Node::Id_Type >(),
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150063) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150064) } }),
//         Way_Skeleton(497u, std::vector< Node::Id_Type >(),
//             std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150073) } }) };
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }), 1500),
//         Attic< Way_Skeleton >(Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }), 2500),
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }), 2600),
//         Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150063) } }), 3600),
//         Attic< Way_Skeleton >(Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150073) } }), 2700),
//         Attic< Way_Skeleton >(Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }), 3700) };
// 
//     std::vector< Way_Skeleton > current_result;
//     std::vector< Attic< Way_Skeleton > > attic_result;
//     Way_Skeleton_Updater::extract_relevant_current_and_attic(
//         pre_event_refs, moved_coords, implicit_pre_events,
//         refs_of(current), refs_of(attic),
//         current_result, attic_result);
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Way_Skeleton >("current_result")
//         (current[0])
//         (current[1])
//         (current[2])
//         (current[3])
//         (current_result);
//     all_ok &= Compare_Vector< Attic< Way_Skeleton > >("attic_result")
//         (attic[1])
//         (attic[2])
//         (attic[3])
//         (attic[4])
//         (attic[5])
//         (attic_result);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("implicit_pre_events")
//         (Way_Implicit_Pre_Event{ Uint32_Index(494u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150041) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(494u), 2000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500411) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 1500, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 2000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 2500, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500531) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(495u), 3000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500522) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500532) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 2000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500621) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 2600, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500621) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500631) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 3000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500622) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500632) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(496u), 3600, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500622) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500632) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500641) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 0, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150073) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 2000, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500731) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 2700, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } },
//             std::vector< Node::Id_Type >() })
//         (Way_Implicit_Pre_Event{ Uint32_Index(497u), 3700, std::vector< Quad_Coord >{
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) },
//             Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500731) } },
//             std::vector< Node::Id_Type >() })
//         (implicit_pre_events);
//   }
  {
    std::cerr<<"\nTest with a single pre_event:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, NOW, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest with a single pre_event and earlier node events:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 3000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 2000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 3000, NOW, 1, 3000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest with a single pre_event and later node events:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002, 496003 }, std::vector< Quad_Coord >(3) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496001ull, 2000, false, 0, true, ll_lower(51.25, 7.1500011), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false },
        Node_Event{ 496002ull, 3000, false, 0, true, ll_lower(51.25, 7.1500021), false },
        Node_Event{ 496003ull, 1000, false, 0, true, ll_lower(51.25, 7.150003), false },
        Node_Event{ 496003ull, 2000, false, 0, true, ll_lower(51.25, 7.1500031), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500011) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500031) } }
                }, 2000, 3000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500011) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500021) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500031) } }
                }, 3000, NOW, 1, 1000, 8128, 28)  })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest index change with a single pre_event:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 8.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 8.150001), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 8.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 8.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
                { ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }
                }, 1000, NOW, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest index change with a single pre_event and later node events:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496001ull, 2000, false, 0, false, 0, false },
        Node_Event{ 496001ull, 3000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false },
        Node_Event{ 496002ull, 2000, false, 0, false, 0, false },
        Node_Event{ 496002ull, 3000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.record(ll_upper_(51.25, 8.15), {
        Node_Event{ 496001ull, 2000, false, 0, true, ll_lower(51.25, 8.150001), false },
        Node_Event{ 496001ull, 3000, false, 0, false, 0, false },
        Node_Event{ 496002ull, 2000, false, 0, true, ll_lower(51.25, 8.150002), false },
        Node_Event{ 496002ull, 3000, false, 0, false, 0, false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 3000, NOW, 1, 1000, 8128, 28) })
        (ll_upper_(51.25, 8.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
                { ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }
                }, 2000, 3000, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest node duplication with a single pre_event:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), true },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, { 496001ull, 0ull }, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, NOW, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest node duplication with a single pre_event and later node events:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496001ull, 2000, false, 0, true, ll_lower(51.25, 7.150001), true },
        Node_Event{ 496001ull, 3000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, { 496001ull, 0ull }, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 2000, 3000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 3000, NOW, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest node absence with a single pre_event:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (0x80000080, {
            make_way_event(Way_Skeleton{ 496u, { 496001ull, 0ull }, {
                { 0u, 0u },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, NOW, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest node deletion with a single pre_event:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, false, 0, false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (0x80000080, {
            make_way_event(Way_Skeleton{ 496u, { 496001ull, 0ull }, {
                { 0u, 0u },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, NOW, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest node deletion with a single pre_event and later node events:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496001ull, 2000, false, 0, false, 0, false },
        Node_Event{ 496001ull, 3000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 3000, NOW, 1, 1000, 8128, 28) })
        (0x80000080, {
            make_way_event(Way_Skeleton{ 496u, { 496001ull, 0ull }, {
                { 0u, 0u },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 2000, 3000, 1, 1000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest multiple meta versions for the same id:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 2, 2000, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 3, 4000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.1500011), false },
        Node_Event{ 496001ull, 2000, false, 0, true, ll_lower(51.25, 7.1500012), false },
        Node_Event{ 496001ull, 5000, false, 0, true, ll_lower(51.25, 7.1500015), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.1500021), false },
        Node_Event{ 496002ull, 2000, false, 0, true, ll_lower(51.25, 7.1500022), false },
        Node_Event{ 496002ull, 3000, false, 0, true, ll_lower(51.25, 7.1500023), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], 2000 }, { entries[1], 4000 }, { entries[2], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500011) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500021) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500012) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500022) } }
                }, 2000, 3000, 2, 2000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500012) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500023) } }
                }, 3000, 4000, 2, 2000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500012) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500023) } }
                }, 4000, 5000, 3, 4000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500015) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500023) } }
                }, 5000, NOW, 3, 4000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest multiple way turns for the same id:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 496u, { 496002, 496001 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 2, 2000, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 3, 4000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.1500011), false },
        Node_Event{ 496001ull, 3000, false, 0, true, ll_lower(51.25, 7.1500013), false },
        Node_Event{ 496001ull, 5000, false, 0, true, ll_lower(51.25, 7.1500015), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], 2000 }, { entries[1], 4000 }, { entries[2], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500011) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500011) } }
                }, 2000, 3000, 1, 2000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500013) } }
                }, 3000, 4000, 1, 2000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500013) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 4000, 5000, 1, 4000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500015) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }
                }, 5000, NOW, 1, 4000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest multiple skeleton versions for the same id:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496003 }, std::vector< Quad_Coord >(2) },
          make_way_meta(496, 1, 1000, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 496u, { 496001, 496002, 496003 }, std::vector< Quad_Coord >(3) },
          make_way_meta(496, 2, 2000, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 2000, false, 0, true, ll_lower(51.25, 7.150002), false },
        Node_Event{ 496003ull, 1000, false, 0, true, ll_lower(51.25, 7.150003), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], 2000 }, { entries[1], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003) } }
                }, 1000, 2000, 1, 1000, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003) } }
                }, 2000, NOW, 1, 2000, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
  {
    std::cerr<<"\nTest multiple ways:\n";

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries = {
        { ll_upper_(51.25, 7.15),
          { 494u, { 496001, 496003, 496004 }, std::vector< Quad_Coord >(3) },
          make_way_meta(494, 1, 1004, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 494u, { 496001, 496002, 496003, 496004 }, std::vector< Quad_Coord >(4) },
          make_way_meta(494, 1, 2004, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 495u, { 496004, 496005, 496006 }, std::vector< Quad_Coord >(3) },
          make_way_meta(495, 1, 1005, 8128, 28), {} },
        { ll_upper_(51.25, 7.15),
          { 496u, { 496006, 496007, 496001 }, std::vector< Quad_Coord >(3) },
          make_way_meta(496, 1, 1006, 8128, 28), {} } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), {
        Node_Event{ 496001ull, 1000, false, 0, true, ll_lower(51.25, 7.150001), false },
        Node_Event{ 496002ull, 1000, false, 0, true, ll_lower(51.25, 7.150002), false },
        Node_Event{ 496003ull, 1000, false, 0, true, ll_lower(51.25, 7.150003), false },
        Node_Event{ 496004ull, 1000, false, 0, true, ll_lower(51.25, 7.150004), false },
        Node_Event{ 496005ull, 1000, false, 0, true, ll_lower(51.25, 7.150005), false },
        Node_Event{ 496006ull, 1000, false, 0, true, ll_lower(51.25, 7.150006), false },
        Node_Event{ 496007ull, 1000, false, 0, true, ll_lower(51.25, 7.150007), false } });
    moved_coords.build_hash();

    std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
    Way_Skeleton_Updater::resolve_coord_events(
        Pre_Event_List< Way_Skeleton >{ { { entries[0], 2004 }, { entries[1], NOW }, { entries[2], NOW }, { entries[3], NOW } }, {} },
        moved_coords, changes_per_idx, deletions);

    bool all_ok = true;
    all_ok &= Compare_Map_Vector< Uint31_Index, Way_Event >("resolve_coord_events::changes_per_idx")
        (ll_upper_(51.25, 7.15), {
            make_way_event(Way_Skeleton{ 494u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004) } }
                }, 1004, 2004, 1, 1004, 8128, 28),
            make_way_event(Way_Skeleton{ 494u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004) } }
                }, 2004, NOW, 1, 2004, 8128, 28),
            make_way_event(Way_Skeleton{ 495u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150005) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }
                }, 1005, NOW, 1, 1005, 8128, 28),
            make_way_event(Way_Skeleton{ 496u, {}, {
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007) },
                { ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) } }
                }, 1006, NOW, 1, 1006, 8128, 28) })
        (changes_per_idx);
    all_ok &= Compare_Vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >("resolve_coord_events::deletions")
        (deletions);
  }
//TODO: deletions
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that obsolete current objects are deleted:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}};
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        std::vector< Way_Event >{},
        std::vector< const Way_Skeleton* >{1, &way}, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (way)
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that obsolete attic objects are deleted:\n";

    Attic< Way_Skeleton > way{Way_Skeleton{496u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}}, 1000};
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        std::vector< Way_Event >{},
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{1, &way});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (way)
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that a single current event is processed:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}};
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW } },
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (way)
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that multiple current events are processed:\n";

    Way_Skeleton way4{494u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}};
    Way_Skeleton way5{495u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}};
    Way_Skeleton way6{496u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}};
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way4,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW },
          Way_Event{ way5,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW },
          Way_Event{ way6,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW } },
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (way4)
        (way5)
        (way6)
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that a single attic event is processed:\n";

    Attic< Way_Skeleton > way{ Way_Skeleton{496u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}}, 2000 };
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 } },
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (way)
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that multiple attic events are processed:\n";

    Attic< Way_Skeleton > way4{ Way_Skeleton{494u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}}, 2004 };
    Attic< Way_Skeleton > way5{ Way_Skeleton{495u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}}, 2005 };
    Attic< Way_Skeleton > way6{ Way_Skeleton{496u, std::vector< Node::Id_Type >{}, std::vector< Quad_Coord >{}}, 2006 };
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way4,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1004, 2004 },
          Way_Event{ way5,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1005, 2005 },
          Way_Event{ way6,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1006, 2006 } },
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (way4)
        (way5)
        (way6)
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test various object conflation patterns:\n";

    Attic< Way_Skeleton > way41{ Way_Skeleton{494u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500041) } } }, 3004 };
    Way_Skeleton way42{494u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500042) } } };
    Attic< Way_Skeleton > way51{ Way_Skeleton{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500051) } } }, 2005 };
    Way_Skeleton way52{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500052) } } };
    Attic< Way_Skeleton > way61{ Way_Skeleton{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500061) } } }, 2006 };
    Attic< Way_Skeleton > way62{ Way_Skeleton{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500062) } } }, 3006 };
    Way_Skeleton way63{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500063) } } };
    Attic< Way_Skeleton > way71{ Way_Skeleton{497u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007) } } }, 2007 };
    Attic< Way_Skeleton > way72{ Way_Skeleton{497u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007) } } }, 4007 };
    Attic< Way_Skeleton > way81{ Way_Skeleton{498u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150008) } } }, 2008 };
    Way_Skeleton way82{498u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150008) } } };
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way41,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1004, 2004 },
          Way_Event{ way41,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2004, 3004 },
          Way_Event{ way42,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3004, NOW },
          Way_Event{ way51,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1005, 2005 },
          Way_Event{ way52,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2005, 3005 },
          Way_Event{ way52,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3005, NOW },
          Way_Event{ way61,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1006, 2006 },
          Way_Event{ way62,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2006, 3006 },
          Way_Event{ way63,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3006, NOW },
          Way_Event{ way71,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1007, 2007 },
          Way_Event{ way72,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3007, 4007 },
          Way_Event{ way81,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1008, 2008 },
          Way_Event{ way82,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3008, NOW } },
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (way41)
        (way51)
        (way61)
        (way62)
        (way71)
        (way72)
        (way81)
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (way42)
        (way52)
        (way63)
        (way82)
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that matching attics cancel out:\n";

    Attic< Way_Skeleton > way41{ Way_Skeleton{494u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500041) } } }, 3004 };
    Way_Skeleton way42{494u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500042) } } };
    Attic< Way_Skeleton > way51{ Way_Skeleton{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500051) } } }, 2005 };
    Attic< Way_Skeleton > way53{ Way_Skeleton{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500053) } } }, 2505 };
    Way_Skeleton way52{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500052) } } };
    Attic< Way_Skeleton > way61{ Way_Skeleton{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500061) } } }, 2006 };
    Attic< Way_Skeleton > way64{ Way_Skeleton{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500062) } } }, 2506 };
    Attic< Way_Skeleton > way62{ Way_Skeleton{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500062) } } }, 3006 };
    Way_Skeleton way63{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500063) } } };
    Attic< Way_Skeleton > way71{ Way_Skeleton{497u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007) } } }, 2007 };
    Attic< Way_Skeleton > way72{ Way_Skeleton{497u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007) } } }, 4007 };
    Attic< Way_Skeleton > way81{ Way_Skeleton{498u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150008) } } }, 2008 };
    Way_Skeleton way82{498u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150008) } } };
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way41,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1004, 2004 },
          Way_Event{ way41,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2004, 3004 },
          Way_Event{ way42,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3004, NOW },
          Way_Event{ way51,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1005, 2005 },
          Way_Event{ way52,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2005, 3005 },
          Way_Event{ way52,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3005, NOW },
          Way_Event{ way61,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1006, 2006 },
          Way_Event{ way62,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2006, 3006 },
          Way_Event{ way63,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3006, NOW },
          Way_Event{ way71,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1007, 2007 },
          Way_Event{ way72,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3007, 4007 },
          Way_Event{ way81,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1008, 2008 },
          Way_Event{ way82,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3008, NOW } },
        std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{
            &way51, &way53, &way64, &way71 });
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (way53)
        (way64)
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (way41)
        (way61)
        (way62)
        (way72)
        (way81)
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (way42)
        (way52)
        (way63)
        (way82)
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test that matching currents cancel out:\n";

    Way_Skeleton way4{494u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004) } }};
    Way_Skeleton way5{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150005) } }};
    Way_Skeleton way6{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way4,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW },
          Way_Event{ way5,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW },
          Way_Event{ way6,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW } },
        std::vector< const Way_Skeleton* >{ &way4, &way6 }, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (way5)
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Skeleton_Delta: Test the usual move from current to attic:\n";

    Attic< Way_Skeleton > way1{ Way_Skeleton{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500061) } }}, 2000 };
    Way_Skeleton way2{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500062) } }};
    Way_Skeleton_Updater::Way_Skeleton_Delta delta(
        { Way_Event{ way1,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way2,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2000, NOW } },
        std::vector< const Way_Skeleton* >{ &way1 }, std::vector< const Attic< Way_Skeleton >* >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("Way_Skeleton_Delta::attic_to_add")
        (way1)
        (delta.attic_to_add);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_delete")
        (way1)
        (delta.current_to_delete);
    all_ok &= Compare_Vector< Way_Skeleton >("Way_Skeleton_Delta::current_to_add")
        (way2)
        (delta.current_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test whether a single event produces empty output:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{}, std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test whether consecutive events produce empty output:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{}, std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test whether a gap produces a result:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{}, std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{496u}, 3000 })
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test whether distributed events produce empty output:\n";

    Way_Skeleton way5{495u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150005) } }};
    Way_Skeleton way6{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way5, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way6, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{}, std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test whether an existing undelete is deleted:\n";

    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        std::vector< Way_Event >{},
        { Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{496u}, 1000 } }, std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        ({ Way_Skeleton::Id_Type{496u}, 1000 })
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test with matching undelete:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3000, NOW } },
        { Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{496u}, 3000 } },
        std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test with non-matching undelete:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3000, NOW } },
        { Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{496u}, 2500 } },
        std::vector< Way_Skeleton::Id_Type >{});
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        ({ Way_Skeleton::Id_Type{496u}, 2500 })
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{496u}, 3000 })
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test with deleted after unchanged with a single event:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{},
        { Way_Skeleton::Id_Type{496u} });
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{496u}, 1000 })
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test with deleted after unchanged with two consecutive events:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 2000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{},
        { Way_Skeleton::Id_Type{496u} });
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{496u}, 1000 })
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test with deleted after unchanged with a single event and a gap:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{},
        { Way_Skeleton::Id_Type{496u} });
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{496u}, 1000 })
        ({ Way_Skeleton::Id_Type{496u}, 3000 })
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test with deleted after unchanged with a single event and a gap:\n";

    Way_Skeleton way{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1000, 2000 },
          Way_Event{ way, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 3000, NOW } },
        std::vector< Attic< Way_Skeleton::Id_Type > >{},
        { Way_Skeleton::Id_Type{496u} });
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{496u}, 1000 })
        ({ Way_Skeleton::Id_Type{496u}, 3000 })
        (undel_delta.undeletes_to_add);
  }
  {
    std::cerr<<"\nWay_Undelete_Delta: Test a complex case:\n";

    Way_Skeleton way2{492u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }};
    Way_Skeleton way4{494u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004) } }};
    Way_Skeleton way6{496u, std::vector< Node::Id_Type >{},
        std::vector< Quad_Coord >{ Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006) } }};
    Way_Skeleton_Updater::Way_Undelete_Delta undel_delta(
        { Way_Event{ way2, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1002, NOW },
          Way_Event{ way4, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1004, NOW },
          Way_Event{ way6, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{}, 1006, NOW } },
        { Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{492u}, 1002 },
          Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{493u}, 1004 },
          Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{495u}, 1004 },
          Attic< Way_Skeleton::Id_Type >{ Way_Skeleton::Id_Type{496u}, 1006 } },
        { Way_Skeleton::Id_Type{492u}, Way_Skeleton::Id_Type{494u}, Way_Skeleton::Id_Type{496u} });
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_delete")
        ({ Way_Skeleton::Id_Type{493u}, 1004 })
        ({ Way_Skeleton::Id_Type{495u}, 1004 })
        (undel_delta.undeletes_to_delete);
    all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("Way_Undelete_Delta::undeletes_to_add")
        ({ Way_Skeleton::Id_Type{494u}, 1004 })
        (undel_delta.undeletes_to_add);
  }

  return 0;
}
