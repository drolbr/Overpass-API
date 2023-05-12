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

#ifndef DE__OSM3S___EXPAT__EXPAT_JUSTPARSE_INTERFACE_H
#define DE__OSM3S___EXPAT__EXPAT_JUSTPARSE_INTERFACE_H

#include <string>

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <expat.h>

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

#define BUFFSIZE        8192


typedef unsigned int uint;

struct Parse_Error
{
  Parse_Error(std::string s) : message(s) {}
  std::string message;
};

struct Script_Parser
{
  Script_Parser() : p(XML_ParserCreate(NULL)), parser_online(true)
  {
    if (!p)
      throw Parse_Error("Couldn't allocate memory for parser.");
  }

  ~Script_Parser()
  {
    XML_ParserFree(p);
  }

  void parse(const std::string& input,
	void (*start)(const char*, const char**),
	void (*end)(const char*))
  {
    working_start = start;
    working_end = end;

    XML_SetElementHandler(p, expat_wrapper_start, expat_wrapper_end);
    XML_SetCharacterDataHandler(p, expat_wrapper_text);
    XML_SetUserData(p, this);

    int done(0);
    int len(input.size());
    int pos(0);
    while ((!done) && (pos < len))
    {
      int buff_len(len - pos);
      if (buff_len > BUFFSIZE-1)
        buff_len = BUFFSIZE-1;
      strcpy(Buff, input.substr(pos, buff_len).c_str());
      pos += buff_len;

      if (XML_Parse(p, Buff, buff_len, done) == XML_STATUS_ERROR)
        throw Parse_Error(XML_ErrorString(XML_GetErrorCode(p)));
    }

    parser_online = false;
  }

  int current_line_number()
  {
    if (parser_online)
      return (XML_GetCurrentLineNumber(p));
    else
      return -1;
  }

  static void XMLCALL expat_wrapper_start
      (void *data, const char *el, const char **attr)
  {
    ((Script_Parser*)data)->working_start(el, attr);
  }

  static void XMLCALL expat_wrapper_end
      (void *data, const char *el)
  {
    ((Script_Parser*)data)->working_end(el);
  }

  static void XMLCALL expat_wrapper_text
      (void *userData, const XML_Char *s, int len)
  {
    ((Script_Parser*)userData)->result_buf.append(s, len);
  }

  std::string get_parsed_text()
  {
    return result_buf;
  }

  void reset_parsed_text()
  {
    result_buf = "";
  }

private:
  XML_Parser p;
  bool parser_online;
  std::string result_buf;

  void (*working_start)(const char*, const char**);
  void (*working_end)(const char*);

  //for the XMLParser
  char Buff[BUFFSIZE];
};

void parse(FILE* in,
	   void (*start)(const char*, const char**),
	   void (*end)(const char*),
           void (*text_handler)(void *data, const XML_Char *s, int len) = 0);

XML_Parser get_current_parser();

#endif
