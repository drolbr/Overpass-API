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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__ITEM_GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__ITEM_GEOMETRY_H


#include "../../expat/escape_json.h"
#include "../../expat/escape_xml.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


/* === Geometry Related Operators ===


==== Closedness ====

The operator <em>is_closed</em> returns whether the element is a closed way.
The operator is undefined for any other type of element.
For ways, it returns "1" if the first member of the way is equal to the last member of the way
and "0" otherwise.
The operators takes no parameters.

The syntax is

  is_closed()
*/

struct Is_Closed_Eval_Task : public Eval_Task
{
  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return !data.object->nds.empty() && data.object->nds.front() == data.object->nds.back() ? "1" : "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return !data.object->nds.empty() && data.object->nds.front() == data.object->nds.back() ? "1" : "0"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return "NaW"; }
};


class Evaluator_Is_Closed : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Is_Closed >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Is_Closed >("eval-is-closed") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Is_Closed > evaluator_maker;

  static std::string stmt_func_name() { return "is_closed"; }
  virtual std::string dump_xml(const std::string& indent) const { return indent + "<eval-is-closed/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const { return "is_closed()"; }

  Evaluator_Is_Closed(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-is-closed"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Is_Closed() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::SKELETON); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Is_Closed_Eval_Task(); }
};


/* ==== Geometry ====

The <em>geometry</em> operator returns the geometry of a single object
as a geometry that can be put into the other geometry converting operators.

Its syntax is:

  geom()
*/

struct Geometry_Geometry_Task : Eval_Geometry_Task
{
  Geometry_Geometry_Task() {}

  virtual Opaque_Geometry* eval() const { return 0; }

  virtual Opaque_Geometry* eval(const Element_With_Context< Node_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Node_Skeleton > >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Way_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Way_Skeleton > >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Relation_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Relation_Skeleton > >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Area_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Derived_Skeleton >& data) const
      { return data.geometry ? data.geometry->clone() : new Null_Geometry(); }
};


class Evaluator_Geometry : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Geometry >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Geometry >("eval-geometry") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Geometry > evaluator_maker;

  static std::string stmt_func_name() { return "geom"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-geometry/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "geom(\"\")"; }

  Evaluator_Geometry(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-geometry"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Geometry() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::GEOMETRY); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::geometry; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task("<Opaque_Geometry>"); }
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context)
  { return new Geometry_Geometry_Task(); }
  virtual bool returns_geometry() const { return true; }
};


/* ==== Length ====

The length operator returns the length of the element.
For ways this is the length of the way.
For relations this is the sum of the lengthes of the members of type way.
For nodes it is always zero.

Its syntax is:

  length()
*/

struct Length_Eval_Task : public Eval_Task
{
  Length_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return "0"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.geometry ? fixed_to_string(length(*data.geometry), 3) : "0"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return "0"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return "0"; }
};


class Evaluator_Length : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Length >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Length >("eval-length") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Length > evaluator_maker;

  static std::string stmt_func_name() { return "length"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-length/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "length()"; }

  Evaluator_Length(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-length"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Length() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::GEOMETRY); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Length_Eval_Task(); }
};


/* ==== Latitude and Longitude ====

The latitude and longitude operators return the respective coordinate of the element or the element's center.
For nodes it is the latitude resp. longitude of the node's coordinate.
For ways and relations it refers to the coordinate derived from the center of the bounding box.

Their syntaxes are:

  lat()

resp.

  lon()
*/

struct Latitude_Eval_Task : public Eval_Task
{
  Latitude_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lat(), 7) : "NaN"; }
};


class Evaluator_Latitude : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Latitude >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Latitude >("eval-lat") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Latitude > evaluator_maker;

  static std::string stmt_func_name() { return "lat"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-lat/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "lat()"; }

  Evaluator_Latitude(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-lat"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Latitude() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::GEOMETRY); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Latitude_Eval_Task(); }
};


struct Longitude_Eval_Task : public Eval_Task
{
  Longitude_Eval_Task() {}

  virtual std::string eval(const std::string* key) const { return ""; }

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
      { return data.geometry &&
          data.geometry->has_center() ? fixed_to_string(data.geometry->center_lon(), 7) : "NaN"; }
};


class Evaluator_Longitude : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Longitude >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Longitude >("eval-lon") {}
  };
  static Statement_Maker statement_maker;
  static Element_Function_Maker< Evaluator_Longitude > evaluator_maker;

  static std::string stmt_func_name() { return "lon"; }
  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-lon/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "lon()"; }

  Evaluator_Longitude(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-lon"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Longitude() {}

  virtual Requested_Context request_context() const { return Requested_Context().add_usage(Set_Usage::GEOMETRY); }

  virtual Statement::Eval_Return_Type return_type() const { return Statement::string; };
  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Longitude_Eval_Task(); }
};


#endif
