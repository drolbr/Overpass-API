#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "user_interface.h"
#include "vigilance_control.h"
#include "backend/file_types.h"

#include <mysql.h>

using namespace std;

const char* types_lowercase[] = { "", "node", "way", "relation", "area" };
const char* types_uppercase[] = { "", "Node", "Way", "Relation", "Area" };

static string rule_name_;
static int rule_id_;
static vector< pair< int, int > > stack;

void set_rule(int rule_id, string rule_name)
{
  rule_id_ = rule_id;
  rule_name_ = rule_name;
}

int get_rule_id()
{
  return rule_id_;
}

string get_rule_name()
{
  return rule_name_;
}

void push_stack(int type, int id)
{
  stack.push_back(make_pair< int, int>(type, id));
}

void pop_stack()
{
  stack.pop_back();
}

const vector< pair< int, int > >& get_stack()
{
  return stack;
}

int next_stmt_id()
{
  static int next_stmt_id(0);
  return ++next_stmt_id;
}

//-----------------------------------------------------------------------------

void eval_cstr_array(string element, map< string, string >& attributes, const char **attr)
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

void substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  add_static_error(temp.str());
  
  delete child;
}

//-----------------------------------------------------------------------------

void assure_no_text(string text, string name)
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
  display_verbatim(get_source(startpos, endpos - startpos), cout);
}

void Statement::display_starttag()
{
  display_verbatim(get_source(startpos, tagendpos - startpos), cout);
}

//-----------------------------------------------------------------------------

int script_timeout(0);
int element_limit(0);

void Root_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["timeout"] = "0";
  attributes["element-limit"] = "0";
  attributes["name"] = "";
  attributes["replace"] = "0";
  attributes["version"] = "0";
  attributes["debug"] = "errors";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  name = attributes["name"];
  replace = atoi(attributes["replace"].c_str());
  timeout = atoi(attributes["timeout"].c_str());
  elem_limit = atoi(attributes["element-limit"].c_str());
  version = atoi(attributes["version"].c_str());
  
  if (attributes["debug"] == "quiet")
    set_debug_mode(QUIET);
  else if (attributes["debug"] == "errors")
    set_debug_mode(ERRORS);
  else if (attributes["debug"] == "verbose")
    set_debug_mode(VERBOSE);
  else if (attributes["debug"] == "static")
    set_debug_mode(STATIC_ANALYSIS);
  else
  {
    ostringstream temp;
    temp<<"For the attribute \"debug\" of the element \"osm-script\""
	<<" the only allowed values are \"quiet\", \"errors\", \"verbose\" or \"static\".";
    add_static_error(temp.str());
  }
  
  script_timeout = timeout;
  element_limit = elem_limit;
  ostringstream temp;
  temp<<"Timeout is set to "<<timeout<<", element_limit is "<<elem_limit;
  add_static_remark(temp.str());
}

void Root_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if ((statement->get_name() == "union") ||
       (statement->get_name() == "id-query") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "recurse") ||
       (statement->get_name() == "foreach") ||
       (statement->get_name() == "make-area") ||
       (statement->get_name() == "coord-query") ||
       (statement->get_name() == "bbox-query") ||
       (statement->get_name() == "area-query") ||
       (statement->get_name() == "print") ||
       (statement->get_name() == "conflict") ||
       (statement->get_name() == "report") ||
       (statement->get_name() == "detect-odd-nodes"))
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Root_Statement::forecast(MYSQL* mysql)
{
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
    (*it)->forecast(mysql);
}

void Root_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  uint max_element_count(get_element_count());
  if (element_limit > 0)
    max_element_count = element_limit;
  if (rule_id_ == 0)
  {
    if (database_id == 0)
      runtime_error("Internal: No database id set.\n", cout);
    register_process(mysql_thread_id(mysql), database_id, timeout, max_element_count);
  }
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->execute(mysql, maps);
    
    statement_finished(*it);
  }

  if (rule_id_ == 0)
    unregister_process(mysql_thread_id(mysql));
}

//-----------------------------------------------------------------------------

vector< string > role_cache;

const vector< string >& get_role_cache()
{
  return role_cache;
}

void prepare_caches(MYSQL* mysql)
{
  role_cache.push_back("");
  
/*  MYSQL_RES* result(mysql_query_wrapper(mysql, "select id, role from member_roles"));
  if (!result)
    return;
  
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]))
  {
    unsigned int id((unsigned int)atol(row[0]));
    if (role_cache.size() <= id)
      role_cache.resize(id+64);
    role_cache[id] = row[1];
    row = mysql_fetch_row(result);
  }*/

  int data_fd = open64((DATADIR + db_subdir + MEMBER_ROLES_FILENAME).c_str(),
                       O_RDONLY);
  if (data_fd < 0)
    throw File_Error(errno, DATADIR + db_subdir + MEMBER_ROLES_FILENAME,
                     "prepare_caches:1");
  
  uint16 size(0);
  char* buf = (char*) malloc(65536);
  while (read(data_fd, &size, 2))
  {
    read(data_fd, buf, size);
    role_cache.push_back(string(buf, size));
  }
  
  close(data_fd);
}

