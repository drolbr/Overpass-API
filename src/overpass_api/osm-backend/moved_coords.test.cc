#include "moved_coords.h"
#include "test_tools.h"


bool operator==(const Move_Coord_Event& lhs, const Move_Coord_Event& rhs)
{
  return lhs.node_id == rhs.node_id && lhs.timestamp == rhs.timestamp
      && lhs.idx == rhs.idx && lhs.ll_lower == rhs.ll_lower
      && lhs.idx_after == rhs.idx_after && lhs.ll_lower_after == rhs.ll_lower_after
      && lhs.visible_after == rhs.visible_after && lhs.multiple_after == rhs.multiple_after;
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";
    Moved_Coords moved_coords;
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
  }
  {
    std::cerr<<"\nTest a single event:\n";
    std::vector< Node_Event > events = {
        Node_Event{ Uint64(496ull), 1000, false, 0u, true, ll_lower(51.25, 7.15), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ Uint31_Index(0u), 0u, 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
  }
  {
    std::cerr<<"\nTest multiple events for a single node:\n";
    std::vector< Node_Event > events = {
        Node_Event{ Uint64(496ull), 1000, false, 0u, true, ll_lower(51.25, 7.15), false },
        Node_Event{ Uint64(496ull), 1100, false, 0u, true, ll_lower(51.25, 7.15001), false },
        Node_Event{ Uint64(496ull), 1200, false, 0u, false, 0u, false },
        Node_Event{ Uint64(496ull), 1300, false, 0u, true, ll_lower(51.25, 7.15), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ Uint31_Index(0u), 0u, 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, false })
        (Move_Coord_Event{ Uint31_Index(0u), 0u, 1100, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), true, false })
        (Move_Coord_Event{ Uint31_Index(0u), 0u, 1200, Uint64(496ull),
            ll_upper_(51.25, 7.15), 0u, false, false })
        (Move_Coord_Event{ Uint31_Index(0u), 0u, 1300, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
  }
  {
    std::cerr<<"\nTest simple node move:\n";
    std::vector< Node_Event > events = {
        Node_Event{ Uint64(496ull), 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15001), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
  }
  {
    std::cerr<<"\nTest node move across idxs:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(496ull), 1000, true, ll_lower(51.25, 7.15), false, 0u, false } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(496ull), 1000, false, 0u, true, ll_lower(51.25, 8.15), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
  }
  {
    std::cerr<<"\nTest complex node move pattern:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(496ull), 1000, true, ll_lower(51.25, 7.15), false, 0u, false },
        Node_Event{ Uint64(496ull), 1100, true, ll_lower(51.25, 7.15), false, 0u, false },
        Node_Event{ Uint64(496ull), 1300, true, ll_lower(51.25, 7.15001), false, 0u, false },
        Node_Event{ Uint64(496ull), 1600, true, ll_lower(51.25, 7.15001), false, 0u, false },
        Node_Event{ Uint64(496ull), 1700, false, 0u, false, 0u, false },
        Node_Event{ Uint64(496ull), 1800, true, ll_lower(51.25, 7.15001), false, 0u, false } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(496ull), 1000, false, 0u, true, ll_lower(51.25, 8.15), false },
        Node_Event{ Uint64(496ull), 1100, false, 0u, true, ll_lower(51.25, 8.15001), false },
        Node_Event{ Uint64(496ull), 1200, false, 0u, true, ll_lower(51.25, 8.15002), false },
        Node_Event{ Uint64(496ull), 1400, false, 0u, true, ll_lower(51.25, 8.15004), false },
        Node_Event{ Uint64(496ull), 1500, false, 0u, false, 0u, false },
        Node_Event{ Uint64(496ull), 1600, false, 0u, false, 0u, false } };
    std::vector< Node_Event > events9 = {
        Node_Event{ Uint64(496ull), 1500, false, 0u, true, ll_lower(51.25, 9.15), false },
        Node_Event{ Uint64(496ull), 1600, false, 0u, true, ll_lower(51.25, 9.15006), false },
        Node_Event{ Uint64(496ull), 1700, false, 0u, true, ll_lower(51.25, 9.15007), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.record(ll_upper_(51.25, 9.15), events9);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15001), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1200, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15002), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1300, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15002), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1400, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15004), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1500, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1600, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15006), true, false })
        (Move_Coord_Event{ 0u, 0u, 1700, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15007), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1800, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15007), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15001), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1200, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15002), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1300, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15002), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15001)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1300, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15002), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1400, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15004), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1500, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1600, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15006), true, false })
        (Move_Coord_Event{ 0u, 0u, 1700, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15007), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), 1800, Uint64(496ull),
            ll_upper_(51.25, 9.15), ll_lower(51.25, 9.15007), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001)));
  }
  {
    std::cerr<<"\nTest node overwriting:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(496ull), 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15001), false },
        Node_Event{ Uint64(496ull), 1100, false, 0u, false, 0u, false } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(496ull), 1100, true, ll_lower(51.25, 8.15), true, ll_lower(51.25, 8.15001), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), 1100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15001), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), 1100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15001), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15001)")
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15001)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 8.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), 1100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15001), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 8.15001)")
        (moved_coords.get_coord(ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15001)));
  }
  {
    std::cerr<<"\nTest multiple nodes:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(494ull), 2000, true, ll_lower(51.25, 7.15004), false, 0u, false },
        Node_Event{ Uint64(495ull), 2000, false, 0u, true, ll_lower(51.25, 7.15005), false },
        Node_Event{ Uint64(496ull), 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15006), false },
        Node_Event{ Uint64(496ull), 1100, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.150061), false } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(494ull), 2000, false, 0u, true, ll_lower(51.25, 8.15004), false },
        Node_Event{ Uint64(497ull), 1000, false, 0u, true, ll_lower(51.25, 8.15007), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15004), 2000, Uint64(494ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15004), true, false })
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (Move_Coord_Event{ 0u, 0u, 2000, Uint64(495ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15005), true, false })
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15006), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1100, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150061), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (Move_Coord_Event{ 0u, 0u, 1000, Uint64(497ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15007), true, false })
        (moved_coords.get_id(Uint64(497ull)));
  }
  {
    std::cerr<<"\nTest multiple nodes moved to the same coordinate:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(495ull), 1000, true, ll_lower(51.25, 7.15005), false, 0u, false },
        Node_Event{ Uint64(496ull), 1000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15), true },
        Node_Event{ Uint64(497ull), 1000, true, ll_lower(51.25, 7.15007), true, ll_lower(51.25, 7.15), true } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(494ull), 1000, false, 0u, true, ll_lower(51.25, 8.15), true },
        Node_Event{ Uint64(495ull), 1000, false, 0u, true, ll_lower(51.25, 8.15), true } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (Move_Coord_Event{ 0u, 0u, 1000, Uint64(494ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, true })
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15005), 1000, Uint64(495ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, true })
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15006), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, true })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15007), 1000, Uint64(497ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, true })
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15005)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15005), 1000, Uint64(495ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, true })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15005)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15006)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15006), 1000, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, true })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15006)));
  }
  {
    std::cerr<<"\nTest multiple nodes moved from the same coordinate:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(495ull), 1500, true, ll_lower(51.25, 7.15), false, 0u, false },
        Node_Event{ Uint64(496ull), 1600, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15006), false },
        Node_Event{ Uint64(497ull), 1600, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false },
        Node_Event{ Uint64(497ull), 1700, true, ll_lower(51.25, 7.15), false, 0u, false } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(497ull), 1700, false, 0u, true, ll_lower(51.25, 8.15), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1500, Uint64(495ull),
            ll_upper_(51.25, 7.15), 0u, false, false })
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1600, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15006), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1600, Uint64(497ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1700, Uint64(497ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, false })
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1500, Uint64(495ull),
            ll_upper_(51.25, 7.15), 0u, false, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1600, Uint64(496ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15006), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1600, Uint64(497ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1700, Uint64(497ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 8.15)")
        (moved_coords.get_coord(ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15)));
  }
  {
    std::cerr<<"\nTest sequential use of a coordinate:\n";
    std::vector< Node_Event > events7 = {
        Node_Event{ Uint64(495ull), 1000, true, ll_lower(51.25, 7.15), false, 0u, false },
        Node_Event{ Uint64(495ull), 2000, false, 0u, false, 0u, false },
        Node_Event{ Uint64(496ull), 2000, true, ll_lower(51.25, 7.15), false, 0u, false },
        Node_Event{ Uint64(496ull), 2100, false, 0u, false, 0u, false },
        Node_Event{ Uint64(497ull), 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15007), false } };
    std::vector< Node_Event > events8 = {
        Node_Event{ Uint64(495ull), 1000, false, 0u, true, ll_lower(51.25, 8.15005), false },
        Node_Event{ Uint64(496ull), 2000, false, 0u, true, ll_lower(51.25, 8.15006), false } };
    Moved_Coords moved_coords;
    moved_coords.record(ll_upper_(51.25, 7.15), events7);
    moved_coords.record(ll_upper_(51.25, 8.15), events8);
    moved_coords.build_hash();
    bool all_ok = true;
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(494)")
        (moved_coords.get_id(Uint64(494ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(495)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(495ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15005), true, false })
        (Move_Coord_Event{ 0u, 0u, 2000, Uint64(495ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15005), true, false })
        (moved_coords.get_id(Uint64(495ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(496)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 2000, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15006), true, false })
        (Move_Coord_Event{ 0u, 0u, 2100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15006), true, false })
        (moved_coords.get_id(Uint64(496ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_id(497)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 3000, Uint64(497ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15007), true, false })
        (moved_coords.get_id(Uint64(497ull)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 7.15)")
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 1000, Uint64(495ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15005), true, false })
        (Move_Coord_Event{ 0u, 0u, 2000, Uint64(495ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15005), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 2000, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15006), true, false })
        (Move_Coord_Event{ 0u, 0u, 2100, Uint64(496ull),
            ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15006), true, false })
        (Move_Coord_Event{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15), 3000, Uint64(497ull),
            ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15007), true, false })
        (moved_coords.get_coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)));
    all_ok &= Compare_Vector< Move_Coord_Event >("get_coord(51.25, 8.15)")
        (moved_coords.get_coord(ll_upper_(51.25, 8.15), ll_lower(51.25, 8.15)));
  }

  return 0;
}
