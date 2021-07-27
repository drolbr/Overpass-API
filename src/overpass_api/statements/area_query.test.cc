/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

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


void draw_item(std::vector< std::string >& visual, uint32 index, const Node_Skeleton& node, char c)
{
  uint i(::lat(index, node.ll_lower) * 2000 - 102315);
  uint j(::lon(index, node.ll_lower) * 2500 - 17495);
  if ((visual[i][j] != '.') && (visual[i][j] != c))
    visual[i][j] = '#';
  else
    visual[i][j] = c;
}

void comp_sets(Resource_Manager& rman, const std::string& set_1, const std::string& set_2)
{
  Set s1;
  rman.swap_set(set_1, s1);
  Set s2;
  rman.swap_set(set_2, s2);

  std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it1(s1.nodes.begin());
  std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it2(s2.nodes.begin());
  std::vector< std::string > visual(600, std::string(1000, '.'));

  while ((it1 != s1.nodes.end()) && (it2 != s2.nodes.end()))
  {
    if (it1->first == it2->first)
    {
      std::sort(it1->second.begin(), it1->second.end());
      std::sort(it2->second.begin(), it2->second.end());

      std::vector< Node_Skeleton >::const_iterator itn1(it1->second.begin());
      std::vector< Node_Skeleton >::const_iterator itn2(it2->second.begin());

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
          std::cout<<"a\t"<<itn1->id.val()<<'\t'
              <<::lat(it1->first.val(), itn1->ll_lower)<<'\t'
	      <<::lon(it1->first.val(), itn1->ll_lower)<<'\n';
	  draw_item(visual, it1->first.val(), *itn1, 'a');
	  ++itn1;
	}
	else
	{
          std::cout<<"b\t"<<itn2->id.val()<<'\t'
              <<::lat(it2->first.val(), itn2->ll_lower)<<'\t'
	      <<::lon(it2->first.val(), itn2->ll_lower)<<'\n';
	  draw_item(visual, it2->first.val(), *itn2, 'b');
	  ++itn2;
	}
      }
      while (itn1 != it1->second.end())
      {
        std::cout<<"a\t"<<itn1->id.val()<<'\t'
            <<::lat(it1->first.val(), itn1->ll_lower)<<'\t'
	    <<::lon(it1->first.val(), itn1->ll_lower)<<'\n';
	draw_item(visual, it1->first.val(), *itn1, 'a');
	++itn1;
      }
      while (itn2 != it2->second.end())
      {
        std::cout<<"b\t"<<itn2->id.val()<<'\t'
            <<::lat(it2->first.val(), itn2->ll_lower)<<'\t'
	    <<::lon(it2->first.val(), itn2->ll_lower)<<'\n';
	draw_item(visual, it2->first.val(), *itn2, 'b');
	++itn2;
      }

      ++it1;
      ++it2;
    }
    else if (it1->first < it2->first)
    {
      for (std::vector< Node_Skeleton >::const_iterator it(it1->second.begin());
          it != it1->second.end(); ++it)
      {
        std::cout<<"a\t"<<it->id.val()<<'\t'
            <<::lat(it1->first.val(), it->ll_lower)<<'\t'
	    <<::lon(it1->first.val(), it->ll_lower)<<'\n';
	draw_item(visual, it1->first.val(), *it, 'a');
      }
      ++it1;
    }
    else
    {
      for (std::vector< Node_Skeleton >::const_iterator it(it2->second.begin());
          it != it2->second.end(); ++it)
      {
        std::cout<<"b\t"<<it->id.val()<<'\t'
            <<::lat(it2->first.val(), it->ll_lower)<<'\t'
            <<::lon(it2->first.val(), it->ll_lower)<<'\n';
	draw_item(visual, it2->first.val(), *it, 'b');
      }
      ++it2;
    }
  }
  while (it1 != s1.nodes.end())
  {
    for (std::vector< Node_Skeleton >::const_iterator it(it1->second.begin());
        it != it1->second.end(); ++it)
    {
      std::cout<<"a\t"<<it->id.val()<<'\t'
          <<::lat(it1->first.val(), it->ll_lower)<<'\t'
	  <<::lon(it1->first.val(), it->ll_lower)<<'\n';
      draw_item(visual, it1->first.val(), *it, 'a');
    }
    ++it1;
  }
  while (it2 != s2.nodes.end())
  {
    for (std::vector< Node_Skeleton >::const_iterator it(it2->second.begin());
        it != it2->second.end(); ++it)
    {
      std::cout<<"b\t"<<it->id.val()<<'\t'
          <<::lat(it2->first.val(), it->ll_lower)<<'\t'
	  <<::lon(it2->first.val(), it->ll_lower)<<'\n';
      draw_item(visual, it2->first.val(), *it, 'b');
    }
    ++it2;
  }

  for (std::vector< std::string >::const_iterator it(visual.begin()); it != visual.end(); ++it)
    std::cout<<*it<<'\n';

  rman.swap_set(set_1, s1);
  rman.swap_set(set_2, s2);
}


