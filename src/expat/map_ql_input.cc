/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#include "map_ql_input.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

template< class In >
class Comment_Replacer
{
    Comment_Replacer(const Comment_Replacer&);
    Comment_Replacer& operator=(const Comment_Replacer&);
    
  public:
    Comment_Replacer(In& in_) : in(in_), buffer(0),
        buffer_state(invalid), state(plainfile), line(1), col(1) {}
    bool good() { return (buffer_state != invalid || in.good()); }
    void get(char& c);
    
    // The line and the column of the next character to read.
    pair< uint, uint > line_col() { return make_pair(line, col); }
    
  private:
    In& in;
    char buffer;
    enum { invalid, read, written, escaped } buffer_state;
    enum { plainfile, single_quot, double_quot } state;
    uint line;
    uint col;
};

template< class In >
inline void Comment_Replacer< In >::get(char& c)
{
  // If a buffer exists, it must be cleared first.
  if (buffer_state == written)
  {
    c = buffer;
    buffer_state = read;
  }
  else if (buffer_state == escaped)
  {
    c = buffer;
    buffer_state = read;
    return;
  }
  else
  {
    buffer_state = invalid;
    in.get(c);
    if (!in.good())
      return;
  }
  
  // Adjust line and column counter
  if (c == '\n')
  {
    ++line;
    col = 1;
  }
  else
    ++col;
  
  // We've got a valid character. Process it with regard to the state.
  if (state == single_quot)
  {
    if (c == '\\')
    {
      in.get(buffer);
      if (in.good())
	buffer_state = escaped;
      else
	buffer_state = read;
    }
    else if (c == '\'')
      state = plainfile;
    return;
  }
  else if (state == double_quot)
  {
    if (c == '\\')
    {
      in.get(buffer);
      if (in.good())
	buffer_state = escaped;
      else
	buffer_state = read;
    }
    else if (c == '"')
      state = plainfile;
    return;
  }

  // state == plainfile
  if (c == '\'')
    state = single_quot;
  else if (c == '"')
    state = double_quot;
  if (c != '/')
    return;

  in.get(buffer);
  if (!in.good())
    buffer_state = read;
  else if (buffer == '/')
  {
    in.get(c);
    while (in.good() && (c != '\n'))
      in.get(c);
    c = ' ';
    
    ++line;
    col = 1;
  }
  else if (buffer == '*')
  {
    while (in.good() && (buffer != '/'))
    {
      if (c == '\n')
      {
	col = 1;
	++line;
      }
      else
	++col;
	
      if (c == '*')
      {
	c = ' ';
	in.get(buffer);

	if (buffer == '\n')
	{
	  col = 1;
	  ++line;
	}
	else
	  ++col;
      }
      else
        in.get(c);
    }
    c = ' ';
  }
  else
    buffer_state = written;
}

//-----------------------------------------------------------------------------

template< class In >
class Whitespace_Compressor
{
  // Don't copy or assign.
  Whitespace_Compressor(const Whitespace_Compressor&);
  Whitespace_Compressor& operator=(const Whitespace_Compressor&);
  
  public:
    Whitespace_Compressor(In& in_) : in(in_), buffer(0),
        buffer_state(invalid), state(plainfile) {}
    bool good() { return (buffer_state != invalid || in.good()); }
    void get(char& c);
    
    // The line and the column of the next character to read.
    pair< uint, uint > line_col();
    
  private:
    In& in;
    char buffer;
    pair< uint, uint > buffer_line_col;
    enum { invalid, read, written, escaped } buffer_state;
    enum { plainfile, single_quot, double_quot } state;
};

template< class In >
inline void Whitespace_Compressor< In >::get(char& c)
{
  // If a buffer exists, it must be cleared first.
  if (buffer_state == written)
  {
    c = buffer;
    buffer_state = read;
  }
  else if (buffer_state == escaped)
  {
    c = buffer;
    buffer_state = read;
    return;
  }
  else
  {
    buffer_state = invalid;
    in.get(c);
    if (!in.good())
      return;
  }  
  
  // We've got a valid character. Process it with regard to the state.
  if (state == single_quot)
  {
    if (c == '\\')
    {
      buffer_line_col = in.line_col();
      in.get(buffer);
      if (in.good())
	buffer_state = escaped;
      else
	buffer_state = read;
    }
    else if (c == '\'')
      state = plainfile;
    return;
  }
  else if (state == double_quot)
  {
    if (c == '\\')
    {
      buffer_line_col = in.line_col();
      in.get(buffer);
      if (in.good())
	buffer_state = escaped;
      else
	buffer_state = read;
    }
    else if (c == '"')
      state = plainfile;
    return;
  }
  
  // state == plainfile
  if (c == '\'')
    state = single_quot;
  else if (c == '"')
    state = double_quot;
  if (!isspace(c))
    return;
  
  while (in.good() && isspace(c))
  {
    buffer_line_col = in.line_col();
    in.get(c);
  }
  if (in.good())
    buffer_state = written;
  else
    buffer_state = read;
  buffer = c;
  c = ' ';
}

