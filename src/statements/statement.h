#ifndef STATEMENT_DEFINED
#define STATEMENT_DEFINED

#include <map>
#include <set>
#include <vector>

#include "../core/datatypes.h"

using namespace std;

/**
 * The base class for all statements
 */
class Statement
{
  public:
    Statement(int line_number_, int stmt_id_) : line_number(line_number_), stmt_id(stmt_id_) {}
    
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() const = 0;
    virtual string get_result_name() const = 0;
    virtual void forecast() = 0;
    virtual void execute(map< string, Set >& maps) = 0;
    virtual ~Statement() {}
    
    int get_line_number() const { return line_number; }
    int get_stmt_id() const { return stmt_id; }
    int get_startpos() const { return startpos; }
    void set_startpos(int pos) { startpos = pos; }
    int get_endpos() const { return endpos; }
    void set_endpos(int pos) { endpos = pos; }
    int get_tagendpos() const { return tagendpos; }
    void set_tagendpos(int pos) { tagendpos = pos; }
    
    void display_full();
    void display_starttag();
    
    static void eval_cstr_array
	(string element, map< string, string >& attributes, const char **attr);
    static void substatement_error(string parent, Statement* child);
    static void reset_stmt_counter() { next_stmt_id = 0; }
    
    const static int NODE = 1;
    const static int WAY = 2;
    const static int RELATION = 3;
    
  private:
    static int next_stmt_id;
    
    int line_number;
    int stmt_id;
    int startpos, endpos, tagendpos;
};

#endif
