#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "bbox_query.h"
#include "foreach.h"
#include "id_query.h"
#include "print.h"
#include "query.h"
#include "recurse.h"
#include "union.h"

using namespace std;

int main(int argc, char* args[])
{
  Resource_Manager rman;
  
  cout<<"Query to create areas:\n";
  {
    Union_Statement* stmt1 = new Union_Statement(0);
    const char* attributes[] = { 0 };
    stmt1->set_attributes(attributes);
    {
      Query_Statement* stmt2 = new Query_Statement(0);
      const char* attributes[] = { "type", "relation", 0 };
      stmt2->set_attributes(attributes);
      {
        Has_Kv_Statement* stmt3 = new Has_Kv_Statement(0);
        const char* attributes[] = { "k", "type", "v", "multipolygon", 0 };
        stmt3->set_attributes(attributes);
        stmt2->add_statement(stmt3, "");
      }
      {
        Has_Kv_Statement* stmt3 = new Has_Kv_Statement(0);
        const char* attributes[] = { "k", "name", 0 };
        stmt3->set_attributes(attributes);
        stmt2->add_statement(stmt3, "");
      }
      stmt1->add_statement(stmt2, "");
    }
    {
      Query_Statement* stmt2 = new Query_Statement(0);
      const char* attributes[] = { "type", "relation", 0 };
      stmt2->set_attributes(attributes);
      {
        Has_Kv_Statement* stmt3 = new Has_Kv_Statement(0);
        const char* attributes[] = { "k", "admin_level", 0 };
        stmt3->set_attributes(attributes);
        stmt2->add_statement(stmt3, "");
      }
      {
        Has_Kv_Statement* stmt3 = new Has_Kv_Statement(0);
        const char* attributes[] = { "k", "name", 0 };
        stmt3->set_attributes(attributes);
        stmt2->add_statement(stmt3, "");
      }
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Foreach_Statement* stmt1 = new Foreach_Statement(0);
    const char* attributes[] = { 0 };
    stmt1->set_attributes(attributes);
    {
      Union_Statement* stmt2 = new Union_Statement(0);
      const char* attributes[] = { 0 };
      stmt2->set_attributes(attributes);
      {
	Recurse_Statement* stmt3 = new Recurse_Statement(0);
	const char* attributes[] = { "type", "relation-way", 0 };
	stmt3->set_attributes(attributes);
	stmt2->add_statement(stmt3, "");
      }
      {
	Recurse_Statement* stmt3 = new Recurse_Statement(0);
	const char* attributes[] = { "type", "way-node", 0 };
	stmt3->set_attributes(attributes);
	stmt2->add_statement(stmt3, "");
      }
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  
  cout<<"Bbox of Wuppertal:\n";
  {
    Union_Statement* stmt1 = new Union_Statement(0);
    const char* attributes[] = { 0 };
    stmt1->set_attributes(attributes);
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "n", "51.4", "s", "51.1", "w", "7.0", "e", "7.3", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "node-relation", "into", "rels", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "node-way", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "way-relation", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }

  cout<<"POIs 1:\n";
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "n", "51.5", "s", "42.5", "w", "-6.0", "e", "8.0", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "railway", "v", "station", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  cout<<"POIs 2:\n";
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "n", "54.0", "s", "51.0", "w", "3.0", "e", "8.0", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  cout<<"POIs 3:\n";
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "n", "49.0", "s", "48.7", "w", "9.4", "e", "8.9", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "amenity", "v", "bank", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }

  cout<<"Relation extension for JOSM:\n";
  {
    Union_Statement* stmt1 = new Union_Statement(0);
    const char* attributes[] = { "into", "full", 0 };
    stmt1->set_attributes(attributes);
    {
      Id_Query_Statement* stmt2 = new Id_Query_Statement(0);
      const char* attributes[] = { "type", "relation", "ref", "163298", "into", "rels", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "relation-way", "from", "rels", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "way-node", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "relation-node", "from", "rels", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Union_Statement* stmt1 = new Union_Statement(0);
    const char* attributes[] = { 0 };
    stmt1->set_attributes(attributes);
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "node-relation", "from", "full", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "way-relation", "from", "full", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Recurse_Statement* stmt2 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "relation-backwards", "from", "full", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "ids_only", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  cout<<"Housenumbers in Europe (but not only at the moment):\n";
  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", 0 };
    stmt1->set_attributes(attributes);
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "n", "65.0", "s", "35.0", "w", "-15.0", "e", "30.0", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "addr:housenumber", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  {
    Union_Statement* stmt1 = new Union_Statement(0);
    const char* attributes[] = { 0 };
    stmt1->set_attributes(attributes);
    {
      Query_Statement* stmt2 = new Query_Statement(0);
      const char* attributes[] = { "type", "way", 0 };
      stmt2->set_attributes(attributes);
      {
        Has_Kv_Statement* stmt3 = new Has_Kv_Statement(0);
        const char* attributes[] = { "k", "addr:housenumber", 0 };
        stmt3->set_attributes(attributes);
        stmt2->add_statement(stmt3, "");
      }
      stmt1->add_statement(stmt2, "");
    }
    {
      Query_Statement* stmt2 = new Query_Statement(0);
      const char* attributes[] = { "type", "way", 0 };
      stmt2->set_attributes(attributes);
      {
        Has_Kv_Statement* stmt3 = new Has_Kv_Statement(0);
        const char* attributes[] = { "k", "addr:interpolation", 0 };
        stmt3->set_attributes(attributes);
        stmt2->add_statement(stmt3, "");
      }
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  {
    Recurse_Statement* stmt1 = new Recurse_Statement(0);
    const char* attributes[] = { "type", "way-node", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  
  return 0;
}
