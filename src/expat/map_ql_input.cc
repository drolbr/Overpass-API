#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

using namespace std;

typedef unsigned int uint;

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
    pair< uint, uint > line_col() { return make_pair< uint, uint >(line, col); }
    
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
  else if (isdigit(buffer[0]))
  {
    uint pos = 1;
    grow_buffer(pos + 1);
    while (buffer.size() > pos && isdigit(buffer[pos]))
      grow_buffer((++pos) + 1);
    s = buffer.substr(0, pos);
    for (uint i = 0; i < pos; ++i)
      line_cols.pop();
    buffer = buffer.substr(pos);
    clear_space();
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
    s = buffer.substr(0, pos+1);
    for (uint i = 0; i < pos+1; ++i)
      line_cols.pop();
    buffer = buffer.substr(pos+1);
    clear_space();
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
    s = buffer.substr(0, pos+1);
    for (uint i = 0; i < pos+1; ++i)
      line_cols.pop();
    buffer = buffer.substr(pos+1);
    clear_space();
  }
  else if (buffer[0] == '-')
    probe(s, "->");
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
  else
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

class Tokenizer_Wrapper
{
  public:
    Tokenizer_Wrapper(istream& in_);
    ~Tokenizer_Wrapper();
    
    const string& operator*() const { return head; }
    void operator++();
    bool good() { return in->good(); }
    const pair< uint, uint >& line_col() const { return line_col_; }
    
  private:
    Tokenizer_Wrapper(const Tokenizer_Wrapper&);
    void operator=(const Tokenizer_Wrapper&);
    
    string head;
    Comment_Replacer< istream >* incr;
    Whitespace_Compressor< Comment_Replacer< istream > >* inwsc;
    Tokenizer< Whitespace_Compressor< Comment_Replacer< istream > > >* in;
    pair< uint, uint > line_col_;
};

Tokenizer_Wrapper::Tokenizer_Wrapper(istream& in_)
{
  incr = new Comment_Replacer< istream >(in_);
  inwsc = new Whitespace_Compressor< Comment_Replacer< istream > >(*incr);
  in = new Tokenizer< Whitespace_Compressor< Comment_Replacer< istream > > >(*inwsc);
  line_col_ = in->line_col();
  in->get(head);
}

void Tokenizer_Wrapper::operator++()
{
  line_col_ = in->line_col();
  in->get(head);
}

Tokenizer_Wrapper::~Tokenizer_Wrapper()
{
  delete in;
  delete inwsc;
  delete incr;
}

//-----------------------------------------------------------------------------

void parse_setup(Tokenizer_Wrapper& token)
{
  while (token.good() && *token != "]")
  {
    cout<<"setup token: "<<*token<<'\n';
    ++token;
  }
  cout<<"setup token: "<<*token<<'\n';
  if (token.good())
    ++token;
  cout<<"setup token done.\n";
}

void parse_statement(Tokenizer_Wrapper& token);

void parse_union(Tokenizer_Wrapper& token)
{
  while (token.good() && *token != ")")
  {
    cout<<"union token: "<<*token<<'\n';
    ++token;
    parse_statement(token);
  }
  cout<<"union token: "<<*token<<'\n';
  if (token.good())
    ++token;

  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    if (token.good() && *token == ".")
      ++token;
    else
      cout<<"Error (line "<<token.line_col().first<<", column "<<token.line_col().second<<"): "
      "Variable expected\n";    
    if (token.good())
    {
      into = *token;
      ++token;
    }
  }
  cout<<"union into "<<into<<" done.\n";
}

void parse_foreach(Tokenizer_Wrapper& token)
{
  string from = "_";
  if (token.good() && *token == ".")
  {
    ++token;
    if (token.good())
    {
      from = *token;
      ++token;
    }
  }
  
  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    if (token.good() && *token == ".")
      ++token;
    else
      cout<<"Error (line "<<token.line_col().first<<", column "<<token.line_col().second<<"): "
      "Variable expected\n";    
    if (token.good())
    {
      into = *token;
      ++token;
    }
  }
  
  while (token.good() && *token != ")")
  {
    cout<<"foreach token: "<<*token<<'\n';
    ++token;
    parse_statement(token);
  }
  cout<<"foreach token: "<<*token<<'\n';
  if (token.good())
    ++token;
  cout<<"foreach from "<<from<<" into "<<into<<" done.\n";
}

void parse_output(const string& from, Tokenizer_Wrapper& token)
{
  while (token.good() && *token != "}")
  {
    cout<<"output token: "<<*token<<'\n';
    ++token;
  }
  cout<<"output token: "<<*token<<'\n';
  if (token.good())
    ++token;
  cout<<"output token done.\n";
}

void parse_query(const string& type, const string& from, Tokenizer_Wrapper& token)
{
  while (token.good() && *token == "[")
  {
    while (token.good() && *token != "]")
    {
      cout<<"query token: "<<*token<<'\n';
      ++token;
    }
    cout<<"query token: ] (condition complete)"<<'\n';
    if (token.good())
      ++token;
  }
  string into = "_";
  if (token.good() && *token == "->")
  {
    ++token;
    if (token.good() && *token == ".")
      ++token;
    else
      cout<<"Error (line "<<token.line_col().first<<", column "<<token.line_col().second<<"): "
      "Variable expected\n";    
    if (token.good())
    {
      into = *token;
      ++token;
    }
  }
  cout<<"query into "<<into<<'\n';
  cout<<"query token done.\n";
}

void parse_statement(Tokenizer_Wrapper& token)
{
  if (!token.good())
    return;
  
  if (*token == "(")
  {
    parse_union(token);
    return;
  }
  else if (*token == "foreach")
  {
    parse_foreach(token);
    return;
  }

  string type = "";
  if (*token != "{" && *token != ".")
  {
    type = *token;
    if (type != "node" && type != "way" && type != "rel" && type != "relation" && type != "all")
      cout<<"Error (line "<<token.line_col().first<<", column "<<token.line_col().second<<"): "
          "Unknown type \""<<type<<"\"\n";
    ++token;
  }
    
  string from = "";
  if (token.good() && *token == ".")
  {
    ++token;
    if (token.good())
    {
      from = *token;
      ++token;
    }
  }

  if (token.good() && type == "" && *token == "{")
    parse_output(from, token);
  else
    parse_query(type, from, token);
  
  cout<<"statement done.\n";
}

int main(int argc, char* args[])
{
  Tokenizer_Wrapper token(cin);

  while (token.good() && *token == "[")
    parse_setup(token);
      
  while (token.good())
    parse_statement(token);
  
  //   char buf;
//   pair< uint, uint > line_col = inwsc.line_col();
//   inwsc.get(buf);
//   while (in.good())
//   {
//     cout<<line_col.first<<", "<<line_col.second<<": "<<buf<<'\n';
//     line_col = inwsc.line_col();
//     inwsc.get(buf);
//   }

/*  string buf;
  pair< uint, uint > line_col = in.line_col();
  in.get(buf);
  while (in.good())
  {
    cout<<line_col.first<<", "<<line_col.second<<": "<<buf<<'\n';
    line_col = in.line_col();
    in.get(buf);
  }*/
  
  return 0;
}
