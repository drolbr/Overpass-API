#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "bbox_query.h"
#include "query.h"
#include "print.h"

using namespace std;

int main(int argc, char* args[])
{
  Query_Statement stmt_1(1), stmt_3(3), stmt_4(4),
      stmt_5(5), stmt_6(6);
  Bbox_Query_Statement stmt_1b(12);
  Has_Key_Value_Statement stmt_1a(11);
  Print_Statement stmt_2(2);
  
  map< string, Set > sets;
  
  const char* attributes[9];
  attributes[0] = "k";
  attributes[1] = "name";
  attributes[2] = "v";
  attributes[3] = "Grenze Jagdhaus";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  
  attributes[0] = "type";
  attributes[1] = "node";
  attributes[2] = 0;

  stmt_1.add_statement(&stmt_1a, "");
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
  
  attributes[0] = "type";
  attributes[1] = "node";
  attributes[2] = 0;
  
  stmt_3.set_attributes(attributes);
  
  attributes[0] = "k";
  attributes[1] = "highway";
  attributes[2] = "v";
  attributes[3] = "bus_stop";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_3.add_statement(&stmt_1a, "");
  
  attributes[0] = "k";
  attributes[1] = "name";
  attributes[2] = "v";
  attributes[3] = "";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_3.add_statement(&stmt_1a, "");
  
  attributes[0] = "k";
  attributes[1] = "shelter";
  attributes[2] = "v";
  attributes[3] = "yes";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_3.add_statement(&stmt_1a, "");
  stmt_3.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  cout<<"print results:\n";
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  attributes[0] = "type";
  attributes[1] = "way";
  attributes[2] = 0;
  
  stmt_4.set_attributes(attributes);
  
  attributes[0] = "k";
  attributes[1] = "highway";
  attributes[2] = "v";
  attributes[3] = "";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_4.add_statement(&stmt_1a, "");
  
  attributes[0] = "k";
  attributes[1] = "name";
  attributes[2] = "v";
  attributes[3] = "Morianstraße";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_4.add_statement(&stmt_1a, "");
  stmt_4.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  cout<<"print results:\n";
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  attributes[0] = "type";
  attributes[1] = "relation";
  attributes[2] = 0;
  
  stmt_5.set_attributes(attributes);
  
  attributes[0] = "k";
  attributes[1] = "type";
  attributes[2] = "v";
  attributes[3] = "route";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_5.add_statement(&stmt_1a, "");
  
  attributes[0] = "k";
  attributes[1] = "route";
  attributes[2] = "v";
  attributes[3] = "bus";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_5.add_statement(&stmt_1a, "");
  
  attributes[0] = "k";
  attributes[1] = "ref";
  attributes[2] = "v";
  attributes[3] = "";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_5.add_statement(&stmt_1a, "");
  stmt_5.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  cout<<"print results:\n";
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  attributes[0] = "type";
  attributes[1] = "node";
  attributes[2] = 0;
  
  stmt_6.set_attributes(attributes);
  
  attributes[0] = "s";
  attributes[1] = "51.2";
  attributes[2] = "n";
  attributes[3] = "51.3";
  attributes[4] = "w";
  attributes[5] = "7.0";
  attributes[6] = "e";
  attributes[7] = "7.3";
  attributes[8] = 0;
  
  stmt_1b.set_attributes(attributes);
  stmt_6.add_statement(&stmt_1b, "");
  
  attributes[0] = "k";
  attributes[1] = "highway";
  attributes[2] = "v";
  attributes[3] = "bus_stop";
  attributes[4] = 0;
  
  stmt_1a.set_attributes(attributes);
  stmt_6.add_statement(&stmt_1a, "");
  stmt_6.execute(sets);
  
  attributes[0] = "mode";
  attributes[1] = "skeleton";
  attributes[2] = "order";
  attributes[3] = "id";
  attributes[4] = 0;
  
  cout<<"print results:\n";
  stmt_2.set_attributes(attributes);
  stmt_2.execute(sets);
  
  return 0;
}
