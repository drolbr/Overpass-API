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

#include "four_field_index.h"

#include <iostream>
#include <vector>


void check_result(const Four_Field_Index& idx, const std::string& expected)
{
  std::string result = idx.to_string();
  if (result == expected)
    std::cout<<"OK"<<'\n';
  else
    std::cout<<"failed: "<<result<<'\n';
}


void dump_four_fields(const Four_Field_Entry& entry)
{
  std::cout<<"retval = ("<<entry.sw<<", "<<entry.se<<", "<<entry.nw<<", "<<entry.ne<<") ... ";
}


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute\n";
    return 0;
  }
  std::string test_to_execute = args[1];

  if (test_to_execute.empty() || test_to_execute == "1")
  {
    Four_Field_Index idx(0);
    check_result(idx, "[]");
    std::cout<<"retval = "<<idx.add_point(51., 7., 256)<<" ... ";
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 256} ]");
    std::cout<<"retval = "<<idx.add_point(51.001, 7.001, 257)<<" ... ";
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 257} ]");
    std::cout<<"retval = "<<idx.add_point(50.995, 7.01)<<" ... ";
    check_result(idx, "[ {(50.9902976, 7.0057984, 50.9968511, 7.0123519), 1}  {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 257} ]");
    std::cout<<"retval = "<<idx.add_point(51., .01, 259)<<" ... ";
    check_result(idx, "[ {(50.9968512, 0.0065536, 51.0034047, 0.0131071), 259}  {(50.9902976, 7.0057984, 50.9968511, 7.0123519), 1}  {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 257} ]");
    std::cout<<"retval = "<<idx.add_point(48., 4., 260)<<" ... ";
    check_result(idx, "[ {(47.9953024, 3.9976960, 48.0018559, 4.0042495), 260}  {(50.9968512, 0.0065536, 51.0034047, 0.0131071), 259}  {(50.9902976, 7.0057984, 50.9968511, 7.0123519), 1}  {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 257} ]");
  }

  if (test_to_execute.empty() || test_to_execute == "2")
  {
    Four_Field_Index idx(0);
    check_result(idx, "[]");
    dump_four_fields(idx.add_segment(51., 7., 51.001, 7.001, 256));
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 256} ]");
    dump_four_fields(idx.add_segment(51.001, 7.001, 51.005, 7.003, 257));
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 257}  {(51.0034048, 6.9992448, 51.0099583, 7.0057983), 257} ]");
    dump_four_fields(idx.add_segment(51.001, 7.007, 51.005, 7.003, 258));
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 258}  {(50.9968512, 7.0057984, 51.0034047, 7.0123519), 258}  {(51.0034048, 6.9992448, 51.0099583, 7.0057983), 258}  {(51.0034048, 7.0057984, 51.0099583, 7.0123519), 258} ]");
  }

  return 0;
}
