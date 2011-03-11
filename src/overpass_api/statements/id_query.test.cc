#include <iostream>
#include "../backend/block_backend.h"
#include "../core/settings.h"
#include "id_query.h"

using namespace std;

int main(int argc, char* args[])
{
  Id_Query_Statement stmt_1(1);
  Resource_Manager rman;
  
  {
    const char* attributes[] = { "type", "node", "ref", "471224123"/*"471224974"*/, 0 };
    stmt_1.set_attributes(attributes);
  }
  stmt_1.execute(rman);
  
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(rman.sets()["_"].nodes);
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it(nodes.begin());
       it != nodes.end(); ++it)
  {
    cout<<hex<<it->first.val()<<":\t";
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin());
	 it2 != it->second.end(); ++it2)
      cout<<dec<<it2->id<<' '<<hex<<it2->ll_lower<<'\t';
    cout<<'\n';
  }
  
  {
    const char* attributes[] = { "type", "way", "ref", "8237924", 0 };
    stmt_1.set_attributes(attributes);
  }
  stmt_1.execute(rman);
  
  map< Uint31_Index, vector< Way_Skeleton > >& ways(rman.sets()["_"].ways);
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator it(ways.begin());
       it != ways.end(); ++it)
  {
    cout<<hex<<it->first.val()<<":\t";
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
	 it2 != it->second.end(); ++it2)
    {
      cout<<dec<<it2->id<<" {";
      for (vector< uint32 >::const_iterator it3(it2->nds.begin());
	   it3 != it2->nds.end(); ++it3)
	cout<<*it3<<' ';
      cout<<"}\t";
    }
    cout<<'\n';
  }
  
  // prepare check update_members - load roles
  map< uint32, string > roles;
  Block_Backend< Uint32_Index, String_Object > roles_db
      (*de_osm3s_file_ids::RELATION_ROLES, true);
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
      it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
    roles[it.index().val()] = it.object().val();
  
  {
    const char* attributes[] = { "type", "relation", "ref", "163298", 0 };
    stmt_1.set_attributes(attributes);
  }
  stmt_1.execute(rman);
  
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(rman.sets()["_"].relations);
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator it(relations.begin());
       it != relations.end(); ++it)
  {
    cout<<hex<<it->first.val()<<":\t";
    for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
	 it2 != it->second.end(); ++it2)
    {
      cout<<dec<<it2->id<<" {";
      for (vector< Relation_Entry >::const_iterator it3(it2->members.begin());
	   it3 != it2->members.end(); ++it3)
	cout<<it3->type<<' '<<it3->ref<<' '<<roles[it3->role]<<", ";
      cout<<"}\t";
    }
    cout<<'\n';
  }
  
  return 0;
}
