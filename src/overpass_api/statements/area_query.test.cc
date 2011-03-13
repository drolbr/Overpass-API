#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "../frontend/console_output.h"
#include "area_query.h"
#include "bbox_query.h"
#include "coord_query.h"
#include "print.h"
#include "query.h"
#include "union.h"

using namespace std;

void draw_item(vector< string >& visual, uint32 index, const Node_Skeleton& node, char c)
{
  uint i(Node::lat(index, node.ll_lower) * 2000 - 102315);
  uint j(Node::lon(index, node.ll_lower) * 2500 - 17495);
  if ((visual[i][j] != '.') && (visual[i][j] != c))
    visual[i][j] = '#';
  else
    visual[i][j] = c;
}

void comp_sets(Set& s1, Set& s2)
{
  map< Uint32_Index, vector< Node_Skeleton > >::iterator it1(s1.nodes.begin());
  map< Uint32_Index, vector< Node_Skeleton > >::iterator it2(s2.nodes.begin());
  vector< string > visual(600, string(1000, '.'));
  
  while ((it1 != s1.nodes.end()) && (it2 != s2.nodes.end()))
  {
    if (it1->first == it2->first)
    {
      sort(it1->second.begin(), it1->second.end());
      sort(it2->second.begin(), it2->second.end());
      
      vector< Node_Skeleton >::const_iterator itn1(it1->second.begin());
      vector< Node_Skeleton >::const_iterator itn2(it2->second.begin());
      
      while ((itn1 != it1->second.end()) && (itn2 != it2->second.end()))
      {
	if (itn1->id == itn2->id)
	{
	  //draw_item(visual, it1->first.val(), *itn1, ':');
	  ++itn1;
	  ++itn2;
	}
	else if (itn1->id < itn2->id)
	{
          cout<<"a\t"<<itn1->id<<'\t'
              <<Node::lat(it1->first.val(), itn1->ll_lower)<<'\t'
	      <<Node::lon(it1->first.val(), itn1->ll_lower)<<'\n';
	  draw_item(visual, it1->first.val(), *itn1, 'a');
	  ++itn1;
	}
	else
	{
          cout<<"b\t"<<itn2->id<<'\t'
              <<Node::lat(it2->first.val(), itn2->ll_lower)<<'\t'
	      <<Node::lon(it2->first.val(), itn2->ll_lower)<<'\n';
	  draw_item(visual, it2->first.val(), *itn2, 'b');
	  ++itn2;
	}
      }
      while (itn1 != it1->second.end())
      {
        cout<<"a\t"<<itn1->id<<'\t'
            <<Node::lat(it1->first.val(), itn1->ll_lower)<<'\t'
	    <<Node::lon(it1->first.val(), itn1->ll_lower)<<'\n';
	draw_item(visual, it1->first.val(), *itn1, 'a');
	++itn1;
      }
      while (itn2 != it2->second.end())
      {
        cout<<"b\t"<<itn2->id<<'\t'
            <<Node::lat(it2->first.val(), itn2->ll_lower)<<'\t'
	    <<Node::lon(it2->first.val(), itn2->ll_lower)<<'\n';
	draw_item(visual, it2->first.val(), *itn2, 'b');
	++itn2;
      }
      
      ++it1;
      ++it2;
    }
    else if (it1->first < it2->first)
    {
      for (vector< Node_Skeleton >::const_iterator it(it1->second.begin());
          it != it1->second.end(); ++it)
      {
        cout<<"a\t"<<it->id<<'\t'
            <<Node::lat(it1->first.val(), it->ll_lower)<<'\t'
	    <<Node::lon(it1->first.val(), it->ll_lower)<<'\n';
	draw_item(visual, it1->first.val(), *it, 'a');
      }
      ++it1;
    }
    else
    {
      for (vector< Node_Skeleton >::const_iterator it(it2->second.begin());
          it != it2->second.end(); ++it)
      {
        cout<<"b\t"<<it->id<<'\t'
            <<Node::lat(it2->first.val(), it->ll_lower)<<'\t'
            <<Node::lon(it2->first.val(), it->ll_lower)<<'\n';
	draw_item(visual, it2->first.val(), *it, 'b');
      }
      ++it2;
    }
  }
  while (it1 != s1.nodes.end())
  {
    for (vector< Node_Skeleton >::const_iterator it(it1->second.begin());
        it != it1->second.end(); ++it)
    {
      cout<<"a\t"<<it->id<<'\t'
          <<Node::lat(it1->first.val(), it->ll_lower)<<'\t'
	  <<Node::lon(it1->first.val(), it->ll_lower)<<'\n';
      draw_item(visual, it1->first.val(), *it, 'a');
    }
    ++it1;
  }
  while (it2 != s2.nodes.end())
  {
    for (vector< Node_Skeleton >::const_iterator it(it2->second.begin());
        it != it2->second.end(); ++it)
    {
      cout<<"b\t"<<it->id<<'\t'
          <<Node::lat(it2->first.val(), it->ll_lower)<<'\t'
	  <<Node::lon(it2->first.val(), it->ll_lower)<<'\n';
      draw_item(visual, it2->first.val(), *it, 'b');
    }
    ++it2;
  }
  
  for (vector< string >::const_iterator it(visual.begin()); it != visual.end(); ++it)
    cout<<*it<<'\n';
}

