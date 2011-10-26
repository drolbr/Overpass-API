#include "index_computations.h"

#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

void cout_ranges(const set< pair< Uint31_Index, Uint31_Index > >& result)
{
  cout<<"0x__80: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0xff) == 0x80)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x__40: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x7f) == 0x40)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x__20: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x3f) == 0x20)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x__10: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x1f) == 0x10)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x___8: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0xf) == 0x8)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x___4: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x7) == 0x4)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x___2: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x3) == 0x2)
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"0x___1: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && (it->first.val() & 0x1))
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<'\n';
  
  cout<<"plain: ";
  for (set< pair< Uint31_Index, Uint31_Index > >::const_iterator it = result.begin();
      it != result.end(); ++it)
  {
    if (!(it->first.val() & 0x80000000))
      cout<<hex<<it->first.val()<<' '<<it->second.val()<<' ';
  }
  cout<<"\n\n";
}

int main(int argc, char* args[])
{
  if (argc < 2)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute\n";
    return 0;
  }
  string test_to_execute = args[1];

  if ((test_to_execute == "1") || (test_to_execute == ""))
  {
    cout<<"Test ll_upper:\n";
  
    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
      cout<<hex<<ll_upper(lat, 0)<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
      cout<<hex<<ll_upper(lat, 0)<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
      cout<<hex<<ll_upper(lat, 0)<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
      cout<<hex<<ll_upper(lat, 0)<<' ';
    cout<<'\n';
  
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
      cout<<hex<<ll_upper(0, lon)<<' ';
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
      cout<<hex<<ll_upper(0, lon)<<' ';
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
      cout<<hex<<ll_upper(0, lon)<<' ';
    cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
      cout<<hex<<ll_upper(0, lon)<<' ';
    cout<<'\n';
  }

  if ((test_to_execute == "2") || (test_to_execute == ""))
  {
    cout<<"\nTest upper_ilat:\n";
  
    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
      cout<<hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
      cout<<hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
      cout<<hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
      cout<<hex<<upper_ilat(ll_upper(lat, 0))<<' ';
    cout<<'\n';
  
    cout<<"\nTest upper_ilon:\n";
  
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
      cout<<hex<<upper_ilon(ll_upper(0, lon))<<' ';
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
      cout<<hex<<upper_ilon(ll_upper(0, lon))<<' ';
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
      cout<<hex<<upper_ilon(ll_upper(0, lon))<<' ';
    cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
      cout<<hex<<upper_ilon(ll_upper(0, lon))<<' ';
    cout<<'\n';
  }
  
  if ((test_to_execute == "3") || (test_to_execute == ""))
  {
    cout<<"\nTest calc_index with 1 entry:\n";

    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000))<<' ';
    cout<<'\n';
  
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
      cout<<hex<<calc_index(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000))<<' ';
    cout<<'\n';
    
    cout<<"\nTest calc_index with 2 entries:\n";
    
    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    
    cout<<"\nTest calc_index with already cumulated entries:\n";
    
    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      cout<<hex<<calc_index(vector< uint32 >(1, calc_index(idxs)))<<' ';
    }
    cout<<'\n';

    cout<<"\nTest calc_index with an already cumulated and another entry:\n";
    
    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(lat, 0) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
    for (int32 lon = 0x90000000; lon > (int32)0x80000000; lon += 0x10000000)
    {
      vector< uint32 > idxs(1, 0x40000000);
      idxs.push_back(ll_upper(0, lon) ^ 0x40000000);
      idxs[1] = calc_index(idxs);
      cout<<hex<<calc_index(idxs)<<' ';
    }
    cout<<'\n';
  }
  
  if ((test_to_execute == "4") || (test_to_execute == ""))
  {
    cout<<"\nTest calc_node_children:\n";
    
    {
      vector< uint32 > idxs(2, 0x3f3f3f3f);
      vector< uint32 > result = calc_node_children(idxs);
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0x40848400));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0848401));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0848402));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0848404));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    // Disabled due to their size.    
/*    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0848008));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0840010));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0800020));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_node_children(vector< uint32 >(1, 0xc0000040));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }*/
  }

  if ((test_to_execute == "5") || (test_to_execute == ""))
  {
    cout<<"\nTest calc_children:\n";
    
    {
      vector< uint32 > idxs(2, 0x3f3f3f3f);
      vector< uint32 > result = calc_children(idxs);
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_children(vector< uint32 >(1, 0x40848400));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_children(vector< uint32 >(1, 0xc0848401));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_children(vector< uint32 >(1, 0xc0848402));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    {
      vector< uint32 > result = calc_children(vector< uint32 >(1, 0xc0848404));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
  }

  if ((test_to_execute == "6") || (test_to_execute == ""))
  {
    cout<<"\nTest calc_parents:\n";
        
    for (uint32 lat = 0; lat <= 0x100000; lat += 0x10000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x1000000; lat += 0x100000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat <= 0x10000000; lat += 0x1000000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    for (uint32 lat = 0; lat < 0x80000000; lat += 0x10000000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(lat, 0) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      vector< uint32 > result = calc_parents(vector< uint32 >(1, ll_upper(0, lon) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
    for (uint32 lon = 0; (uint32)lon < (uint32)0x80000000; lon += 0x10000000)
    {
      cout<<hex<<lon<<' ';
      vector< uint32 > result = calc_parents(vector< uint32 >
          (1, ll_upper(0, (int32)lon) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    for (uint32 lon = 0x80000000; (uint32)lon != (uint32)0; lon += 0x10000000)
    {
      cout<<hex<<lon<<' ';
      vector< uint32 > result = calc_parents(vector< uint32 >
      (1, ll_upper(0, (int32)lon) ^ 0x40000000));
      for (vector< uint32 >::const_iterator it = result.begin(); it != result.end(); ++it)
	cout<<hex<<*it<<' ';
      cout<<'\n';
    }
    cout<<'\n';
  }    
  
  if ((test_to_execute == "7") || (test_to_execute == ""))
  {
    cout<<"\nTest calc_parents(ranges):\n";
    
    for (uint32 lat = 0x40000000; lat <= 0x40100000; lat += 0x10000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    for (uint32 lat = 0x40000000; lat <= 0x41000000; lat += 0x100000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    for (uint32 lat = 0x40000000; lat <= 0x50000000; lat += 0x1000000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    for (uint32 lat = 0x40000000; lat < 0x80000000; lat += 0x10000000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(lat, 0) ^ 0x40000000, (ll_upper(lat, 0) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    
    for (int32 lon = 0; lon <= 0x100000; lon += 0x10000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(0x40000000, lon) ^ 0x40000000,
			     (ll_upper(0x40000000, lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x1000000; lon += 0x100000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(0x40000000, lon) ^ 0x40000000,
			     (ll_upper(0x40000000, lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    for (int32 lon = 0; lon <= 0x10000000; lon += 0x1000000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair(ll_upper(0x40000000, lon) ^ 0x40000000,
			     (ll_upper(0x40000000, lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    for (uint32 lon = 0; (uint32)lon < (uint32)0x80000000; lon += 0x10000000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40000000, lon) ^ 0x40000000,
	   (ll_upper(0x40000000, (int32)lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    for (uint32 lon = 0x80000000; (uint32)lon != (uint32)0; lon += 0x10000000)
    {
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40000000, lon) ^ 0x40000000,
	   (ll_upper(0x40000000, (int32)lon) ^ 0x40000000) + 1));
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';    
    {
      cout<<"Size 1\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40010000, 0) ^ 0x40000000, (ll_upper(0x40010000, 0) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 2\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40020000, 0) ^ 0x40000000, (ll_upper(0x40030000, 0x10000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 3\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40040000, 0) ^ 0x40000000, (ll_upper(0x40070000, 0x30000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 4\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40080000, 0) ^ 0x40000000, (ll_upper(0x400f0000, 0x70000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 5\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40100000, 0) ^ 0x40000000, (ll_upper(0x401f0000, 0xf0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 6\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40200000, 0) ^ 0x40000000, (ll_upper(0x403f0000, 0x1f0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 7\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40400000, 0) ^ 0x40000000, (ll_upper(0x407f0000, 0x3f0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 8\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x40800000, 0) ^ 0x40000000, (ll_upper(0x40ff0000, 0x7f0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 9\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x41000000, 0) ^ 0x40000000, (ll_upper(0x41ff0000, 0xff0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 10\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x42000000, 0) ^ 0x40000000, (ll_upper(0x43ff0000, 0x1ff0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 11\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x44000000, 0) ^ 0x40000000, (ll_upper(0x47ff0000, 0x3ff0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 12\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x48000000, 0) ^ 0x40000000, (ll_upper(0x4fff0000, 0x7ff0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 13\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x50000000, 0) ^ 0x40000000, (ll_upper(0x5fff0000, 0xfff0000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Two Size 1\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x3fff0000, 0x7fff0000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 1 + Size 2\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x3fff0000, 0x7fff0000) ^ 0x40000000,
	   (ll_upper(0x40010000, 0x10000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 1 + Size 2 + Size 1\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x3fff0000, 0x7fff0000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0x20000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 3 + Size 2 + Size 1\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x3ffc0000, 0x7ffc0000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0x20000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
    {
      cout<<"Size 4 + Size 2 + Size 1\n";
      set< pair< Uint32_Index, Uint32_Index > > input;
      input.insert(make_pair
          (ll_upper(0x3ff80000, 0x7ff80000) ^ 0x40000000,
	   (ll_upper(0x40000000, 0x20000) ^ 0x40000000) + 1));      
      cout_ranges(calc_parents(input));
    }
    cout<<'\n';
  }    
  
  return 0;
}
