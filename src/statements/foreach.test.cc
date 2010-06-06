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
  Foreach_Statement stmt_1(1, 101), stmt_7(7, 107), stmt_12(12, 1012);
  Print_Statement stmt_2(2, 102);
  Query_Statement stmt_3(3, 103), stmt_5(5, 105), stmt_10(10, 1010);
  Has_Key_Value_Statement stmt_4(4, 104), stmt_6(6, 106), stmt_11(11, 1011);
  Recurse_Statement stmt_8(8, 108);
  
  map< string, Set > sets;
  
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
  stmt_3.execute(sets);
  {
    const char* attributes[] = { 0 };
    stmt_1.set_attributes(attributes);
    {
      const char* attributes[] = { "mode", "body", 0 };
      stmt_2.set_attributes(attributes);
    }
    stmt_1.add_statement(&stmt_2, "");
  }
  stmt_1.execute(sets);
  
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
  stmt_5.execute(sets);
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
  stmt_7.execute(sets);
  
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
  stmt_10.execute(sets);
  {
    const char* attributes[] = { 0 };
    stmt_12.set_attributes(attributes);
    {
      const char* attributes[] = { "mode", "body", 0 };
      stmt_2.set_attributes(attributes);
    }
    stmt_12.add_statement(&stmt_2, "");
  }
  stmt_12.execute(sets);
  
  return 0;
}
