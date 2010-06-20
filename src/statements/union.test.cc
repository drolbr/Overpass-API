#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "union.h"
#include "print.h"
#include "query.h"

using namespace std;

int main(int argc, char* args[])
{
  Union_Statement stmt_1(1);
  Print_Statement stmt_2(2);
  Query_Statement stmt_3(3), stmt_5(5), stmt_7(7), stmt_9(9), stmt_11(11), stmt_13(13);
  Has_Key_Value_Statement stmt_4(4), stmt_6(6), stmt_8(8), stmt_10(10), stmt_12(12), stmt_14(14);
  
  map< string, Set > sets;
  
  cout<<"Print nodes 1:\n";
  {
    const char* attributes[] = { "type", "node", 0 };
    stmt_3.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "name", "v", "Karlsplatz", 0 };
      stmt_4.set_attributes(attributes);
    }
    stmt_3.add_statement(&stmt_4, "");
  }
  stmt_3.execute(sets);
  {
    const char* attributes[] = { "mode", "body", 0 };
    stmt_2.set_attributes(attributes);
  }
  stmt_2.execute(sets);
  
  cout<<"Print nodes 2:\n";
  {
    const char* attributes[] = { "type", "node", 0 };
    stmt_5.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "name", "v", "Alter Markt", 0 };
      stmt_6.set_attributes(attributes);
    }
    stmt_5.add_statement(&stmt_6, "");
  }
  stmt_5.execute(sets);
  stmt_2.execute(sets);
  
  cout<<"Print ways 1:\n";
  {
    const char* attributes[] = { "type", "way", 0 };
    stmt_7.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "name", "v", "Barmer Straße", 0 };
      stmt_8.set_attributes(attributes);
    }
    stmt_7.add_statement(&stmt_8, "");
  }
  stmt_7.execute(sets);
  stmt_2.execute(sets);
  
  cout<<"Print ways 2:\n";
  {
    const char* attributes[] = { "type", "way", 0 };
    stmt_9.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "name", "v", "Elberfelder Straße", 0 };
      stmt_10.set_attributes(attributes);
    }
    stmt_9.add_statement(&stmt_10, "");
  }
  stmt_9.execute(sets);
  stmt_2.execute(sets);
  
  cout<<"Print relations 1:\n";
  {
    const char* attributes[] = { "type", "relation", 0 };
    stmt_11.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "ref", "v", "603", 0 };
      stmt_12.set_attributes(attributes);
    }
    stmt_11.add_statement(&stmt_12, "");
  }
  stmt_11.execute(sets);
  stmt_2.execute(sets);
  
  cout<<"Print relations 2:\n";
  {
    const char* attributes[] = { "type", "relation", 0 };
    stmt_13.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "to", "v", "W-Katernberg, Am Eckbusch", 0 };
      stmt_14.set_attributes(attributes);
    }
    stmt_13.add_statement(&stmt_14, "");
  }
  stmt_13.execute(sets);
  stmt_2.execute(sets);
  
  cout<<"Print result of union:\n";
  {
    const char* attributes[] = { 0 };
    stmt_1.set_attributes(attributes);
  }
  stmt_1.add_statement(&stmt_3, "");
  stmt_1.add_statement(&stmt_5, "");
  stmt_1.add_statement(&stmt_7, "");
  stmt_1.add_statement(&stmt_9, "");
  stmt_1.add_statement(&stmt_11, "");
  stmt_1.add_statement(&stmt_13, "");
  stmt_1.execute(sets);
  stmt_2.execute(sets);
  
  return 0;
}
