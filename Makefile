main: expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o script-interpreter.o query_statement.o id_query_statement.o recurse_statement.o make_area_statement.o print_statement.o
	g++ -o cgi-bin/input-interpreter -O3 -Wall -lexpat expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o script-interpreter.o query_statement.o id_query_statement.o recurse_statement.o make_area_statement.o print_statement.o `mysql_config --libs`
	
init_db: expat_justparse_interface.o osm2load_infile.o
	g++ -o osm2load_infile -O3 -Wall -lexpat expat_justparse_interface.o osm2load_infile.o `mysql_config --libs`
	
expat_justparse_interface.o: expat_justparse_interface.c
	g++ -c -O3 -Wall expat_justparse_interface.c
	
cgi-helper.o: cgi-helper.c
	g++ -c -O3 -Wall cgi-helper.c
	
script_queries.o: script_queries.c
	g++ -c -O3 -Wall `mysql_config --include` script_queries.c
	
script_tools.o: script_tools.c
	g++ -c -O3 -Wall `mysql_config --include` script_tools.c
	
query_statement.o: query_statement.c
	g++ -c -O3 -Wall `mysql_config --include` query_statement.c
	
id_query_statement.o: id_query_statement.c
	g++ -c -O3 -Wall `mysql_config --include` id_query_statement.c
	
recurse_statement.o: recurse_statement.c
	g++ -c -O3 -Wall `mysql_config --include` recurse_statement.c
	
make_area_statement.o: make_area_statement.c
	g++ -c -O3 -Wall `mysql_config --include` make_area_statement.c
	
print_statement.o: print_statement.c
	g++ -c -O3 -Wall `mysql_config --include` print_statement.c
	
script-interpreter.o: script-interpreter.c
	g++ -c -O3 -Wall `mysql_config --include` script-interpreter.c
	
osm2load_infile.o: osm2load_infile.c
	g++ -c -O3 -Wall `mysql_config --include`  osm2load_infile.c
	
clean:
	rm -f script-interpreter.o osm2load_infile.o expat_justparse_interface.o cgi-helper.o script_queries.o script_tools.o query_statement.o id_query_statement.o recurse_statement.o make_area_statement.o print_statement.o