template< class In >
inline pair< uint, uint > Whitespace_Compressor< In >::line_col()
{
  if (buffer_state == written || buffer_state == escaped)
    return buffer_line_col;
  else
    return in.line_col();
}

//-----------------------------------------------------------------------------

template< class In >
class Tokenizer
{
  Tokenizer(const Tokenizer&);
  Tokenizer& operator=(const Tokenizer&);
  
  public:
    Tokenizer(In& in_);
    bool good() { return (buffer != "" || in.good()); }
    void get(string& s);
    
    // The line and the column of the next token to read.
    pair< uint, uint > line_col();
    
  private:
    In& in;
    string buffer;
    queue< pair< uint, uint > > line_cols;
    void grow_buffer(unsigned int size);
    void probe(string& s, const string& probe1, const string& probe2 = "");
    void clear_space();
};

template< class In >
Tokenizer< In >::Tokenizer(In& in_) : in(in_)
{
  clear_space();
}

template< class In >
inline void Tokenizer< In >::grow_buffer(unsigned int size)
{
  if (buffer.size() >= size)
    return;
  
  char c;
  line_cols.push(in.line_col());
  in.get(c);
  while (in.good())
  {
    buffer += c;
    if (buffer.size() == size)
      break;
    line_cols.push(in.line_col());
    in.get(c);
  }
}

template< class In >
inline void Tokenizer< In >::probe
    (string& s, const string& probe1, const string& probe2)
{
  grow_buffer(2);
  if (buffer.substr(0, 2) == probe1)
  {
    s = buffer.substr(0, 2);
    line_cols.pop();
    line_cols.pop();
    buffer = buffer.substr(2);
    clear_space();
  }
  else if (buffer.substr(0, 2) == probe2)
  {
    s = buffer.substr(0, 2);
    line_cols.pop();
    line_cols.pop();
    buffer = buffer.substr(2);
    clear_space();
  }
  else
  {
    s = buffer.substr(0, 1);
    line_cols.pop();
    buffer = buffer.substr(1);
    clear_space();
  }
}

template< class In >
inline void Tokenizer< In >::clear_space()
{
  grow_buffer(1);
  if (buffer == " ")
  {
    line_cols.pop();
    buffer = buffer.substr(1);
    grow_buffer(1);
  }
}
  
