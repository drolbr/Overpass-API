#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__WEB_OUTPUT_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__WEB_OUTPUT_H

#include "../core/datatypes.h"

using namespace std;

struct Web_Output : public Error_Output
{
  Web_Output(uint log_level_) : header_written(not_yet), encoding_errors(false), parse_errors(false), static_errors(false), log_level(log_level_) {}
  
  ~Web_Output() { write_footer(); }
  
  virtual void add_encoding_error(const string& error);
  virtual void add_parse_error(const string& error, int line_number);
  virtual void add_static_error(const string& error, int line_number);
  
  virtual void add_encoding_remark(const string& error);
  virtual void add_parse_remark(const string& error, int line_number);
  virtual void add_static_remark(const string& error, int line_number);
  
  virtual void runtime_error(const string& error);
  virtual void runtime_remark(const string& error);
  virtual void display_statement_stopwatch
    (const string& name,
     const vector< double >& stopwatches,
     const vector< uint >& read_counts);
  
  virtual bool display_encoding_errors() { return encoding_errors; }
  virtual bool display_parse_errors() { return parse_errors; }
  virtual bool display_static_errors() { return static_errors; }
  
  void enforce_header();
  void write_html_header
      (const string& timestamp = "", const string& area_timestamp = "");
  void write_xml_header
      (const string& timestamp = "", const string& area_timestamp = "");
  void write_footer();
  
private:
  enum { not_yet, xml, html, final } header_written;
  bool encoding_errors;
  bool parse_errors;
  bool static_errors;
  uint log_level;
  
  void display_remark(const string& text);
  void display_error(const string& text);
};

#endif
