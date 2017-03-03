/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__DISPATCH__DISPATCHER_STUB_H
#define DE__OSM3S___OVERPASS_API__DISPATCH__DISPATCHER_STUB_H

#include "../statements/statement.h"
#include "../../template_db/dispatcher_client.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


struct Exit_Error {};


class Dispatcher_Stub : public Watchdog_Callback
{
  public:
    // Opens the connection to the database, sets db_dir accordingly
    // and registers the process. error_output_ must remain valid over the
    // entire lifetime of this object.
    Dispatcher_Stub(std::string db_dir_, Error_Output* error_output_, std::string xml_raw,
		    meta_modes meta_, int area_level,
		    uint32 max_allowed_time, uint64 max_allowed_space, Parsed_Query& global_settings_);
    
    // Called once per minute from the resource manager
    virtual void ping() const;

    ~Dispatcher_Stub();
    
    std::string get_db_dir() { return (db_dir == "" ? dispatcher_client->get_db_dir() : db_dir); }
    std::string get_timestamp() { return timestamp; }
    std::string get_area_timestamp() { return area_timestamp; }
    Resource_Manager& resource_manager() { return *rman; }

  private:
    std::string db_dir, timestamp, area_timestamp;
    
    Error_Output* error_output;
    Dispatcher_Client* dispatcher_client;
    Dispatcher_Client* area_dispatcher_client;
    Nonsynced_Transaction* transaction;
    Nonsynced_Transaction* area_transaction;
    Resource_Manager* rman;
    meta_modes meta;
};


#endif
