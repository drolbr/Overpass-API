#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct InputAnalizer
{
  InputAnalizer(const string& input);
  
  string south, north, east, west;
  bool bbox_found;
  vector< pair< string, string > > key_value;
  string user;
  string uid;
  string newer;
  string timeout;
  bool meta_found;
};

InputAnalizer::InputAnalizer(const string& input_) : bbox_found(false), meta_found(false)
{
  string input = input_;
  while (!input.empty())
  {
    if (input[0] != '[')
    {
      cout<<"Error: Text before '[' found.\n";
      throw string();
    }
    if (input.substr(0, 6) == "[bbox=")
    {
      if (bbox_found)
      {
	cout<<"Error: At most one bbox allowed.\n";
	throw string();
      }
      bbox_found = true;
      input = input.substr(6);
      west = input.substr(0, input.find(','));
      input = input.substr(input.find(',')+1);
      south = input.substr(0, input.find(','));
      input = input.substr(input.find(',')+1);
      east = input.substr(0, input.find(','));
      input = input.substr(input.find(',')+1);
      north = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 7) == "[@meta]")
    {
      meta_found = true;
      input = input.substr(7);
    }
    else if (input.substr(0, 7) == "[@user=")
    {
      input = input.substr(7);
      user = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 6) == "[@uid=")
    {
      input = input.substr(6);
      uid = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 8) == "[@newer=")
    {
      input = input.substr(8);
      newer = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 10) == "[@timeout=")
    {
      input = input.substr(10);
      timeout = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else
    {
      key_value.push_back(make_pair("", ""));
      input = input.substr(1);
      if (input.find('=') != string::npos && input.find(']') != string::npos &&
	 input.find('=') < input.find(']'))
      {
        key_value.back().first = input.substr(0, input.find('='));
        input = input.substr(input.find('=')+1);
        key_value.back().second = input.substr(0, input.find(']'));
      }
      else
      {
	if (input.find(']') == string::npos)
	  cout<<"Error: Expected ']' after value.\n";
	else
	  cout<<"Error: Expected '=' after key.\n";
	throw string();
      }
      if (key_value.back().second == "*")
	key_value.back().second="";
      input = input.substr(input.find(']')+1);
    }
  }
}

void print_meta(const InputAnalizer& analizer, string prefix)
{
  if (analizer.user != "")
    cout<<prefix<<"<user name=\""<<analizer.user<<"\"/>\n";
  if (analizer.uid != "")
    cout<<prefix<<"<user uid=\""<<analizer.uid<<"\"/>\n";
  if (analizer.newer != "")
    cout<<prefix<<"<newer than=\""<<analizer.newer<<"\"/>\n";
}

void print_meta(const InputAnalizer& analizer, string prefix, string type)
{
  if (analizer.user != "")
    cout<<prefix<<"<user type=\""<<type<<"\" name=\""<<analizer.user<<"\"/>\n";
  else if (analizer.uid != "")
    cout<<prefix<<"<user type=\""<<type<<"\" uid=\""<<analizer.uid<<"\"/>\n";
  if (analizer.newer != "")
    cout<<prefix<<"<query type=\""<<type<<"\">\n"
                  "  <item/>\n"
		  "  <newer than=\""<<analizer.newer<<"\"/>\n"
		  "</query>\n";
}

void print_bbox(const InputAnalizer& analizer, string prefix)
{
  cout<<prefix<<
      "<bbox-query s=\""<<analizer.south<<"\" n=\""<<analizer.north<<"\" w=\""
      <<analizer.west<<"\" e=\""<<analizer.east<<"\"/>\n";
}

void print_print(const InputAnalizer& analizer)
{
  if (analizer.meta_found)
    cout<<"<print mode=\"meta\"/>\n";
  else
    cout<<"<print/>\n";
}

void process_nodes(string input, bool is_star = false)
{
  InputAnalizer analizer(input);
  if (analizer.timeout != "")
    cout<<"<osm-script timeout=\""<<analizer.timeout<<"\">\n\n";
  if (analizer.key_value.size() == 1
      && analizer.key_value.front().second.find('|') != string::npos)
  {
    string key = analizer.key_value.front().first;
    string buf = analizer.key_value.front().second;
    analizer.key_value.clear();
    while (buf.find('|') != string::npos)
    {
      analizer.key_value.push_back(make_pair(key, buf.substr(0, buf.find('|'))));
      buf = buf.substr(buf.find('|') + 1);
    }
    analizer.key_value.push_back(make_pair(key, buf));
    cout<<"<union>\n";
    for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
        it != analizer.key_value.end(); ++it)
    {
      cout<<"  <query type=\"node\">\n";
      if (analizer.bbox_found)
	print_bbox(analizer, "    ");
      cout<<"    <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
      print_meta(analizer, "    ");
      cout<<"  </query>\n";
    }
    cout<<"</union>\n";
  }
  else
  {
    cout<<"<query type=\"node\">\n";
    if (analizer.bbox_found)
      print_bbox(analizer, "  ");
    for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
        it != analizer.key_value.end(); ++it)
    {
      if (it->second == "*")
	cout<<"  <has-kv k=\""<<it->first<<"\"/>\n";
      else
	cout<<"  <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
    }
    print_meta(analizer, "  ");
    cout<<"</query>\n";
  }
  if (!is_star)
  {
    print_print(analizer);
    if (analizer.timeout != "")
      cout<<"\n</osm-script>\n";
  }
}

