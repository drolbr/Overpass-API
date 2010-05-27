#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "print.h"

using namespace std;

int main(int argc, char* args[])
{
  Id_Query_Statement stmt_1(1, 101);
  Print_Statement stmt_2(2, 102);
  
  map< string, Set > sets;
  
  for (uint32 i(471224000); i < 471225000; ++i)
  {
    const char* attributes[5];
    attributes[0] = "type";
    attributes[1] = "node";
    attributes[2] = "ref";
    attributes[3] = "471224974";
    attributes[4] = 0;
    
/*    attributes[3][6] = (i/100) + 48;
    attributes[3][7] = (i/10%10) + 48;
    attributes[3][8] = (i%10) + 48;*/
/*    ostringstream buf("");
    buf<<i;
    attributes[3] = buf.str().c_str();*/
    stmt_1.set_attributes(attributes);  
    stmt_1.execute(sets);
    
    attributes[0] = "mode";
    attributes[1] = "body";
    attributes[2] = "order";
    attributes[3] = "quadtile";

    stmt_2.set_attributes(attributes);
    stmt_2.execute(sets);
  }
    
  return 0;
}
