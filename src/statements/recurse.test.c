#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "print.h"
#include "recurse.h"

using namespace std;

int main(int argc, char* args[])
{
  Id_Query_Statement stmt_1(1, 101);
  Recurse_Statement stmt_2(2, 102);
  Print_Statement stmt_3(3, 103);
  
  map< string, Set > sets;
  
  const char* attributes[5];
  attributes[0] = "type";
  attributes[1] = "relation";
  attributes[2] = "ref";
  attributes[3] = "122635"/*"163298"*/;
  attributes[4] = 0;
    
  stmt_1.set_attributes(attributes);
  stmt_1.execute(sets);
    
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  stmt_3.set_attributes(attributes);
  stmt_3.execute(sets);
  
  attributes[0] = "type";
  attributes[1] = "relation-relation";
  attributes[2] = 0;
  
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  stmt_3.set_attributes(attributes);
  stmt_3.execute(sets);
  
  attributes[0] = "type";
  attributes[1] = "relation-way";
  attributes[2] = 0;
  
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  stmt_3.set_attributes(attributes);
  stmt_3.execute(sets);
  
  attributes[0] = "type";
  attributes[1] = "way-node";
  attributes[2] = 0;
  
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  stmt_3.set_attributes(attributes);
  stmt_3.execute(sets);
  
  return 0;
}
