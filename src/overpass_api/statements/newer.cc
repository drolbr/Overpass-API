#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "meta_collector.h"
#include "newer.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

//-----------------------------------------------------------------------------

void Newer_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["than"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  string timestamp = attributes["than"];
  
  than_timestamp = 0;
  than_timestamp |= (atoll(timestamp.c_str())<<26); //year
  than_timestamp |= (atoi(timestamp.c_str()+5)<<22); //month
  than_timestamp |= (atoi(timestamp.c_str()+8)<<17); //day
  than_timestamp |= (atoi(timestamp.c_str()+11)<<12); //hour
  than_timestamp |= (atoi(timestamp.c_str()+14)<<6); //minute
  than_timestamp |= atoi(timestamp.c_str()+17); //second
  
  if (than_timestamp == 0)
  {
    ostringstream temp;
    temp<<"The attribute than must contain a timestamp exactly in the form yyyy-mm-ddThh:mm:ssZ.";
    add_static_error(temp.str());
  }
}

void Newer_Statement::forecast() {}

void Newer_Statement::execute(Resource_Manager& rman) {}
