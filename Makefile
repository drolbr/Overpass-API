stmts = \
coord_query_$(suffix) \
conflict_$(suffix) \
detect_odd_nodes_$(suffix) \
foreach_$(suffix) \
id_query_$(suffix) \
item_$(suffix) \
make_area_$(suffix) \
print_$(suffix) \
query_$(suffix) \
recurse_$(suffix) \
report_$(suffix) \
union_$(suffix)

objects = expat_justparse_interface.o cgi-helper.o user_interface.o script_queries.o script_tools.o vigilance_control.o $(stmts)
executable_objects = script-interpreter.o add_rule.o get_rule.o update_rule.o osm2load_infile.o
executables = cgi-bin/interpreter cgi-bin/add_rule cgi-bin/get_rule cgi-bin/update_rule import_osm osmy_vigilance
tool_headers = expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h user_interface.h

main: $(executables)

suffix = statement.o
cgi-bin/interpreter: suffix = statement.o
cgi-bin/interpreter: $(objects) script-interpreter.o
	g++ -o cgi-bin/interpreter -O3 -Wall -lexpat $(objects) script-interpreter.o `mysql_config --libs`
	
suffix = statement.o
cgi-bin/add_rule: suffix = statement.o
cgi-bin/add_rule: $(objects) add_rule.o
	g++ -o cgi-bin/add_rule -O3 -Wall -lexpat $(objects) add_rule.o `mysql_config --libs`
	
suffix = statement.o
cgi-bin/get_rule: suffix = statement.o
cgi-bin/get_rule: $(objects) get_rule.o
	g++ -o cgi-bin/get_rule -O3 -Wall -lexpat $(objects) get_rule.o `mysql_config --libs`
	
suffix = statement.o
cgi-bin/update_rule: suffix = statement.o
cgi-bin/update_rule: $(objects) update_rule.o
	g++ -o cgi-bin/update_rule -O3 -Wall -lexpat $(objects) update_rule.o `mysql_config --libs`
	
import_osm: expat_justparse_interface.o cgi-helper.o user_interface.o osm2load_infile.o
	g++ -o import_osm -O3 -Wall -lexpat expat_justparse_interface.o cgi-helper.o user_interface.o osm2load_infile.o `mysql_config --libs`
	
osmy_vigilance: osmy_vigilance.c
	g++ -o osmy_vigilance -O3 -Wall `mysql_config --include` osmy_vigilance.c `mysql_config --libs`
	
expat_justparse_interface.o: expat_justparse_interface.c expat_justparse_interface.h user_interface.h
	g++ -c -O3 -Wall expat_justparse_interface.c
	
cgi-helper.o: cgi-helper.c cgi-helper.h
	g++ -c -O3 -Wall cgi-helper.c
	
user_interface.o: user_interface.c user_interface.h expat_justparse_interface.h cgi-helper.h
	g++ -c -O3 -Wall user_interface.c
	
script_queries.o: script_queries.c script_queries.h script_datatypes.h vigilance_control.h user_interface.h
	g++ -c -O3 -Wall `mysql_config --include` script_queries.c
	
script_tools.o: script_tools.c $(tool_headers) vigilance_control.h
	g++ -c -O3 -Wall `mysql_config --include` script_tools.c
	
%_statement.o: %_statement.c $(tool_headers) %_statement.h
	g++ -c -O3 -Wall `mysql_config --include` $<
	
vigilance_control.o: vigilance_control.c vigilance_control.h script_datatypes.h expat_justparse_interface.h cgi-helper.h user_interface.h
	g++ -c -O3 -Wall vigilance_control.c

suffix = statement.h
script-interpreter.o: script-interpreter.c expat_justparse_interface.h user_interface.h script_datatypes.h script_queries.h script_tools.h $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` script-interpreter.c
	
suffix = statement.h
add_rule.o: add_rule.c expat_justparse_interface.h user_interface.h script_datatypes.h script_queries.h script_tools.h $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` add_rule.c
	
suffix = statement.h
get_rule.o: get_rule.c expat_justparse_interface.h user_interface.h script_datatypes.h script_queries.h script_tools.h $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` get_rule.c
	
suffix = statement.h
update_rule.o: update_rule.c expat_justparse_interface.h user_interface.h script_datatypes.h script_queries.h script_tools.h $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` update_rule.c
	
osm2load_infile.o: osm2load_infile.c script_datatypes.h expat_justparse_interface.h cgi-helper.h user_interface.h
	g++ -c -O3 -Wall `mysql_config --include` osm2load_infile.c

clean: suffix = statement.o
clean:
	rm -f $(objects) $(executable_objects)
