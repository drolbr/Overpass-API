#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__NEWER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__NEWER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Newer_Statement : public Statement
{
  public:
    Newer_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "newer"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Newer_Statement() {}

    uint64 get_timestamp() const { return than_timestamp; }

  private:
    uint64 than_timestamp;
};

#endif
