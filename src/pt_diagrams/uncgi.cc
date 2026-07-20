/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

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

bool decode_cgi_to_plain(const string& raw, string& result)
{
  result.clear();
  string::size_type pos(0);

  while (pos < raw.size())
  {
    if (raw[pos] == '%')
    {
      if (pos + 2 >= raw.size())
        return false;
      char a(hex_digit(raw[pos+1])), b(hex_digit(raw[pos+2]));
      if ((a < 16) && (b < 16))
      {
        result += (char)(a*16 + b);
        pos += 3;
      }
      else
        return false;
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

  return true;
}

int main(int argc, char *argv[])
{
  char c;
  string buf;
  while (cin.get(c))
    buf += c;

  string result;
  if (!decode_cgi_to_plain(buf, result))
    return 1;

  cout<<result;

  return 0;
}
