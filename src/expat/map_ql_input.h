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

#ifndef MAP_QL_INPUT
#define MAP_QL_INPUT

#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>


typedef unsigned int uint;

template< class T > class Comment_Replacer;
template< class T > class Whitespace_Compressor;
template< class T > class Tokenizer;

class Tokenizer_Wrapper
{
  public:
    Tokenizer_Wrapper(std::istream& in_);
    ~Tokenizer_Wrapper();

    const std::string& operator*() const { return head; }
    void operator++();
    bool good() { return good_; }
    const std::pair< uint, uint >& line_col() const { return line_col_; }

  private:
    Tokenizer_Wrapper(const Tokenizer_Wrapper&);
    void operator=(const Tokenizer_Wrapper&);

    std::string head;
    bool good_;
    Comment_Replacer< std::istream >* incr;
    Whitespace_Compressor< Comment_Replacer< std::istream > >* inwsc;
    Tokenizer< Whitespace_Compressor< Comment_Replacer< std::istream > > >* in;
    std::pair< uint, uint > line_col_;
};

#endif
