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
  
  map< string, Set > sets, total_sets;
  
  cout<<"Print each item alone:\n";
  for (uint32 i(471224000); i < 471225000; ++i)
  {
    const char* attributes[5];
    attributes[0] = "type";
    attributes[1] = "node";
    attributes[2] = "ref";
    attributes[3] = (const char*) malloc(10);
    attributes[4] = 0;
    
/*    attributes[3][6] = (i/100) + 48;
    attributes[3][7] = (i/10%10) + 48;
    attributes[3][8] = (i%10) + 48;*/
    ostringstream buf("");
    buf<<i;
    buf.str().copy((char*)attributes[3], string::npos);
    ((char*)attributes[3])[9] = 0;
    stmt_1.set_attributes(attributes);  
    stmt_1.execute(sets);
    
    attributes[0] = "mode";
    attributes[1] = "body";
    attributes[2] = "order";
    if (argc >= 2)
      attributes[3] = args[1];
    else
      attributes[3] = "quadtile";
    
    stmt_2.set_attributes(attributes);
    stmt_2.execute(sets);
    
    if (!sets["_"].nodes.empty())
      total_sets["_"].nodes[sets["_"].nodes.begin()->first].push_back(sets["_"].nodes.begin()->second.front());
  }
  cout<<"Print all items together:\n";

/*  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it(total_sets["_"].nodes.begin());
      it != total_sets["_"].nodes.end(); ++it)
  {
    cout<<hex<<it->first.val()<<dec<<'\t';
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      cout<<it2->id<<' ';
    cout<<'\n';
  }*/
  
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
  stmt_2.execute(total_sets);
  
  return 0;
}
