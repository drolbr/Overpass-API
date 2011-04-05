/*****************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the license contained in the
 * COPYING file that comes with the expat distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Must be used with Expat compiled for UTF-8 output.
 */

#ifndef ORG__OVERPASS_API__EXPAT_JUSTPARSE_INTERFACE
#define ORG__OVERPASS_API__EXPAT_JUSTPARSE_INTERFACE

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

using namespace std;

struct Parse_Error
{
  Parse_Error(string s) : message(s) {}
  string message;
};

struct Script_Parser
{
  Script_Parser() : p(XML_ParserCreate(NULL)), parser_online(true), counter(MAXCOUNT)
  {
    if (!p)
      throw Parse_Error("Couldn't allocate memory for parser.");
  }
  
  ~Script_Parser()
  {
    XML_ParserFree(p);
  }
  
  void parse(const string& input,
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

  string get_parsed_text()
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
  string result_buf;

  void (*working_start)(const char*, const char**);
  void (*working_end)(const char*);
  
  //for the XMLParser
  char Buff[BUFFSIZE];
  
  const static uint MAXCOUNT = 1048576;
  uint counter;
};

void parse(FILE* in,
	   void (*start)(const char*, const char**),
	   void (*end)(const char*));

#endif
