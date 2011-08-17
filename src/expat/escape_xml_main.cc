#include <iostream>

#include "escape_xml.h"

using namespace std;

int main(int argc, char *argv[])
{
  char c;
  string buf;
  while (!cin.eof())
  {
    cin.get(c);
    buf += c;
  }
  
  cout<<escape_xml(buf);
  
  return 0;
}
