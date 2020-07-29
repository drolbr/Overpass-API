#include "prepare_node_update.h"
#include "test_tools.h"


int main(int argc, char* args[])
{
  {
    std::cerr<<"Test empty input:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (nodes_undelete_to_add);
  }
  {
    std::cerr<<"\nTest with a new node:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    events.data = {
        Node_Event{ 496ull, 1000, false, 0u, true, ll_lower(51.25, 7.15), false } };
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (Node_Skeleton(496ull, ll_lower(51.25, 7.15)))
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (nodes_undelete_to_add);
  }
  {
    std::cerr<<"\nTest with a changed node:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    events.data = {
        Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false },
        Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15), false } };
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (Node_Skeleton(496ull, ll_lower(51.25, 7.15006)))
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (Node_Skeleton(496ull, ll_lower(51.25, 7.15)))
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15006)), 2000))
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (nodes_undelete_to_add);
  }
  {
    std::cerr<<"\nTest with a deleted node:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    events.data = {
        Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false },
        Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15006), false, 0u, false } };
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (Node_Skeleton(496ull, ll_lower(51.25, 7.15006)))
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15006)), 2000))
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (nodes_undelete_to_add);
  }
  {
    std::cerr<<"\nTest with an undeleted node:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    events.data = {
        Node_Event{ 496ull, 1000, false, 0u, false, 0u, false },
        Node_Event{ 496ull, 2000, false, 0u, true, ll_lower(51.25, 7.15), false } };
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (Node_Skeleton(496ull, ll_lower(51.25, 7.15)))
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 2000))
        (nodes_undelete_to_add);
  }
  {
    std::cerr<<"\nTest an attic change:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    events.data = {
        Node_Event{ 495ull, 1000, true, ll_lower(51.25, 7.15005), true, ll_lower(51.25, 7.15005), false },
        Node_Event{ 495ull, 2000, true, ll_lower(51.25, 7.15005), true, ll_lower(51.25, 7.150052), false },
        Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15005), true, ll_lower(51.25, 7.15005), false },
        Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false },
        Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.150062), false },
        Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.150063), false },
        Node_Event{ 497ull, 1000, true, ll_lower(51.25, 7.15007), true, ll_lower(51.25, 7.15007), false },
        Node_Event{ 497ull, 2000, true, ll_lower(51.25, 7.15007), true, ll_lower(51.25, 7.150073), false },
        Node_Event{ 497ull, 3000, true, ll_lower(51.25, 7.150073), true, ll_lower(51.25, 7.150073), false } };
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15006)), 3000))
        (Attic< Node_Skeleton >(Node_Skeleton(497ull, ll_lower(51.25, 7.15007)), 3000))
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15005)), 2000))
        (Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.150052)), 3000))
        (Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15006)), 2000))
        (Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150062)), 3000))
        (Attic< Node_Skeleton >(Node_Skeleton(497ull, ll_lower(51.25, 7.15007)), 2000))
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (nodes_undelete_to_add);
  }
  {
    std::cerr<<"\nTest deletion of undelete markers:\n";

    Node_Event_List events(ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_List());
    events.data = {
        Node_Event{ 495ull, 1000, false, 0u, false, 0u, false },
        Node_Event{ 495ull, 2000, true, ll_lower(51.25, 7.15005), false, 0u, false },
        Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15005), true, ll_lower(51.25, 7.15005), false },
        Node_Event{ 496ull, 1000, false, 0u, false, 0u, false },
        Node_Event{ 496ull, 2000, false, 0u, true, ll_lower(51.25, 7.15006), false },
        Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false } };
    bool all_ok = true;

    std::set< Node_Skeleton > nodes_to_delete;
    std::set< Node_Skeleton > nodes_to_add;
    Prepare_Node_Update::create_update_for_nodes(events, nodes_to_delete, nodes_to_add);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_delete")
        (nodes_to_delete);
    all_ok &= Compare_Set< Node_Skeleton >("create_update_for_nodes.to_add")
        (nodes_to_add);

    std::set< Attic< Node_Skeleton > > nodes_attic_to_delete;
    std::set< Attic< Node_Skeleton > > nodes_attic_to_add;
    Prepare_Node_Update::create_update_for_nodes_attic(events, nodes_attic_to_delete, nodes_attic_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_delete")
        (nodes_attic_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton > >("create_update_for_nodes_attic.to_add")
        (nodes_attic_to_add);

    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_delete;
    std::set< Attic< Node_Skeleton::Id_Type > > nodes_undelete_to_add;
    Prepare_Node_Update::create_update_for_nodes_undelete(events, nodes_undelete_to_delete, nodes_undelete_to_add);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_delete")
        (Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 2000))
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 3000))
        (nodes_undelete_to_delete);
    all_ok &= Compare_Set< Attic< Node_Skeleton::Id_Type > >("create_update_for_nodes_undelete.to_add")
        (Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 3000))
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 2000))
        (nodes_undelete_to_add);
  }

  return 0;
}
