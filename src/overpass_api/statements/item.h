#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__ITEM_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__ITEM_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"

using namespace std;

class Item_Statement : public Statement
{
  public:
    Item_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "item"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman) {}
    virtual ~Item_Statement() {}
    
  private:
    string output;
};

#endif
