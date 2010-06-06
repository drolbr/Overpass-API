#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <time.h>

#include "statement.h"

using namespace std;

void Statement::eval_cstr_array(string element, map< string, string >& attributes, const char **attr)
{
  for (unsigned int i(0); attr[i]; i += 2)
  {
    map< string, string >::iterator it(attributes.find(attr[i]));
    if (it != attributes.end())
      it->second = attr[i+1];
    else
    {
      ostringstream temp;
      temp<<"Unknown attribute \""<<attr[i]<<"\" in element \""<<element<<"\".";
      //add_static_error(temp.str());
    }
  }
}

void Statement::assure_no_text(string text, string name)
{
  for (unsigned int i(0); i < text.size(); ++i)
  {
    if (!isspace(text[i]))
    {
      ostringstream temp;
      temp<<"Element \""<<name<<"\" must not contain text.";
      //add_static_error(temp.str());
      break;
    }
  }
}

void Statement::substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  //add_static_error(temp.str());
  
  delete child;
}

void Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  substatement_error(get_name(), statement);
}

void Statement::add_final_text(string text)
{
  assure_no_text(text, this->get_name());
}

void Statement::display_full()
{
  //display_verbatim(get_source(startpos, endpos - startpos));
}

void Statement::display_starttag()
{
  //display_verbatim(get_source(startpos, tagendpos - startpos));
}

void Statement::stopwatch_start()
{
  for (uint i(0); i < stopwatches.size(); ++i)
    stopwatches[i] = 0;
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatch = ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0;
}

void Statement::stopwatch_skip()
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatch = ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0;
}

void Statement::stopwatch_stop(uint32 account)
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatches[account] += (ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0)
    - stopwatch;
  clock_gettime(CLOCK_REALTIME, &ts);
  stopwatch = ts.tv_sec + ((double)ts.tv_nsec)/1000000000.0;
}

void Statement::stopwatch_report() const
{
  cout<<"Stopwatch "<<get_name();
  for (vector< double >::const_iterator it(stopwatches.begin());
      it != stopwatches.end(); ++it)
    cout<<setprecision(3)<<fixed<<'\t'<<*it;
  cout<<'\n';
}

void Statement::stopwatch_sum(const Statement* s)
{
  for (uint i(0); i < stopwatches.size(); ++i)
    stopwatches[i] += s->stopwatches[i];
}