//-----------------------------------------------------------------------------

struct Flow_Forecast {
  int time_used_so_far;
  vector< pair< int, string > > pending_sets;
};

map< string, Set_Forecast > sets;
uint64 max_element_count(0);

vector< Flow_Forecast > forecast_stack;

void declare_used_time(int milliseconds)
{
  forecast_stack.back().time_used_so_far += milliseconds;
}

const Set_Forecast& declare_read_set(string name)
{
  forecast_stack.back().pending_sets.push_back
	(make_pair< int, string >(READ_FORECAST, name));
  
  map< string, Set_Forecast >::const_iterator it(sets.find(name));
  if (it != sets.end())
    return it->second;
  else
  {
    ostringstream temp;
    temp<<"Statement reads from set \""<<name<<"\" which has not been used before.\n";
    add_sanity_remark(temp.str());
    return sets[name];
  }
}

Set_Forecast& declare_write_set(string name)
{
  forecast_stack.back().pending_sets.push_back
	(make_pair< int, string >(WRITE_FORECAST, name));
  
  sets[name] = Set_Forecast();
  return sets[name];
}

Set_Forecast& declare_union_set(string name)
{
  forecast_stack.back().pending_sets.push_back
	(make_pair< int, string >(UNION_FORECAST, name));
  
  return sets[name];
}

void inc_stack()
{
  int time_used_so_far(0);
  if (!(forecast_stack.empty()))
    time_used_so_far = forecast_stack.back().time_used_so_far;
  forecast_stack.push_back(Flow_Forecast());
  forecast_stack.back().time_used_so_far = time_used_so_far;
  if (forecast_stack.size() > 20)
    add_sanity_error("Stack exceeds size limit of 20.");
}

void dec_stack()
{
  int time_used_so_far(forecast_stack.back().time_used_so_far);
  forecast_stack.pop_back();
  if (!(forecast_stack.empty()))
    forecast_stack.back().time_used_so_far = time_used_so_far;
}

const vector< pair< int, string > >& pending_stack()
{
  return forecast_stack.back().pending_sets;
}

int stack_time_offset()
{
  if (forecast_stack.size() > 1)
    return (forecast_stack.back().time_used_so_far
	- forecast_stack[forecast_stack.size() - 2].time_used_so_far);
  else
    return forecast_stack.back().time_used_so_far;
}

void finish_statement_forecast()
{
  if ((script_timeout == 0) && (forecast_stack.back().time_used_so_far > 180*1000))
    add_sanity_error("Time exceeds limit of 180 seconds.");
  unsigned int element_count(0);
  for (map< string, Set_Forecast >::const_iterator it(sets.begin()); it != sets.end(); ++it)
  {
    element_count += it->second.node_count;
    element_count += 10*it->second.way_count;
    element_count += 20*it->second.relation_count;
  }
  if (element_count > max_element_count)
    max_element_count = element_count;
  if ((element_limit == 0) && (element_count > 10*1000*1000))
    add_sanity_error("Number of elements exceeds limit of 10,000,000 elements.");
}

uint64 get_element_count()
{
  return max_element_count;
}

void display_state()
{
  ostringstream temp;
  temp<<"The script will reach this point "
      <<((double)forecast_stack.back().time_used_so_far)/1000
      <<" seconds after start.<br/>\n";
  for (map< string, Set_Forecast >::const_iterator it(sets.begin()); ; )
  {
    temp<<"Set \""<<it->first<<"\" will contain here "
	<<it->second.node_count<<" nodes, "
	<<it->second.way_count<<" ways, "
	<<it->second.relation_count<<" relations and "
	<<it->second.area_count<<" areas.";
    if (++it != sets.end())
      temp<<"<br/>\n";
    else
      break;
  }
  display_state(temp.str(), cout);
}

//-----------------------------------------------------------------------------

string detect_active_database()
{
  uint32 database_id, available_memory;
  ifstream ssin("/tmp/small_status");
  if (!ssin)
  {
    runtime_error("Status file not found. Check if the dispatcher is running.", cout);
    //throw File_Error(errno, "/tmp/small_status", "detect_active_database:1");
  }
  ssin>>database_id;
  ssin>>available_memory;
  
  uint max_element_count(get_element_count());
  if (element_limit > 0)
    max_element_count = element_limit;
  if (available_memory < max_element_count)
  {
    runtime_error("The Server currently doesn't have enough free memory to execute your request. Please try again later.", cout);
  }
  
  ssin.close();
  
  ostringstream temp;
  temp<<"osm_"<<database_id;
  return temp.str();
}
