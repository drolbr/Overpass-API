#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "../frontend/console_output.h"
#include "bbox_query.h"
#include "foreach.h"
#include "id_query.h"
#include "make_area.h"
#include "print.h"
#include "query.h"
#include "recurse.h"
#include "union.h"

using namespace std;

int main(int argc, char* args[])
{
  vector< Aligned_Segment > segs;
  
  uint64 ll_total(((uint64)Node::ll_upper(51.25, 7.15)<<32) | Node::ll_lower(51.25, 7.15));
  cout<<Node::lat(ll_total>>32, ll_total)<<'\t'
      <<Node::lon(ll_total>>32, ll_total)<<'\n';
  Node::calc_aligned_segments
    (segs,
     ((uint64)Node::ll_upper(51.25, 7.15)<<32) | Node::ll_lower(51.25, 7.15),
     ((uint64)Node::ll_upper(51.251, 7.151)<<32) | Node::ll_lower(51.251, 7.151));
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_, 0)<<'\t'
        <<Node::lon(it->ll_upper_, 0)<<'\t'
	<<Node::lat(it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lon(it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lat(it->ll_lower_b>>32, it->ll_lower_b)<<'\t'
	<<Node::lon(it->ll_lower_b>>32, it->ll_lower_b)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Error_Output* error_output(new Console_Output(false));
  Statement::set_error_output(error_output);
  
  map< string, Set > sets;
  
  cout<<"Query to create areas:\n";
  {
    Id_Query_Statement* stmt1 = new Id_Query_Statement(0);
    const char* attributes[] = { "type", "relation", "ref", "62478", "into", "rels", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(sets);
  }
  {
    Union_Statement* stmt2 = new Union_Statement(0);
    const char* attributes[] = { 0 };
    stmt2->set_attributes(attributes);
    {
      Recurse_Statement* stmt3 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "relation-way", "from", "rels", 0 };
      stmt3->set_attributes(attributes);
      stmt2->add_statement(stmt3, "");
    }
    {
      Recurse_Statement* stmt3 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "way-node", 0 };
      stmt3->set_attributes(attributes);
      stmt2->add_statement(stmt3, "");
    }
    stmt2->execute(sets);
  }
  {
    Make_Area_Statement* stmt1 = new Make_Area_Statement(0);
    const char* attributes[] = { "pivot", "rels", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(sets);
  }
/*  {
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "body", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(sets);
  }*/

  cout<<"Query to create areas:\n";
  {
    Id_Query_Statement* stmt1 = new Id_Query_Statement(0);
    const char* attributes[] = { "type", "relation", "ref", "34631", "into", "rels", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(sets);
  }
  {
    Union_Statement* stmt2 = new Union_Statement(0);
    const char* attributes[] = { 0 };
    stmt2->set_attributes(attributes);
    {
      Recurse_Statement* stmt3 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "relation-way", "from", "rels", 0 };
      stmt3->set_attributes(attributes);
      stmt2->add_statement(stmt3, "");
    }
    {
      Recurse_Statement* stmt3 = new Recurse_Statement(0);
      const char* attributes[] = { "type", "way-node", 0 };
      stmt3->set_attributes(attributes);
      stmt2->add_statement(stmt3, "");
    }
    stmt2->execute(sets);
  }
  {
    Make_Area_Statement* stmt1 = new Make_Area_Statement(0);
    const char* attributes[] = { "pivot", "rels", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(sets);
  }

  return 0;
}
