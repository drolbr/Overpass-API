#include <iostream>

using namespace std;

string hexcode(char c)
{
  string result = "%##";
  if ((unsigned char)c / 16 < 10)
    result[1] = (unsigned char)c / 16 + '0';
  else
    result[1] = (unsigned char)c / 16 - 10 + 'A';
  if ((unsigned char)c % 16 < 10)
    result[2] = (unsigned char)c % 16 + '0';
  else
    result[2] = (unsigned char)c % 16 - 10 + 'A';
  return result;
}

string encode_plain_to_cgi(const string& raw)
{
  string result;
  string::size_type pos(0);
  
  while (pos < raw.size())
  {
    if (isalnum(raw[pos]))
      result += raw[pos];
    else
      result += hexcode(raw[pos]);
    ++pos;
  }
  
  return result;
}

int main(int argc, char *argv[])
{
  char c;
  string buf;
  cin.get(c);
  while (cin.good())
  {
    buf += c;
    cin.get(c);
  }
  
  cout<<encode_plain_to_cgi(buf);
  
  return 0;
}
