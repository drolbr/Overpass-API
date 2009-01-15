stmts = \
union_$(suffix) \
query_$(suffix) \
id_query_$(suffix) \
recurse_$(suffix) \
foreach_$(suffix) \
item_$(suffix) \
make_area_$(suffix) \
coord_query_$(suffix) \
print_$(suffix) \
conflict_$(suffix) \
report_$(suffix) \
detect_odd_nodes_$(suffix)

objects = expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o script-interpreter.o $(stmts)

tool_headers = expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h

suffix = statement.o
main: suffix = statement.o
main: $(objects)
	g++ -o cgi-bin/input-interpreter -O3 -Wall -lexpat $(objects) `mysql_config --libs`
	
init_db: expat_justparse_interface.o osm2load_infile.o
	g++ -o osm2load_infile -O3 -Wall -lexpat expat_justparse_interface.o osm2load_infile.o `mysql_config --libs`
	
expat_justparse_interface.o: expat_justparse_interface.c expat_justparse_interface.h
	g++ -c -O3 -Wall expat_justparse_interface.c
	
cgi-helper.o: cgi-helper.c cgi-helper.h
	g++ -c -O3 -Wall cgi-helper.c
	
script_queries.o: script_queries.c script_datatypes.h script_queries.h
	g++ -c -O3 -Wall `mysql_config --include` script_queries.c
	
script_tools.o: script_tools.c $(tool_headers)
	g++ -c -O3 -Wall `mysql_config --include` script_tools.c
	
%_statement.o: %_statement.c $(tool_headers) %_statement.h
	g++ -c -O3 -Wall `mysql_config --include` $<
	
suffix = statement.h
script-interpreter.o: script-interpreter.c expat_justparse_interface.h cgi-helper.h script_datatypes.h script_queries.h script_tools.h $(stmts)
	g++ -c -O3 -Wall `mysql_config --include` script-interpreter.c
	
osm2load_infile.o: osm2load_infile.c script_datatypes.h expat_justparse_interface.h
	g++ -c -O3 -Wall `mysql_config --include`  osm2load_infile.c

clean: suffix = statement.o
clean:
	rm -f $(objects)
