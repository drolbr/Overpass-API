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


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "geometry_endomorphisms.h"


Geometry_Endom_Statement_Maker< Evaluator_Center > Evaluator_Center::statement_maker;
Geometry_Endom_Evaluator_Maker< Evaluator_Center > Evaluator_Center::evaluator_maker;


Opaque_Geometry* Evaluator_Center::process(Opaque_Geometry* geom) const
{
  if (!geom)
    return 0;
  
  Opaque_Geometry* result = 0;
  if (geom->has_center())
    result = new Point_Geometry(geom->center_lat(), geom->center_lon());
  else
    result = new Null_Geometry();
  
  delete geom;
  return result;
}


//-----------------------------------------------------------------------------


Geometry_Endom_Statement_Maker< Evaluator_Trace > Evaluator_Trace::statement_maker;
Geometry_Endom_Evaluator_Maker< Evaluator_Trace > Evaluator_Trace::evaluator_maker;


Opaque_Geometry* Evaluator_Trace::process(Opaque_Geometry* geom) const
{
  if (!geom)
    return 0;
  
  Opaque_Geometry* result = make_trace(*geom);
  
  delete geom;
  return result;
}


//-----------------------------------------------------------------------------


Geometry_Endom_Statement_Maker< Evaluator_Hull > Evaluator_Hull::statement_maker;
Geometry_Endom_Evaluator_Maker< Evaluator_Hull > Evaluator_Hull::evaluator_maker;


Opaque_Geometry* Evaluator_Hull::process(Opaque_Geometry* geom) const
{
  if (!geom)
    return 0;
  
  Opaque_Geometry* result = make_hull(*geom);
  
  delete geom;
  return result;
}
