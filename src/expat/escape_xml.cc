#include <string>

#include "escape_xml.h"

using namespace std;

string escape_xml(const string& s)
{
  string result;
  result.reserve(s.length()*2);
  for (int i(0); i < s.length(); ++i)
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
      result += '?';
    else
      result += s[i];
  }
  return result;
}
