main: expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o script-interpreter.o query_statement.o id_query_statement.o recurse_statement.o foreach_statement.o make_area_statement.o coord_query_statement.o print_statement.o
	g++ -o cgi-bin/input-interpreter -O3 -Wall -lexpat expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o script-interpreter.o query_statement.o id_query_statement.o recurse_statement.o foreach_statement.o make_area_statement.o coord_query_statement.o print_statement.o `mysql_config --libs`
	
init_db: expat_justparse_interface.o osm2load_infile.o
	g++ -o osm2load_infile -O3 -Wall -lexpat expat_justparse_interface.o osm2load_infile.o `mysql_config --libs`
	
expat_justparse_interface.o: expat_justparse_interface.c expat_justparse_interface.h
	g++ -c -O3 -Wall expat_justparse_interface.c
	
cgi-helper.o: cgi-helper.c cgi-helper.h
	g++ -c -O3 -Wall cgi-helper.c
	
script_queries.o: script_queries.c script_datatypes.h script_queries.h
	g++ -c -O3 -Wall `mysql_config --include` script_queries.c
	
script_tools.o: script_tools.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h
	g++ -c -O3 -Wall `mysql_config --include` script_tools.c
	
query_statement.o: query_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h query_statement.h
	g++ -c -O3 -Wall `mysql_config --include` query_statement.c
	
id_query_statement.o: id_query_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h id_query_statement.h
	g++ -c -O3 -Wall `mysql_config --include` id_query_statement.c
	
recurse_statement.o: recurse_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h recurse_statement.h
	g++ -c -O3 -Wall `mysql_config --include` recurse_statement.c
	
foreach_statement.o: foreach_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h foreach_statement.h
	g++ -c -O3 -Wall `mysql_config --include` foreach_statement.c
	
make_area_statement.o: make_area_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h make_area_statement.h
	g++ -c -O3 -Wall `mysql_config --include` make_area_statement.c
	
coord_query_statement.o: coord_query_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h coord_query_statement.h
	g++ -c -O3 -Wall `mysql_config --include` coord_query_statement.c
	
print_statement.o: print_statement.c expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h print_statement.h
	g++ -c -O3 -Wall `mysql_config --include` print_statement.c
	
script-interpreter.o: script-interpreter.c expat_justparse_interface.h cgi-helper.h script_datatypes.h script_queries.h script_tools.h query_statement.h id_query_statement.h recurse_statement.h foreach_statement.h make_area_statement.h coord_query_statement.h print_statement.h
	g++ -c -O3 -Wall `mysql_config --include` script-interpreter.c
	
osm2load_infile.o: osm2load_infile.c script_datatypes.h expat_justparse_interface.h
	g++ -c -O3 -Wall `mysql_config --include`  osm2load_infile.c
	
clean:
	rm -f script-interpreter.o osm2load_infile.o expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o query_statement.o id_query_statement.o recurse_statement.o foreach_statement.o make_area_statement.o coord_query_statement.o print_statement.o