template< class In >
inline void Tokenizer< In >::get(string& s)
{
  if (buffer == "")
    return;
  
  if (isalpha(buffer[0]) || buffer[0] == '_')
  {
    uint pos = 1;
    grow_buffer(pos + 1);
    while (buffer.size() > pos && (isalnum(buffer[pos]) || buffer[pos] == '_'))
      grow_buffer((++pos) + 1);
    s = buffer.substr(0, pos);
    for (uint i = 0; i < pos; ++i)
      line_cols.pop();
    buffer = buffer.substr(pos);
    clear_space();
  }
  else if (isdigit(buffer[0]) || buffer[0] == '-')
  {
    uint pos = 1;
    grow_buffer(pos + 1);
    if (isdigit(buffer[0]) ||
        (buffer.size() > pos && (isdigit(buffer[pos]) || buffer[pos] == '.')))
    {
      while (buffer.size() > pos && isdigit(buffer[pos]))
        grow_buffer((++pos) + 1);
      
      if (buffer.size() > pos && buffer[pos] == '.')
      {
        grow_buffer((++pos) + 1);
        while (buffer.size() > pos && isdigit(buffer[pos]))
	  grow_buffer((++pos) + 1);
      }
      
      if (buffer.size() > pos && buffer[pos] == 'e')
      {
        grow_buffer((++pos) + 1);
        if (buffer.size() > pos && buffer[pos] == '-')
	  grow_buffer((++pos) + 1);
        while (buffer.size() > pos && isdigit(buffer[pos]))
	  grow_buffer((++pos) + 1);
      }
    
      s = buffer.substr(0, pos);
      for (uint i = 0; i < pos; ++i)
        line_cols.pop();
      buffer = buffer.substr(pos);
      clear_space();
    }
    else
      probe(s, "->");
  }
  else if (buffer[0] == '\'')
  {
    uint pos = 1;
    grow_buffer(pos + 1);
    while (buffer.size() > pos && (buffer[pos] != '\''))
    {
      if (buffer[pos] == '\\')
	pos += 2;
      else
	++pos;
      grow_buffer(pos + 1);
    }
    if (buffer.size() == pos)
    {
      s = buffer.substr(0, pos);
      for (uint i = 0; i < pos; ++i)
        line_cols.pop();
      buffer = "";
    }
    else
    {
      s = buffer.substr(0, pos+1);
      for (uint i = 0; i < pos+1; ++i)
        line_cols.pop();
      buffer = buffer.substr(pos+1);
      clear_space();
    }
  }
  else if (buffer[0] == '"')
  {
    unsigned int pos = 1;
    grow_buffer(pos + 1);
    while (buffer.size() > pos && (buffer[pos] != '"'))
    {
      if (buffer[pos] == '\\')
	pos += 2;
      else
	++pos;
      grow_buffer(pos + 1);
    }
    if (buffer.size() == pos)
    {
      s = buffer.substr(0, pos);
      for (uint i = 0; i < pos; ++i)
        line_cols.pop();
      buffer = "";
    }
    else
    {
      s = buffer.substr(0, pos+1);
      for (uint i = 0; i < pos+1; ++i)
        line_cols.pop();
      buffer = buffer.substr(pos+1);
      clear_space();
    }
  }
  else if (buffer[0] == ':')
    probe(s, "::");
  else if (buffer[0] == '=')
    probe(s, "==");
  else if (buffer[0] == '!')
    probe(s, "!=");
  else if (buffer[0] == '|')
    probe(s, "||");
  else if (buffer[0] == '&')
    probe(s, "&&");
  else if (buffer[0] == '<')
    probe(s, "<<", "<=");
  else if (buffer[0] == '>')
    probe(s, ">>", ">=");
  else if ((buffer[0] & 0x80) == 0)
  {
    s = buffer.substr(0, 1);
    line_cols.pop();
    buffer = buffer.substr(1);
    clear_space();
  }
  else if ((buffer[0] & 0xe0) == 0xc0)
  {
    grow_buffer(2);
    s = buffer.substr(0, 2);
    line_cols.pop();
    buffer = buffer.substr(2);
    clear_space();
  }
  else if ((buffer[0] & 0xf0) == 0xe0)
  {
    grow_buffer(3);
    s = buffer.substr(0, 3);
    line_cols.pop();
    buffer = buffer.substr(3);
    clear_space();
  }
  else if ((buffer[0] & 0xf8) == 0xf0)
  {
    grow_buffer(4);
    s = buffer.substr(0, 4);
    line_cols.pop();
    buffer = buffer.substr(4);
    clear_space();
  }
  else // The input is invalid UTF-8. 
  {
    s = buffer.substr(0, 1);
    line_cols.pop();
    buffer = buffer.substr(1);
    clear_space();
  }
}

template< class In >
inline pair< uint, uint > Tokenizer< In >::line_col()
{
  if (line_cols.empty())
    return in.line_col();
  else
    return line_cols.front();
}

//-----------------------------------------------------------------------------

Tokenizer_Wrapper::Tokenizer_Wrapper(istream& in_)
{
  incr = new Comment_Replacer< istream >(in_);
  inwsc = new Whitespace_Compressor< Comment_Replacer< istream > >(*incr);
  in = new Tokenizer< Whitespace_Compressor< Comment_Replacer< istream > > >(*inwsc);
  line_col_ = in->line_col();
  good_ = in->good();
  in->get(head);
}

void Tokenizer_Wrapper::operator++()
{
  line_col_ = in->line_col();
  good_ = in->good();
  in->get(head);
}

Tokenizer_Wrapper::~Tokenizer_Wrapper()
{
  delete in;
  delete inwsc;
  delete incr;
}
