stmts = \
area_query_$(suffix) \
bbox_query_$(suffix) \
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

objects = expat_justparse_interface.o cgi-helper.o user_interface.o script_queries.o node_strings_file_io.o way_strings_file_io.o relation_strings_file_io.o script_tools.o vigilance_control.o $(stmts)
executable_objects = script-interpreter.o add_rule.o dump_rules.o get_rule.o load_rules.o update_rule.o import_osm.o apply_osc.o
executables = cgi-bin/interpreter cgi-bin/add_rule cgi-bin/get_rule cgi-bin/update_rule bin/dump_rules bin/load_rules
shortcuts = cgi-bin/download cgi-bin/poi cgi-bin/elems bin/nodes-csv-lat-lon-name cgi-bin/nodes-csv bin/memberlist cgi-bin/nodes-csv-gzip cgi-bin/sketch-route cgi-bin/sketch-line bin/sketch-route-svg cgi-bin/sketch-options bin/uncgi bin/escape_xml bin/bbox-brim-query
tool_headers = expat_justparse_interface.h script_datatypes.h script_queries.h script_tools.h user_interface.h

backend_generic_h = backend/raw_file_db.h backend/file_types.h script_datatypes.h
backend_types_h = backend/node_strings_file_io.h backend/way_strings_file_io.h backend/relation_strings_file_io.h
backend_objects = node_strings_file_io.o way_strings_file_io.o relation_strings_file_io.o
backend_executables = bin/import_osm bin/apply_osc

dispatcher_executables = bin/dispatcher bin/database_daemon bin/timestamp_of bin/fetch_osc bin/apply_gz_osc bin/init_dispatcher bin/autorestart

main: $(executables) $(backend_executables) $(dispatcher_executables) $(shortcuts)

suffix = statement.o
cgi-bin/interpreter: suffix = statement.o
cgi-bin/interpreter: $(objects) script-interpreter.o
	g++ -o cgi-bin/interpreter -O3 -Wall -lexpat -lz $(objects) script-interpreter.o `mysql_config --libs`

suffix = statement.o
cgi-bin/add_rule: suffix = statement.o
cgi-bin/add_rule: $(objects) add_rule.o
	g++ -o cgi-bin/add_rule -O3 -Wall -lexpat -lz $(objects) add_rule.o `mysql_config --libs`

suffix = statement.o
cgi-bin/get_rule: suffix = statement.o
cgi-bin/get_rule: $(objects) get_rule.o
	g++ -o cgi-bin/get_rule -O3 -Wall -lexpat -lz $(objects) get_rule.o `mysql_config --libs`

check_xml: suffix = statement.o
check_xml: $(objects) check_xml.o
	g++ -o check_xml -O3 -Wall -lexpat -lz $(objects) check_xml.o `mysql_config --libs`

suffix = statement.o
cgi-bin/update_rule: suffix = statement.o
cgi-bin/update_rule: $(objects) update_rule.o
	g++ -o cgi-bin/update_rule -O3 -Wall -lexpat -lz  $(objects) update_rule.o `mysql_config --libs`

suffix = statement.o
bin/dump_rules: suffix = statement.o
bin/dump_rules: $(objects) dump_rules.o
	g++ -o $@ -O3 -Wall -lexpat -lz $(objects) dump_rules.o `mysql_config --libs`

suffix = statement.o
bin/load_rules: suffix = statement.o
bin/load_rules: $(objects) load_rules.o
	g++ -o $@ -O3 -Wall -lexpat -lz $(objects) load_rules.o `mysql_config --libs`

expat_justparse_interface.o: expat_justparse_interface.c expat_justparse_interface.h user_interface.h
	g++ -c -O3 -Wall `mysql_config --include` expat_justparse_interface.c

cgi-helper.o: cgi-helper.c cgi-helper.h
	g++ -c -O3 -Wall cgi-helper.c

user_interface.o: user_interface.c user_interface.h expat_justparse_interface.h cgi-helper.h
	g++ -c -O3 -Wall `mysql_config --include` user_interface.c

script_queries.o: script_queries.c script_queries.h script_datatypes.h user_interface.h vigilance_control.h $(backend_types_h) $(backend_generic_h)
	g++ -c -O3 -Wall `mysql_config --include` script_queries.c

script_tools.o: script_tools.c expat_justparse_interface.h vigilance_control.h $(tool_headers) $(backend_generic_h)
	g++ -o $@ -c -O3 -Wall `mysql_config --include` $<

