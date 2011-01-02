#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"
#include "print.h"

using namespace std;

int main(int argc, char* args[])
{
  Id_Query_Statement stmt_1(1);
  Print_Statement stmt_2(2);
  
  Resource_Manager rman, total_rman;
  
  cout<<"Print each item alone:\n";
  for (uint32 i(7778000/*471224000*/); i < 7779000/*471225000*/; ++i)
  {
    const char* attributes[5];
    attributes[0] = "type";
    attributes[1] = "node";
    attributes[2] = "ref";
    attributes[3] = (const char*) malloc(11);
    attributes[4] = 0;
    
    ostringstream buf("");
    buf<<i;
    buf.str().copy((char*)attributes[3], string::npos);
    ((char*)attributes[3])[buf.str().size()] = 0;
    stmt_1.set_attributes(attributes);  
    stmt_1.execute(rman);
    
    attributes[0] = "mode";
    attributes[1] = "body";
    attributes[2] = "order";
    if (argc >= 2)
      attributes[3] = args[1];
    else
      attributes[3] = "quadtile";
    
    stmt_2.set_attributes(attributes);
    stmt_2.execute(rman);
    
    if (!rman.sets()["_"].nodes.empty())
      total_rman.sets()["_"].nodes[rman.sets()["_"].nodes.begin()->first].push_back(rman.sets()["_"].nodes.begin()->second.front());
  }
  for (uint32 i(8237000); i < 8238000; ++i)
  {
    const char* attributes[5];
    attributes[0] = "type";
    attributes[1] = "way";
    attributes[2] = "ref";
    attributes[3] = (const char*) malloc(11);
    attributes[4] = 0;
    
    ostringstream buf("");
    buf<<i;
    buf.str().copy((char*)attributes[3], string::npos);
    ((char*)attributes[3])[buf.str().size()] = 0;
    stmt_1.set_attributes(attributes);  
    stmt_1.execute(rman);
    
    attributes[0] = "mode";
    attributes[1] = "body";
    attributes[2] = "order";
    if (argc >= 2)
      attributes[3] = args[1];
    else
      attributes[3] = "quadtile";
    
    stmt_2.set_attributes(attributes);
    stmt_2.execute(rman);
    
    if (!rman.sets()["_"].ways.empty())
      total_rman.sets()["_"].ways[rman.sets()["_"].ways.begin()->first].push_back(rman.sets()["_"].ways.begin()->second.front());
  }
  for (uint32 i(163000); i < 164000; ++i)
  {
    const char* attributes[5];
    attributes[0] = "type";
    attributes[1] = "relation";
    attributes[2] = "ref";
    attributes[3] = (const char*) malloc(11);
    attributes[4] = 0;
    
    ostringstream buf("");
    buf<<i;
    buf.str().copy((char*)attributes[3], string::npos);
    ((char*)attributes[3])[buf.str().size()] = 0;
    stmt_1.set_attributes(attributes);  
    stmt_1.execute(rman);
    
    attributes[0] = "mode";
    attributes[1] = "body";
    attributes[2] = "order";
    if (argc >= 2)
      attributes[3] = args[1];
    else
      attributes[3] = "quadtile";
    
    stmt_2.set_attributes(attributes);
    stmt_2.execute(rman);
    
    if (!rman.sets()["_"].relations.empty())
      total_rman.sets()["_"].relations[rman.sets()["_"].relations.begin()->first].push_back(rman.sets()["_"].relations.begin()->second.front());
  }
  cout<<"Print all items together:\n";

  const char* attributes[5];
  attributes[0] = "mode";
  attributes[1] = "body";
  attributes[2] = "order";
  if (argc >= 2)
    attributes[3] = args[1];
  else
    attributes[3] = "quadtile";
  attributes[4] = 0;
  
  stmt_2.set_attributes(attributes);
  stmt_2.execute(total_rman);
  
  return 0;
}
