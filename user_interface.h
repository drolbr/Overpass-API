#ifndef USER_INTERFACE
#define USER_INTERFACE

#include <iostream>
#include <string>
#include "script_tools.h"

using namespace std;

string get_xml_raw();

void add_encoding_error(const string& error);
void add_parse_error(const string& error);
void add_static_error(const string& error);
void add_sanity_error(const string& error);

void add_encoding_remark(const string& error);
void add_parse_remark(const string& error);
void add_static_remark(const string& error);
void add_sanity_remark(const string& error);

int display_encoding_errors(ostream& out);
int display_parse_errors(ostream& out, const string& input);
int display_static_errors(ostream& out, const string& input);
int display_sanity_errors(ostream& out, const string& input);

void static_analysis(ostream& out, const string& input);

void display_verbatim(const string& text, ostream& out);
void display_state(const string& text, ostream& out);

void runtime_error(const string& error, ostream& out);
void runtime_remark(const string& error, ostream& out);

void statement_finished(const Statement* stmt);

const int MIXED_XML = 1;
const int HTML = 2;
const int NOTHING = 3;

ostream& out_header(ostream& out, int type);

ostream& out_footer(ostream& out, int type);

const int QUIET = 1;
const int ERRORS = 2;
const int VERBOSE = 3;
const int STATIC_ANALYSIS = 4;

void set_debug_mode(int mode);
int get_debug_mode();

#endif