std::set< std::pair< Uint32_Index, Uint32_Index > > range_union(
    const std::set< std::pair< Uint32_Index, Uint32_Index > >& lhs,
    const std::set< std::pair< Uint32_Index, Uint32_Index > >& rhs);


int main(int argc, char* args[])
{
  Uint32_Index idx = ll_upper_(51.25, 7.15);
  std::cout<<std::fixed<<std::setprecision(7)<<lat(idx.val(), 0u)<<' '<<lon(idx.val(), 0u)<<'\n';
  return 0;
  
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test empty lhs and rhs:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    lhs.insert(std::make_pair(1, 2));
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test single entry in lhs:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    rhs.insert(std::make_pair(3, 5));
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test single entry in rhs:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    lhs.insert(std::make_pair(6, 8));
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    rhs.insert(std::make_pair(9, 10));
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test lhs before rhs:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    lhs.insert(std::make_pair(15, 17));
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    rhs.insert(std::make_pair(11, 14));
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test rhs before lhs:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    lhs.insert(std::make_pair(16, 18));
    lhs.insert(std::make_pair(18, 20));
    lhs.insert(std::make_pair(26, 28));
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    rhs.insert(std::make_pair(20, 22));
    rhs.insert(std::make_pair(22, 24));
    rhs.insert(std::make_pair(24, 26));
    rhs.insert(std::make_pair(28, 30));
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test tail matching next head:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    lhs.insert(std::make_pair(16, 24));
    lhs.insert(std::make_pair(32, 40));
    lhs.insert(std::make_pair(52, 68));
    lhs.insert(std::make_pair(80, 88));
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    rhs.insert(std::make_pair(20, 36));
    rhs.insert(std::make_pair(48, 56));
    rhs.insert(std::make_pair(64, 72));
    rhs.insert(std::make_pair(84, 92));
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test overlap between lhs and rhs:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > lhs;
    lhs.insert(std::make_pair(64, 96));
    lhs.insert(std::make_pair(128, 132));
    lhs.insert(std::make_pair(136, 152));
    lhs.insert(std::make_pair(156, 160));
    std::set< std::pair< Uint32_Index, Uint32_Index > > rhs;
    rhs.insert(std::make_pair(64, 68));
    rhs.insert(std::make_pair(72, 88));
    rhs.insert(std::make_pair(92, 96));
    rhs.insert(std::make_pair(128, 160));
    std::set< std::pair< Uint32_Index, Uint32_Index > > result = range_union(lhs, rhs);
    std::cout<<"Test segment contained in other segments:\n";
    for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator it = result.begin();
        it != result.end(); ++it)
      std::cout<<'\t'<<std::hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
    std::cout<<'\n';
  }
  
