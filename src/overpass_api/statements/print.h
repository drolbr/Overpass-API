#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Print_Statement() {}
    
  private:
    string input;
    unsigned int mode;
    unsigned int order;

    template< class TIndex, class TObject >
    void tags_quadtile
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop, uint32 mode, uint32 stopwatch_account,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* meta_file_prop = 0);
    
    template< class TIndex, class TObject >
    void tags_by_id
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop,
       uint32 FLUSH_SIZE, uint32 mode, uint32 stopwatch_account,
       Resource_Manager& rman, Transaction& transaction,
       const File_Properties* meta_file_prop = 0);
};

template< class TIndex >
void formulate_range_query
    (set< pair< Tag_Index_Local, Tag_Index_Local > >& range_set,
     const set< TIndex >& coarse_indices);

#endif
