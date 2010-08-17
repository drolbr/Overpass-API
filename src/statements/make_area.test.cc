#include <iostream>
#include <sstream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "../frontend/console_output.h"
#include "bbox_query.h"
#include "coord_query.h"
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
  
  //tests for functions to calculate tile clipping
/*  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 71500000, 1422100000, 71510000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 71500000, 1423000000, 71510000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 71500000, 1452000000, 71510000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1423000000, 71500000, 1422000000, 71510000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 71510000, 1423000000, 71500000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 71500000, 1423000000, 72500000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 72500000, 1423000000, 71500000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 71500000, 1423000000, 81500000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_vert_aligned_segments
    (segs,
     1422000000, 81500000, 1423000000, 71500000);
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
        <<Node::lon(it->ll_upper_ | (it->ll_lower_a>>32), it->ll_lower_a & 0xffffffff)<<'\t'
	<<Node::lat(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\t'
	<<Node::lon(it->ll_upper_ | (it->ll_lower_b>>32), it->ll_lower_b & 0xffffffff)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_aligned_segments
    (segs,
     ((uint64)Node::ll_upper(51.2, 7.16)<<32) | Node::ll_lower(51.2, 7.16),
     ((uint64)Node::ll_upper(51.3, 7.15)<<32) | Node::ll_lower(51.3, 7.15));
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_, 0)<<'\t'
        <<Node::lon(it->ll_upper_, 0)<<'\t'
	<<Node::lat(it->ll_upper_ | it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lon(it->ll_upper_ | it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lat(it->ll_upper_ | it->ll_lower_b>>32, it->ll_lower_b)<<'\t'
	<<Node::lon(it->ll_upper_ | it->ll_lower_b>>32, it->ll_lower_b)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_aligned_segments
    (segs,
     ((uint64)Node::ll_upper(51.0, -179.0)<<32) | Node::ll_lower(51.0, -179.0),
     ((uint64)Node::ll_upper(52.0, 179.0)<<32) | Node::ll_lower(52.0, 179.0));
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_, 0)<<'\t'
        <<Node::lon(it->ll_upper_, 0)<<'\t'
	<<Node::lat(it->ll_upper_ | it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lon(it->ll_upper_ | it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lat(it->ll_upper_ | it->ll_lower_b>>32, it->ll_lower_b)<<'\t'
	<<Node::lon(it->ll_upper_ | it->ll_lower_b>>32, it->ll_lower_b)<<'\n';
  cout<<'\n';
  segs.clear();
  
  Area::calc_aligned_segments
    (segs,
     ((uint64)Node::ll_upper(51.0, 179.0)<<32) | Node::ll_lower(51.0, 179.0),
     ((uint64)Node::ll_upper(52.0, -179.0)<<32) | Node::ll_lower(52.0, -179.0));
  for (vector< Aligned_Segment >::const_iterator it(segs.begin());
      it != segs.end(); ++it)
    cout<<Node::lat(it->ll_upper_, 0)<<'\t'
        <<Node::lon(it->ll_upper_, 0)<<'\t'
	<<Node::lat(it->ll_upper_ | it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lon(it->ll_upper_ | it->ll_lower_a>>32, it->ll_lower_a)<<'\t'
	<<Node::lat(it->ll_upper_ | it->ll_lower_b>>32, it->ll_lower_b)<<'\t'
	<<Node::lon(it->ll_upper_ | it->ll_lower_b>>32, it->ll_lower_b)<<'\n';
  cout<<'\n';
  segs.clear();*/
  
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
  {
    Coord_Query_Statement* stmt1 = new Coord_Query_Statement(0);
    const char* attributes[] = { "lat", "51.25", "lon", "7.15", 0 };
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

  cout<<"Query to create areas:\n";
  {
    Query_Statement* stmt2 = new Query_Statement(0);
    const char* attributes[] = { "type", "relation", 0 };
    stmt2->set_attributes(attributes);
    {
      Has_Key_Value_Statement* stmt3 = new Has_Key_Value_Statement(0);
      const char* attributes[] = { "k", "admin_level", 0 };
      stmt3->set_attributes(attributes);
      stmt2->add_statement(stmt3, "");
    }
    stmt2->execute(sets);
  }
  {
    Foreach_Statement* stmt3 = new Foreach_Statement(0);
    const char* attributes[] = { "into", "rels", 0 };
    stmt3->set_attributes(attributes);
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
      stmt3->add_statement(stmt2, "");
    }
    {
      Make_Area_Statement* stmt1 = new Make_Area_Statement(0);
      const char* attributes[] = { "pivot", "rels", 0 };
      stmt1->set_attributes(attributes);
      stmt3->add_statement(stmt1, "");
    }
    {
      Print_Statement* stmt1 = new Print_Statement(0);
      const char* attributes[] = { "mode", "ids_only", "from", "rels", 0 };
      stmt1->set_attributes(attributes);
      stmt3->add_statement(stmt1, "");
    }
    stmt3->execute(sets);
  }
  
