#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "bbox_query.h"
#include "query.h"
#include "print.h"

using namespace std;

int main(int argc, char* args[])
{
  Query_Statement stmt_5(5), stmt_6(6);
  
  Resource_Manager rman;
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "name", "v", "Grenze Jagdhaus", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "name", "v", "", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "shelter", "v", "yes", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "way", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "name", "v", "Morianstraße", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "shelter", "v", "yes", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "relation", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "type", "v", "route", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "route", "v", "bus", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "ref", "v", "", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2", "n", "51.3", "w", "7.0", "e", "7.3", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  cout<<"print results:\n";
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "skeleton", "order", "id", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  return 0;
}
