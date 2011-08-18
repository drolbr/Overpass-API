#include <string>

#include "escape_xml.h"

using namespace std;

string escape_xml(const string& s)
{
  string result;
  result.reserve(s.length()*2);
  for (string::size_type i(0); i < s.size(); ++i)
  {
    if (s[i] == '&')
      result += "&amp;";
    else if (s[i] == '\"')
      result += "&quot;";
    else if (s[i] == '<')
      result += "&lt;";
    else if (s[i] == '>')
      result += "&gt;";
    else if ((unsigned char)s[i] < 32)
    {
      if ((s[i] == '\n') || (s[i] == '\t') || (s[i] == '\r'))
	result += s[i];
      else
        result += '?';
    }
    else
      result += s[i];
  }
  return result;
}
