/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#ifndef DE__OVERPASS_API__LIBOVERPASS
#define DE__OVERPASS_API__LIBOVERPASS

#ifdef __cplusplus
extern "C" {
#endif


struct Overpass_C_Node
{
  int id;
  double lat;
  double lon;
  
  int num_tags;
  char** tags;
};


struct Coord
{
  double lat;
  double lon;
};


struct Overpass_C_Way
{
  int id;
  int num_coords;
  Coord* coords;
  
  int num_tags;
  char** tags;
};


struct Overpass_C_Handle
{
};


void alloc_overpass_handle(Overpass_C_Handle** handle);

void overpass_bbox(Overpass_C_Handle* handle,
		   double south, double west, double north, double east,
		   char* condition);

// Returns
// 0 if no elements are left
// 1 if next element is a node
// 2 if next element is a way
int has_next_overpass_handle(Overpass_C_Handle* handle);

// The library keeps ownership of the pointer
Overpass_C_Node* next_node_overpass_handle(Overpass_C_Handle* handle);

// The library keeps ownership of the pointer
Overpass_C_Way* next_way_overpass_handle(Overpass_C_Handle* handle);


void free_overpass_handle(Overpass_C_Handle* handle);


#ifdef __cplusplus
}
#endif

#endif
