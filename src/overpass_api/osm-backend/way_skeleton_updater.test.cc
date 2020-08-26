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
  {
    std::cerr<<"\nTest for current ways combinations of moved_coords and pre_event_refs:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(495001ull), 5000,
            true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
        Node_Event{ Uint64(496001ull), 6000,
            true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false },
        Node_Event{ Uint64(497001ull), 7100,
            true, ll_lower(51.25, 7.150071), true, ll_lower(51.25, 7.1500711), false },
        Node_Event{ Uint64(497002ull), 7200,
            true, ll_lower(51.25, 7.150072), true, ll_lower(51.25, 7.1500721), false },
        Node_Event{ Uint64(498001ull), 8100,
            true, ll_lower(51.25, 7.150081), true, ll_lower(51.25, 7.1500811), false },
        Node_Event{ Uint64(498002ull), 8200,
            true, ll_lower(51.25, 7.150082), true, ll_lower(51.25, 7.1500821), false } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(494u), 1000ull, 0 },
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 },
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 1000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Way_Skeleton > current = {
        Way_Skeleton(493u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } }),
        Way_Skeleton(494u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150041) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }),
        Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }),
        Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }),
        Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }),
        Way_Skeleton(498u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150081) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }) };
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, current);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (current[1])
        (current[2])
        (current[3])
        (current[4])
        (current[5])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500621) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7100, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7200, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500721) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8100, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8200, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500821) } }, std::vector< Node::Id_Type >() })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest for attic ways combinations of moved_coords and pre_event_refs:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(495001ull), 5000,
            true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
        Node_Event{ Uint64(496001ull), 6000,
            true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false },
        Node_Event{ Uint64(497001ull), 7100,
            true, ll_lower(51.25, 7.150071), true, ll_lower(51.25, 7.1500711), false },
        Node_Event{ Uint64(497002ull), 7200,
            true, ll_lower(51.25, 7.150072), true, ll_lower(51.25, 7.1500721), false },
        Node_Event{ Uint64(498001ull), 8100,
            true, ll_lower(51.25, 7.150081), true, ll_lower(51.25, 7.1500811), false },
        Node_Event{ Uint64(498002ull), 8200,
            true, ll_lower(51.25, 7.150082), true, ll_lower(51.25, 7.1500821), false } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(494u), 1000ull, 0 },
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 1000ull, 0 },
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 1000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton > > attic = {
        Attic< Way_Skeleton >(Way_Skeleton(493u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } }), 13000),
        Attic< Way_Skeleton >(Way_Skeleton(494u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150041) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }), 14000),
        Attic< Way_Skeleton >(Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }), 15000),
        Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }), 16000),
        Attic< Way_Skeleton >(Way_Skeleton(497u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150071) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }), 17000),
        Attic< Way_Skeleton >(Way_Skeleton(498u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150081) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }), 18000) };
    auto retval = Way_Skeleton_Updater::extract_relevant_attic(
        pre_event_refs, moved_coords, implicit_pre_events, attic);
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("extract_relevant_attic::retval")
        (attic[1])
        (attic[2])
        (attic[3])
        (attic[4])
        (attic[5])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_attic::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500621) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7100, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150072) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(497u), 7200, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500711) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500721) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8100, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150082) } }, std::vector< Node::Id_Type >() })
        (Way_Implicit_Pre_Event{ Uint32_Index(498u), 8200, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500811) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500821) } }, std::vector< Node::Id_Type >() })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest for current ways concurrency of id based and coord based changes:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(493002ull), 3000,
            true, ll_lower(51.25, 7.150032), true, ll_lower(51.25, 7.1500321), false },
        Node_Event{ Uint64(494001ull), 4100,
            true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500411), false },
        Node_Event{ Uint64(494002ull), 4200,
            true, ll_lower(51.25, 7.150042), true, ll_lower(51.25, 7.1500421), false },
        Node_Event{ Uint64(495001ull), 5100,
            true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500511), false },
        Node_Event{ Uint64(495002ull), 5200,
            true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500521), false },
        Node_Event{ Uint64(496001ull), 6000,
            true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500611), false },
        Node_Event{ Uint64(496002ull), 6000,
            true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.1500621), false } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Way_Skeleton > current = {
        Way_Skeleton(493u, std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) },
            std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150032) } }),
        Way_Skeleton(494u, std::vector< Node::Id_Type >{ Uint64(494001ull), Uint64(0ull) },
            std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } }),
        Way_Skeleton(495u, std::vector< Node::Id_Type >{ Uint64(495001ull), Uint64(495002ull) },
            std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } }),
        Way_Skeleton(496u, std::vector< Node::Id_Type >{ Uint64(496001ull), Uint64(0ull) },
            std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }) };
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, current);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (current[0])
        (current[1])
        (current[2])
        (current[3])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(493u), 3000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150031) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500321) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
        (Way_Implicit_Pre_Event{ Uint32_Index(494u), 4100, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500411) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150042) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
        (Way_Implicit_Pre_Event{ Uint32_Index(494u), 4200, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500411) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500421) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
        (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5100, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500511) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(495002ull) } })
        (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5200, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500511) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500521) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
        (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500611) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(0ull) } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest whether timestamp after attic ways end is ignored:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(495001ull), 1900,
            true, ll_lower(51.25, 7.150051), true, ll_lower(51.25, 7.1500511), false },
        Node_Event{ Uint64(495002ull), 2100,
            true, ll_lower(51.25, 7.150052), true, ll_lower(51.25, 7.1500521), false },
        Node_Event{ Uint64(496001ull), 6000,
            true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.1500621), false } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton > > attic = {
        Attic< Way_Skeleton >(Way_Skeleton(495u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }), 2000),
        Attic< Way_Skeleton >(Way_Skeleton(496u, std::vector< Node::Id_Type >(), std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }), 2000) };
    auto retval = Way_Skeleton_Updater::extract_relevant_attic(
        pre_event_refs, moved_coords, implicit_pre_events, attic);
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Way_Skeleton > >("extract_relevant_attic::retval")
        (attic[0])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_attic::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(495u), 1900, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500511) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }, std::vector< Node::Id_Type >() })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest for current ways whether deletion and node conflation works:\n";

    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), std::vector< Node_Event >{
        Node_Event{ Uint64(495002ull), 5000,
            true, ll_lower(51.25, 7.150052), false, 0u, false },
        Node_Event{ Uint64(496002ull), 6000,
            true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.15), true } } );
    moved_coords.build_hash();
    Way_Pre_Event_Refs pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Way_Skeleton > current = {
        Way_Skeleton(495u, std::vector< Node::Id_Type >(),
            std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150052) } }),
        Way_Skeleton(496u, std::vector< Node::Id_Type >(),
            std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150062) } }) };
    auto retval = Way_Skeleton_Updater::extract_relevant_current(
        pre_event_refs, moved_coords, implicit_pre_events, current);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Skeleton >("extract_relevant_current::retval")
        (current[0])
        (current[1])
        (retval);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("extract_relevant_current::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Uint32_Index(495u), 5000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150051) },
            Quad_Coord{ 0u, 0u } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(495002ull) } })
        (Way_Implicit_Pre_Event{ Uint32_Index(496u), 6000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15) } },
            std::vector< Node::Id_Type >{ Uint64(0ull), Uint64(496002ull) } })
        (implicit_pre_events);
  }

  return 0;
}
