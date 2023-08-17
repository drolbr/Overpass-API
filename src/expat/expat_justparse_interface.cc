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

#include "expat_justparse_interface.h"

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


void (*working_start)(const char*, const char**);
void (*working_end)(const char*);
void (*working_text_handler)(void *data, const XML_Char *s, int len);

//for the XMLParser
char Buff[BUFFSIZE];

const uint MAXCOUNT = 1048576;
uint counter(MAXCOUNT);

static void XMLCALL
expat_wrapper_start(void *data, const char *el, const char **attr)
{
  working_start(el, attr);
}

static void XMLCALL
expat_wrapper_end(void *data, const char *el)
{
  working_end(el);
}

static void XMLCALL
expat_wrapper_text_handler(void *data, const XML_Char *s, int len)
{
  if (working_text_handler)
    working_text_handler(data, s, len);
}


namespace
{
  XML_Parser current_parser = 0;
}


void parse(FILE* in,
	   void (*start)(const char*, const char**),
	   void (*end)(const char*),
           void (*text_handler)(void *data, const XML_Char *s, int len))
{
  working_start = start;
  working_end = end;
  working_text_handler = text_handler;

  XML_Parser p = XML_ParserCreate(NULL);
  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    exit(-1);
  }
  current_parser = p;

  XML_SetElementHandler(p, expat_wrapper_start, expat_wrapper_end);

  XML_SetCharacterDataHandler(p, expat_wrapper_text_handler);

  for (;;) {
    int done;
    int len;

    len = (int)fread(Buff, 1, BUFFSIZE, in);
    if (ferror(in)) {
      fprintf(stderr, "Read error\n");
      exit(-1);
    }
    done = feof(in);

    if (XML_Parse(p, Buff, len, done) == XML_STATUS_ERROR) {
      fprintf(stderr, "Parse error at line %" XML_FMT_INT_MOD "u:\n%s\n",
	      XML_GetCurrentLineNumber(p),
				       XML_ErrorString(XML_GetErrorCode(p)));
      exit(-1);
    }

    if (done)
      break;
  }

  XML_ParserFree(p);
  current_parser = 0;
}


XML_Parser get_current_parser()
{
  return current_parser;
}
