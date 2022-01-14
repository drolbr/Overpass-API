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

#include "../data/tag_store.h"
#include "../data/utils.h"
#include "string_endomorphisms.h"


String_Endom_Statement_Maker< Evaluator_Number > Evaluator_Number::statement_maker;
String_Endom_Evaluator_Maker< Evaluator_Number > Evaluator_Number::evaluator_maker;


std::string Evaluator_Number::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;
  if (try_int64(rhs_s, rhs_l))
    return to_string(rhs_l);

  double rhs_d = 0;
  if (try_starts_with_double(rhs_s, rhs_d))
    return to_string(rhs_d);

  return "NaN";
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Is_Num > Evaluator_Is_Num::statement_maker;
String_Endom_Evaluator_Maker< Evaluator_Is_Num > Evaluator_Is_Num::evaluator_maker;


std::string Evaluator_Is_Num::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;
  if (try_int64(rhs_s, rhs_l))
    return "1";

  double rhs_d = 0;
  if (try_starts_with_double(rhs_s, rhs_d))
    return "1";

  return "0";
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Suffix > Evaluator_Suffix::statement_maker;
String_Endom_Evaluator_Maker< Evaluator_Suffix > Evaluator_Suffix::evaluator_maker;


std::string Evaluator_Suffix::process(const std::string& rhs_s) const
{
  return double_suffix(rhs_s);
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Abs > Evaluator_Abs::statement_maker;
String_Endom_Evaluator_Maker< Evaluator_Abs > Evaluator_Abs::evaluator_maker;


std::string Evaluator_Abs::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;
  if (try_int64(rhs_s, rhs_l))
    return to_string(std::abs(rhs_l));

  double rhs_d = 0;
  if (try_starts_with_double(rhs_s, rhs_d))
    return to_string(std::abs(rhs_d));

  return "NaN";
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Date > Evaluator_Date::statement_maker;
String_Endom_Evaluator_Maker< Evaluator_Date > Evaluator_Date::evaluator_maker;


std::string Evaluator_Date::process(const std::string& rhs_s) const
{
  //First run: try for year, month, day, hour, minute, second
  std::string::size_type pos = 0;

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int year = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    year = 10*year + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int month = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    month = 10*month + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int day = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    day = 10*day + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int hour = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    hour = 10*hour + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int minute = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    minute = 10*minute + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int second = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    second = 10*second + (rhs_s[pos] - '0');
    ++pos;
  }

  if (year < 1000 || month > 12 || day > 31 || hour > 23 || minute > 59 || second > 60)
    return "NaD";

  return to_string(year + month/16. + day/(16.*32)
      + hour/(16.*32*32) + minute/(16.*32*32*64) + second/(16.*32*32*64*64));
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Is_Date > Evaluator_Is_Date::statement_maker;
String_Endom_Evaluator_Maker< Evaluator_Is_Date > Evaluator_Is_Date::evaluator_maker;


std::string Evaluator_Is_Date::process(const std::string& rhs_s) const
{
  //First run: try for year, month, day, hour, minute, second
  std::string::size_type pos = 0;

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int year = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    year = 10*year + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int month = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    month = 10*month + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int day = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    day = 10*day + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int hour = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    hour = 10*hour + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int minute = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    minute = 10*minute + (rhs_s[pos] - '0');
    ++pos;
  }

  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int second = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    second = 10*second + (rhs_s[pos] - '0');
    ++pos;
  }

  if (year < 1000 || month > 12 || day > 31 || hour > 23 || minute > 59 || second > 60)
    return "0";

  return "1";
}
