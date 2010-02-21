/*****************************************************************
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <iostream>

#include "../expat_justparse_interface.h"

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

string decode_cgi_to_plain(const string& raw)
{
  string result;
  string::size_type pos(0);
  
  while (pos < raw.size())
  {
    if (raw[pos] == '%')
    {
      if (pos >= raw.size()+2)
	return (result + raw.substr(0, pos));
      char a(hex_digit(raw[pos+1])), b(hex_digit(raw[pos+2]));
      if ((a < 16) && (b < 16))
      {
	result += (char)(a*16 + b);
	pos += 3;
      }
      else
	result += raw[pos++];
    }
    else if (raw[pos] == '+')
    {
      result += ' ';
      ++pos;
    }
    else if (raw[pos] == '&')
      pos = raw.size();
    else
      result += raw[pos++];
  }
  
  return result;
}

int main(int argc, char *argv[])
{
  char c;
  string buf;
  while (!cin.eof())
  {
    cin.get(c);
    buf += c;
  }
  
  cout<<decode_cgi_to_plain(buf);
  
  return 0;
}
