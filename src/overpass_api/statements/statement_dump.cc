#include "../frontend/output.h"
#include "statement_dump.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

Statement_Dump::~Statement_Dump()
{
  for (vector< Statement_Dump* >::iterator it = substatements.begin();
      it != substatements.end(); ++it)
    delete *it;
}

void Statement_Dump::add_statement(Statement_Dump* statement, string text)
{
  substatements.push_back(statement);
}

string Statement_Dump::dump_xml() const
{
  string result;
  
  if (substatements.empty())
  {
    result = string("<") + name;
    for (map< string, string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
      result += string(" ") + it->first + "=\"" + escape_xml(it->second) + "\"";
    result += "/>\n";
  }
  else
  {
    result = string("<") + name;
    for (map< string, string >::const_iterator it = attributes.begin();
        it != attributes.end(); ++it)
      result += string(" ") + it->first + "=\"" + escape_xml(it->second) + "\"";
    result += ">\n";

    for (vector< Statement_Dump* >::const_iterator it = substatements.begin();
        it != substatements.end(); ++it)
    {
      string subresult = (*it)->dump_xml();
      string::size_type pos = 0;
      string::size_type next = subresult.find('\n', pos);
      while (next != string::npos)
      {
	result += string("  ") + subresult.substr(pos, next-pos) + '\n';
	pos = next + 1;
	next = subresult.find('\n', pos);
      }
    }
    
    result += string("</") + name + ">\n";
  }

  return result;
}

string Statement_Dump::dump_pretty_map_ql() const
{
  string result;
  return result;
}

  
string Statement_Dump::dump_compact_map_ql() const
{
  string result;
  return result;
}

Statement_Dump* Statement_Dump::create_statement
    (string element, int line_number, const map< string, string >& attributes)
{
  return new Statement_Dump(element, attributes);
}
