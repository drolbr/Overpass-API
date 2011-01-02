#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "foreach.h"
#include "print.h"
#include "query.h"
#include "recurse.h"

using namespace std;

int main(int argc, char* args[])
{
  Foreach_Statement stmt_1(1), stmt_7(7), stmt_12(12);
  Print_Statement stmt_2(2);
  Query_Statement stmt_3(3), stmt_5(5), stmt_10(10);
  Has_Kv_Statement stmt_4(4), stmt_6(6), stmt_11(11);
  Recurse_Statement stmt_8(8);
  
  Resource_Manager rman;
  
  cout<<"Print nodes:\n";
  {
    const char* attributes[] = { "type", "node", 0 };
    stmt_3.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "name", "v", "Karlsplatz", 0 };
      stmt_4.set_attributes(attributes);
    }
    stmt_3.add_statement(&stmt_4, "");
  }
  stmt_3.execute(rman);
  {
    const char* attributes[] = { 0 };
    stmt_1.set_attributes(attributes);
    {
      const char* attributes[] = { "mode", "body", 0 };
      stmt_2.set_attributes(attributes);
    }
    stmt_1.add_statement(&stmt_2, "");
  }
  stmt_1.execute(rman);
  
  cout<<"Print ways:\n";
  {
    const char* attributes[] = { "type", "way", 0 };
    stmt_5.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "name", "v", "Friedrich-Ebert-Straße", 0 };
      stmt_6.set_attributes(attributes);
    }
    stmt_5.add_statement(&stmt_6, "");
  }
  stmt_5.execute(rman);
  {
    const char* attributes[] = { 0 };
    stmt_7.set_attributes(attributes);
    {
      const char* attributes[] = { "type", "way-node", 0 };
      stmt_8.set_attributes(attributes);
    }
    stmt_7.add_statement(&stmt_8, "");
    {
      const char* attributes[] = { "mode", "body", 0 };
      stmt_2.set_attributes(attributes);
    }
    stmt_7.add_statement(&stmt_2, "");
  }
  stmt_7.execute(rman);
  
  cout<<"Print relations:\n";
  {
    const char* attributes[] = { "type", "relation", 0 };
    stmt_10.set_attributes(attributes);
    {
      const char* attributes[] = { "k", "operator", "v", "WSW", 0 };
      stmt_11.set_attributes(attributes);
    }
    stmt_10.add_statement(&stmt_11, "");
  }
  stmt_10.execute(rman);
  {
    const char* attributes[] = { 0 };
    stmt_12.set_attributes(attributes);
    {
      const char* attributes[] = { "mode", "body", 0 };
      stmt_2.set_attributes(attributes);
    }
    stmt_12.add_statement(&stmt_2, "");
  }
  stmt_12.execute(rman);
  
  return 0;
}
