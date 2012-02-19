/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PRINT_TARGET
#define PRINT_TARGET

#include "../statements/print.h"

using namespace std;

class Output_Handle
{
  public:
    Output_Handle(string type_) : type(type_), mode(0), print_target(0) {}
    ~Output_Handle();
    
    Print_Target& get_print_target(uint32 mode, Transaction& transaction);
    
  private:
    string type;
    uint32 mode;
    Print_Target* print_target;
};

#endif