%_statement.o: %_statement.c $(tool_headers) %_statement.h
	g++ -c -O3 -Wall `mysql_config --include` $<

vigilance_control.o: vigilance_control.c vigilance_control.h script_datatypes.h expat_justparse_interface.h cgi-helper.h user_interface.h
	g++ -c -O3 -Wall `mysql_config --include` vigilance_control.c


suffix = statement.h
script-interpreter.o: script-interpreter.c $(tool_headers) $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` $<

suffix = statement.h
add_rule.o: add_rule.c $(tool_headers) $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` $<

suffix = statement.h
get_rule.o: get_rule.c $(tool_headers) $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` $<

suffix = statement.h
check_xml.o: check_xml.c $(tool_headers) $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` $<

suffix = statement.h
update_rule.o: update_rule.c $(tool_headers) $(stmts) statement_factory.h
	g++ -c -O3 -Wall `mysql_config --include` $<


suffix = statement.h
dump_rules.o: maintenance/dump_rules.c $(tool_headers) $(stmts) statement_factory.h
	g++ -o $@ -c -O3 -Wall `mysql_config --include` $<

suffix = statement.h
load_rules.o: maintenance/load_rules.c $(tool_headers) $(stmts) statement_factory.h
	g++ -o $@ -c -O3 -Wall `mysql_config --include` $<


bin/import_osm: import_osm.o expat_justparse_interface.o $(backend_objects)
	g++ -o $@ -O3 -Wall -lexpat -lz $^

import_osm.o: backend/import_osm.c expat_justparse_interface.h $(backend_types_h) $(backend_generic_h)
	g++ -o $@ -c -O3 -Wall $<


bin/apply_osc: apply_osc.o expat_justparse_interface.o $(backend_objects)
	g++ -o $@ -O3 -Wall -lexpat -lz $^
	
apply_osc.o: backend/apply_osc.c expat_justparse_interface.h $(backend_types_h) $(backend_generic_h)
	g++ -o $@ -c -O3 -Wall $<


node_strings_file_io.o: backend/node_strings_file_io.c backend/node_strings_file_io.h $(backend_generic_h)
	g++ -o $@ -c -O3 -Wall $<

way_strings_file_io.o: backend/way_strings_file_io.c backend/way_strings_file_io.h $(backend_generic_h)
	g++ -o $@ -c -O3 -Wall $<

relation_strings_file_io.o: backend/relation_strings_file_io.c backend/relation_strings_file_io.h $(backend_generic_h)
	g++ -o $@ -c -O3 -Wall $<


bin/dispatcher: dispatcher/dispatcher.c
	g++ -o $@ -O3 -Wall `mysql_config --include` $< `mysql_config --libs`

suffix = statement.o
bin/database_daemon: suffix = statement.o
bin/database_daemon: database_daemon.o process_rules.o $(objects)
	g++ -o $@ -O3 -Wall -lexpat -lz $^ `mysql_config --libs`

database_daemon.o: dispatcher/database_daemon.c dispatcher/process_rules.h
	g++ -o $@ -c -O3 -Wall `mysql_config --include` $<

bin/timestamp_of: dispatcher/timestamp_of.c
	g++ -o $@ -O3 -Wall $<

process_rules.o: dispatcher/process_rules.c dispatcher/process_rules.h $(tool_headers) $(stmts) script_datatypes.h
	g++ -o $@ -c -O3 -Wall `mysql_config --include` $<

bin/fetch_osc: dispatcher/fetch_osc
	cp $< $@
	chmod 755 $@

bin/apply_gz_osc: dispatcher/apply_gz_osc
	cp $< $@
	chmod 755 $@

bin/init_dispatcher: dispatcher/init_dispatcher
	cp $< $@
	chmod 755 $@

bin/autorestart: dispatcher/autorestart
	cp $< $@
	chmod 755 $@

cgi-bin/poi: query_shortcuts/poi.sh
	cp $< $@
	chmod 755 $@

cgi-bin/download: query_shortcuts/download.sh
	cp $< $@
	chmod 755 $@

cgi-bin/elems: query_shortcuts/elems.sh
	cp $< $@
	chmod 755 $@

cgi-bin/%: utils/%.sh
	cp $< $@
	chmod 755 $@

bin/%: utils/%.c expat_justparse_interface.h expat_justparse_interface.o
	g++ -lexpat -o $@ -O3 -Wall $^

clean: suffix = statement.o
clean:
	rm -f $(objects) $(executable_objects) $(backend_objects) backend/*~ dispatcher/*~