int main(int argc, char* args[])
{
  vector< Aligned_Segment > segs;
    
  Error_Output* error_output(new Console_Output(false));
  Statement::set_error_output(error_output);
  
  Resource_Manager rman;
  
  {
    Union_Statement* stmt1 = new Union_Statement(0);
    const char* attributes[] = { "into", "comp", 0 };
    stmt1->set_attributes(attributes);
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1675", "n", "51.1725", "w", "7.134", "e", "7.138", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1725", "n", "51.1775", "w", "7.126", "e", "7.150", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1775", "n", "51.1825", "w", "7.126", "e", "7.150", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1825", "n", "51.1875", "w", "7.114", "e", "7.146", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1875", "n", "51.1925", "w", "7.110", "e", "7.142", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1925", "n", "51.1975", "w", "7.106", "e", "7.154", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.1975", "n", "51.2025", "w", "7.106", "e", "7.158", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2025", "n", "51.2075", "w", "7.106", "e", "7.162", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2075", "n", "51.2125", "w", "7.046", "e", "7.050", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2075", "n", "51.2125", "w", "7.106", "e", "7.166", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2075", "n", "51.2125", "w", "7.266", "e", "7.302", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2125", "n", "51.2175", "w", "7.038", "e", "7.062", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2125", "n", "51.2175", "w", "7.102", "e", "7.206", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2125", "n", "51.2175", "w", "7.262", "e", "7.298", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2175", "n", "51.2225", "w", "7.038", "e", "7.082", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2175", "n", "51.2225", "w", "7.094", "e", "7.218", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2175", "n", "51.2225", "w", "7.254", "e", "7.294", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2225", "n", "51.2275", "w", "7.042", "e", "7.234", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2225", "n", "51.2275", "w", "7.238", "e", "7.302", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2275", "n", "51.2325", "w", "7.042", "e", "7.046", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2275", "n", "51.2325", "w", "7.050", "e", "7.310", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2325", "n", "51.2375", "w", "7.038", "e", "7.310", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2375", "n", "51.2425", "w", "7.030", "e", "7.306", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2425", "n", "51.2475", "w", "7.014", "e", "7.298", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2475", "n", "51.2525", "w", "7.038", "e", "7.282", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2475", "n", "51.2525", "w", "7.290", "e", "7.306", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2525", "n", "51.2575", "w", "7.042", "e", "7.278", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2575", "n", "51.2625", "w", "7.062", "e", "7.266", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2625", "n", "51.2675", "w", "7.070", "e", "7.270", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2675", "n", "51.2725", "w", "7.074", "e", "7.274", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2725", "n", "51.2775", "w", "7.078", "e", "7.274", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2775", "n", "51.2825", "w", "7.082", "e", "7.270", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2825", "n", "51.2875", "w", "7.098", "e", "7.258", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2825", "n", "51.2875", "w", "7.262", "e", "7.266", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2875", "n", "51.2925", "w", "7.098", "e", "7.258", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2925", "n", "51.2975", "w", "7.098", "e", "7.194", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2925", "n", "51.2975", "w", "7.198", "e", "7.262", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2975", "n", "51.3025", "w", "7.102", "e", "7.174", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.2975", "n", "51.3025", "w", "7.206", "e", "7.262", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.3025", "n", "51.3075", "w", "7.130", "e", "7.182", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.3025", "n", "51.3075", "w", "7.222", "e", "7.262", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.3075", "n", "51.3125", "w", "7.162", "e", "7.174", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.3075", "n", "51.3125", "w", "7.238", "e", "7.266", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0);
      const char* attributes[] = { "s", "51.3125", "n", "51.3175", "w", "7.254", "e", "7.266", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    Area_Query_Statement* stmt1 = new Area_Query_Statement(0);
    const char* attributes[] = { "ref", "3600062478", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  /*{
    Print_Statement* stmt1 = new Print_Statement(0);
    const char* attributes[] = { "mode", "ids_only", "from", "_", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }*/
  comp_sets(rman.sets()["comp"], rman.sets()["_"]);

  {
    Query_Statement* stmt1 = new Query_Statement(0);
    const char* attributes[] = { "type", "node", "into", "comp", 0 };
    stmt1->set_attributes(attributes);
    {
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0);
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    {
      Area_Query_Statement* stmt2 = new Area_Query_Statement(0);
      const char* attributes[] = { "ref", "3600062478", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  comp_sets(rman.sets()["comp"], rman.sets()["_"]);
  
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
      const char* attributes[] = { "s", "51.1675", "n", "51.3175", "w", "7.014", "e", "7.310", 0 };
      stmt2->set_attributes(attributes);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  comp_sets(rman.sets()["comp"], rman.sets()["_"]);
  
  return 0;
}
