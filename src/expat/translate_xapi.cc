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
};

InputAnalizer::InputAnalizer(const string& input_) : bbox_found(false)
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
    else
    {
      key_value.push_back(make_pair("", ""));
      input = input.substr(1);	
      key_value.back().first = input.substr(0, input.find('='));
      input = input.substr(input.find('=')+1);
      key_value.back().second = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
  }
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    return 1;
  
  string input(argv[1]);
  
  if (input.substr(0, 4) == "node")
  {
    input = input.substr(4);
    try
    {
      InputAnalizer analizer(input);
      if (analizer.key_value.size() == 1)
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
	    cout<<"    <bbox-query s=\""<<analizer.south<<"\" n=\""<<analizer.north<<"\" w=\""
	        <<analizer.west<<"\" e=\""<<analizer.east<<"\"/>\n";
	  cout<<"    <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n"
	        "  </query>\n";
	}
	cout<<"</union>\n<print/>\n";
      }
      else
      {
	cout<<"<query type=\"node\">\n";
	if (analizer.bbox_found)
	  cout<<"  <bbox-query s=\""<<analizer.south<<"\" n=\""<<analizer.north<<"\" w=\""
	      <<analizer.west<<"\" e=\""<<analizer.east<<"\"/>\n";
	for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
	    it != analizer.key_value.end(); ++it)
	{
	  if (it->second == "*")
	    cout<<"  <has-kv k=\""<<it->first<<"\"/>\n";
	  else
	    cout<<"  <has-kv k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
	}
	cout<<"</query>\n<print/>\n";
      }
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 3) == "way")
  {
    cout<<"<query type=\"way\"/> (not yet implemented)\n";
    return 1;
  }
  else if (input.substr(0, 8) == "relation")
  {
    cout<<"<query type=\"relation\"/> (not yet implemented)\n";
    return 1;
  }
  else if (input.substr(0, 1) == "*")
  {
    input = input.substr(1);
    try
    {
      InputAnalizer analizer(input);
      if (analizer.key_value.size() > 0)
      {
	cout<<"<query type=\"all\"/> (not yet implemented)\n";
	return 1;
      }
      else
      {
	if (!analizer.bbox_found)
	{
	  cout<<"Please specify a bounding box\n";
	  return 1;
	}
	cout<<"<union>\n"
              "  <bbox-query s=\""<<analizer.south<<"\" n=\""<<analizer.north<<"\" w=\""
	    <<analizer.west<<"\" e=\""<<analizer.east<<"\"/>\n"
	      "  <recurse type=\"node-relation\" into=\"rels\"/>\n"
	      "  <recurse type=\"node-way\"/>\n"
	      "  <recurse type=\"way-relation\"/>\n"
	      "</union>\n"
	      "<print/>\n";
      }
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else
  {
    cout<<"Error: Query must start with 'node', 'way', 'relation', or '*'\n";
    return 1;
  }
  
  return 0;
}
