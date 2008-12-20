#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;

char hex_digit(char c)
{
  if (c <= 57)
  {
    if (c >= 48)
      return c - 48;
    return 16;
  }
  if (c <= 70)
  {
    if (c >= 65)
      return c - 55;
    return 16;
  }
  if (c <= 102)
  {
    if (c >= 97)
      return c - 87;
    return 16;
  }
  return 16;
}

string cgi_form_to_text()
{
  string raw;
  char* method;
  method = getenv("REQUEST_METHOD");
  if ((method != NULL) && (!strcmp(method, "GET")))
    raw = getenv("QUERY_STRING");
  else
  {
    string buf;
    while (!cin.eof())
    {
      getline(cin, buf);
      raw += buf + '\n';
    }
  }
  
  string result;
  unsigned int pos(0);
  while (pos < raw.size())
  {
    if (raw[pos] == '%')
    {
      if (pos >= raw.size()+2)
	return (result + raw.substr(0, pos));
      char a(hex_digit(raw[pos+1])), b(hex_digit(raw[pos+2]));
      if ((a < 16) && (b < 16))
      {
	result += raw.substr(0, pos);
	result += (char)(a*16 + b);
	raw = raw.substr(pos+3);
	pos = 0;
      }
      else
	++pos;
    }
    else if (raw[pos] == '+')
    {
      raw[pos] = ' ';
      ++pos;
    }
    else if (raw[pos] == '=')
    {
      result = "";
      raw = raw.substr(pos+1);
      pos = 0;
    }
    else
      ++pos;
  }
  result += raw;
  
  return result;
}

string input;
int line_offset(0);

void return_error(string error, int type = 0)
{
  cout<<"Content-type: text/html\n\n";
  
  cout<<"<html>\n<head>\n<title>Error!</title>\n</head>\n<body>\n<pre>";

  if (current_line_number >= 0)
    cout<<"The following error has occured in line "<<(current_line_number + line_offset)
	<<" while parsing the input:\n\n";
  else
    cout<<"The following error has occured while parsing the input:\n\n";
  cout<<error;
  cout<<"\n\nYour input:\n";
  cout<<"-- Begin of Input --\n";
  unsigned int pos(0);
  unsigned int line_number(1);
  if ((current_line_number >= 0) && (current_line_number + line_offset == 1))
  {
    cout<<"<strong>";
  }
  while (pos < input.size())
  {
    if (input[pos] == '\n')
    {
      ++line_number;
      if ((current_line_number >= 0) && ((int)line_number == current_line_number + line_offset))
      {
	cout<<input.substr(0, pos)<<"\n<strong>";
	input = input.substr(pos+1);
	pos = 0;
      }
      else if ((current_line_number >= 0) && ((int)line_number == current_line_number + line_offset + 1))
      {
	cout<<input.substr(0, pos)<<"\n</strong>";
	input = input.substr(pos+1);
	pos = 0;
      }
    }
    if (input[pos] == '<')
    {
      cout<<input.substr(0, pos)<<"&lt;";
      input = input.substr(pos+1);
      pos = 0;
    }
    else if (input[pos] == '&')
    {
      cout<<input.substr(0, pos)<<"&amp;";
      input = input.substr(pos+1);
      pos = 0;
    }
    else
      ++pos;
  }
  cout<<input;
  cout<<"-- End of Input --\n";
  
  cout<<"</pre>\n</body>\n</html>\n";
}

string get_xml_raw()
{
  input = cgi_form_to_text();
  
  unsigned int pos(0);
  while ((pos < input.size()) && (isspace(input[pos])))
    ++pos;
  if (pos == input.size())
  {
    return_error("Empty input!");
    return 0;
  }
  
  string xml_raw(input);
  if ((input[pos] == '<') && (input[pos+1] != '?'))
  {
    xml_raw = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-script>\n"
	+ input + "\n</osm-script>\n";
    line_offset = -2;
  }

  if (xml_raw.size() > BUFFSIZE-1)
  {
    ostringstream temp;
    temp<<"Input too long (length: "<<xml_raw.size()<<", max. allowed: "<<BUFFSIZE-1<<')';
    return_error(temp.str());
    return 0;
  }
  
  return xml_raw;
}