/*  cout<<Coord_Query_Statement::check_segment(0, 0, 0, 0, 0, 0)<<'\n';
  cout<<Coord_Query_Statement::check_segment(1, 1, 1, 1, 1, 1)<<'\n';
  cout<<Coord_Query_Statement::check_segment(1, -1, 1, -1, 1, -1)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(5, 5, 15, 5, 10, 5)<<'\n';
  cout<<Coord_Query_Statement::check_segment(5, -5, 15, -5, 10, -5)<<'\n';
  cout<<'\n';
 
  cout<<Coord_Query_Statement::check_segment(0, 0, 0, 10, 0, 5)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -5, 0, 5, 0, 0)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, 0, 0, 10, 5, 5)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -5, 0, 5, 5, 0)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(0, 5, 10, 15, 5, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -15, 10, -5, 5, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 15, 0, 5, 5, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -5, 0, -15, 5, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 5, 0, 15, 5, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, 15, 10, 5, 5, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -15, 0, -5, 5, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -5, 10, -15, 5, -10)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(0, 5, 10, 15, 15, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -15, 10, -5, 15, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 15, 0, 5, 15, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -5, 0, -15, 15, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 5, 0, 15, 15, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, 15, 10, 5, 15, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -15, 0, -5, 15, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -5, 10, -15, 15, -10)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(10, 0, 10, 0, 0, 0)<<'\n';
  cout<<Coord_Query_Statement::check_segment(11, 1, 11, 1, 1, 1)<<'\n';
  cout<<Coord_Query_Statement::check_segment(11, -1, 11, -1, 1, -1)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(5, 5, 15, 5, 0, 5)<<'\n';
  cout<<Coord_Query_Statement::check_segment(5, -5, 15, -5, 0, -5)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(10, 0, 10, 10, 0, 5)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -5, 10, 5, 0, 0)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 0, 10, 10, 5, 5)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -5, 10, 5, 5, 0)<<'\n';
  cout<<'\n';
  
  cout<<Coord_Query_Statement::check_segment(0, 5, 10, 15, 0, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -15, 10, -5, 0, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 15, 0, 5, 0, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -5, 0, -15, 0, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, 5, 0, 15, 0, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, 15, 10, 5, 0, 10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(10, -15, 0, -5, 0, -10)<<'\n';
  cout<<Coord_Query_Statement::check_segment(0, -5, 10, -15, 0, -10)<<'\n';
  cout<<'\n';*/
  
/*  set< Uint31_Index > req;
  req.insert(Uint31_Index(Node::ll_upper(51.25, 7.15) & 0xffffff00));
  
  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (*de_osm3s_file_ids::AREA_BLOCKS, false);
  for (Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      it(area_blocks_db.discrete_begin(req.begin(), req.end()));
      !(it == area_blocks_db.discrete_end()); ++it)
  {
    cout<<"0x"<<hex<<it.index().val()<<' '<<dec<<it.object().id<<": ";
    for (uint i(0); i < it.object().coors.size(); ++i)
      cout<<Coord_Query_Statement::shifted_lat(it.index().val(), it.object().coors[i])
          <<' '<<Coord_Query_Statement::lon_(it.index().val(), it.object().coors[i])
	  <<", ";
    cout<<'\n';
  }*/

  for (uint j(51400); j >= 51100; j -= 5) 
  {
    if (j%100 == 95)
      cout<<'\n';
    for (uint i(6900); i <= 7500; i += 4)
    {
      if (i%100 == 0)
	cout<<' ';
      
      ostringstream temp;
      char lat[40];
      temp<<((double)j/1000);
      strcpy(lat, temp.str().c_str());
      temp.str("");
      char lon[40];
      temp<<((double)i/1000);
      strcpy(lon, temp.str().c_str());
      
      Coord_Query_Statement* stmt1 = new Coord_Query_Statement(0);
      const char* attributes[] = { "lat", "51.25", "lon", "7.15", 0 };
      attributes[1] = (const char*)lat;
      attributes[3] = (const char*)lon;
      stmt1->set_attributes(attributes);
      stmt1->execute(sets);
    }
    cout<<'\n';
  }

  return 0;
}
