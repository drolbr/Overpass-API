#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "bbox_query.h"
#include "print.h"

using namespace std;

int main(int argc, char* args[])
{
  Bbox_Query_Statement stmt_1(1, 101);
  Print_Statement stmt_2(2, 102);
  
  map< string, Set > sets;
  
  const char* attributes[9];
  attributes[0] = "s";
  attributes[1] = "-90.0";
  attributes[2] = "n";
  attributes[3] = "90.0";
  attributes[4] = "w";
  attributes[5] = "179.0";
  attributes[6] = "e";
  attributes[7] = "-179.0";
  attributes[8] = 0;
  
  stmt_1.set_attributes(attributes);
  stmt_1.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  cout<<"print results:\n";
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  return 0;
}