/*  std::vector< Aligned_Segment > segs;

  Error_Output* error_output(new Console_Output(false));
  Statement::set_error_output(error_output);

  Nonsynced_Transaction transaction(false, false, "./", "");
  Resource_Manager rman(transaction);

  Parsed_Query global_settings;

  {
    const char* attributes[] = { "into", "comp", 0 };
    Union_Statement* stmt1 = new Union_Statement(0, convert_c_pairs(attributes), global_settings);
    {
      const char* attributes[] = { "s", "51.1675", "n", "51.1725", "w", "7.134", "e", "7.138", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1725", "n", "51.1775", "w", "7.126", "e", "7.150", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1775", "n", "51.1825", "w", "7.126", "e", "7.150", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1825", "n", "51.1875", "w", "7.114", "e", "7.146", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1875", "n", "51.1925", "w", "7.110", "e", "7.142", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1925", "n", "51.1975", "w", "7.106", "e", "7.154", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1975", "n", "51.2025", "w", "7.106", "e", "7.158", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2025", "n", "51.2075", "w", "7.106", "e", "7.162", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2075", "n", "51.2125", "w", "7.046", "e", "7.050", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2075", "n", "51.2125", "w", "7.106", "e", "7.166", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2075", "n", "51.2125", "w", "7.266", "e", "7.302", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2125", "n", "51.2175", "w", "7.038", "e", "7.062", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2125", "n", "51.2175", "w", "7.102", "e", "7.206", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2125", "n", "51.2175", "w", "7.262", "e", "7.298", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2175", "n", "51.2225", "w", "7.038", "e", "7.082", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2175", "n", "51.2225", "w", "7.094", "e", "7.218", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2175", "n", "51.2225", "w", "7.254", "e", "7.294", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2225", "n", "51.2275", "w", "7.042", "e", "7.234", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2225", "n", "51.2275", "w", "7.238", "e", "7.302", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2275", "n", "51.2325", "w", "7.042", "e", "7.046", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2275", "n", "51.2325", "w", "7.050", "e", "7.310", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2325", "n", "51.2375", "w", "7.038", "e", "7.310", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2375", "n", "51.2425", "w", "7.030", "e", "7.306", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2425", "n", "51.2475", "w", "7.014", "e", "7.298", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2475", "n", "51.2525", "w", "7.038", "e", "7.282", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2475", "n", "51.2525", "w", "7.290", "e", "7.306", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2525", "n", "51.2575", "w", "7.042", "e", "7.278", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2575", "n", "51.2625", "w", "7.062", "e", "7.266", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2625", "n", "51.2675", "w", "7.070", "e", "7.270", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2675", "n", "51.2725", "w", "7.074", "e", "7.274", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2725", "n", "51.2775", "w", "7.078", "e", "7.274", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2775", "n", "51.2825", "w", "7.082", "e", "7.270", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2825", "n", "51.2875", "w", "7.098", "e", "7.258", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2825", "n", "51.2875", "w", "7.262", "e", "7.266", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2875", "n", "51.2925", "w", "7.098", "e", "7.258", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2925", "n", "51.2975", "w", "7.098", "e", "7.194", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2925", "n", "51.2975", "w", "7.198", "e", "7.262", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2975", "n", "51.3025", "w", "7.102", "e", "7.174", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.2975", "n", "51.3025", "w", "7.206", "e", "7.262", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.3025", "n", "51.3075", "w", "7.130", "e", "7.182", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.3025", "n", "51.3075", "w", "7.222", "e", "7.262", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.3075", "n", "51.3125", "w", "7.162", "e", "7.174", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.3075", "n", "51.3125", "w", "7.238", "e", "7.266", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.3125", "n", "51.3175", "w", "7.254", "e", "7.266", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  {
    const char* attributes[] = { "ref", "3600062478", 0 };
    Area_Query_Statement* stmt1 = new Area_Query_Statement(0, convert_c_pairs(attributes), global_settings);
    stmt1->execute(rman);
  }
  {
    Print_Statement* stmt1 = new Print_Statement(0, convert_c_pairs(attributes), global_settings);
    const char* attributes[] = { "mode", "ids_only", "from", "_", 0 };
    stmt1->set_attributes(attributes);
    stmt1->execute(rman);
  }
  comp_sets(rman, "comp", "_");

  {
    const char* attributes[] = { "type", "node", "into", "comp", 0 };
    Query_Statement* stmt1 = new Query_Statement(0, convert_c_pairs(attributes), global_settings);
    {
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "ref", "3600062478", 0 };
      Area_Query_Statement* stmt2 = new Area_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  comp_sets(rman, "comp", "_");

  {
    const char* attributes[] = { "type", "node", 0 };
    Query_Statement* stmt1 = new Query_Statement(0, convert_c_pairs(attributes), global_settings);
    {
      const char* attributes[] = { "k", "highway", "v", "bus_stop", 0 };
      Has_Kv_Statement* stmt2 = new Has_Kv_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    {
      const char* attributes[] = { "s", "51.1675", "n", "51.3175", "w", "7.014", "e", "7.310", 0 };
      Bbox_Query_Statement* stmt2 = new Bbox_Query_Statement(0, convert_c_pairs(attributes), global_settings);
      stmt1->add_statement(stmt2, "");
    }
    stmt1->execute(rman);
  }
  comp_sets(rman, "comp", "_");
*/

  return 0;
}