void process_ways(string input, bool is_star = false)
{
  InputAnalizer analizer(input);
  if (!is_star && analizer.timeout != "")
    cout<<"<osm-script timeout=\""<<analizer.timeout<<"\">\n\n";
  cout<<"<union>\n";
  if (is_star)
    cout<<"  <item/>\n";
  if (analizer.key_value.size() == 1
      && analizer.key_value.front().second.find('|') != string::npos)
  {
    string key = analizer.key_value.front().first;
    string buf = analizer.key_value.front().second;
    analizer.key_value.clear();
    while (buf.find('|') != string::npos)
    {
      analizer.key_value.push_back(make_pair(key, buf.substr(0, buf.find('|'))));
      buf = buf.substr(buf.find('|') + 1);
    }
    analizer.key_value.push_back(make_pair(key, buf));
    
    cout<<"  <union>\n";
    for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
        it != analizer.key_value.end(); ++it)
    {
      cout<<"    <query type=\"way\">\n";
      if (analizer.bbox_found)
	print_bbox(analizer, "      ");
      cout<<"      <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
      print_meta(analizer, "      ");
      cout<<"    </query>\n";
    }
    cout<<"  </union>\n";
  }
  else
  {
    cout<<"  <query type=\"way\">\n";
    if (analizer.bbox_found)
      print_bbox(analizer, "    ");
    for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
        it != analizer.key_value.end(); ++it)
    {
      if (it->second == "*")
	cout<<"    <has-kv k=\""<<it->first<<"\"/>\n";
      else
	cout<<"    <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
    }
    print_meta(analizer, "    ");
    cout<<"  </query>\n";
  }
  cout<<"  <recurse type=\"way-node\"/>\n"
        "</union>\n";
  print_print(analizer);
  if (!is_star && analizer.timeout != "")
    cout<<"\n</osm-script>\n";
}

void process_relations(string input, bool is_star = false)
{
  InputAnalizer analizer(input);
  if (!is_star && analizer.timeout != "")
    cout<<"<osm-script timeout=\""<<analizer.timeout<<"\">\n\n";
  if (analizer.key_value.size() == 1
      && analizer.key_value.front().second.find('|') != string::npos)
  {
    string key = analizer.key_value.front().first;
    string buf = analizer.key_value.front().second;
    analizer.key_value.clear();
    while (buf.find('|') != string::npos)
    {
      analizer.key_value.push_back(make_pair(key, buf.substr(0, buf.find('|'))));
      buf = buf.substr(buf.find('|') + 1);
    }
    analizer.key_value.push_back(make_pair(key, buf));
    cout<<"<union>\n";
    for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
        it != analizer.key_value.end(); ++it)
    {
      cout<<"  <query type=\"relation\">\n";
      if (analizer.bbox_found)
	print_bbox(analizer, "    ");
      cout<<"    <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
      print_meta(analizer, "    ");
      cout<<"  </query>\n";
    }
    cout<<"</union>\n";
  }
  else
  {
    cout<<"<query type=\"relation\">\n";
    if (analizer.bbox_found)
      print_bbox(analizer, "  ");
    for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
        it != analizer.key_value.end(); ++it)
    {
      if (it->second == "*")
	cout<<"  <has-kv k=\""<<it->first<<"\"/>\n";
      else
	cout<<"  <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
    }
    print_meta(analizer, "  ");
    cout<<"</query>\n";
  }
  print_print(analizer);
  if (analizer.timeout != "")
    cout<<"\n</osm-script>\n";
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    return 1;
  
  string input(argv[1]);
  
  if (input.substr(0, 4) == "node")
  {
    try
    {
      process_nodes(input.substr(4));
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 3) == "way")
  {
    try
    {
      process_ways(input.substr(3));
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 8) == "relation")
  {
    try
    {
      process_relations(input.substr(8));
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 1) == "*")
  {
    try
    {
      process_nodes(input.substr(1), true);
      process_ways(input.substr(1), true);
      process_relations(input.substr(1), true);
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 9) == "map?bbox=")
  {
    string south, north, east, west;
    input = input.substr(9);
    west = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    south = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    east = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    north = input;
    cout<<"<union>\n"
          "  <bbox-query s=\""<<south<<"\" n=\""<<north<<"\" w=\""
            <<west<<"\" e=\""<<east<<"\"/>\n"
          "  <recurse type=\"node-relation\" into=\"rels\"/>\n"
          "  <recurse type=\"node-way\"/>\n"
          "  <recurse type=\"way-relation\"/>\n"
          "</union>\n"
          "<print/>\n";
  }
  else
  {
    cout<<"Error: Query must start with 'node', 'way', 'relation', or '*'\n";
    return 1;
  }
  
  return 0;
}
