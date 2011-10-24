#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "meta_collector.h"
#include "print.h"

using namespace std;

const unsigned int PRINT_IDS = 1;
const unsigned int PRINT_COORDS = 2;
const unsigned int PRINT_NDS = 4;
const unsigned int PRINT_MEMBERS = 8;
const unsigned int PRINT_TAGS = 16;
const unsigned int PRINT_VERSION = 32;
const unsigned int PRINT_META = 64;

const unsigned int ORDER_BY_ID = 1;
const unsigned int ORDER_BY_QUADTILE = 2;

const unsigned int NODE_FLUSH_SIZE = 1024*1024;
const unsigned int WAY_FLUSH_SIZE = 512*1024;
const unsigned int RELATION_FLUSH_SIZE = 512*1024;
const unsigned int AREA_FLUSH_SIZE = 64*1024;

const char* MEMBER_TYPE[] = { 0, "node", "way", "relation" };

void Print_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["mode"] = "body";
  attributes["order"] = "id";
  attributes["limit"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  
  if (attributes["mode"] == "ids_only")
    mode = PRINT_IDS;
  else if (attributes["mode"] == "skeleton")
    mode = PRINT_IDS|PRINT_COORDS|PRINT_NDS|PRINT_MEMBERS;
  else if (attributes["mode"] == "body")
    mode = PRINT_IDS|PRINT_COORDS|PRINT_NDS|PRINT_MEMBERS|PRINT_TAGS;
  else if (attributes["mode"] == "meta")
    mode = PRINT_IDS|PRINT_COORDS|PRINT_NDS|PRINT_MEMBERS|PRINT_TAGS|PRINT_VERSION|PRINT_META;
  else
  {
    mode = 0;
    ostringstream temp;
    temp<<"For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\", \"body\", or \"meta\".";
    add_static_error(temp.str());
  }
  if (attributes["order"] == "id")
    order = ORDER_BY_ID;
  else if (attributes["order"] == "quadtile")
    order = ORDER_BY_QUADTILE;
  else
  {
    order = 0;
    ostringstream temp;
    temp<<"For the attribute \"order\" of the element \"print\""
        <<" the only allowed values are \"id\" or \"quadtile\".";
    add_static_error(temp.str());
  }
  if (attributes["limit"] != "")
    limit = atoll(attributes["limit"].c_str());
}

