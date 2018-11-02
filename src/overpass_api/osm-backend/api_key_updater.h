/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__API_KEY_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__API_KEY_UPDATER_H


#include "../../template_db/dispatcher_client.h"


#include <string>


class Api_Key_Updater
{
public:
  Api_Key_Updater();
  Api_Key_Updater(const std::string& db_dir);
  ~Api_Key_Updater();

  void parse_file_completely(FILE* in);
  void parse_completely(const std::string& input);
  void finish_updater();

private:
  std::string db_dir_;
  Dispatcher_Client* dispatcher_client;
};


#endif
