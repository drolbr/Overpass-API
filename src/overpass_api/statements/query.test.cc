#include <iostream>
#include <sstream>
#include "../../template_db/block_backend.h"
#include "../core/settings.h"
#include "around.h"
#include "bbox_query.h"
#include "id_query.h"
#include "item.h"
#include "query.h"
#include "print.h"

using namespace std;

template < class TStatement >
class SProxy
{
  public:
    SProxy() : stmt_(0) {}
    
    SProxy& operator()(const string& s)
    {
      attributes.push_back(s);
      return *this;
    }
    
    TStatement& stmt()
    {
      c_attributes.clear();
      for (vector< string >::const_iterator it = attributes.begin();
          it != attributes.end(); ++it)
	c_attributes.push_back(it->c_str());
      c_attributes.push_back(0);
      stmt_.set_attributes(&c_attributes[0]);
      return stmt_;
    }
    
  private:
    TStatement stmt_;
    vector< string > attributes;
    vector< const char* > c_attributes;
};

void perform_print(Resource_Manager& rman, string from = "_")
{
  SProxy< Print_Statement >()("order")("id")("from")(from).stmt().execute(rman);
}

void perform_query(string type, string key, string value, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key)("v")(value).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key)("v")(value).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("b");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set")("a").stmt(), "");
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["b"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["b"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["b"].relations))
    {
      cout<<"Sets \"_\" and \"b\" differ:\n";
      perform_print(rman, "b");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query
    (string type, string key1, string value1, string key2, string value2,
     string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k")(key2)("v")(value2).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("b");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set")("a").stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k")(key2)("v")(value2).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["b"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["b"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["b"].relations))
    {
      cout<<"Sets \"_\" and \"b\" differ:\n";
      perform_print(rman, "b");
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("c");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k")(key2)("v")(value2).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("d");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set")("a").stmt(), "");
      SProxy< Item_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("set")("c").stmt(), "");
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["d"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["d"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["d"].relations))
    {
      cout<<"Sets \"_\" and \"d\" differ:\n";
      perform_print(rman, "d");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query
    (string type, string key1, string value1, string key2, string value2,
     string key3, string value3, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k")(key2)("v")(value2).stmt(), "");
      SProxy< Has_Kv_Statement > stmt4;
      stmt1.stmt().add_statement(&stmt4("k")(key3)("v")(value3).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("b");
      SProxy< Has_Kv_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("k")(key2)("v")(value2).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("c");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set")("a").stmt(), "");
      SProxy< Item_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("set")("b").stmt(), "");
      SProxy< Has_Kv_Statement > stmt4;
      stmt1.stmt().add_statement(&stmt4("k")(key3)("v")(value3).stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["c"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["c"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["c"].relations))
    {
      cout<<"Sets \"_\" and \"c\" differ:\n";
      perform_print(rman, "c");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query_with_around
    (string type, string key1, string value1, string db_dir, uint pattern_size)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1);
      SProxy< Id_Query_Statement >()("type")(type)("ref")(buf.str()).stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type);
      SProxy< Has_Kv_Statement > stmt2;
      if (value1 != "")
        stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      else
	stmt1.stmt().add_statement(&stmt2("k")(key1).stmt(), "");
      SProxy< Around_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("radius")("200.1").stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      ostringstream buf;
      buf<<(2*pattern_size*pattern_size + 1);
      SProxy< Id_Query_Statement >()("type")(type)("ref")(buf.str())("into")("a")
          .stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("b");
      SProxy< Has_Kv_Statement > stmt2;
      if (value1 != "")
        stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      else
	stmt1.stmt().add_statement(&stmt2("k")(key1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("c");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set")("b").stmt(), "");
      SProxy< Around_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("radius")("200.1")("from")("a").stmt(), "");      
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["c"].nodes) ||
        (rman.sets()["_"].ways != rman.sets()["c"].ways) ||
        (rman.sets()["_"].relations != rman.sets()["c"].relations))
    {
      cout<<"Sets \"_\" and \"c\" differ:\n";
      perform_print(rman, "c");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void perform_query_with_bbox
    (string type, string key1, string value1,
     string south, string north, string west, string east, string db_dir)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, db_dir, "");
    Resource_Manager rman(transaction);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type);
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      SProxy< Bbox_Query_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("n")(north)("s")(south)("e")(east)("w")(west).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    perform_print(rman);
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("a");
      SProxy< Has_Kv_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("k")(key1)("v")(value1).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    {
      SProxy< Query_Statement > stmt1;
      stmt1("type")(type)("into")("b");
      SProxy< Item_Statement > stmt2;
      stmt1.stmt().add_statement(&stmt2("set")("a").stmt(), "");
      SProxy< Bbox_Query_Statement > stmt3;
      stmt1.stmt().add_statement(&stmt3("n")(north)("s")(south)("e")(east)("w")(west).stmt(), "");
      stmt1.stmt().execute(rman);
    }
    if ((rman.sets()["_"].nodes != rman.sets()["b"].nodes) ||
      (rman.sets()["_"].ways != rman.sets()["b"].ways) ||
      (rman.sets()["_"].relations != rman.sets()["b"].relations))
    {
      cout<<"Sets \"_\" and \"b\" differ:\n";
      perform_print(rman, "b");
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

int main(int argc, char* args[])
{
  if (argc < 4)
  {
    cout<<"Usage: "<<args[0]<<" test_to_execute pattern_size db_dir\n";
    return 0;
  }
  string test_to_execute = args[1];
  uint pattern_size = 0;
  pattern_size = atoi(args[2]);
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  // Test queries for nodes.
  if ((test_to_execute == "") || (test_to_execute == "1"))
    // Test a key and value which appears only locally
    perform_query("node", "node_key_11", "node_value_2", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    // Test a key and value which appears almost everywhere
    perform_query("node", "node_key_5", "node_value_5", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    // Test a key only which has multiple values
    perform_query("node", "node_key_11", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "4"))
    // Test a key only which has only one value
    perform_query("node", "node_key_15", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "5"))
    // Test a key only which doesn't appear at all
    perform_query("node", "nowhere", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "6"))
    // Test a key intersected with a small key and value pair
    perform_query("node", "node_key_7", "", "node_key_11", "node_value_8", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "7"))
    // Test a key intersected with a large key and value pair
    perform_query("node", "node_key_7", "", "node_key_15", "node_value_15", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "8"))
    // Test a bbox combined with a local key-value pair
    perform_query_with_bbox("node", "node_key_11", "node_value_2",
			    "51.0", "51.2", "7.0", "8.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "9"))
    // Test a bbox combined with a global key-value pair
    perform_query_with_bbox("node", "node_key_5", "node_value_5",
			    "-10.0", "-1.0", "-15.0", "-3.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "10"))
    // Test a bbox combined with a global key-value pair
    perform_query_with_bbox("node", "node_key_7", "",
			    "-10.0", "-1.0", "-15.0", "-3.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "11"))
    // Test three key-values intersected
    perform_query("node", "node_key_5", "node_value_5", "node_key_7", "node_value_0",
		  "node_key_15", "node_value_15", args[3]);
		  
  // Test queries for ways.
  if ((test_to_execute == "") || (test_to_execute == "12"))
    // Test a key and value which appears only locally
    perform_query("way", "way_key_11", "way_value_2", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "13"))
    // Test a key and value which appears almost everywhere
    perform_query("way", "way_key_5", "way_value_5", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "14"))
    // Test a key only which has multiple values
    perform_query("way", "way_key_11", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "15"))
    // Test a key only which has only one value
    perform_query("way", "way_key_15", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "16"))
    // Test a key only which doesn't appear at all
    perform_query("way", "nowhere", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "17"))
    // Test a key intersected with a small key and value pair
    perform_query("way", "way_key_7", "", "way_key_11", "way_value_8", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "18"))
    // Test a key intersected with a large key and value pair
    perform_query("way", "way_key_7", "", "way_key_15", "way_value_15", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "19"))
    // Test three key-values intersected
    perform_query("way", "way_key_5", "way_value_5", "way_key_7", "way_value_0",
		  "way_key_15", "way_value_15", args[3]);

  // Test queries for relations.
  if ((test_to_execute == "") || (test_to_execute == "20"))
    // Test a key and value which appears only locally
    perform_query("relation", "relation_key_11", "relation_value_2", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "21"))
    // Test a key and value which appears almost everywhere
    perform_query("relation", "relation_key_2/4", "relation_value_1", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "22"))
    // Test a key only which has multiple values
    perform_query("relation", "relation_key_2/4", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "23"))
    // Test a key only which has only one value
    perform_query("relation", "relation_key_5", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "24"))
    // Test a key only which doesn't appear at all
    perform_query("relation", "nowhere", "", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "25"))
    // Test two key-values intersected. This tests also whether
    // relations with index zero appear in the results.
    perform_query("relation", "relation_key_2/4", "relation_value_0",
		  "relation_key_5", "relation_value_5", args[3]);

  if ((test_to_execute == "") || (test_to_execute == "26"))
    // Test an around combined with a local key-value pair
    perform_query_with_around("node", "node_key_11", "", args[3], pattern_size);
  if ((test_to_execute == "") || (test_to_execute == "27"))
    // Test an around combined with a global key-value pair
    perform_query_with_around("node", "node_key_7", "node_value_1", args[3], pattern_size);

  if ((test_to_execute == "") || (test_to_execute == "28"))
    // Test a bbox combined with a global key-value pair, yielding diagonal ways.
    perform_query_with_bbox("way", "way_key_5", "way_value_5",
			    "12.5", "35.0", "-15.0", "45.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "29"))
    // Test a bbox combined with a global key-value pair, yielding horizontal and vertical ways.
    perform_query_with_bbox("way", "way_key_5", "way_value_5",
			    "57.5", "80.0", "75.0", "105.0", args[3]);

  if ((test_to_execute == "") || (test_to_execute == "30"))
    // Test a bbox combined with a global key-value pair, yielding diagonal ways.
    perform_query_with_bbox("relation", "relation_key_2/4", "",
			    "12.5", "35.0", "-15.0", "45.0", args[3]);
  if ((test_to_execute == "") || (test_to_execute == "31"))
    // Test a bbox combined with a global key-value pair, yielding horizontal and vertical ways.
    perform_query_with_bbox("relation", "relation_key_2/4", "",
			    "57.5", "80.0", "75.0", "105.0", args[3]);

  cout<<"</osm>\n";
  return 0;
}
