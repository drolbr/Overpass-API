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

Aligned_Segment segment_from_ll_quad
    (uint32 from_lat, int32 from_lon, uint32 to_lat, int32 to_lon)
{
  cerr<<from_lat<<'\t'<<from_lon<<'\t'<<to_lat<<'\t'<<to_lon<<'\n';
  Aligned_Segment result;
  uint32 a_ll_upper(Node::ll_upper(from_lat, from_lon));
  uint32 b_ll_upper(Node::ll_upper(to_lat, to_lon));
  result.ll_upper_ = a_ll_upper & 0xffffff00;
  result.ll_lower_a = (uint64)Node::ll_lower(from_lat, from_lon) |
    (((uint64)a_ll_upper & 0xff)<<32);
  result.ll_lower_b = (uint64)Node::ll_lower(to_lat, to_lon) |
    (((uint64)b_ll_upper & 0xff)<<32);
  
  return result;
}

int32 proportion(int32 clow, int32 cmid, int32 cup, int32 low, int32 up)
{
  cerr<<"P\t"<<clow<<'\t'<<cmid<<'\t'<<cup<<'\t'<<low<<'\t'<<up<<'\n';
  return ((int64)(up - low))*(cmid - clow)/(cup - clow) + low;
}

static void calc_horiz_aligned_segments
    (vector< Aligned_Segment >& aligned_segments,
     uint32 from_lat, uint32 from_lon, uint32 to_lat, uint32 to_lon)
{
  if ((from_lat & 0xfff00000) == (to_lat & 0xfff00000))
  {
    aligned_segments.push_back(segment_from_ll_quad
        (from_lat, from_lon, to_lat, to_lon));
  }
  else if (from_lat < to_lat)
  {
    uint32 split_lat((from_lat & 0xfff00000) + 0x100000);
    cerr<<"HA\t"<<from_lat<<'\t'<<split_lat<<'\t'<<to_lat<<'\n';
    aligned_segments.push_back(segment_from_ll_quad
        (from_lat, from_lon, split_lat - 1,
	 proportion(from_lat, split_lat - 1, to_lat, from_lon, to_lon)));
    for (; split_lat < (to_lat & 0xfff00000); split_lat += 0x100000)
    {
      aligned_segments.push_back(segment_from_ll_quad
        (split_lat, proportion(from_lat, split_lat, to_lat, from_lon, to_lon),
	 split_lat + 0xfffff,
	 proportion(from_lat, split_lat + 0xfffff, to_lat, from_lon, to_lon)));
    }
    aligned_segments.push_back(segment_from_ll_quad
        (split_lat, proportion(from_lat, split_lat, to_lat, from_lon, to_lon),
	 to_lat, to_lon));
  }
  else
  {
    uint32 split_lat((to_lat & 0xfff00000) + 0x100000);
    cerr<<"HB\t"<<from_lat<<'\t'<<split_lat<<'\t'<<to_lat<<'\n';
    aligned_segments.push_back(segment_from_ll_quad
        (to_lat, to_lon, split_lat - 1,
         proportion(to_lat, split_lat - 1, from_lat, to_lon, from_lon)));
    for (; split_lat < (to_lat & 0xfff00000); split_lat += 0x100000)
    {
      aligned_segments.push_back(segment_from_ll_quad
          (split_lat, proportion(to_lat, split_lat, from_lat, to_lon, from_lon),
           split_lat + 0xfffff,
	   proportion(to_lat, split_lat + 0xfffff, from_lat, to_lon, from_lon)));
    }
    aligned_segments.push_back(segment_from_ll_quad
        (split_lat, proportion(to_lat, split_lat, from_lat, to_lon, from_lon),
         from_lat, from_lon));
  }
}

void calc_vert_aligned_segments
    (vector< Aligned_Segment >& aligned_segments,
     uint32 from_lat, uint32 from_lon, uint32 to_lat, uint32 to_lon)
{
  if ((from_lon & 0xfff00000) == (to_lon & 0xfff00000))
  {
    calc_horiz_aligned_segments
        (aligned_segments, from_lat, from_lon, to_lat, to_lon);
    return;
  }
  int32 split_lon((from_lon & 0xfff00000) + 0x100000);
  calc_horiz_aligned_segments
      (aligned_segments, from_lat, from_lon,
       proportion(from_lon, split_lon - 1, to_lon, from_lat, to_lat), split_lon - 1);
  for (; split_lon < (to_lon & 0xfff00000); split_lon += 0x100000)
     calc_horiz_aligned_segments
         (aligned_segments,
	  proportion(from_lon, split_lon, to_lon, from_lat, to_lat), split_lon,
	  proportion(from_lon, split_lon + 0xfffff, to_lon, from_lat, to_lat),
          split_lon + 0xfffff);
  calc_horiz_aligned_segments
      (aligned_segments,
       proportion(from_lon, split_lon, to_lon, from_lat, to_lat), split_lon,
       to_lat, to_lon);
}

int main(int argc, char* args[])
{
  vector< Aligned_Segment > segs;
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  calc_vert_aligned_segments
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
  
  Node::calc_aligned_segments
    (segs,
     ((uint64)Node::ll_upper(51.2, 7.16)<<32) | Node::ll_lower(51.2, 7.16),
     ((uint64)Node::ll_upper(51.3, 7.15)<<32) | Node::ll_lower(51.3, 7.15));
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
  
  return 0;
  
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
