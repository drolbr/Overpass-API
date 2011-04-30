#ifndef ORG__OVERPASS_API__DISPATCHER
#define ORG__OVERPASS_API__DISPATCHER

#include "../core/basic_types.h"
#include "../../template_db/types.h"

#include <set>
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

class Idx_Footprints
{
  public:
    typedef uint pid_t;
    
    void set_current_footprint(const vector< bool >& footprint);
    void register_pid(pid_t pid); 
    void unregister_pid(pid_t pid); 
    vector< pid_t > registered_processes() const;
    vector< bool > total_footprint() const;
    
  private:
    vector< bool > current_footprint;
    map< pid_t, vector< bool > > footprint_per_pid;
};

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

    /** Write operations: -------------------------------------------------- */
	       
    /** Allocates a write lock if possible. Returns without doing anything
        otherwise. */
    void write_start(pid_t pid);
    
    /** Removes the mutex for the write process without changing any
        index file. */
    void write_rollback();
    
    /** Copies the shadow files onto the main index files. A lock prevents
        that incomplete copies after a crash may leave the database in an
	unstable state. Removes the mutex for the write process. */
    void write_commit();
    
    /** Read operations: --------------------------------------------------- */

    /** Request the index for a read operation and registers the reading process.
        Reading the index files should be taking a quick copy, because if any process
	is in this state, write_commits are blocked. */
    void request_read_and_idx(pid_t pid);
    
    /** Changes the registered state from reading the index to reading the
        database. Can be safely called multiple times for the same process. */
    void read_idx_finished(pid_t pid);
    
    /** Unregisteres a reading process. */
    void read_finished(pid_t pid);
    
    /** Other operations: -------------------------------------------------- */
	       
  private:
    vector< File_Properties* > controlled_files;
    vector< Idx_Footprints > data_footprints;
    vector< Idx_Footprints > map_footprints;
    set< pid_t > processes_reading_idx;
    string shadow_name;
    
    void copy_shadows_to_mains();
    void copy_mains_to_shadows();
    void remove_shadows();
    void set_current_footprints();
    void write_index_of_empty_blocks();
};

bool file_exists(const string& filename);

#endif
