#include "moved_coords.h"
#include "way_skeleton_updater.h"
#include "test_tools.h"


bool operator==(const Way_Implicit_Pre_Event& lhs, const Way_Implicit_Pre_Event& rhs)
{
  return lhs.id == rhs.id && lhs.begin == rhs.begin && lhs.geometry == rhs.geometry && lhs.nds == rhs.nds;
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input for current:\n";

    Moved_Coords moved_coords;
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, std::vector< Way_Skeleton >());
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest empty input for attic:\n";

    Moved_Coords moved_coords;
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    auto retval = Way_Skeleton_Updater::extract_relevant_attic(
        pre_event_refs, moved_coords, implicit_pre_events, std::vector< Attic< Way_Skeleton > >());
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("extract_relevant_attic::retval")
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_attic::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest irrelevant current way:\n";

    Moved_Coords moved_coords;
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Way_Skeleton > current = {
        Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ) };
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, current);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest irrelevant attic way:\n";

    Moved_Coords moved_coords;
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton > > attic = {
        Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ), 1000) };
    auto retval = Way_Skeleton_Updater::extract_relevant_attic(
        pre_event_refs, moved_coords, implicit_pre_events, attic);
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("extract_relevant_attic::retval")
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_attic::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest current way that is relevant by pre_event_refs:\n";

    Moved_Coords moved_coords;
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Way_Skeleton > current = {
        Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ) };
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, current);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (current[0])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest attic way that is relevant by pre_event_refs:\n";

    Moved_Coords moved_coords;
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton > > attic = {
        Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ), 2000) };
    auto retval = Way_Skeleton_Updater::extract_relevant_attic(
        pre_event_refs, moved_coords, implicit_pre_events, attic);
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("extract_relevant_attic::retval")
        (attic[0])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_attic::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest current way that is relevant by moved_coords:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(496001ull), 1000,
            true, ll_lower(51.25, 7.15002), true, ll_lower(51.25, 7.150021), false } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Way_Skeleton > current = {
        Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ) };
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, current);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (current[0])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(496u), 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150021) } }, std::vector< Node::Id_Type >() })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest attic way that is relevant by moved_coords:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(496001ull), 1000,
            true, ll_lower(51.25, 7.15002), true, ll_lower(51.25, 7.150021), false } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton > > attic = {
        Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15002) } } ), 2000) };
    auto retval = Way_Skeleton_Updater::extract_relevant_attic(
        pre_event_refs, moved_coords, implicit_pre_events, attic);
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("extract_relevant_attic::retval")
        (attic[0])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_attic::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(496u), 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150021) } }, std::vector< Node::Id_Type >() })
        (implicit_pre_events);
  }

  return 0;
}