void Print_Statement::forecast()
{
/*  const Set_Forecast& sf_in(declare_read_set(input));
    
  if (mode == PRINT_IDS_ONLY)
    declare_used_time(1 + sf_in.node_count/100 + sf_in.way_count/100 + sf_in.relation_count/100
	+ sf_in.area_count/20);
  else if (mode == PRINT_SKELETON)
    declare_used_time(1 + sf_in.node_count/50 + sf_in.way_count/5 + sf_in.relation_count/5
	+ sf_in.area_count/5);
  else if (mode == PRINT_BODY)
    declare_used_time(10 + sf_in.node_count + sf_in.way_count + sf_in.relation_count
	+ sf_in.area_count);
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

template< class TIndex >
void formulate_range_query
  (set< pair< Tag_Index_Local, Tag_Index_Local > >& range_set,
   const set< TIndex >& coarse_indices)
{
  for (typename set< TIndex >::const_iterator
    it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->val();
    lower.key = "";
    lower.value = "";
    upper.index = it->val() + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }
}

template< class TIndex, class TObject >
void generate_ids_by_coarse
  (set< TIndex >& coarse_indices,
   map< uint32, vector< uint32 > >& ids_by_coarse,
   const map< TIndex, vector< TObject > >& items)
{
  for (typename map< TIndex, vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    coarse_indices.insert(TIndex(it->first.val() & 0x7fffff00));
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      ids_by_coarse[it->first.val() & 0x7fffff00].push_back(it2->id);
  }
}

void collect_tags
  (map< uint32, vector< pair< string, string > > >& tags_by_id,
   const Block_Backend< Tag_Index_Local, Uint32_Index >& items_db,
   Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator& tag_it,
   map< uint32, vector< uint32 > >& ids_by_coarse,
   uint32 coarse_index)
{
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if (binary_search(ids_by_coarse[coarse_index].begin(),
        ids_by_coarse[coarse_index].end(), tag_it.object().val()))
      tags_by_id[tag_it.object().val()].push_back
          (make_pair(tag_it.index().key, tag_it.index().value));
    ++tag_it;
  }
}

void collect_tags_framed
  (map< uint32, vector< pair< string, string > > >& tags_by_id,
   const Block_Backend< Tag_Index_Local, Uint32_Index >& items_db,
   Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator& tag_it,
   map< uint32, vector< uint32 > >& ids_by_coarse,
   uint32 coarse_index,
   uint32 lower_id_bound, uint32 upper_id_bound)
{
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if ((tag_it.object().val() >= lower_id_bound) &&
      (tag_it.object().val() < upper_id_bound) &&
      (binary_search(ids_by_coarse[coarse_index].begin(),
	ids_by_coarse[coarse_index].end(), tag_it.object().val())))
      tags_by_id[tag_it.object().val()].push_back
      (make_pair(tag_it.index().key, tag_it.index().value));
    ++tag_it;
  }
}

void print_meta(const OSM_Element_Metadata_Skeleton& meta,
		const map< uint32, string >& users)
{
  uint32 year = (meta.timestamp)>>26;
  uint32 month = ((meta.timestamp)>>22) & 0xf;
  uint32 day = ((meta.timestamp)>>17) & 0x1f;
  uint32 hour = ((meta.timestamp)>>12) & 0x1f;
  uint32 minute = ((meta.timestamp)>>6) & 0x3f;
  uint32 second = meta.timestamp & 0x3f;
  string timestamp("    -  -  T  :  :  Z");
  timestamp[0] = (year / 1000) % 10 + '0';
  timestamp[1] = (year / 100) % 10 + '0';
  timestamp[2] = (year / 10) % 10 + '0';
  timestamp[3] = year % 10 + '0';
  timestamp[5] = (month / 10) % 10 + '0';
  timestamp[6] = month % 10 + '0';
  timestamp[8] = (day / 10) % 10 + '0';
  timestamp[9] = day % 10 + '0';
  timestamp[11] = (hour / 10) % 10 + '0';
  timestamp[12] = hour % 10 + '0';
  timestamp[14] = (minute / 10) % 10 + '0';
  timestamp[15] = minute % 10 + '0';
  timestamp[17] = (second / 10) % 10 + '0';
  timestamp[18] = second % 10 + '0';
  cout<<" version=\""<<meta.version<<"\" timestamp=\""<<timestamp
      <<"\" changeset=\""<<meta.changeset<<"\" uid=\""<<meta.user_id<<"\"";
  map< uint32, string >::const_iterator it = users.find(meta.user_id);
  if (it != users.end())
    cout<<" user=\""<<escape_xml(it->second)<<"\"";
}

void print_item(uint32 ll_upper, const Node_Skeleton& skel, uint32 mode,
		Transaction& transaction,
		const vector< pair< string, string > >* tags = 0,
		const OSM_Element_Metadata_Skeleton* meta = 0,
		const map< uint32, string >* users = 0)
{
  cout<<"  <node";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if (mode & PRINT_COORDS)
    cout<<" lat=\""<<fixed<<setprecision(7)<<Node::lat(ll_upper, skel.ll_lower)
        <<"\" lon=\""<<fixed<<setprecision(7)<<Node::lon(ll_upper, skel.ll_lower)<<'\"';
  if (meta)
    print_meta(*meta, *users);
  if ((tags == 0) || (tags->empty()))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< pair< string, string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
	cout<<"    <tag k=\""<<escape_xml(it->first)
	    <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    cout<<"  </node>\n";
  }
}

void print_item(uint32 ll_upper, const Way_Skeleton& skel, uint32 mode,
		Transaction& transaction,
		const vector< pair< string, string > >* tags = 0,
		const OSM_Element_Metadata_Skeleton* meta = 0,
		const map< uint32, string >* users = 0)
{
  cout<<"  <way";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if (meta)
    print_meta(*meta, *users);
  if (((tags == 0) || (tags->empty())) && ((mode & PRINT_NDS) == 0))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    if (mode & PRINT_NDS)
    {
      for (uint i(0); i < skel.nds.size(); ++i)
	cout<<"    <nd ref=\""<<skel.nds[i]<<"\"/>\n";
    }
    if ((tags != 0) && (!tags->empty()))
    {
      for (vector< pair< string, string > >::const_iterator it(tags->begin());
          it != tags->end(); ++it)
	  cout<<"    <tag k=\""<<escape_xml(it->first)
	      <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    }
    cout<<"  </way>\n";
  }
}

void print_item(uint32 ll_upper, const Relation_Skeleton& skel, uint32 mode,
		Transaction& transaction,
		const vector< pair< string, string > >* tags = 0,
		const OSM_Element_Metadata_Skeleton* meta = 0,
		const map< uint32, string >* users = 0)
{ 
  static map< uint32, string > roles;
  if (roles.empty())
  {
    // prepare check update_members - load roles
    Block_Backend< Uint32_Index, String_Object > roles_db
        (transaction.data_index(osm_base_settings().RELATION_ROLES));
    for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
        it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
      roles[it.index().val()] = it.object().val();
  }
  
  cout<<"  <relation";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if (meta)
    print_meta(*meta, *users);
  if (((tags == 0) || (tags->empty())) && ((mode & PRINT_NDS) == 0))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    if (mode & PRINT_MEMBERS)
    {
      for (uint i(0); i < skel.members.size(); ++i)
	cout<<"    <member type=\""<<MEMBER_TYPE[skel.members[i].type]
	    <<"\" ref=\""<<skel.members[i].ref
	    <<"\" role=\""<<escape_xml(roles[skel.members[i].role])<<"\"/>\n";
    }
    if ((tags != 0) && (!tags->empty()))
    {
      for (vector< pair< string, string > >::const_iterator it(tags->begin());
          it != tags->end(); ++it)
	  cout<<"    <tag k=\""<<escape_xml(it->first)
	      <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    }
    cout<<"  </relation>\n";
  }
}

void print_item(uint32 ll_upper, const Area_Skeleton& skel, uint32 mode,
		Transaction& transaction,
		const vector< pair< string, string > >* tags = 0,
		const OSM_Element_Metadata_Skeleton* meta = 0,
		const map< uint32, string >* users = 0)
{
  cout<<"  <area";
  if (mode & PRINT_IDS)
    cout<<" id=\""<<skel.id<<'\"';
  if ((tags == 0) || (tags->empty()))
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< pair< string, string > >::const_iterator it(tags->begin());
        it != tags->end(); ++it)
      cout<<"    <tag k=\""<<escape_xml(it->first)
          <<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
    cout<<"  </area>\n";
  }
}

template< class TIndex, class TObject >
void quadtile
    (const map< TIndex, vector< TObject > >& items, uint32 mode,
     Transaction& transaction, uint32 limit, uint32& element_count)
{
  typename map< TIndex, vector< TObject > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename vector< TObject >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++element_count > limit)
	return;
      print_item(item_it->first.val(), *it2, mode, transaction);
    }
    ++item_it;
  }
};

template< class TIndex, class TObject >
void Print_Statement::tags_quadtile
    (const map< TIndex, vector< TObject > >& items,
     const File_Properties& file_prop, uint32 mode, uint32 stopwatch_account,
     Resource_Manager& rman, Transaction& transaction,
     const File_Properties* meta_file_prop, uint32& element_count)
{
  //generate set of relevant coarse indices
  set< TIndex > coarse_indices;
  map< uint32, vector< uint32 > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< TIndex, TObject > meta_printer(items, transaction, meta_file_prop);
  
  // iterate over the result
  stopwatch.stop(Stopwatch::NO_DISK);
  uint coarse_count = 0;
  Block_Backend< Tag_Index_Local, Uint32_Index > items_db
      (transaction.data_index(&file_prop));
  Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    tag_it(items_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  typename map< TIndex, vector< TObject > >::const_iterator
      item_it(items.begin());
  for (typename set< TIndex >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    if (++coarse_count >= 1024)
    {
      coarse_count = 0;
      rman.health_check(*this);
    }
    
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
    map< uint32, vector< pair< string, string > > > tags_by_id;
    collect_tags(tags_by_id, items_db, tag_it, ids_by_coarse, it->val());
    
    // print the result
    while ((item_it != items.end()) &&
        ((item_it->first.val() & 0x7fffff00) == it->val()))
    {
      for (typename vector< TObject >::const_iterator it2(item_it->second.begin());
          it2 != item_it->second.end(); ++it2)
      {
	if (++element_count > limit)
	  return;
	print_item(item_it->first.val(), *it2, mode, transaction, &(tags_by_id[it2->id]),
		   meta_printer.get(item_it->first, it2->id), &(meta_printer.users()));
      }
      ++item_it;
    }
  }
  stopwatch.add(stopwatch_account, items_db.read_count());
  stopwatch.stop(stopwatch_account);
};

template< class TComp >
struct Skeleton_Comparator_By_Id {
  bool operator() (const pair< const TComp*, uint32 >& a, 
		   const pair< const TComp*, uint32 >& b)
  {
    return (a.first->id < b.first->id);
  }
};

template< class TIndex, class TObject >
void by_id
  (const map< TIndex, vector< TObject > >& items, uint32 mode,
   Transaction& transaction, uint32 limit, uint32& element_count)
{
  // order relevant elements by id
  vector< pair< const TObject*, uint32 > > items_by_id;
  for (typename map< TIndex, vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< TObject >());
  
  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    print_item(items_by_id[i].second, *(items_by_id[i].first), mode, transaction);
  }
};

template< class TIndex, class TObject >
void collect_metadata(set< OSM_Element_Metadata_Skeleton >& metadata,
		      const map< TIndex, vector< TObject > >& items,
		      uint32 lower_id_bound, uint32 upper_id_bound,
		      Meta_Collector< TIndex, TObject>& meta_printer)
{
  for (typename map< TIndex, vector< TObject > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if ((it2->id >= lower_id_bound) && (it2->id < upper_id_bound))
      {
	const OSM_Element_Metadata_Skeleton* meta = meta_printer.get(it->first, it2->id);
	if (meta)
	  metadata.insert(*meta);
      }
    }
  }
}

template< class TIndex, class TObject >
void Print_Statement::tags_by_id
  (const map< TIndex, vector< TObject > >& items,
   const File_Properties& file_prop,
   uint32 FLUSH_SIZE, uint32 mode, uint32 stopwatch_account,
   Resource_Manager& rman, Transaction& transaction,
   const File_Properties* meta_file_prop, uint32& element_count)
{
  // order relevant elements by id
  vector< pair< const TObject*, uint32 > > items_by_id;
  for (typename map< TIndex, vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< TObject >());
  
  //generate set of relevant coarse indices
  set< TIndex > coarse_indices;
  map< uint32, vector< uint32 > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  for (typename set< TIndex >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< TIndex, TObject > meta_printer(items, transaction, meta_file_prop);
  
  // iterate over the result
  stopwatch.stop(Stopwatch::NO_DISK);
  Block_Backend< Tag_Index_Local, Uint32_Index > items_db
      (transaction.data_index(&file_prop));
  for (uint32 id_pos(0); id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    rman.health_check(*this);
    
    map< uint32, vector< pair< string, string > > > tags_by_id;
    uint32 lower_id_bound(items_by_id[id_pos].first->id);
    uint32 upper_id_bound(0);
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[id_pos + FLUSH_SIZE].first->id;
    else
      upper_id_bound = items_by_id[items_by_id.size()-1].first->id + 1;
    
    Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
        tag_it(items_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    for (typename set< TIndex >::const_iterator
        it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
      collect_tags_framed(tags_by_id, items_db, tag_it, ids_by_coarse, it->val(),
			  lower_id_bound, upper_id_bound);
    
    // collect metadata if required
    set< OSM_Element_Metadata_Skeleton > metadata;
    collect_metadata(metadata, items, lower_id_bound, upper_id_bound, meta_printer);
    meta_printer.reset();

    // print the result
    for (uint32 i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      set< OSM_Element_Metadata_Skeleton >::const_iterator meta_it
          = metadata.find(OSM_Element_Metadata_Skeleton(items_by_id[i].first->id));
      if (++element_count > limit)
	return;
      print_item(items_by_id[i].second, *(items_by_id[i].first), mode,
		 transaction, &(tags_by_id[items_by_id[i].first->id]),
		 meta_it != metadata.end() ? &*meta_it : 0, &(meta_printer.users()));
    }
  }
  stopwatch.add(stopwatch_account, items_db.read_count());
  stopwatch.stop(stopwatch_account);
};

void Print_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();
  if (rman.area_updater())
    rman.area_updater()->flush(&stopwatch);
  
  map< string, Set >::const_iterator mit(rman.sets().find(input));
  uint32 element_count = 0;
  if (mit == rman.sets().end())
    return;
  if (mode & PRINT_TAGS)
  {
    if (order == ORDER_BY_ID)
    {
      tags_by_id(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		 NODE_FLUSH_SIZE, mode, Stopwatch::NODE_TAGS_LOCAL, rman,
		 *rman.get_transaction(),
		 (mode & PRINT_META) ? meta_settings().NODES_META : 0, element_count);
      tags_by_id(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		 WAY_FLUSH_SIZE, mode, Stopwatch::WAY_TAGS_LOCAL, rman,
		 *rman.get_transaction(),
		 (mode & PRINT_META) ? meta_settings().WAYS_META : 0, element_count);
      tags_by_id(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		 RELATION_FLUSH_SIZE, mode, Stopwatch::RELATION_TAGS_LOCAL, rman,
		 *rman.get_transaction(),
		 (mode & PRINT_META) ? meta_settings().RELATIONS_META : 0, element_count);
      if (rman.get_area_transaction())
      {
	tags_by_id(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		   AREA_FLUSH_SIZE, mode, Stopwatch::AREA_TAGS_LOCAL, rman,
		   *rman.get_area_transaction(), 0, element_count);
      }
    }
    else
    {
      tags_quadtile(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		    mode, Stopwatch::NODE_TAGS_LOCAL, rman,
		    *rman.get_transaction(),
		    (mode & PRINT_META) ? meta_settings().NODES_META : 0, element_count);
      tags_quadtile(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		    mode, Stopwatch::WAY_TAGS_LOCAL, rman,
		    *rman.get_transaction(),
		    (mode & PRINT_META) ? meta_settings().WAYS_META : 0, element_count);
      tags_quadtile(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		    mode, Stopwatch::RELATION_TAGS_LOCAL, rman,
		    *rman.get_transaction(),
		    (mode & PRINT_META) ? meta_settings().RELATIONS_META : 0, element_count);
      if (rman.get_area_transaction())
      {
        tags_quadtile(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		      mode, Stopwatch::AREA_TAGS_LOCAL, rman,
		      *rman.get_area_transaction(), 0, element_count);
      }
    }
  }
  else
  {
    if (order == ORDER_BY_ID)
    {
      by_id(mit->second.nodes, mode, *rman.get_transaction(), limit, element_count);
      by_id(mit->second.ways, mode, *rman.get_transaction(), limit, element_count);
      by_id(mit->second.relations, mode, *rman.get_transaction(), limit, element_count);
      if (rman.get_area_transaction())
	by_id(mit->second.areas, mode, *rman.get_area_transaction(), limit, element_count);
    }
    else
    {
      quadtile(mit->second.nodes, mode, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.ways, mode, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.relations, mode, *rman.get_transaction(), limit, element_count);
      if (rman.get_area_transaction())
	quadtile(mit->second.areas, mode, *rman.get_area_transaction(), limit, element_count);
    }
  }
  
  stopwatch.stop(Stopwatch::NO_DISK);
  stopwatch.report(get_name());
  rman.health_check(*this);
}
