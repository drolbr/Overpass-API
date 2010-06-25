#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <time.h>

#include "statement.h"
#include "bbox_query.h"
#include "foreach.h"
#include "id_query.h"
#include "osm_script.h"
#include "print.h"
#include "query.h"
#include "recurse.h"
#include "union.h"

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
      add_static_error(temp.str());
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
      add_static_error(temp.str());
      break;
    }
  }
}

void Statement::substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  add_static_error(temp.str());
  
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
  if (error_output)
    error_output->display_statement_stopwatch(get_name(), stopwatches);
}

void Statement::stopwatch_sum(const Statement* s)
{
  for (uint i(0); i < stopwatches.size(); ++i)
    stopwatches[i] += s->stopwatches[i];
}

Statement::Statement* Statement::create_statement(string element, int line_number)
{
  /*if (element == "area-query")
    return new Area_Query_Statement();
  else */if (element == "bbox-query")
    return new Bbox_Query_Statement(line_number);
/*  else if (element == "conflict")
    return new Conflict_Statement(line_number);*/
/*  else if (element == "coord-query")
    return new Coord_Query_Statement();*/
/*  else if (element == "detect-odd-nodes")
    return new Detect_Odd_Nodes_Statement();*/
  else if (element == "foreach")
    return new Foreach_Statement(line_number);
  else if (element == "has-kv")
    return new Has_Key_Value_Statement(line_number);
  else if (element == "id-query")
    return new Id_Query_Statement(line_number);
/*  else if (element == "item")
    return new Item_Statement();*/
/*  else if (element == "make-area")
    return new Make_Area_Statement();*/
  else if (element == "osm-script")
    return new Osm_Script_Statement(line_number);
  else if (element == "print")
    return new Print_Statement(line_number);
  else if (element == "query")
    return new Query_Statement(line_number);
  else if (element == "recurse")
    return new Recurse_Statement(line_number);
/*  else if (element == "report")
    return new Report_Statement();*/
  else if (element == "union")
    return new Union_Statement(line_number);
  
  ostringstream temp;
  temp<<"Unknown tag \""<<element<<"\" in line "<<line_number<<'.';
  if (error_output)
    error_output->add_static_error(temp.str(), line_number);
  
  return 0;
}

Error_Output* Statement::error_output = 0;

void Statement::add_static_error(string error)
{
  if (error_output)
    error_output->add_static_error(error, line_number);
}

void Statement::add_static_remark(string remark)
{
  if (error_output)
    error_output->add_static_remark(remark, line_number);
}
