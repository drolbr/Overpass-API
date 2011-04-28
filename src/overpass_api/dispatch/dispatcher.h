#ifndef ORG__OVERPASS_API__DISPATCHER
#define ORG__OVERPASS_API__DISPATCHER

#include "../core/basic_types.h"
#include "../../template_db/types.h"

#include <vector>

using namespace std;

const int SHM_SIZE = 20+12+2*(256+4);
const int OFFSET_BACK = 20;
const int OFFSET_DB_1 = OFFSET_BACK+12;
const int OFFSET_DB_2 = OFFSET_DB_1+(256+4);

const uint32 REGISTER_PID = 16;
const uint32 SET_LIMITS = 17;
const uint32 UNREGISTER_PID = 18;
const uint32 SERVER_STATE = 19;

const uint32 QUERY_REJECTED = 32;

static const string shared_name("/osm3s_v0.6.1");

class Dispatcher
{
  public:
    typedef uint32 pid_t;
    
    /** Opens a shared memory for dispatcher communication. Furthermore,
      * detects whether idx or idy are valid, clears to idx if necessary,
      * and loads them into the shared memory idx_share_name. */
    Dispatcher(string dispatcher_share_name,
	       string index_share_name,
	       string shadow_name,
	       const vector< File_Properties* >& controlled_files);

    void write_start(pid_t pid);
    void write_rollback();
    void write_commit();
    
  private:
    vector< File_Properties* > controlled_files;
    string shadow_name;
    
    void copy_shadows_to_mains();
    void copy_mains_to_shadows();
    void remove_shadows();
};

#endif
