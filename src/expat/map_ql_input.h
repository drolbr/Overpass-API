#ifndef MAP_QL_INPUT
#define MAP_QL_INPUT

#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

using namespace std;

typedef unsigned int uint;

template< class T > class Comment_Replacer;
template< class T > class Whitespace_Compressor;
template< class T > class Tokenizer;

class Tokenizer_Wrapper
{
  public:
    Tokenizer_Wrapper(istream& in_);
    ~Tokenizer_Wrapper();
    
    const string& operator*() const { return head; }
    void operator++();
    bool good() { return good_; }
    const pair< uint, uint >& line_col() const { return line_col_; }
    
  private:
    Tokenizer_Wrapper(const Tokenizer_Wrapper&);
    void operator=(const Tokenizer_Wrapper&);
    
    string head;
    bool good_;
    Comment_Replacer< istream >* incr;
    Whitespace_Compressor< Comment_Replacer< istream > >* inwsc;
    Tokenizer< Whitespace_Compressor< Comment_Replacer< istream > > >* in;
    pair< uint, uint > line_col_;
};

#endif
