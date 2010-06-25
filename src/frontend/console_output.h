#ifndef CONSOLE_OUTPUT_DEFINED
#define CONSOLE_OUTPUT_DEFINED

#include "../core/datatypes.h"

using namespace std;

struct Console_Output : public Error_Output
{
  Console_Output(bool quiet_ = false) : encoding_errors(false), parse_errors(false),
    static_errors(false), quiet(quiet_) {}
  
  virtual void add_encoding_error(const string& error);
  virtual void add_parse_error(const string& error, int line_number);
  virtual void add_static_error(const string& error, int line_number);
  
  virtual void add_encoding_remark(const string& error);
  virtual void add_parse_remark(const string& error, int line_number);
  virtual void add_static_remark(const string& error, int line_number);
  
  virtual void runtime_error(const string& error);
  virtual void runtime_remark(const string& error);
  virtual void display_statement_stopwatch
    (const string& name, const vector< double >& stopwatches);
  
  virtual bool display_encoding_errors() { return encoding_errors; }
  virtual bool display_parse_errors() { return parse_errors; }
  virtual bool display_static_errors() { return static_errors; }
  
private:
  bool encoding_errors;
  bool parse_errors;
  bool static_errors;
  bool quiet;
};

#endif
