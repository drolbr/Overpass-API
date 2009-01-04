#ifndef EXPAT_JUSTPARSE_INTERFACE
#define EXPAT_JUSTPARSE_INTERFACE

#include <string>
#include <sstream>

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <expat.h>

#define BUFFSIZE        65536

using namespace std;

void parse(FILE* in,
		  void (*start)(const char*, const char**),
			 void (*end)(const char*));

void set_line_offset(int line_offset_);
int current_line_number();

string parse(string input,
	     void (*start)(const char*, const char**),
		    void (*end)(const char*));

template < class Ostream >
inline Ostream& escape_infile_xml(Ostream& out, string s)
{
  unsigned int i(0);
  while (i < s.size())
  {
    if (s[i] == '\t')
    {
      out<<s.substr(0, i)<<"\\\t";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '\n')
    {
      out<<s.substr(0, i)<<"\\\n";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '"')
    {
      out<<s.substr(0, i)<<"&quot;";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '&')
    {
      out<<s.substr(0, i)<<"&amp;";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '<')
    {
      out<<s.substr(0, i)<<"&lt;";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '>')
    {
      out<<s.substr(0, i)<<"&gt;";
      s = s.substr(i+1);
      i = 0;
    }
    else
      ++i;
  }
  out<<s;
  return out;
}

template < class Ostream >
inline Ostream& escape_xml(Ostream& out, string s)
{
  unsigned int i(0);
  while (i < s.size())
  {
    if (s[i] == '"')
    {
      out<<s.substr(0, i)<<"&quot;";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '&')
    {
      out<<s.substr(0, i)<<"&amp;";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '<')
    {
      out<<s.substr(0, i)<<"&lt;";
      s = s.substr(i+1);
      i = 0;
    }
    else if (s[i] == '>')
    {
      out<<s.substr(0, i)<<"&gt;";
      s = s.substr(i+1);
      i = 0;
    }
    else
      ++i;
  }
  out<<s;
  return out;
}

#endif
