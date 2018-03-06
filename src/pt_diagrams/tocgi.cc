/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

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
