#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "bbox_query.h"
#include "print.h"

using namespace std;

int main(int argc, char* args[])
{
  Resource_Manager rman;
  {
    Bbox_Query_Statement* stmt1 = new Bbox_Query_Statement(0);
    const char* attributes[] = { "s", "-90.0", "n", "90.0", "w", "179.0", "e", "-179.0", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  return 0;
}
