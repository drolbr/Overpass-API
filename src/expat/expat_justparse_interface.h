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

void (*working_start)(const char*, const char**);
void (*working_end)(const char*);

//for the XMLParser
char Buff[BUFFSIZE];

const uint MAXCOUNT = 1048576;
uint counter(MAXCOUNT);

static void XMLCALL
expat_wrapper_start(void *data, const char *el, const char **attr)
{
//   if (--counter == 0)
//   {
//     cerr<<'.';
//     counter = MAXCOUNT;
//   }

  working_start(el, attr);
  
}

static void XMLCALL
expat_wrapper_end(void *data, const char *el)
{
  working_end(el);
}

void parse(FILE* in,
	   void (*start)(const char*, const char**),
	   void (*end)(const char*))
{
  working_start = start;
  working_end = end;
  
  XML_Parser p = XML_ParserCreate(NULL);
  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    exit(-1);
  }

  XML_SetElementHandler(p, expat_wrapper_start, expat_wrapper_end);

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
}
