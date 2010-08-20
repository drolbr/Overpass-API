#ifndef PRINT_STATEMENT_DEFINED
#define PRINT_STATEMENT_DEFINED

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
    virtual void execute(map< string, Set >& maps);
    virtual ~Print_Statement() {}
    
  private:
    string input;
    unsigned int mode;
    unsigned int order;

    template< class TIndex, class TObject >
    void tags_quadtile
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop, uint32 mode, uint32 stopwatch_account);
    
    template< class TIndex, class TObject >
    void tags_by_id
      (const map< TIndex, vector< TObject > >& items,
       const File_Properties& file_prop,
       uint32 FLUSH_SIZE, uint32 mode, uint32 stopwatch_account);
};

template< class TIndex >
void formulate_range_query
    (set< pair< Tag_Index_Local, Tag_Index_Local > >& range_set,
     const set< TIndex >& coarse_indices);

#endif
